#include "Keypad_I2C.h"
#include "LCD_I2C.h"
#include "FR_RotaryEncoder.h"

// I2C address for MCP23017
// if needed it can be reconfigured at back of the board via A0,A1,A2
// A0   A1  A2 Address
// 0    0   0     0x27 (default)  
// 1    0   0     0x26
// 0    1   0     0x25   
// 1    1   0     0x24
// 0    0   1     0x23   
// 1    0   1     0x22
// 0    1   1     0x21   
// 1    1   1     0x20 
// 0's No connection
// 1's Need soldering
#define I2CADDR 0x27

// Relay configuration 
#define Relay 6 

bool dispTitle = false;

// LDR configuration
#define LDR A7
int Lx = 0;  // LDR reading

// 4x4 Keypad configuration 
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = { // keypad mapping
  {'1','2','3','*'},     //  Button1,  Button2,  Button3,  Button4,
  {'4','5','6','/'},     //  Button5,  Button6,  Button7,  Button8,
  {'7','8','9','-'},     //  Button9,  Button10, Button11, Button12,
  {'.','0','=','+'}      //  Button13, Button14, Button15, Button16
};
byte rowPins[] = {3,2,1,0}; //connections of the row pinouts of the keypad
byte colPins[] = {4,5,6,7}; //connections of the column pinouts of the keypad
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS,I2CADDR);

// Encoder configuration on Nano
int pinSCK = 8; // Interrupt pin for rotary encoder. Can be 2 or 3      
int pinDT = 9; // Better select a pin from 4 to 7
int pinSW = 7; // Interrupt pin for switch. Can be 2 or 3 

// Definition encoder using "FR_RotaryEncoder.h" library 
RotaryEncoder Encoder(pinSCK, pinDT, pinSW);
int lastPosition;
int currentPosition=0;

// NTC 10K confugiration
// These coefficent for NTC 10K give more accurate result in temp calculation
// For a 10 kohm thermistor, the value of constants A, B and C are:
#define NTC A6  // connection pin on Nano
float A = 0.001125308852122;
float B = 0.000234711863267;
float C = 0.000000085663516;
float R1 = 10000;
float Temp;
float R2;
float Rlog;

//LCD configuration
LCD_I2C lcd(I2CADDR);

int LDRCount = 0; 
int NTCCount = 0; 

int menuLength = 5;// number of test

void setup() {
  Serial.begin(9600); 
  keypad.begin( );
  lcd.begin(16, 2); 
  lcd.clear();  
  pinMode(LDR,INPUT); 
  pinMode(NTC,INPUT);      
  pinMode(Relay,OUTPUT); 
 
  Encoder.enableInternalSwitchPullup(); 
  Encoder.setRotaryLogic(true);    // Reverses the CW - CCW direction if needed
  Encoder.setRotaryLimits(0, menuLength-1, false);   // Sets the limits and mode
 
  dispTitle = true;
  displayTest( 0 );
  dispTitle = false;
}

void displayTest(int c)
{
  int Button = 0;
  switch(c) {
  case 0 : if (dispTitle) { 
              lcd.setCursor(0,0);lcd.print("Key test --->   ");
              lcd.setCursor(0,1);lcd.print("Key :           ");
              dispTitle = false; 
           }
           getKey();
           break;
 
  case 1 : if (dispTitle)
           {       
              lcd.setCursor(0,0);lcd.print("LDR test --->   ");
              lcd.setCursor(0,1);lcd.print("LDR :           ");
              dispTitle = false;
            }
           getLDR();  // if reading is less than 350 then the backlight of LCD will be OFF 
           break;
 
  case 2 : if (dispTitle)
           {
              lcd.setCursor(0,0);lcd.print("NTC test --->   ");
              lcd.setCursor(0,1);lcd.print("Temp :          ");
              dispTitle = false;
           }
           getNTC();
           break;
 
  case 3 : if (dispTitle)
           {
              lcd.setCursor(0,0);lcd.print("Relay test ---> ");
              lcd.setCursor(0,1);lcd.print("Relay is        ");                   
              dispTitle = false;
           }
           // if encoder button pressed           
           Button = Encoder.getSwitchState();
           if ( Button  ) 
            { 
              int relayStatus = digitalRead(Relay);
              digitalWrite(Relay,!relayStatus);
              lcd.setCursor(9,1);
              if (!relayStatus) lcd.print("ON ");
              else              lcd.print("OFF");
              delay(250);// wait until the button is released
            }
           break;
  
  case 4 : if (dispTitle)
           {
              lcd.setCursor(0,0);lcd.print("Encoder button->");
              lcd.setCursor(0,1);lcd.print("is          ");      
              dispTitle = false;
           }
           // if a test chosen (button pressed)           
           Button = Encoder.getSwitchState();
           lcd.setCursor(3,1);
           if ( Button )  lcd.print("pressed ");
           else           lcd.print("released");
            
           break;
  default: break;
  }
}

void getKey()
{
  char key = keypad.getKey();
  if (key){ lcd.setCursor(6,1); lcd.print(key); }
}

// reads LDR every 10000th
void getLDR()
{
  if ( LDRCount % 10000 == 0 ){
    Lx = analogRead(LDR);
    lcd.setCursor(6,1);lcd.print(Lx);
    if (Lx < 350) lcd.setBacklight(LOW);
    else lcd.setBacklight(HIGH);
  }
  LDRCount++;
}

//The Steinhart and Hart equation is an empirical expression that has been determined to be the best
//mathematical expression for the resistance - temperature relationship of a negative temperature
//coefficient thermistor. It is usually found explicit in T where T is expressed in degrees Kelvin.
//    Steinhart - Hart Equation 1/T = A+B(LnR)+C(LnR)^3
//       T = Temperature in degrees Kelvin, 
//       LnR is the Natural Log of the measured resistance of the thermistor, 
//       A, B and C are constants.
//For a 10 kohm thermistor, the value of constants A, B and C are:
//A = 0.001125308852122
//B = 0.000234711863267
//C = 0.000000085663516
//The coefficients A, B and C are found by taking the resistance of the thermistor at three
//temperatures and solving three simultaneous equations.

float Thermistor(int Vo) {
  // Vin --------
  //            |
  //           R1 
  //            |
  //             ------ Vo
  //            |
  //           R2 (NTC 10K)
  //            |
  // GND---------
  // This the circuit for NTC on Nano PRO board
  
  R2 = ( (Vo*R1) / (1024-Vo) ); // this is the right formula for nano pro board 
  Rlog = log( R2 ) ;   // convert R2 into log 
  Temp = ( 1 / (A + B * Rlog + C * Rlog * Rlog * Rlog )) ; // in Kelvin
  Temp = Temp - 273.15; // Convert it to Celcius        
  return Temp;   // return it
}


void getNTC()
{
  if (NTCCount % 10000 == 0 ) 
  { 
    lcd.setCursor(7,1); 
    lcd.print( Thermistor( analogRead(NTC) ) );
    lcd.print((char)223);lcd.print("C"); // display celcius sign
  }
  NTCCount++;
}

void loop()
{
  Encoder.update();// update both for switch and rotary
  currentPosition = Encoder.getPosition();  // Rotary 
  while (lastPosition != currentPosition)
  {
    lastPosition = currentPosition;
    //delay(10);
    dispTitle = true;
  }
  displayTest( lastPosition );
}

 
 
