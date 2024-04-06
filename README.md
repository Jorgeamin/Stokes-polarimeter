# Stokes-polarimeter
Supplement documents for article "Homemade open-source full-Stokes polarimeter based on division of amplitude"

* [python](python): This folder contains python scripts for user interface.\
    main application file **stokes_polarimeter.py**\
     -line 517:  ### Here your put the result of the calibration of the photodiodes ###\
     -line 526:  ### Here your put the result of the calibration to obtain the Stokes parameters ###
  
* [firmware](firmware): This folder contains MPLAB Harmony 3 firmware that should be installed in an ATSAML21E16B\
    file **main.c**\
	-line 97:  /// Here your put the result of the calibration of the photodiodes ///\
	-line 106: /// Here your put the result of the calibration to obtain the Stokes parameters ///
	
* [pcb](pcb): This folder contains the Autodesk Eagle electronic design files.
* [3d](3d): This folder contains the 3D Autodesk Inventor, STEP and STL files.
* [pdf](pdf): This folder contains the electronic diagrams and the 3D assembly in PDF format.

-this repository uses submodules