#include <PS2_pad_lib.h>  //for v1.6

/******************************************************************
 * set pins connected to PS2 controller:
 *   - 1e column: original 
 *   - 2e colmun: Stef?
 * replace pin numbers by the ones you use
 ******************************************************************/
#define PS2_DAT        9  //14    
#define PS2_CMD        11  //15
#define PS2_SEL        10  //16
#define PS2_CLK        12  //17

/******************************************************************
 * select modes of PS2 controller:
 *   - pressures = analog reading of push-butttons 
 *   - rumble    = motor rumbling
 * uncomment 1 of the lines for each mode selection
 ******************************************************************/
//#define pressures   true
#define pressures   true
//#define rumble      true
#define rumble      true

#define PTCL_INPUT  1
#define PTCL_OUTPUT 2

// Controller inputs
#define PTCL_TOGGLE_ANALOG 1
#define PTCL_MOTOR_POWER 2
#define PTCL_CALIBRATE 4

// Controller outputs
//#define PTCL_DIGITAL_DOWN 1
//#define PTCL_DIGITAL_UP 2
#define PTCL_DIGITAL 4
#define PTCL_ANALOG_STICK 8
#define PTCL_ANALOG_DPAD 16
#define PTCL_ANALOG_FACE 32
#define PTCL_ANALOG_BACK 64

PS2X ps2x; // create PS2 Controller Class

//right now, the library does NOT support hot pluggable controllers, meaning 
//you must always either restart your Arduino after you connect the controller, 
//or call config_gamepad(pins) again after connecting the controller.

int error = 0;
byte type = 0;
byte vibrate = 0;

void setup(){
 
  Serial.begin(57600);
  
  delay(300);  //added delay to give wireless ps2 module some time to startup, before configuring it
   
  //CHANGES for v1.6 HERE!!! **************PAY ATTENTION*************
  
  //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
  
  if(error == 0){
    Serial.print("Found Controller, configured successful ");
    Serial.print("pressures = ");
	if (pressures)
	  Serial.println("true ");
	else
	  Serial.println("false");
	Serial.print("rumble = ");
	if (rumble)
	  Serial.println("true)");
	else
	  Serial.println("false");
    Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
    Serial.println("holding L1 or R1 will print out the analog stick values.");
    Serial.println("Note: Go to www.billporter.info for updates and to report bugs.");
  }  
  else if(error == 1)
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
   
  else if(error == 2)
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");

  else if(error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
  
//  Serial.print(ps2x.Analog(1), HEX);
  
  type = ps2x.readType(); 
  switch(type) {
    case 0:
      Serial.print("Unknown Controller type found ");
      break;
    case 1:
      Serial.print("DualShock Controller found ");
      break;
    case 2:
      Serial.print("GuitarHero Controller found ");
      break;
	case 3:
      Serial.print("Wireless Sony DualShock Controller found ");
      break;
   }
}

void loop() {
  /* You must Read Gamepad to get new values and set vibration values
     ps2x.read_gamepad(small motor on/off, larger motor strenght from 0-255)
     if you don't enable the rumble, use ps2x.read_gamepad(); with no values
     You should call this at least once a second
   */  
  if(error == 1) //skip loop if no controller found
    return; 
  
  if(type == 2){ //Guitar Hero Controller
     
  }
  else { //DualShock Controller
    ps2x.read_gamepad(vibrate, vibrate); //read controller and set large motor to spin at 'vibrate' speed

    vibrate = 0;
    if(ps2x.Button(PSB_CIRCLE)){               //will be TRUE if button was JUST pressed
        vibrate = 255;  
    }

    if(ps2x.NewButtonState()){
      Serial.print(PTCL_OUTPUT);
      Serial.print(PTCL_DIGITAL);
      Serial.println(ps2x.ButtonDataByte(), HEX);
    }
    
    if(ps2x.NewAnalogStickState()){
      Serial.print(PTCL_OUTPUT);
      Serial.print(PTCL_ANALOG_STICK);
      Serial.println(ps2x.AnalogStickByte(), HEX);
    }  
    
    if(ps2x.NewAnalogDPadState()){
      //Serial.print(PTCL_OUTPUT);
      //Serial.print(PTCL_ANALOG_DPAD);
      Serial.print(ps2x.Analog(PSAB_PAD_UP), DEC);
      Serial.print(", ");
      Serial.print(ps2x.Analog(PSAB_PAD_DOWN), DEC);
      Serial.print(", ");
      Serial.print(ps2x.Analog(PSAB_PAD_LEFT), DEC);
      Serial.print(", ");
      Serial.println(ps2x.Analog(PSAB_PAD_RIGHT), DEC);
      //Serial.println(ps2x.AnalogDPadByte(), HEX);
    }  
    
    if(ps2x.NewAnalogFaceButtonState()){
      Serial.print(ps2x.Analog(PSAB_TRIANGLE), DEC);
      Serial.print(", ");
      Serial.print(ps2x.Analog(PSAB_CIRCLE), DEC);
      Serial.print(", ");
      Serial.print(ps2x.Analog(PSAB_CROSS), DEC);
      Serial.print(", ");
      Serial.println(ps2x.Analog(PSAB_SQUARE), DEC);
    }  
    
    if(ps2x.NewAnalogBackButtonState()){
      Serial.print(ps2x.Analog(PSAB_L2), DEC);
      Serial.print(", ");
      Serial.print(ps2x.Analog(PSAB_R2), DEC);
      Serial.print(", ");
      Serial.print(ps2x.Analog(PSAB_L1), DEC);
      Serial.print(", ");
      Serial.println(ps2x.Analog(PSAB_R1), DEC);
    }  
  }
  delay(50);  
}
