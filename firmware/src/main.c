#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes

#define SSD1306_I2C_ADDRESS 0x3c
#include "oled.h"

#include <math.h>

uint8_t usb_flag = 0, T0_count = 0;
//------------------------------------------------------------------------------USBconnected
void USBconnected( bool flag ){
    if (flag) {
        TC0_TimerStop( );
        usb_flag = 1;
    } 
    else{
        TC0_TimerStart( );
        usb_flag = 0;
    }  
}
//------------------------------------------------------------------------------TC0_Callback_InterruptHandler
void TC0_Callback_InterruptHandler(TC_TIMER_STATUS status, uintptr_t context){
    T0_count ++;   
}
//------------------------------------------------------------------------------main
int main ( void ){    
    char text[50];
    uint8_t i=0;
    
    uint8_t adc_channel = 0;
    uint32_t adc_result[6] = {0,0,0,0,0,0};
    float P1, P2, P3, P4, P5, P6;
    float s0, s1, s2, s3, sp;
    float rho, theta, phi; 
    
    SYS_Initialize ( NULL );
    
    TC0_TimerCallbackRegister(TC0_Callback_InterruptHandler, (uintptr_t)NULL);
    TC0_TimerStop( );
    
    SYS_Tasks ( );
    SSD1306_Begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
    SSD1306_ClearDisplay( );
    
    ADC_Enable();
    SYSTICK_TimerStart();

    sprintf(text, "STOKES POLARIMETER");
    SSD1306_DrawText(7, 0, text,1);

    sprintf(text, "Laboratorio de");
    SSD1306_DrawText(0, 20, text,1);
    sprintf(text, "Materia Ultrafr%ca",132);
    SSD1306_DrawText(0, 32, text,1);
    sprintf(text, "Instituto de F%csica,",132);
    SSD1306_DrawText(0, 44, text,1);
    sprintf(text, "UNAM, M%cxico.",131);
    SSD1306_DrawText(0, 56, text,1);
    SSD1306_Display( );
    
    for(i=0;i<30;i++){ SYSTICK_DelayMs(100); SYS_Tasks ( ); }
    
    while ( true ){
        SYS_Tasks ( );
        
        if (usb_flag == 1){
            usb_flag = 2;
            SSD1306_ClearDisplay( );
            sprintf(text, "USB connected...");
            SSD1306_DrawText(0, 20, text,1);
            SSD1306_Display( );
        }
        
        if (T0_count >= 5){
            T0_count = 0;
            
            for(adc_channel=0; adc_channel<6; adc_channel++) adc_result[adc_channel] = 0;
            
            for(i=0;i<100;i++){
                for(adc_channel=0; adc_channel<6; adc_channel++){
                    switch(adc_channel){
                        case 0: ADC_ChannelSelect(ADC_POSINPUT_AIN5 , ADC_NEGINPUT_GND); break;
                        case 1: ADC_ChannelSelect(ADC_POSINPUT_AIN4 , ADC_NEGINPUT_GND); break;
                        case 2: ADC_ChannelSelect(ADC_POSINPUT_AIN0 , ADC_NEGINPUT_GND); break;
                        case 3: ADC_ChannelSelect(ADC_POSINPUT_AIN1 , ADC_NEGINPUT_GND); break;
                        case 4: ADC_ChannelSelect(ADC_POSINPUT_AIN6 , ADC_NEGINPUT_GND); break;
                        case 5: ADC_ChannelSelect(ADC_POSINPUT_AIN7 , ADC_NEGINPUT_GND); break;
                    }
                    ADC_ConversionStart( );
                    while(!ADC_ConversionStatusGet( ));
                    adc_result[adc_channel] += ADC_ConversionResultGet( );
                }
            }

            /// Here your put the result of the calibration of the photodiodes ///

            P1 = 1.2968033653428095e-06  * (float)adc_result[0]/100.0 +  3.7680259412021290e-06;
            P2 = 1.2092812926212499e-06  * (float)adc_result[1]/100.0 +  7.2934964128664330e-08;
            P3 = 1.3203468312698400e-06  * (float)adc_result[2]/100.0 +  1.6556008864497854e-06;
            P4 = 1.3103954362061186e-06  * (float)adc_result[3]/100.0 +  2.5665541251775657e-06;
            P5 = 1.3046381319438402e-06  * (float)adc_result[4]/100.0 +  3.3777689166981804e-06;
            P6 = 1.2173226504424466e-06  * (float)adc_result[5]/100.0 +  5.1301012725581920e-07;
 
            /// Here your put the result of the calibration to obtain the Stokes parameters ///

            s0 =  2.2677783618600*P1 + 2.0873706253300*P2;
            s1 = -2.2677783618600*P1 + 2.1712784247200*P2;
            s2 = -0.0394642125641*P1 + 0.0377849064663*P2 + 4.825616947450*P3 - 4.930836536400*P4 - 0.927906399583*P5 + 0.949198337031*P6;
            s3 = -0.0637676073187*P1 + 0.0610541278178*P2 + 0.410277000493*P3 - 0.419222836397*P4 - 5.198585720690*P5 + 5.317873573470*P6;

            sp = sqrtf(powf(s1,2.0) + powf(s2,2.0) + powf(s3,2.0));

            rho =   sqrtf(powf(s1/sp,2.0) + powf(s2/sp,2.0) + powf(s3/sp,2.0));
            
            theta = acosf((s3/sp)/(rho))     * 180/M_PI ;
            phi =   atanf((s2/sp) / (s1/sp)) * 180/M_PI ;
            

            SSD1306_ClearDisplay( );
            
            sprintf(text, "s0=%.04gmW",s0*1e3);         SSD1306_DrawText(0, 0, text,1);
            sprintf(text, "s1=%.04gmW",s1*1e3);         SSD1306_DrawText(0, 10, text,1);
            sprintf(text, "s2=%.04gmW",s2*1e3);         SSD1306_DrawText(0, 20, text,1);
            sprintf(text, "s3=%.04gmW",s3*1e3);         SSD1306_DrawText(0, 30, text,1);

            sprintf(text, "%c=%0.4g",127, rho);         SSD1306_DrawText(70, 30, text,1);//rho
            sprintf(text, "%c=%0.4g%c",126,theta,129);  SSD1306_DrawText(70, 40, text,1);//theta
            sprintf(text, "%c=%0.4g%c",128,phi,129);    SSD1306_DrawText(70, 50, text,1);//phi
            SSD1306_Display( );
        } 
    }
    return ( EXIT_FAILURE );
}

