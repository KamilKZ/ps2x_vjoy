# PS2X to vJoy (xJoy) #

Allows you to connect a Playstation 2 controller to an Arduino, and use it for games in windows and linux. 

Written solely for that purpose. [because who spends money on something when you can get it free!]

## Introduction ##

Uses PS2 controller Arduino Library v1.8 by Shutter, Bill Porter, Eric Wetzel, Kurt Eckhardt.

Official [vJoy](http://vjoystick.sourceforge.net/site/) SDK for Windows support

and

[Linux vJoy port](https://github.com/zpon/Linux-Virtual-Joystick-cpp) for Linux support

## Installation ##

####Windows####

* Compile main solution
* Copy vJoyInterface.dll from the lib folder for the compiled exe
* Done!

####Linux####

* Download sources for [Linux vJoy port](https://github.com/zpon/Linux-Virtual-Joystick-cpp) 
* Compile

## Usage ##

#### Windows ####

Just start the exe and you're ready to go! (or look below if it does not work)

The Windows version provides a few command line options:

* `-mode <mode name>`
	* `xjoy` (default) - attempts to emulate an xbox360 controller with the buttons and axes.
	* `xjoy2` - similar to xjoy but uses combined axes (Right Analog Y) in place of the two triggers, this is useful for some racing games such as Need for Speed which only has one combined axis for throttle and brake. 
	* `digital` (not implemented) - all buttons digital
	* `analog` (not implemented) - all buttons pressure sensitive

	
* `-id <number 1...16>`
	* vjoy target joystick number (default is 1)

	
* `-port <port name>`
	* Serial port name for Arduino communication (default is COM4) 

	
Example:
`vJoyDualShock2.exe -mode xjoy2 -id 2 -port COM5`


#### Linux ####

* Run `vjoy arduino_serial_drive.py`

There's options for serial port inside the python script but not much configuration other than that.


## Contact ##

[kamil.zmich@gmail.com](mailto:kamil.zmich@gmail.com)

