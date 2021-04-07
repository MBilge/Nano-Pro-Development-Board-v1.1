#include "LCD_I2C.h"

/*]
  LCD_I2C High Performance i2c LCD driver for MCP23017
  by Mehmet S. Bilge m.selcukbilge@hotmail.com inspired by 
  LiquidTWI by Matt Falcon (FalconFour) / http://falconfour.com
*/

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#if defined (__AVR_ATtiny84__) || defined(__AVR_ATtiny85__) || (__AVR_ATtiny2313__)
#include "TinyWireM.h"
#define Wire TinyWireM
#else
#include <Wire.h>
#endif
#if defined(ARDUINO) && (ARDUINO >= 100) //scl
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//  Pinout for LCD interfacing with MCP23017
//  LCD ->  MCP23017
//  BL  ->   8 (PB0) (backlight)
//  D4  ->   9 (PB1)
//  D5  ->   10(PB2)
//  D6  ->   11(PB3)
//  D7  ->   12(PB4)
//  E   ->   13(PB5)
//  RS  ->   15(PB7)
//  RW  ->   GND
//

// Data pins on MCP23017
#define M17_BIT_D4 0x0200  // pin 9
#define M17_BIT_D5 0x0400  // pin 10
#define M17_BIT_D6 0x0800  // pin 11
#define M17_BIT_D7 0x1000  // pin 12 

// Control Pins on MCP23017
#define M17_BIT_EN 0x2000  // pin 13
#define M17_BIT_RW 0x4000  // pin 14
#define M17_BIT_RS 0x8000  // pin 15
#define M17_BIT_BL 0x100   // pin 8 

static inline void wiresend(uint8_t x) {
#if ARDUINO >= 100
  Wire.write((uint8_t)x);
#else
  Wire.send(x);
#endif
}

static inline uint8_t wirerecv(void) {
#if ARDUINO >= 100
  return Wire.read();
#else
  return Wire.receive();
#endif
}



// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 0; 4-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1 
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LCD_I2C constructor is called). This is why we save the init commands
// for when the sketch calls begin(), except configuring the expander, which
// is required by any setup.

LCD_I2C::LCD_I2C(uint8_t i2cAddr) {
  // Construction for LCD
  _backlightval = LCD_BACKLIGHT;
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS; // in case they forget to call begin() at least we have something
  dotsize = LCD_5x10DOTS;

  _i2cAddr = i2cAddr;
}

void LCD_I2C::begin(uint8_t cols, uint8_t rows){

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  delay(50);

  Wire.begin();

  Wire.beginTransmission(MCP23017_ADDRESS | _i2cAddr);
  wiresend(IODIRB);
  wiresend(0x00); 
  Wire.endTransmission();

  if (rows > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numrows = rows;
  _currline = 0;

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != 0) && (rows == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // put the LCD into 4 bit mode
  // start with a non-standard command to make it realize we're speaking 4-bit here
  // per LCD datasheet, first command is a single 4-bit burst, 0011.
  //-----
  //  we cannot assume that the LCD panel is powered at the same time as
  //  the arduino, so we have to perform a software reset as per page 45
  //  of the HD44780 datasheet - (kch)


  for (uint8_t i=0;i < 3;i++) {
    burstBits8b((M17_BIT_EN|M17_BIT_D5|M17_BIT_D4) >> 8);
    burstBits8b((M17_BIT_D5|M17_BIT_D4) >> 8);
  }
  burstBits8b((M17_BIT_EN|M17_BIT_D5) >> 8);
  burstBits8b(M17_BIT_D5 >> 8);

  delay(5); // this shouldn't be necessary, but sometimes 16MHz is stupid-fast.

  command(LCD_FUNCTIONSET | _displayfunction); // then send 0010NF00 (N=rows, F=font)
  delay(5); // for safe keeping...
  command(LCD_FUNCTIONSET | _displayfunction); // ... twice.
  delay(5); // done!

  // turn on the LCD with our defaults. since these libs seem to use personal preference, I like a cursor.
  _displaycontrol = (LCD_DISPLAYON|M17_BIT_BL);
  display();
  // clear it off
  clear();

  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);
}

/********** high level commands, for the user! */
void LCD_I2C::clear()
{
  command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LCD_I2C::home()
{
  command(LCD_RETURNHOME);  // set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!
}

void LCD_I2C::setCursor(uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if ( row > _numrows ) row = _numrows - 1;    // we count rows starting from 0
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LCD_I2C::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD_I2C::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LCD_I2C::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_I2C::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LCD_I2C::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_I2C::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LCD_I2C::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LCD_I2C::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LCD_I2C::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LCD_I2C::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LCD_I2C::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LCD_I2C::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LCD_I2C::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) { write(charmap[i]); }
}

/*********** mid level commands, for sending data/cmds */
inline void LCD_I2C::command(uint8_t value) {
  send(value, LOW);
}

#if defined(ARDUINO) && (ARDUINO >= 100) //scl
inline size_t LCD_I2C::write(uint8_t value) {
  send(value, HIGH);
  return 1;
}
#else
inline void LCD_I2C::write(uint8_t value) {
  send(value, HIGH);
}
#endif


// Allows to set the backlight, if the LCD backpack is used
void LCD_I2C::setBacklight(uint8_t status) {
  if (status == HIGH) _backlightval = M17_BIT_BL;
  else _backlightval = LCD_NOBACKLIGHT;
  // we can't use burstBits16 it will damage bank A as well
  burstBits8b(_backlightval >> 8);  // this is neccessary because of modifying only bank B
}

// write either command or data, burst it to the expander over I2C.
void LCD_I2C::send(uint8_t value, uint8_t mode) {
   
    // n.b. RW bit stays LOW to write
    uint8_t buf = _backlightval >> 8;
    // send high 4 bits
    if (value & 0x10) buf |= M17_BIT_D4 >> 8;
    if (value & 0x20) buf |= M17_BIT_D5 >> 8;
    if (value & 0x40) buf |= M17_BIT_D6 >> 8;
    if (value & 0x80) buf |= M17_BIT_D7 >> 8;
    
    // if mode is HIGH data is sent otherwise commmand is sent
    if (mode) buf |= (M17_BIT_RS|M17_BIT_EN) >> 8; // RS+EN
    else buf |= M17_BIT_EN >> 8; // EN

    burstBits8b(buf);

    // resend w/ EN turned off
    buf &= ~(M17_BIT_EN >> 8);
    burstBits8b(buf);
    
    // send low 4 bits
    buf = _backlightval >> 8;
    // send high 4 bits
    if (value & 0x01) buf |= M17_BIT_D4 >> 8;
    if (value & 0x02) buf |= M17_BIT_D5 >> 8;
    if (value & 0x04) buf |= M17_BIT_D6 >> 8;
    if (value & 0x08) buf |= M17_BIT_D7 >> 8;
    
    if (mode) buf |= (M17_BIT_RS|M17_BIT_EN) >> 8; // RS+EN
    else buf |= M17_BIT_EN >> 8; // EN
    
    burstBits8b(buf);
    
    // resend w/ EN turned off
    buf &= ~(M17_BIT_EN >> 8);
    burstBits8b(buf);
}


// value byte order is BA
void LCD_I2C::burstBits16(uint16_t value) {
  // we use this to burst bits to the GPIO chip whenever we need to. avoids repetitive code.
  Wire.beginTransmission(MCP23017_ADDRESS | _i2cAddr);
  wiresend(GPIOA);
  wiresend(value & 0xFF); // send A bits
  wiresend(value >> 8);   // send B bits
  while(Wire.endTransmission());
}

void LCD_I2C::burstBits8b(uint8_t value) {
  // we use this to burst bits to the GPIO chip whenever we need to. avoids repetitive code.
  Wire.beginTransmission(MCP23017_ADDRESS | _i2cAddr);
  wiresend(GPIOB);
  wiresend(value); // last bits are crunched, we're done.
  while(Wire.endTransmission());
}

//direct access to the registers for interrupt setting and reading, also the tone function using buzzer pin
uint8_t LCD_I2C::readRegister(uint8_t reg) {
  // read a register
  Wire.beginTransmission(MCP23017_ADDRESS | _i2cAddr);
  wiresend(reg);  
  Wire.endTransmission();
  
  Wire.requestFrom(MCP23017_ADDRESS | _i2cAddr, 1);
  return wirerecv();
}


//set registers
void LCD_I2C::setRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MCP23017_ADDRESS | _i2cAddr);
    wiresend(reg);
    wiresend(value);
    Wire.endTransmission();
}
