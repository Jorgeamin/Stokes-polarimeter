from PyQt5.QtWidgets import QApplication, QMainWindow, QHBoxLayout, QVBoxLayout, QPlainTextEdit, QMessageBox
from PyQt5.QtCore import QUrl, QTimer, QThread
from PyQt5.QtGui import QColor, QVector3D
from PyQt5.QtWebEngineWidgets import QWebEngineView

from stokes_polarimeter_form import Ui_MainWindow
from pyqtgraph.opengl import GLMeshItem, GLViewWidget, GLGridItem, MeshData, GLScatterPlotItem, GLTextItem, GLLinePlotItem
from numpy import linspace, zeros, pi, sin, cos, arccos, arctan, array, newaxis, cross, linalg, dot, ones, sqrt, random, asarray, degrees, divide

from sys import argv
from os import path, makedirs

from serial import Serial
from serial.tools import list_ports

from functools import reduce
from datetime import datetime

points = 100
p_data = [[0,0,0,0,0,0]]
stokes_data = [[0,0,0,0]]
lagger_data = [[0,0,0]]
serial_flag = False

project_path = path.dirname(path.realpath(__file__)).replace('\\', '/') + '/'
save_path = path.realpath(path.dirname(argv[0])).replace('\\', '/') + '/'


class PolarizationAnalyzerWindow(QMainWindow, Ui_MainWindow):
    def __init__(self, parent=None):
        global serial_flag, points
    
        super(PolarizationAnalyzerWindow, self).__init__(parent=parent)
        self.setupUi(self)
        
        # SphereBox
        SphereLayout = QHBoxLayout( )
        self.SphereBox.setLayout(SphereLayout)
        
        # TabBox
        TabBoxLayout = QHBoxLayout( )
        self.TabBox.setLayout(TabBoxLayout)   
             
        # Graph      
        self.view_sphere = GLViewWidget( )
        self.view_sphere.setBackgroundColor('w')
        self.view_sphere.opts['distance'] = 3.0
        SphereLayout.addWidget(self.view_sphere)
        
        # Add grid
        g = GLGridItem(color = QColor('gray'), glOptions='translucent')
        #self.view_sphere.addItem(g)
        
        # Add sphere
        self.md_s = MeshData.sphere(rows=15, cols=15)
        self.mesh_s = GLMeshItem(meshdata=self.md_s, drawFaces=False, drawEdges=True, edgeColor=(0.7,0.7,0.7,1.0), color=(0.94,0.94,0.94,1))
        self.view_sphere.addItem(self.mesh_s)
 
        colors =  [[0,0,(3-i*(3.0/points)),0.5-i*0.5/points] for i in range(points)]
        colors.insert(0,[1,0,0,1])
        
        self.scatter = GLScatterPlotItem(pos=[0,0,0], size=5, color = asarray(colors), pxMode=True, glOptions='translucent')
        self.view_sphere.addItem(self.scatter)
        self.laggers = GLScatterPlotItem(pos=[0,0,0], size=5, color = QColor('black'), pxMode=True, glOptions='translucent')
        self.view_sphere.addItem(self.laggers)  
        self.cross_point = GLScatterPlotItem(pos=[0,0,0], size=5, color = QColor('green'), pxMode=True, glOptions='opaque'); 
        self.view_sphere.addItem(self.cross_point)
        
        self.circle_fit = GLLinePlotItem(pos=[[0,0,0],[0,0,0]], width=1, color = QColor('red'),antialias=False, glOptions='opaque')
        self.view_sphere.addItem(self.circle_fit)
        self.circle_line = GLLinePlotItem(pos=[[0,0,0],[0,0,0]], width=2, color = QColor('red'),antialias=False, glOptions='opaque'); 
        self.view_sphere.addItem(self.circle_line)

        # Add arc
        arc_th = linspace(0,2.0*pi,30)
        arc_pos = zeros((len(arc_th), 3))
        arc_pos[:,0] = cos(arc_th)
        arc_pos[:,1] = sin(arc_th)
        arc = GLLinePlotItem(pos=arc_pos, width=2, color = QColor('blue'),antialias=False, glOptions='opaque')
        self.view_sphere.addItem(arc)
        
        # Add axis
        self.axis_items=[ ]
        for item in ([[[0,0,0],[0, 0,-1]],[[0,0,0],[0, -1, 0]],[[0,0,0],[1, 0, 0]]]):
            self.axis_items.append(GLLinePlotItem(pos=item, width=2, color = QColor('black'),antialias=False, glOptions='opaque'))
            self.view_sphere.addItem(self.axis_items[-1])
        
        # Add labls  
        self.add_labls( )        

        
        # InfoWebView
        info =  """
        <html>
          <head>
            <script src="./Mathjax/es5/tex-chtml.js" id="MathJax-script" async></script>
          </head>
            <body>
              <h3>El analizador</h3>
              
              <p>El funcionamiento del analizador se basa en la técnica de división de amplitud. 
              El esquema plantea la utilización de cubos divisores de polarización y láminas 
              retardadoras para descomponer la luz incidente en función de su estado inicial 
              y detectar la intensidad de cada componente de manera independiente mediante
              un conjunto de fotodiodos, con los que se puede determinar en tiempo real el
              vector de Stokes. Las láminas tienen el defecto de volver el dispositivo
              monocromático ya que son únicamente para la longitud de onda utilizada en el
              laboratorio (670 nm). Si se deseara un analizador para un rango amplio de
              longitudes de onda sería necesario utilizar láminas de este tipo que son
              significativamente más caras.</p>

              <p>La luz entra al dispositivo a través de una fibra óptica insertada en un acoplador 
              (modelo KAD11F, marca <em>Thorlabs</em>). A continuación, pasa por un cubo divisor de haz
              50:50 (modelo BS010, marca <em>Thorlabs</em>). La mitad reflejada pasa por un segundo cubo 
              divisor que en este caso es polarizador (PBS 102, <em>Thorlabs</em>), de manera que la 
              componente polarizada horizontalmente es transmitida al fotodiodo \(V_{2}\) 
              (FDS100, marca <em>Thorlabs</em>) mientras que la componente polarizada verticalmente es 
              reflejada hacia el fotodiodo \(V_{1}\).</p>
              
              <p>La mitad transmitida por el primer cubo atraviesa un segundo divisor 50:50 no 
              polarizador que de nuevo divide la amplitud de la luz a la mitad, i.e. a una cuarta
              parte de la intensidad inicial.</p>
              
              <p>La fracción reflejada por el segundo cubo divisor atraviesa una lámina retardadora 
              de \(\lambda/2\) (WPH05ME-670, <em>Thorlabs</em>) colocada previamente con el eje rápido a 
              22.5\(^\circ\) de la horizontal de tal forma que rota la polarización inicial en 
              45\(^\circ\) y así, siguiendo el mismo principio del primer cubo polarizador, la 
              intensidad recibida por los fotodiodos \(V_{3}\) y \(V_{4}\) corresponde a la 
              polarización a 45\(^\circ\). 
              
              <p>Finalmente, la fracción transmitida por el segundo divisor atraviesa una lámina 
              retardadora de \(\lambda /4\) (WPQ05M-670, <em>Thorlabs</em>), que en este caso está colocada
              con el eje rápido a 45\(^\circ\) de la horizontal para convertir la luz circularmente 
              polarizada en linealmente polarizada y así medirse de la misma forma que el resto en 
              el último par de fotodiodos.</p>
              
              <figure>
                <img src='./figures/Polarization_analyzer_pds.png' width=100% />
                <figcaption>
                Esquema óptico del analizador de polarización. La luz en el estado inicial se 
              indica en rojo mientras que la luz con polarización modificada por los retardadores, se 
              indica con el color asignado al retardador. Los haces reflejados se representan con líneas 
              punteadas, los transmitidos con líneas continuas.
                </figcaption>
              </figure>
              
              <p>En resumen, los parámetros de Stokes se obtienen a partir de las potencias medidas 
              \(P_{i}\) por el i-ésimo fotodiodo (proporcionales a los voltajes \(V_{i}\) de los 6 fotodiodos) 
              usando las siguientes expresiones</p>

              <p>\(S_{0}= \sum_{1}^{6} P_{i},\)      \(\qquad S_{1}= \dfrac{P_{2}-P_{1}}{P},\) </p>
              <p>\(S_{2}= \dfrac{ P_{4}-P_{3}}{P},\) \(\qquad S_{3}=  \dfrac{P_{6}-P_{5}}{P}\) </p>
              
              <p> donde recordamos que \(P\) es el grado de polarización de la luz incidente.</p>
      
            </body>
        </html>
        """
        
        self.InfoWebView = QWebEngineView( )
        self.InfoWebView.setHtml(info, baseUrl=QUrl.fromLocalFile(project_path))
        
        TabInfoLayout = QVBoxLayout( )
        self.TabInfo.setLayout(TabInfoLayout)
        TabInfoLayout.addWidget(self.InfoWebView)
        
        
        for item in [self.OverP1, self.OverP2, self.OverP3, self.OverP4, self.OverP5, self.OverP6]:
            item.setStyleSheet('QRadioButton::indicator {border:1px solid}'
                               'QRadioButton::indicator:checked {background-color : red}'
                               'QRadioButton::indicator:unchecked {background-color : transparent}'
                               )
                               
        TabStokesLayout = QVBoxLayout( )
        self.TabStokes.setLayout(TabStokesLayout)
        self.TextStokes = QPlainTextEdit( )
        TabStokesLayout.addWidget(self.TextStokes)
        
        TabStokesNLayout = QVBoxLayout( )
        self.TabStokesN.setLayout(TabStokesNLayout)
        self.TextStokesN = QPlainTextEdit( )
        TabStokesNLayout.addWidget(self.TextStokesN)
        
        self.hold_flag = False
        
        self.work = Worker()
        
        self.sample_time = self.SliderSampleo.value( )
        self.lag_time = self.SliderLagging.value( )
        self.erase_time = self.SliderErasing.value( )
        
        self.timer = QTimer( )
        self.timer.timeout.connect(self.update_data)
        
        self.timer_lag = QTimer( )
        self.timer_lag.timeout.connect(self.update_laggers)
        
        self.timer_erase = QTimer( )
        self.timer_erase.timeout.connect(self.update_erase)
               
        delay = QTimer.singleShot(1000,self.connect)
           
    def solid(self):
        self.view_sphere.removeItem(self.mesh_s)

        if self.checkBox_solid.isChecked( ): 
            self.mesh_s.opts = {
                'color': (0.94,0.94,0.94,1),
                'drawEdges': True,
                'drawFaces': True,
                'edgeColor': (0.7,0.7,0.7,1.0),
                'shader': None,
            }
        else:
            self.mesh_s.opts = {
                'color': (0.94,0.94,0.94,1),
                'drawEdges': True,
                'drawFaces': False,
                'edgeColor': (0.7,0.7,0.7,1.0),
                'shader': None,
            }
        self.view_sphere.addItem(self.mesh_s)
        self.add_labls( )
        
    def add_labls(self):       
    
        self.text_items=[ ]
        for item in (([0,0,1], '\u03c3+'),
                     ([0,0,-1], '\u03c3-'),
                     ([0,1,0], '+ 45'),
                     ([0,-1,0], '- 45'),
                     ([1,0,0], 'H'),
                     ([-1,0,0], 'V')):
            self.text_items.append(GLTextItem(text = item[1], color=QColor('black'), pos=item[0])) 
            self.view_sphere.addItem(self.text_items[-1])
        
    def connect(self):
        global serial_flag
        
        while(serial_flag == False):
            reply = QMessageBox.question(self, 'Question', 'Device not found, try again?' , QMessageBox.Yes | QMessageBox.No)
            if reply == QMessageBox.No:
                self.work.stop( )
                self.close( )
                break
         
        if (serial_flag): 
            self.timer.start(self.sample_time)
            self.timer_lag.start(self.lag_time)
            self.timer_erase.start(self.erase_time)
            
    def sampling(self):
        self.sample_time = self.SliderSampleo.value( )
        self.timer.setInterval(self.sample_time)
        self.groupBoxSampleo.setTitle('Sampling: %d ms'%self.sample_time)
        
    def lagging(self):
        self.lag_time = self.SliderLagging.value( )
        self.timer_lag.setInterval(self.lag_time)
        self.groupBoxLagging.setTitle('Lagging: %d ms'%self.lag_time)    
        
    def erasing(self):
        step = self.SliderErasing.value( )
        
        if   step == 0:  self.erase_time = 0
        elif step == 1:  self.erase_time = 500
        elif step == 2:  self.erase_time = 1000
        elif step == 3:  self.erase_time = 2000
        elif step == 4:  self.erase_time = 3000
        elif step == 5:  self.erase_time = 4000
        elif step == 6:  self.erase_time = 5000
        elif step == 7:  self.erase_time = 6000
        elif step == 8:  self.timer_erase.stop( )
        
        self.timer_erase.setInterval(self.erase_time)
        if step != 8: 
            self.groupBoxErasing.setTitle('Erasing: %.1f s'%(self.erase_time/1000))    
            if not self.timer_erase.isActive( ): self.timer_erase.start(self.erase_time)
        else:
            self.groupBoxErasing.setTitle('Erasing: \u221e')    
     
    def hold(self):
        self.hold_flag = not self.hold_flag
        
        if(self.hold_flag): 
            self.hold_btn.setText("Release")
            self.timer.stop( )
            self.timer_lag.stop( )
            self.timer_erase.stop( )
        else: 
            self.hold_btn.setText("Hold")
            self.timer.start(self.sample_time)
            self.timer_lag.start(self.lag_time)
            self.timer_erase.start(self.erase_time)
            
    def reset(self):  
        global lagger_data
        self.TextStokes.clear( )
        self.TextStokesN.clear( )
        lagger_data = [ ]
        self.laggers.setData(pos=[0,0,0])
        
    def fit_data(self):
        global lagger_data

        if self.fit_btn.isChecked( ):        
            data = asarray(lagger_data)
            if len(data)<2: 
                msgBox = QMessageBox.information(self, 'Infomation', 'No lagging data')
                self.fit_btn.setChecked(False)
                return
           
            data_mean = data.mean(axis=0)
            data_centered = data - data_mean
            U,s,V = linalg.svd(data_centered)
           
            # Normal vector of fitting plane is given by 3rd column in V
            # Note linalg.svd returns V^T, so we need to select 3rd row from V^T
            normal = V[2,:]
            d = -dot(data_mean, normal)  # d = -<p,n>
            #-------------------------------------------------------------------------------
            # (2) Project points to coords X-Y in 2D plane
            #-------------------------------------------------------------------------------
            data_xy = self.rodrigues_rot(data_centered, normal, [0,0,1])
            #-------------------------------------------------------------------------------
            # (3) Fit circle in new 2D coords
            #-------------------------------------------------------------------------------
            xc, yc, r = self.fit_circle_2d(data_xy[:,0], data_xy[:,1])

            #--- Generate circle points in 2D
            circle_th = linspace(0,2.0*pi,100)
            circle_pos = zeros((len(circle_th), 3))
            circle_pos[:,0] = xc + r*cos(circle_th)
            circle_pos[:,1] = yc + r*sin(circle_th)
            
            #--- Transform circle center back to 3D coords
            C = self.rodrigues_rot(array([xc,yc,0]), [0,0,1], normal) + data_mean
            C = C.flatten( )
            
            t = linspace(0, 2*pi, 100)
            u = data[0] - C
            data_fitcircle = self.generate_circle_by_vectors(t, C, r, normal, u)
            
         
            self.circle_fit.setData(pos=data_fitcircle)
            self.cross_point.setData(pos=C)
            self.LabelRadio.setText('r: %1.6f'%r)
        
        else:
            self.circle_fit.setData(pos=[[0,0,0],[0,0,0]])
            self.cross_point.setData(pos=[0,0,0])
            self.LabelRadio.setText('r: ?')
        
    def generate_circle_by_vectors(self, t, C, r, n, u):
        n = n/linalg.norm(n)
        u = u/linalg.norm(u)
        P_circle = r*cos(t)[:,newaxis]*u + r*sin(t)[:,newaxis]*cross(n,u) + C
        return P_circle
        
    def generate_circle_by_angles(self, t, C, r, theta, phi):
        # Orthonormal vectors n, u, <n,u>=0
        n = array([cos(phi)*sin(theta), sin(phi)*sin(theta), cos(theta)])
        u = array([-sin(phi), cos(phi), 0])
        
        # P(t) = r*cos(t)*u + r*sin(t)*(n x u) + C
        P_circle = r*cos(t)[:,newaxis]*u + r*sin(t)[:,newaxis]*cross(n,u) + C
        return P_circle
        
    def fit_circle_2d(self, x, y, w=[]):
        A = array([x, y, ones(len(x))]).T
        b = x**2 + y**2
        
        # Modify A,b for weighted least squares
        if len(w) == len(x):
            W = diag(w)
            A = dot(W,A)
            b = dot(W,b)
        
        # Solve by method of least squares
        c = linalg.lstsq(A,b,rcond=None)[0]
        
        # Get circle parameters from solution c
        xc = c[0]/2
        yc = c[1]/2
        r = sqrt(c[2] + xc**2 + yc**2)
        return xc, yc, r
        
    def rodrigues_rot(self, P, n0, n1):
        # If P is only 1d array (coords of single point), fix it to be matrix
        if P.ndim == 1: P = P[newaxis,:]
        
        # Get vector of rotation k and angle theta
        n0 = n0/linalg.norm(n0)
        n1 = n1/linalg.norm(n1)
        k = cross(n0,n1)
        k = k/linalg.norm(k)
        theta = arccos(dot(n0,n1))
        
        # Compute rotated points
        P_rot = zeros((len(P),3))
        for i in range(len(P)): P_rot[i] = P[i]*cos(theta) + cross(k,P[i])*sin(theta) + k*dot(k,P[i])*(1-cos(theta))

        return P_rot
    
    def update_laggers(self):
        global lagger_data, stokes_data 
         
        lagger_data.insert(0,divide(stokes_data[0][1:],sqrt(stokes_data[0][1]**2 + stokes_data[0][2]**2 + stokes_data[0][3]**2)))
        self.laggers.setData(pos=lagger_data)
        
    def update_erase(self):
        global lagger_data
         
        if len(lagger_data) > 1: 
            del lagger_data[-1]
        else:
            lagger_data = [[0,0,0]]
            
        self.laggers.setData(pos=lagger_data)
        
    def update_data(self): 
        global p_data, stokes_data
             
        data = asarray([divide(i[1:],sqrt(i[1]**2 + i[2]**2 + i[3]**2)) for i in stokes_data])
        self.circle_line.setData(pos=[[0,0,0], data[0]])
        self.scatter.setData(pos=data)
        
        rho =  sqrt(data[0][0]**2 + data[0][1]**2 + data[0][2]**2)
        theta = arccos(data[0][2]/rho)
        phi = arctan(data[0][1] / data[0][0])

        self.LabelRho.setText('ρ: %1.5f'%rho)
        self.LabelTheta.setText('θ: %3.5f°'%degrees(theta))
        self.LabelPhi.setText('φ: %3.5f°'%degrees(phi))
               
        #Overload alerts
        self.OverP1.setChecked(p_data[0][0] > 4e-3) #5
        self.OverP2.setChecked(p_data[0][1] > 4e-3) #5
        self.OverP3.setChecked(p_data[0][2] > 2e-3) #2.5
        self.OverP4.setChecked(p_data[0][3] > 2e-3) #2.5
        self.OverP5.setChecked(p_data[0][4] > 2e-3) #2.5
        self.OverP6.setChecked(p_data[0][5] > 2e-3) #2.5
        
        self.TextStokes.appendPlainText(u'S\u2080={:>.4f}mW    S\u2081={:<.4f}mW    S\u2082={:<.4f}mW    S\u2083={:<.4f}mW'.format(stokes_data[0][0]/1e-3, stokes_data[0][1]/1e-3, stokes_data[0][2]/1e-3, stokes_data[0][3]/1e-3))
        self.TextStokesN.appendPlainText(u'S\u2080={:<12.3f}    S\u2081={:<12.3f}    S\u2082={:<12.3f}    S\u2083={:<12.3f}'.format(stokes_data[0][0]/stokes_data[0][0], stokes_data[0][1]/stokes_data[0][0], stokes_data[0][2]/stokes_data[0][0], stokes_data[0][3]/stokes_data[0][0]))
        
    def save(self):      
        filename = datetime.now( ).strftime('%Y%m%d_%H%M%S')
        filename_path = save_path + 'data/' + filename + '/'
     
        if not path.exists(filename_path): makedirs(filename_path)
        
        
        self.view_sphere.grabFramebuffer( ).save(filename_path + filename + '.png')
        
        lines = str(self.TextStokes.toPlainText( )).rstrip('\n')
        for char in [u'S\u2080',u'S\u2081',u'S\u2082',u'S\u2083','=','mW']: lines = lines.replace(char, '')
        
        with open(filename_path + filename + '.csv', 'w') as fp: 
            fp.write('S0[mW], S1[mW], S2[mW], S3[mW]\n')
            fp.write(lines.replace('   ',','))
            
        lines = str(self.TextStokesN.toPlainText( )).rstrip('\n')
        for char in [u'S\u2080',u'S\u2081',u'S\u2082',u'S\u2083','=','mW']: lines = lines.replace(char, '')
        
        with open(filename_path + filename +'_N.csv', 'w') as fp: 
            fp.write('S0, S1, S2, S3\n')
            fp.write(lines.replace('         ',','))
        
#------------------------------------------------------------------------------ thread
class Worker(QThread):
    def __init__(self):
        super(Worker, self).__init__( )
        
        self.running = False
        self.start( )
         
    def run(self):
        global p_data, points, serial_flag, stokes_data
        self.running = True
        
        while(serial_flag == False):
            for port in list(list_ports.comports( )):
                try:  
                    self.ser = Serial( )
                    self.ser.baudrate = 115200
                    self.ser.timeout= 0.01
                    self.ser.port=port[0]
                    self.ser.open( )
                    
                    self.ser.write(('<i>').encode( ))
                    line = ''.join(chr(i) for i in bytearray(self.ser.readline( )))
                    
                    if (line.split(',')[0] == 'Polarization_analyzer'):
                        serial_flag = True
                        break
                    else: self.ser.close( )
                    
                except Exception as e: print(e)
                
        v=[[ ],[ ],[ ],[ ],[ ],[ ]]             
        while self.running:
            try:  
                self.ser.write(('<r>').encode( ))
                line = ''.join(chr(i) for i in bytearray(self.ser.readline( )))
                line = line.replace('[','')
                line = line.replace(']','')
                
                vprom=[]
                for i,data in enumerate(line.split(',')): 
                    v[i].append(float(data))
                    del v[i][:-points]
                    vprom.append(reduce(lambda a, b:a + b, v[i]) / points)

                for i,ADC in enumerate(vprom, start=1): 
                    
                    ### Here your put the result of the calibration of the photodiodes ###
                    
                    if    i==1: P1 = 1.2968033653428095e-06  * ADC +  3.7680259412021290e-06
                    elif  i==2: P2 = 1.2092812926212499e-06  * ADC +  7.2934964128664330e-08
                    elif  i==3: P3 = 1.3203468312698400e-06  * ADC +  1.6556008864497854e-06
                    elif  i==4: P4 = 1.3103954362061186e-06  * ADC +  2.5665541251775657e-06
                    elif  i==5: P5 = 1.3046381319438402e-06  * ADC +  3.3777689166981804e-06
                    elif  i==6: P6 = 1.2173226504424466e-06  * ADC +  5.1301012725581920e-07
                
                ### Here your put the result of the calibration to obtain the Stokes parameters ###
                
                s0 =  2.2677783618600*P1 + 2.0873706253300*P2
                s1 = -2.2677783618600*P1 + 2.1712784247200*P2
                s2 = -0.0394642125641*P1 + 0.0377849064663*P2 + 4.825616947450*P3 - 4.930836536400*P4 - 0.927906399583*P5 + 0.949198337031*P6
                s3 = -0.0637676073187*P1 + 0.0610541278178*P2 + 0.410277000493*P3 - 0.419222836397*P4 - 5.198585720690*P5 + 5.317873573470*P6
                        
                p_data.insert(0,[P1, P2, P3, P4, P5, P6])
                stokes_data.insert(0,[s0, s1, s2, s3])
                      
                del p_data[points:]            
                del stokes_data[points:]
                #print(stokes_data)
                
            except Exception as e: print(e)
                
        if self.ser.isOpen( ): self.ser.close( )

    def stop(self):    
        self.running = False
        self.wait( )
        self.terminate( )
            

if __name__ == '__main__':
    app = QApplication(argv)
    MainWindow = PolarizationAnalyzerWindow( )
    MainWindow.show( )
    app.exec_( )
    MainWindow.work.stop( )
    