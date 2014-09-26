/*
@file	MCP23017.h
Header file for MCP23017
16-Bit I/O Expander with I2C Interface
See MCP23017.pdf datasheet for details

@author	Thomas Oppenhoff

https://github.com/TOppenhoff/MCP23017

Language: C++
License: GNU Public License

*/


#ifndef __MCP23017_H
#define  __MCP23017_H

#include <stdio.h>
#include <Arduino.h>

// Activate debugging output on Serial
//#define MCP23017_SERIAL_DEBUG

// Constants for IO PINs
#define MCP23017_GPA0	0
#define MCP23017_GPA1	1
#define MCP23017_GPA2	2
#define MCP23017_GPA3	3
#define MCP23017_GPA4	4
#define MCP23017_GPA5	5
#define MCP23017_GPA6	6
#define MCP23017_GPA7	7

#define MCP23017_GPB0	8
#define MCP23017_GPB1	9
#define MCP23017_GPB2	10
#define MCP23017_GPB3	11
#define MCP23017_GPB4	12
#define MCP23017_GPB5	13
#define MCP23017_GPB6	14
#define MCP23017_GPB7	15

// Number of IP pins on MCP23017
#define MCP23017_NUM_PINS 16


class MCP23017
{
public:
  class Pin
  {
    friend class MCP23017;
  private:

    // bit field
    // 0IIIPRMV
    // III = Interrupt definition (enabled, onchange, compare value)
    // P = Polarity Invert
    // R = Internal 10K Pullup Resistor
    // M = Pin Mode
    // V = Value
    uint8_t _state;

    Pin();

  public:

    uint8_t getPinMode() const;
    void setPinMode(uint8_t mode);

    uint8_t getInterrupt() const;
    void setInterrupt(uint8_t interrupt);

    bool getPolarityInvert() const;
    void setPolarityInvert(bool polarityInvert);

    uint8_t getValue() const;
    void setValue(uint8_t value);
  };

private:
  uint8_t _i2cArd;
  Pin _pins[16];

public:
  MCP23017();
  MCP23017(uint8_t i2cSlaveAdr);

  Pin& getPin(uint8_t pin);

  void setup();
  void read();
  void write() const;

protected:

  void readBankA();
  void readBankB();
  void writeBankA() const;
  void writeBankB() const;

  uint8_t collectBit(uint8_t bit, uint8_t bank) const;
  void distributeBit(uint8_t value, uint8_t bit, uint8_t bank);
  void write(uint8_t reg, uint8_t value) const;
  uint8_t read(uint8_t reg) const;
};


#endif // #ifndef __MCP23017_H