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

PS2X ps2x; // create PS2 Controller Class

//right now, the library does NOT support hot pluggable controllers, meaning 
//you must always either restart your Arduino after you connect the controller, 
//or call config_gamepad(pins) again after connecting the controller.

int error = 0;
byte type = 0;
byte vibrate = 0;

unsigned char ps2data[21];

void setup(){
 
  Serial.begin(57600);
  
  delay(300);  //added delay to give wireless ps2 module some time to startup, before configuring it
   
  //CHANGES for v1.6 HERE!!! **************PAY ATTENTION*************
  
  //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
}

void loop() {
  /* You must Read Gamepad to get new values and set vibration values
     ps2x.read_gamepad(small motor on/off, larger motor strenght from 0-255)
     if you don't enable the rumble, use ps2x.read_gamepad(); with no values
     You should call this at least once a second
   */  
  if(error == 1) //skip loop if no controller found
    return; 
  

  //DualShock Controller
   if (Serial.available() > 0)
   {
      int vibrate1 = Serial.read();
      int vibrate2 = Serial.read();
      ps2x.read_gamepad(vibrate1, vibrate2); //read controller and set large motor to spin at 'vibrate' speed

      /*ps2x.getData(ps2data);

      //Serial.write(ps2data);
      for(int i=0;i<21;i++){
        Serial.write(ps2data[i]);
        //Serial.print(ps2data[i], HEX);
      }*/
      ps2x.writeData();
  }
  delay(50);  
}
