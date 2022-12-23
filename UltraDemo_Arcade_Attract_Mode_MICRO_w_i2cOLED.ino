/*******************************************************************
   A multi-mode Macro keyboard with Arduino Pro Micro using
   row column matrix.
   This sketch is an hardware enable demo (attract) mode for RetroPie. 
   Mode0: RetroArch emulator shortcuts 
   Mode1: Attract mode that will auto exit and select a new game with an adjustable timer. 
   Also features:
   -Arrow keys bound to rotary encoder (can be changed by modifying the encoder profile)
   -i2c OLED readout with key definitions and refresh updates every ~3 seconds (code is not optimized)

   (c) 2022 Ryan Bates

 *******************************************************************/
// ----------------------------
// Standard Libraries
// ----------------------------
#include <Wire.h>
#include <Keyboard.h>2
//#include <LiquidCrystal_I2C.h>
//LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address for a 40 chars and 4 line display
#include <GyverOLED.h>
//GyverOLED<SSD1306_128x32, OLED_BUFFER> oled;
//GyverOLED<SSD1306_128x32, OLED_NO_BUFFER> oled;
//GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;
//GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
//GyverOLED<SSD1306_128x64, OLED_BUFFER, OLED_SPI, 8, 7, 6> oled;
GyverOLED<SSH1106_128x64> oled;
/* TIP! use Use the F macro to store data in flash (program) memory instead of SRAM. Saves SRAM memory (your Global variable memory). 
Serial.print("this string") is stored in SRAM, but Serial.print(F("this string") is moved to program memory.

*/
//const int LCD_NB_ROWS = 4 ;        //for the 4x20 LCD lcd.begin(), but i think this is kinda redundant 
//const int LCD_NB_COLUMNS = 20 ;

#include <Adafruit_NeoPixel.h>
#define PIN            A2
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      7
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int colorUpdate = 0;   //setting a flag to only update colors once when the mode is switched. 
const int b = 3;       // Brightness control variable. Used to divide the RBG vales set for the RGB LEDs. full range is 0-255. 255 is super bright
                       // In fact 255 is obnoxiously bright, so this use this variable to reduce the value. It also reduces the current draw on the USB

// Library with a lot of the HID definitions and methods
// Can be useful to take a look at it see whats available
// https://github.com/arduino-libraries/Keyboard/blob/master/src/Keyboard.h

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------
#include <Encoder.h> //Library for simple interfacing with encoders (up to two)
//low performance ender response, pins do not have interrupts
Encoder RotaryEncoderA(10,16); //the LEFT encoder (encoder A)
//Encoder RotaryEncoderB(14,15);  //the RIGHT encoder (encoder B)

#include <Keypad.h>
// This library is for interfacing with the 4x4 Matrix
//
// Can be installed from the library manager, search for "keypad"
// and install the one by Mark Stanley and Alexander Brevig
// https://playground.arduino.cc/Code/Keypad/

const byte ROWS = 2; //four rows
const byte COLS = 4; //four columns

char previousPressedKey;
boolean hasReleasedKey = false;  //use for button Hold mode. Only works with one button at a time for now...


char keys[ROWS][COLS] = {
  {'1', '2', '3', '4'},  //  the keyboard hardware is  a 2x4 grid...
  {'5', '6', '7', '8'},

};
// The library will return the character inside this array when the appropriate
// button is pressed; then look for that case statement. This is the key assignment lookup table.
// Layout(key/button order) looks like this
//     |------------------------|
//     | [4/8]*                 |     *TRS breakout connection
//     |      (1) [2] [3]       |     ** (x) denotes encoder positions
//     |      [5] [6] [7]  [M]  |     [M] = mode button  A0
//     |------------------------|



// Variables will change:
int modePushCounter = 0;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button

long positionEncoderA  = -999; //encoderA LEFT position variable
long positionEncoderB  = -999; //encoderB RIGHT position variable

const int ModeButton = A0;    // the pin that the Modebutton is attached to
const int pot = A3;         // pot for adjusting attract mode demoTime
long int demoTime;


byte rowPins[ROWS] = {4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9 }; //connect to the column pinouts of the keypad  
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

int demoLoops = 0;                   // counts how many demo loops have occured since active
unsigned long previousMillis = 0;     // values to compare last time interval was checked (For LCD refreshing)
int check_State = 0;                    // state to check and trigger the demo interrupt
int x0;                               // character position limits in LCD (for progress bar animation i didnt write yet?)
int x1;
int x_pos = 0;                 

void setup() {
pinMode(LED_BUILTIN_TX,INPUT); //trick to disable the LEDs on Rx and Tx lines. This sketch makes a USB device, so these pins are busy blinking during USB stuff
pinMode(LED_BUILTIN_RX,INPUT); // might cause complile issues
//  lcd.init(); //initialize the 4x20 lcd
//  lcd.backlight(); //turn on the backlight
//  lcd.begin(LCD_NB_COLUMNS , LCD_NB_ROWS);
  pinMode(ModeButton, INPUT_PULLUP);  // initialize the button pin as a input:
  
  oled.init();              // initialize the oled?
  oled.setContrast(255);   // contrast level 0..255
  
  //oled.setPower(true);    // true/false - включить/выключить дисплей
  oled.flipH(0);       // true/false - flip horizontally
  oled.flipV(0);       // true/false - flip vertically

  
  oled.setScale(1);         // smallest text size (range is 1-4)
  oled.setCursorXY(0,0);  oled.print("Attract Mode uMacroKB");
  oled.setCursorXY(0,30); oled.print("(c) 2022 Ryan Bates");
  oled.setCursorXY(0,50); oled.print("now with OLED!");
  oled.update();
  delay(300);
  oled.clear();

  Keyboard.begin();
    pixels.begin(); // This initializes the NeoPixel library.
}


void loop() {
  char key = keypad.getKey();
  checkModeButton();
  switch (modePushCounter) { // switch between keyboard configurations:

    case 0:    //  Application Alpha or MODE 0
      LCD_update_0();
      setColorsMode0();                                   //indicate what mode is loaded by changing the key colors
      //encoderA_Mode2();
      
      if (key) {
        //Serial.println(key);
        switch (key) {
          case '1': Keyboard.press(KEY_F1);               // retroarch menu F1
              pixels.setPixelColor(0, pixels.Color(0,150,0)); // change the color to green when pressed, wait 100ms so the color change can be observed
               break;
               
          case '2': Keyboard.press(KEY_F2);          // Save State F2
              pixels.setPixelColor(1, pixels.Color(0,150,0)); 
              break;
              
          case '3': Keyboard.press(KEY_F4);           // Load State F4
              pixels.setPixelColor(2, pixels.Color(0,150,0)); 
              break;
              
          case '4': Keyboard.press(KEY_ESC);           //  'esc' Quit game
              break;
              
          case '5': Keyboard.press(KEY_F8);            // Screen Shot F8
              pixels.setPixelColor(3, pixels.Color(0,150,0)); 
              break;
              
          case '6': Keyboard.press(KEY_F3);            // F3 Show FPS
              pixels.setPixelColor(4, pixels.Color(0,150,0)); 
              break;
              
          case '7': Keyboard.press(32);              // 'space bar' Fast Forward (button is toggle type)
              pixels.setPixelColor(5, pixels.Color(0,150,0)); 
              break;
              
          case '8': Keyboard.press('h');    break;     // 'h' Reset 
  
             
        }
        pixels.show();                                      //update the color after the button press
        delay(100); Keyboard.releaseAll(); // this releases the buttons
        colorUpdate = 0;  //call the color update to change the color back to Mode settings   
      }
      //Serial.print("Mode0");
      break;

   
    case 1: //Auto Attract (DEMO) Mode
        setColorsMode1();
        getDemoTime();
        key_sequencerRPi();
      if (key) {
        switch (key) {
                                }
               }
          delay(1); Keyboard.releaseAll(); // this releases the buttons
         break;
      delay(1);  // delay in between reads for stability

  }
}
//============================Sub Routines========================================================



void checkModeButton() {
  buttonState = digitalRead(ModeButton);
  if (buttonState != lastButtonState) { // compare the buttonState to its previous state
    if (buttonState == LOW) { // if the state has changed, increment the counter
      // if the current state is LOW then the button cycled:
      modePushCounter++;
      colorUpdate = 0;      // set the color change flag ONLY when we know the mode button has been pressed. 
      oled.clear(); //clearing the LCD between modes
    }
    delay(50); // Delay a little bit to avoid bouncing
  }
  lastButtonState = buttonState; // save the current state as the last state, for next time through the loop
  if (modePushCounter > 1) { //reset the counter after 2 presses (remember we start counting at 0)
    modePushCounter = 0;
    oled.clear();
  }
}




//-------This section converts demo loop idle seconds to MM:SS time format
//------- and runs a key command sequence to 1) quit a game, 2) select a new game, 3) launch the new game.
//------- It does this without relying on delay() for the demoTimer. 

void key_sequencerRPi() {
  unsigned long currentMillis = millis();
  oled.setCursorXY(0, 1); oled.print(" --Auto Demo Active--");
  oled.line(0,10,128,10);
  oled.setCursorXY(0,41); oled.print("Loop counter: "); oled.print(demoLoops);
  oled.setCursorXY(0, 55); oled.print("Next game in: ");
  //this section is the demo time counting down. has leading zero for minutes and seconds. don't mess with this. 
  if ((previousMillis / 1000) + (demoTime / 1000) - (currentMillis / 1000) <= 99) {
    oled.print("0"); //leading zero
  }
  if ((previousMillis / 1000) + (demoTime / 1000) - (currentMillis / 1000) <= 9) {
    oled.print("0"); //leading zero
  }
  oled.print( (previousMillis / 1000) + (demoTime / 1000) - (currentMillis / 1000) ); //the actual demo time left
  // end of the timer with formatting

  //------loading bar animation for elasped time. auto scales based on the demoTime pot.
  x0=((currentMillis - previousMillis) /1000); //elapsed time
  x_pos = map(x0,0,x1,5,128);
  x_pos = round(x_pos);
  oled.setCursorXY(0, 16); oled.print("[");oled.setCursorXY(123, 16);oled.print("]");
  oled.setCursorXY(x_pos, 16); oled.print("="); //oled.print(x_pos);  //oled.print(x1);
  oled.update();
  
  //----------------- cycle that tracks time.....
  //----------------- a note about this cycle, this follows the example sketch "Blink without Delay"
  if (currentMillis - previousMillis >= demoTime) {                     //if the elasped time is a multiple of the demoTime...
    previousMillis = currentMillis;                                    // save the last time you checked the interval
    //... trigger the 'Select a New Game Sequence'
    if (check_State == 0) {                                             //do the update only once per interval
      oled.clear();
      colorUpdate=0;setColorsMode2();
      oled.setCursorXY(0, 13); oled.print(F("exiting game... [ESC]"));   oled.update();
      Keyboard.press(KEY_ESC); delay(50); Keyboard.release(KEY_ESC); delay(1000);        // ESC quits game
      oled.setCursorXY(0, 25); oled.print(F("selecting game...")); 
      oled.setCursorXY(0, 44); oled.print(F("[down], [a]")); oled.update();delay(1500);  
      Keyboard.press(KEY_DOWN_ARROW); Keyboard.release(KEY_DOWN_ARROW); delay(100);       // KEY_DOWN_ARROW selects next game in list
      Keyboard.write('a'); delay(10);                                                     // a launches game
      oled.clear();
      demoLoops = demoLoops + 1;
      oled.update();
      colorUpdate=0;
      check_State = 1;
         }

    else {
      check_State = 0;
      oled.clear();
      colorUpdate=0;setColorsMode2();
      oled.setCursorXY(0, 13); oled.print(F("exiting game... [ESC]"));
      Keyboard.press(KEY_ESC); delay(50); Keyboard.release(KEY_ESC); delay(1000);        // ESC quits game
      oled.setCursorXY(0, 25); oled.print(F("selecting game...")); 
      oled.setCursorXY(0, 44); oled.print(F("[down], [a]")); oled.update();delay(1500);   
      Keyboard.press(KEY_DOWN_ARROW); Keyboard.release(KEY_DOWN_ARROW); delay(100);      // down_arrow selects next game in list
      Keyboard.write('a'); delay(10);                                                    // a launches game
      oled.clear();
      demoLoops = demoLoops + 1;
      oled.update();
      colorUpdate=0;
        }

       if (check_State == 2) {}
        }}

void getDemoTime() {
  demoTime = analogRead(pot);
  demoTime = map(demoTime, 0, 1023, 30, 599); //30 to 599 ms
  demoTime = demoTime * 1000; //30 to 599 sec 
   x1 = demoTime/1000; //cap for OLED position animation
  //prints time as raw seconds
  oled.setCursorXY(0, 29);  oled.print(F("Demo Time: ")); // lcd.print(demoTime / 1000); lcd.print("s "); 
    //  if ((demoTime / 1000) <= 100) {
    //    lcd.print(" "); //overwrite number space when missing leading zero
    //  }
    
  // prints demo time as MM:SS
  long minRemainder;
  minRemainder = (demoTime / 1000) % 60 ;
  oled.print(round(demoTime / 60000)); oled.print(F(":"));
  if (minRemainder <= 9) {
    oled.print("0"); //add leading zero for remainder seconds under 10
  }
  oled.print(round(minRemainder));
  oled.update();
}

void setColorsMode0(){
  if (colorUpdate == 0){                                           // have the neopixels been updated?
      for(int i=0;i<NUMPIXELS-1;i++){      //  Red,Green,Blue      // pixels.Color takes RGB values; range is (0,0,0) to (255,255,255)
        pixels.setPixelColor(i, pixels.Color(0,   0,   150));      // Moderately bright blue color.
        pixels.setPixelColor(6, pixels.Color(0,   150,   0));
         }                                                         
      pixels.show();                                               // This pushes the updated pixel color to the hardware.
      delay(5);                                                    // Delay for a period of time (in milliseconds).
      colorUpdate=1;   }                                           // Mark the color flag so neopixels are no longer updated in the loop
}

void setColorsMode1(){
  if (colorUpdate == 0){                                     // have the neopixels been updated?
      pixels.setPixelColor(0,  pixels.Color( 80,  0,200));    //gradient mix
      pixels.setPixelColor(1,  pixels.Color( 10,  0,200));    //gradient mix
      pixels.setPixelColor(2,  pixels.Color( 20,  0,200));
      pixels.setPixelColor(3,  pixels.Color( 40,  0,200));
      pixels.setPixelColor(4,  pixels.Color( 60,  0,200));
      pixels.setPixelColor(5,  pixels.Color( 80,  0,200));
      pixels.setPixelColor(6,  pixels.Color(120,  0,0));
      //pixels.setPixelColor(7,  pixels.Color(120,  0,  0));
      
      pixels.show();
      colorUpdate=1;              }                           // neoPixels have been updated. 
                                                              // Set the flag to 1; so they are not updated until a Mode change
}

void setColorsMode2(){
  if (colorUpdate == 0){                                           // have the neopixels been updated?
      for(int i=0;i<NUMPIXELS-1;i++){      //  Red,Green,Blue      // pixels.Color takes RGB values; range is (0,0,0) to (255,255,255)
        pixels.setPixelColor(i, pixels.Color(150,   0,   0));      // Moderately bright blue color.
        pixels.setPixelColor(6, pixels.Color(0,   150,   0));
         }                                                         
      pixels.show();                                               // This pushes the updated pixel color to the hardware.
      delay(5);                                                    // Delay for a period of time (in milliseconds).
      colorUpdate=1;   }                                           // Mark the color flag so neopixels are no longer updated in the loop
}

void LCD_update_0() { //This method is less heavy on tying up the arduino cycles to update the LCD; instead 
                      //this updates the LCD every 3 seconds. Putting the LCD.write commands
                      //in the key function loops breaks the 'feel' and responsiveness of the keys. 
  unsigned long currentMillis = millis();

  //================= a note about this cycle, this follows the example sketch "Blink without Delay"

  if (currentMillis - previousMillis >= 3000) {                       //if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
  
    if (check_State == 0) {                                            //do the update only once per interval
 oled.clear();
 drawKeyboard(); 
 oled.setCursorXY(3,20);  oled.print(F(" F1"));   // key 1,, also stored this string into program memory instead of SRAM (save memory for Global varibles )
 oled.setCursorXY(35,20); oled.print(F(" F2"));   // key 2
 oled.setCursorXY(67,20); oled.print(F(" F4"));   // key 3
 oled.setCursorXY(3,44);  oled.print(F(" F8"));   // key 5
 oled.setCursorXY(35,44); oled.print(F(" F3"));   // key 6
 oled.setCursorXY(67,44); oled.print(F(">> "));    // key 7
 oled.setCursorXY(100,38); oled.print(F("Mode")); // key Mode
 oled.update();
      check_State = 1;    }

    else {
      check_State = 0;
 //oled.clear();    
 drawKeyboard();  
 oled.setCursorXY(3,20);  oled.print(F("menu")); // key 1
 oled.setCursorXY(35,20); oled.print(F("save")); // key 2
 oled.setCursorXY(67,20); oled.print(F("load")); // key 3
 oled.setCursorXY(3,44);  oled.print(F("PrtS"));  // key 5
 oled.setCursorXY(35,44); oled.print(F("FPS"));    // key 6
 oled.setCursorXY(67,44); oled.print(F(" >>"));    // key 7
 oled.setCursorXY(100,34); oled.print(F("Mode"));// key mode
 oled.update();
  }
}}




void LCD_update_1() { //This method is less heavy on tying up the arduino cycles to update the LCD; instead 
                      //this updates the LCD every 3 seconds. Putting the LCD.write commands
                      //in the key function loops breaks the 'feel' and responsiveness of the keys. 
  unsigned long currentMillis = millis();

  //================= a note about this cycle, this follows the example sketch "Blink without Delay"

  if (currentMillis - previousMillis >= 3000) {                       //if the elasped time greater than 3 seconds
    previousMillis = currentMillis;                                   // save the last time you checked the interval
  
    if (check_State == 0) {  //do the update only once per interval
 drawKeyboard();
 oled.setCursorXY(3,20);  oled.print(F("1"));
 oled.setCursorXY(35,20); oled.print(F("2")); 
 oled.setCursorXY(67,20); oled.print(F("3"));
 oled.setCursorXY(3,48);  oled.print(F("5"));
 oled.setCursorXY(35,48); oled.print(F("6")); 
 oled.setCursorXY(67,48); oled.print(F("7"));
 oled.setCursorXY(100,38); oled.print(F("mode"));
 oled.update();
      check_State = 1;
    }

    else {
      check_State = 0;
 drawKeyboard(); 
 oled.setCursorXY(3,20);  oled.print(F("A"));
 oled.setCursorXY(35,20); oled.print(F("B")); 
 oled.setCursorXY(67,20); oled.print(F("C"));
 oled.setCursorXY(3,48);  oled.print(F("E"));
 oled.setCursorXY(35,48); oled.print(F("F"));
 oled.setCursorXY(67,48); oled.print(F("G"));
 oled.setCursorXY(100,38); oled.print(F("Mode"));
 oled.update();
    }
  }
}

//---encoder profiles-----------
void encoderA() {
  long newPos = RotaryEncoderA.read() / 4; //When the encoder lands on a valley, this is an increment of 4.

  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_LEFT_ARROW);
    Keyboard.release(KEY_LEFT_ARROW);
  }

  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_RIGHT_ARROW);
    Keyboard.release(KEY_RIGHT_ARROW);
  }
}


void encoderA_Mode2() {
  long newPos = RotaryEncoderA.read() / 4; //When the encoder lands on a valley, this is an increment of 4.

  if (newPos != positionEncoderA && newPos > positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_UP_ARROW);
    Keyboard.release(KEY_UP_ARROW);
  }

  if (newPos != positionEncoderA && newPos < positionEncoderA) {
    positionEncoderA = newPos;
    //Serial.println(positionEncoderA);
    Keyboard.press(KEY_DOWN_ARROW);
    Keyboard.release(KEY_DOWN_ARROW);
  }
}

void drawKeyboard(){
 oled.setScale(1);
 oled.setCursorXY(10,1);  oled.print("Mode 0: RetroArch");
 oled.setCursorXY(27,35); 
 oled.setCursorXY(27,67); 
 oled.rect(1, 12, 97, 60, OLED_STROKE); //draw the keyboard grid
 oled.rect(97,24,126, 48, OLED_STROKE);
 oled.line(33,12,33, 60);
 oled.line(65, 12, 65, 60);
 oled.line(1, 36, 97, 36);
 oled.update();
}
