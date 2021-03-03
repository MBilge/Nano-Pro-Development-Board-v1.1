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


//MCP23017 LCD Shield
//
//  B7 B6 B5 B4 B3 B2 B1 B0 A7 A6 A5 A4 A3 A2 A1 A0 - MCP23017 
//  RS RW EN D4 D5 D6 D7 BL C4 C3 C2 C1 R4 R3 R2 R1 
//  15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0  
// Data pins on MCP23017
#define M17_BIT_D4 0x0200  // pin 9
#define M17_BIT_D5 0x0400  // pin 10
#define M17_BIT_D6 0x0800  // pin 11
#define M17_BIT_D7 0x1000  // pin 12 

// Control Pins on MCP23017
#define M17_BIT_EN 0x2000  // pin 13
#define M17_BIT_RW 0x4000  // pin 14
#define M17_BIT_RS 0x8000  // pin 15

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

  // Wire.beginTransmission(MCP23017_ADDRESS | _i2cAddr);
  // wiresend(IODIRA);
  // wiresend(0x1F); 
  // Wire.endTransmission();

  // Wire.beginTransmission(MCP23017_ADDRESS | _i2cAddr);
  // wiresend(GPPUA);
  // wiresend(0x1F); 
  // Wire.endTransmission();

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

  //put the LCD into 4 bit mode
  // start with a non-standard command to make it realize we're speaking 4-bit here
  // per LCD datasheet, first command is a single 4-bit burst, 0011.
  //-----
  //  we cannot assume that the LCD panel is powered at the same time as
  //  the arduino, so we have to perform a software reset as per page 45
  //  of the HD44780 datasheet - (kch)
  //-----
    //
    //  B7 B6 B5 B4 B3 B2 B1 B0 A7 A6 A5 A4 A3 A2 A1 A0 - MCP23017 
    //  15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0  
    //  RS RW EN D4 D5 D6 D7 B  G  R     B4 B3 B2 B1 B0 
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
  _displaycontrol = (LCD_DISPLAYON|LCD_BACKLIGHT);
  display();
  // clear it off
  clear();

  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

  //------ Keypad Setup  ----------------------





  //-------Keypad Setup  ----------------------
}

// void LCD_I2C::Keypad_begin()){
// // Construction for Keypad
//   TwoWire::begin(_i2cAddr);
//   pinState = pinState_set();
//   iodir_state = 0x00FF; // set all pins on bank A as input
//   word iodirec = 0x00ff;  //0xffff  //direction of each bit - reset state = all inputs.
//   byte iocon =  0x10;   // reset state for bank, disable slew control
//   TwoWire::beginTransmission( (int)_i2caddr );
//   TwoWire::write( IOCONA ); // same as when reset
//   TwoWire::write( iocon );
//   TwoWire::endTransmission( );
//   TwoWire::beginTransmission( (int)_i2caddr );
//   TwoWire::write( GPPUA ); // enable pullups on all inputs
//   TwoWire::write( 0xff );
//   TwoWire::write( 0xff );
//   TwoWire::endTransmission( );
//   TwoWire::beginTransmission( (int)_i2caddr );
//   TwoWire::write( IODIRA ); // setup port direction - all inputs to start
//   TwoWire::write( lowByte( iodirec ) );
//   TwoWire::write( highByte( iodirec ) );
//   TwoWire::endTransmission( );
//   TwoWire::beginTransmission( (int)_i2caddr );
//   TwoWire::write( GPIOA );  //point register pointer to gpio reg
//   TwoWire::write( lowByte(iodirec) ); // make o/p latch agree with pulled-up pins
//   TwoWire::write( highByte(iodirec) );
//   TwoWire::endTransmission( );

// }

//void LCD_I2C::begin(uint8_t cols, uint8_t rows);//,char *userKeymap ) {
  
  //Keypad_begin(char *userKeymap );
  //LCD_begin(uint8_t cols, uint8_t rows);
  
//}

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
  int row_offsets[] = { 0x00, 0x40, 0x10, 0x50 };
  if ( row > _numrows ) row = _numrows - 1;    // we count rows starting w/0
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
  if (status == HIGH) _backlightval = LCD_BACKLIGHT;
  else _backlightval = LCD_NOBACKLIGHT;
  // we can't use burstBits16 it will damage bank A as well
  burstBits8b(_backlightval >> 8);  // this is neccessary because of modifying only bank B
}

// write either command or data, burst it to the expander over I2C.
void LCD_I2C::send(uint8_t value, uint8_t mode) {

    // BURST SPEED, OH MY GOD
    // the (now High Speed!) I/O expander pinout
    //  B7 B6 B5 B4 B3 B2 B1 B0 A7 A6 A5 A4 A3 A2 A1 A0 - MCP23017 
    //  15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0  
    //  RS RW EN D4 D5 D6 D7 B  G  R     B4 B3 B2 B1 B0 
    
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

// // individual pin setup - modify pin bit in IODIR reg.
// void LCD_I2C::pin_mode(byte pinNum, byte mode) {
//   word mask = 0b0000000000000001 << pinNum;
//   if( mode == OUTPUT ) {
//     iodir_state &= ~mask;
//   } else {
//     iodir_state |= mask;
//   } // if mode
//   TwoWire::beginTransmission((int)_i2caddr);
//   TwoWire::write( IODIRA );
//   TwoWire::write( lowByte( iodir_state ) );
//   TwoWire::write( highByte( iodir_state ) );
//   TwoWire::endTransmission();
// } // pin_mode( )

// void LCD_I2C::pin_write(byte pinNum, boolean level) {
//   word mask = 1<<pinNum;
//   if( level == HIGH ) {
//     pinState |= mask;
//   } else {
//     pinState &= ~mask;
//   }
//   port_write( pinState );
// } // MC17xWrite( )


// int LCD_I2C::pin_read(byte pinNum) {
//   TwoWire::beginTransmission((int)_i2caddr);
//   TwoWire::write( GPIOA );
//   TwoWire::endTransmission( );
//   word mask = 0x1<<pinNum;
//   TwoWire::requestFrom((int)_i2caddr, 2);
//   word pinVal = 0;
//   pinVal = TwoWire::read( );
//   pinVal |= ( TwoWire::read( )<<8 );
//   pinVal &= mask;
//   if( pinVal == mask ) {
//     return 1;
//   } else {
//     return 0;
//   }
// }

// void LCD_I2C::port_write( word i2cportval ) {
// // MCP23017 requires a register address on each write
//   TwoWire::beginTransmission((int)__i2caddr);
//   TwoWire::write( GPIOA );
//   TwoWire::write( lowByte( i2cportval ) );
//   TwoWire::write( highByte( i2cportval ) );
//   TwoWire::endTransmission();
//   pinState = i2cportval;
// } // port_write( )

// word LCD_I2C::pinState_set( ) {
//   TwoWire::beginTransmission((int)_i2caddr);
//   TwoWire::write( GPIOA );
//   TwoWire::endTransmission( );
//   TwoWire::requestFrom( (int)__i2caddr, 2 );
//   pinState = 0;
//   pinState = TwoWire::read( );
//   pinState |= (TwoWire::read( )<<8);
//   return pinState;
// } // set_pinState( )

// // access functions for IODIR state copy
// word LCD_I2C::iodir_read( ) {
//   return iodir_state;  // local copy is always same as chip's register
// } // iodir_read( )

// void LCD_I2C::iodir_write( word iodir ) {
//   iodir_state = iodir;
//   TwoWire::beginTransmission((int)_i2caddr);   // read current IODIR reg 
//   TwoWire::write( IODIRA );
//   TwoWire::write( lowByte( iodir_state ) );
//   TwoWire::write( highByte( iodir_state ) );
//   TwoWire::endTransmission();
// } // iodir_write( )
