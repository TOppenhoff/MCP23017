/*
@file	MCP23017.h
Code file for MCP23017
16-Bit I/O Expander with I2C Interface
See MCP23017.pdf datasheet for details

@author	Thomas Oppenhoff

https://github.com/TOppenhoff/MCP23017

Language: C++
License: GNU Public License

*/

#include "MCP23017.h"
#include <Wire.h>

#define MCP23017_BASEADR	0x20 // I2C base address

// 1 = input, 0 = output (default)
#define MCP23017_IODIRA	0x00 // I/O direction register bank A
#define MCP23017_IODIRB	0x01 // I/O direction register bank B

// 1 = mirror input logic level, 0 = no mirror (default)
#define MCP23017_IPOLA	0x02 // input polarity register bank A
#define MCP23017_IPOLB	0x03 // input polarity register bank B

// 1 = interrupt enabled, 0 = no interrupt (default)
#define MCP23017_GPINTENA	0x04 // interrupt-on-change control register bank A
#define MCP23017_GPINTENB	0x05 // interrupt-on-change control register bank B

// when INTCON == 1 -> 1 = FALLING, 0 = RISING (default)
#define MCP23017_DEFVALA	0x06 // default compare register for interrupt-on-change bank A
#define MCP23017_DEFVALB	0x07 // default compare register for interrupt-on-change bank B

// 1 = compare againt DEFVAL register, 0 = CHANGE (default)
#define MCP23017_INTCONA	0x08 // interrupt control register bank A
#define MCP23017_INTCONB	0x09 // interrupt control register bank B

//#define MCP23017_IOCON	0A // I/O expander configuration register - NOT USED
//#define MCP23017_IOCON	0B // I/O expander configuration register - NOT USED

// 1 = pull-up with 10K, 0 = no pull-up (default)
#define MCP23017_GPPUA	0x0C // GPIO pull-up resistor register bank A
#define MCP23017_GPPUB	0x0D // GPIO pull-up resistor register bank B

// will be 1 when pin caused interrupt
#define MCP23017_INTFA	0x0E // interrupt flag register bank A
#define MCP23017_INTFB	0x0F // interrupt flag register bank B

// value of pins causing interrupt
#define MCP23017_INTCAPA	0x10 // interrupt captured value for port register bank A
#define MCP23017_INTCAPB	0x11 // interrupt captured value for port register bank B

// read/write values
#define MCP23017_GPIOA	0x12 // general purpose I/O port register bank A
#define MCP23017_GPIOB	0x13 // general purpose I/O port register bank A

//#define MCP23017_OLATA	14 // output latch register bank A - NOT USED
//#define MCP23017_OLATB	15 // output latch register bank B - NOT USED

#define MCP23017_PINSTATE_INTERRUPT_ENABLED	0x40
#define MCP23017_PINSTATE_INTERRUPT_CHANGE	0x20
#define MCP23017_PINSTATE_INTERRUPT_VALUE	0x10
#define MCP23017_PINSTATE_POLARITY	0x8
#define MCP23017_PINSTATE_PULLUP	0x4
#define MCP23017_PINSTATE_MODE	0x2
#define MCP23017_PINSTATE_VALUE	0x1

#define MCP23017_BANK_A	0x0
#define MCP23017_BANK_B 0x1

#define MCP23017_LAST_PIN_BANK_A 7

MCP23017::Pin::Pin()
{
  // be careful!
  // for arduino INPUT is 0x0 and OUTPUT is 0x1
  // but for MCP23017 it is the other way round!
  // INPUT is 0x1 and OUTPUT is 0x0

  _state = MCP23017_PINSTATE_MODE; // default on chip is input
}

uint8_t MCP23017::Pin::getPinMode() const
{
  if ((_state & MCP23017_PINSTATE_MODE) == MCP23017_PINSTATE_MODE)
  {
    if ((_state & MCP23017_PINSTATE_PULLUP) == MCP23017_PINSTATE_PULLUP)
    {
      return INPUT_PULLUP;
    }
    return INPUT;
  }

  return OUTPUT;
}

void MCP23017::Pin::setPinMode(uint8_t mode)
{
  if (mode == INPUT_PULLUP)
  {
    _state |= MCP23017_PINSTATE_MODE;
    _state |= MCP23017_PINSTATE_PULLUP;
  }
  else if (mode == INPUT)
  {
    _state |= MCP23017_PINSTATE_MODE;
    _state &= ~MCP23017_PINSTATE_PULLUP;
  }
  else
  {
    _state &= ~MCP23017_PINSTATE_MODE;
    _state &= ~MCP23017_PINSTATE_PULLUP;
  }
}

uint8_t MCP23017::Pin::getInterrupt() const
{
  if ((_state & MCP23017_PINSTATE_INTERRUPT_ENABLED) == MCP23017_PINSTATE_INTERRUPT_ENABLED)
  {
    // interrupt enabled
    if ((_state & MCP23017_PINSTATE_INTERRUPT_CHANGE) == MCP23017_PINSTATE_INTERRUPT_CHANGE)
    {
      // value from default register
      if ((_state & MCP23017_PINSTATE_INTERRUPT_VALUE) == MCP23017_PINSTATE_INTERRUPT_VALUE)
      {
        // falling interrupt
        return FALLING;
      }
      // rising interrupt
      return RISING;
    }

    // on change interrupt
    return CHANGE;
  }

  // no interrupt
  return 0x0;
}

void MCP23017::Pin::setInterrupt(uint8_t interrupt)
{
  if (interrupt == CHANGE)
  {
    _state |= MCP23017_PINSTATE_INTERRUPT_ENABLED;
    _state &= ~MCP23017_PINSTATE_INTERRUPT_CHANGE;
    _state &= ~MCP23017_PINSTATE_INTERRUPT_VALUE;
  }
  else if (interrupt == FALLING)
  {
    _state |= MCP23017_PINSTATE_INTERRUPT_ENABLED;
    _state |= MCP23017_PINSTATE_INTERRUPT_CHANGE;
    _state |= MCP23017_PINSTATE_INTERRUPT_VALUE;
  }
  else if (interrupt == RISING)
  {
    _state |= MCP23017_PINSTATE_INTERRUPT_ENABLED;
    _state |= MCP23017_PINSTATE_INTERRUPT_CHANGE;
    _state &= ~MCP23017_PINSTATE_INTERRUPT_VALUE;
  }
  else
  {
    _state &= ~MCP23017_PINSTATE_INTERRUPT_ENABLED;
    _state &= ~MCP23017_PINSTATE_INTERRUPT_CHANGE;
    _state &= ~MCP23017_PINSTATE_INTERRUPT_VALUE;
  }
}

bool MCP23017::Pin::getPolarityInvert() const
{
  return (_state & MCP23017_PINSTATE_POLARITY) == MCP23017_PINSTATE_POLARITY;
}

void MCP23017::Pin::setPolarityInvert(bool polarityInvert)
{
  if (polarityInvert)
  {
    _state |= MCP23017_PINSTATE_POLARITY;
  }
  else
  {
    _state &= ~MCP23017_PINSTATE_POLARITY;
  }
}

uint8_t MCP23017::Pin::getValue() const
{
  if ((_state & MCP23017_PINSTATE_VALUE) == MCP23017_PINSTATE_VALUE)
  {
    return HIGH;
  }
  return LOW;
}

void MCP23017::Pin::setValue(uint8_t value)
{
  if ((~_state & MCP23017_PINSTATE_MODE) == MCP23017_PINSTATE_MODE)
  {
    // pin is really an output pin
    if (value)
    {
      _state |= MCP23017_PINSTATE_VALUE;
    }
    else
    {
      _state &= ~MCP23017_PINSTATE_VALUE;
    }
  }
}

MCP23017::MCP23017()
{
  _i2cArd = MCP23017_BASEADR;
}

MCP23017::MCP23017(uint8_t i2cSlaveAdr)
{
  _i2cArd = MCP23017_BASEADR + i2cSlaveAdr;
}

MCP23017::Pin& MCP23017::getPin(uint8_t pin)
{
  if (pin < 0 || pin > 15)
    pin = 0; // playing safe with dummys :-)

  return _pins[pin];
}

void MCP23017::setup()
{
#ifdef MCP23017_SERIAL_DEBUG
  Serial.println("MCP23017::setup()");
#endif

  // i/o direction (incl. pullup)
  write(MCP23017_IODIRA, collectBit(MCP23017_PINSTATE_MODE, MCP23017_BANK_A));
  write(MCP23017_IODIRB, collectBit(MCP23017_PINSTATE_MODE, MCP23017_BANK_B));
  write(MCP23017_GPPUA, collectBit(MCP23017_PINSTATE_PULLUP, MCP23017_BANK_A));
  write(MCP23017_GPPUB, collectBit(MCP23017_PINSTATE_PULLUP, MCP23017_BANK_B));

  // polarity
  write(MCP23017_IPOLA, collectBit(MCP23017_PINSTATE_POLARITY, MCP23017_BANK_A));
  write(MCP23017_IPOLB, collectBit(MCP23017_PINSTATE_POLARITY, MCP23017_BANK_B));

  // interrupt
  write(MCP23017_GPINTENA, collectBit(MCP23017_PINSTATE_INTERRUPT_ENABLED, MCP23017_BANK_A));
  write(MCP23017_GPINTENB, collectBit(MCP23017_PINSTATE_INTERRUPT_ENABLED, MCP23017_BANK_B));
  write(MCP23017_INTCONA, collectBit(MCP23017_PINSTATE_INTERRUPT_CHANGE, MCP23017_BANK_A));
  write(MCP23017_INTCONB, collectBit(MCP23017_PINSTATE_INTERRUPT_CHANGE, MCP23017_BANK_B));
  write(MCP23017_DEFVALA, collectBit(MCP23017_PINSTATE_INTERRUPT_VALUE, MCP23017_BANK_A));
  write(MCP23017_DEFVALB, collectBit(MCP23017_PINSTATE_INTERRUPT_VALUE, MCP23017_BANK_B));
}

void MCP23017::read()
{
#ifdef MCP23017_SERIAL_DEBUG
  Serial.println("MCP23017::read()");
#endif
  readBankA();
  readBankB();
}

void MCP23017::write() const
{
#ifdef MCP23017_SERIAL_DEBUG
  Serial.println("MCP23017::write()");
#endif
  writeBankA();
  writeBankB();
}

void MCP23017::readBankA()
{
  if (collectBit(MCP23017_PINSTATE_MODE, MCP23017_BANK_A) != 0x0)
  {
    // at least 1 input pin on bank a
    distributeBit(read(MCP23017_GPIOA), MCP23017_PINSTATE_VALUE, MCP23017_BANK_A);
  }
#ifdef MCP23017_SERIAL_DEBUG
  else
  {
    Serial.println("No input pins on bank A");
  }
#endif
}

void MCP23017::readBankB()
{
  if (collectBit(MCP23017_PINSTATE_MODE, MCP23017_BANK_B) != 0x0)
  {
    // at least 1 input pin on bank b
    distributeBit(read(MCP23017_GPIOB), MCP23017_PINSTATE_VALUE, MCP23017_BANK_B);
  }
#ifdef MCP23017_SERIAL_DEBUG
  else
  {
    Serial.println("No input pins on bank B");
  }
#endif
}

void MCP23017::writeBankA() const
{
  if (collectBit(MCP23017_PINSTATE_MODE, MCP23017_BANK_A) != 0xF)
  {
    // at least 1 output pin on bank a
    write(MCP23017_GPIOA, collectBit(MCP23017_PINSTATE_VALUE, MCP23017_BANK_A));
  }
#ifdef MCP23017_SERIAL_DEBUG
  else
  {
    Serial.println("No output pins on bank A");
  }
#endif
}

void MCP23017::writeBankB() const
{
  if (collectBit(MCP23017_PINSTATE_MODE, MCP23017_BANK_B) != 0xF)
  {
    // at least 1 output pin on bank b
    write(MCP23017_GPIOB, collectBit(MCP23017_PINSTATE_VALUE, MCP23017_BANK_B));
  }
#ifdef MCP23017_SERIAL_DEBUG
  else
  {
    Serial.println("No output pins on bank A");
  }
#endif
}

uint8_t MCP23017::collectBit(uint8_t bit, uint8_t bank) const
{
  uint16_t result = 0x0;
  int i;
  int offset = 0;
  if (bank == MCP23017_BANK_B)
  {
    offset = 8;
  }
  for (i = 0; i < 8; ++i)
  {
    if ((_pins[i + offset]._state & bit) == bit)
    {
      result |= bit << i;
    }
  }

  return result;
}

void MCP23017::distributeBit(uint8_t value, uint8_t bit, uint8_t bank)
{
  int i;
  int offset = 0;
  if (bank == MCP23017_BANK_B)
  {
    offset = 8;
  }
  for (i = 0; i < 8; ++i)
  {
    if (((2 ^ i) & value) == (2 ^ i))
    {
      // pin is HIGH
      _pins[i + offset]._state |= bit;
    }
    else
    {
      // pin is LOW
      _pins[i + offset]._state &= ~bit;
    }
  }
}

void MCP23017::write(uint8_t reg, uint8_t value) const
{
  Wire.beginTransmission(_i2cArd);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();

#ifdef MCP23017_SERIAL_DEBUG
  Serial.print("Write: Register 0x");
  Serial.print(reg, HEX);
  Serial.print(" Value=");
  Serial.println(value, BIN);
#endif
}

uint8_t MCP23017::read(uint8_t reg) const
{
  uint8_t value;
  Wire.beginTransmission(_i2cArd);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(_i2cArd, (uint8_t)1); // request one byte of data from MCP20317
  if (1 == Wire.available())
  {
    value = Wire.read();
  }
  else
  {
    value = 0x0;
  }

#ifdef MCP23017_SERIAL_DEBUG
  Serial.print("Read: Register 0x");
  Serial.print(reg, HEX);
  Serial.print(" Value=");
  Serial.println(value, BIN);
#endif

  return value;
}


