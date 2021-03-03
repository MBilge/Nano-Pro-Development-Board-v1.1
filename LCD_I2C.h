#ifndef LCD_I2C_h
#define LCD_I2C_h

#include <inttypes.h>
#include "Print.h"
#include "Wire.h"

#define MCP23017_ADDRESS 0x27  // default i2c address

// Registers

// I/O expander configuration register
// bit7<R/W-0> BANK: Controls how the registers are addressed
//     1 = The registers associated with each port are separated into different
//         banks
//     0 = The registers are in the same bank (addresses are sequential)
// bit6<R/W-0> MIRROR: INT Pins Mirror bit
//     1 = The INT pins are internally connected
//     0 = The INT pins are not connected. INTA is associated with  PORTA and
//         INTB is associated with PORTB
// bit5<R/W-0> SEQOP: Sequential Operation mode bit
//     1 = Sequential operation disabled, address pointer does not  increment
//     0 = Sequential operation enabled, address pointer increments
// bit4<R/W-0> DISSLW: Slew Rate control bit for SDA output
//     1 = Slew rate disabled
//     0 = Slew rate enabled
// bit3<R/W-0> HAEN: Hardware Address Enable bit (MCP23S17 only) (Note 1)
//     1 = Enables the MCP23S17 address pins.
//     0 = Disables the MCP23S17 address pins.
// bit2<R/W-0> ODR: Configures the INT pin as an open-drain output
//     1 = Open-drain output (overrides the INTPOL bit.)
//     0 = Active driver output (INTPOL bit sets the polarity.)
// bit1<R/W-0> INTPOL: This bit sets the polarity of the INT output pin
//     1 = Active-high
//     0 = Active-low
// bit0<U-0> Unimplemented: Read as '0'
#define IOCONA 0x0A
#define IOCONB 0x0B

// PIN registers for direction IO<7:0> <R/W-1> (default: 0b11111111)
#define IODIRA 0x00  //   1 = Pin is configured as an input
#define IODIRB 0x01  //   0 = Pin is configured as an output

// Input polarity registers  IP<7:0> <R/W-0> (default: 0b00000000)
#define IPOLA 0x02 //   1 = GPIO register bit reflects the opposite logic state of the input pin
#define IPOLB 0x03 //   0 = GPIO register bit reflects the same logic state of the input pin

 // Interrupt-on-change control registers   GPINT<7:0> <R/W-0> (default: 0b00000000)
#define GPINTENA 0x04  //   1 = Enables GPIO input pin for interrupt-on-change event
#define GPINTENB 0x05  //   0 = Disables GPIO input pin for interrupt-on-change event
    
// Default compare registers for interrupt-on-change
#define DEFVALA 0x06  // DEF<7:0> <R/W-0> (default: 0b00000000)
#define DEFVALB 0x07    

// Interrupt control register IOC<7:0> <R/W-0> (default: 0b00000000)   
#define INTCONA 0x08  //   1 = Pin value is compared against the associated bit in the DEFVAL register.
#define INTCONB 0x09  //   0 = Pin value is compared against the previous pin value.

// Pull-up resistor configuration registers PU<7:0> <R/W-0> (default: 0b00000000)
#define GPPUA 0x0C  //   1 = Pull-up enabled
#define GPPUB 0x0D  //   0 = Pull-up disabled

// Interrupt flag registers  INT<7:0> <R-0> (default: 0b00000000)
#define INTFA 0x0E //   1 = Pin caused interrupt.
#define INTFB 0x0F //   0 = Interrupt not pending

// Interrupt captured registers  ICP<7:0> <R-x>
#define INTCAPA 0x10  //   1 = Logic-high
#define INTCAPB 0x11  //   0 = Logic-low

// Port registers  GP<7:0> <R/W-0> (default: 0b00000000)
#define GPIOA 0x12  //   1 = Logic-high
#define GPIOB 0x13  //   0 = Logic-low

// Output latch registers  OL<7:0> <R/W-0> (default: 0b00000000)  
#define OLATA 0x14  //   1 = Logic-high
#define OLATB 0x15  //   0 = Logic-low



// commands
#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT    0x10
#define LCD_FUNCTIONSET    0x20
#define LCD_SETCGRAMADDR   0x40
#define LCD_SETDDRAMADDR   0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_BLINKON    0x01
#define LCD_BLINKOFF   0x00
#define LCD_CURSORON   0x02
#define LCD_CURSOROFF  0x00
#define LCD_DISPLAYON  0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_BACKLIGHT  0x100// used to pick out the backlight flag since _displaycontrol will never set itself
#define LCD_NOBACKLIGHT 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
//we only support 4-bit mode #define LCD_8BITMODE 0x10
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

class LCD_I2C : public Print{
public:
	LCD_I2C(uint8_t i2cAddr);
	void begin(uint8_t cols, uint8_t rows);
	void clear();
	void home();
	void noDisplay();
	void display();
	void noBlink();
	void blink();
	void noCursor();
	void cursor();
	void scrollDisplayLeft();
	void scrollDisplayRight();
	void leftToRight();
	void rightToLeft();
	void autoscroll();
	void noAutoscroll();
	void setBacklight(uint8_t status); 
	void createChar(uint8_t, uint8_t[]);
	void setCursor(uint8_t, uint8_t); 

	#if defined(ARDUINO) && (ARDUINO >= 100) // scl
		virtual size_t write(uint8_t);
	#else
		virtual void write(uint8_t);
	#endif
	void command(uint8_t);
    uint8_t readRegister(uint8_t);
    void setRegister(uint8_t, uint8_t);
	
private:
	// LCD functions and variables
	void send(uint8_t, uint8_t);
	void burstBits16(uint16_t);
	void burstBits8b(uint8_t);
	uint8_t _displayfunction;
	uint8_t _displaycontrol;
	uint8_t _displaymode;
	uint8_t _numrows,_currline;
	uint8_t _i2cAddr;
	uint8_t dotsize;
	uint16_t _backlightval; // only for MCP23017
};


#endif // LCD_I2C_h
