/*
@file	PCA9685_example.ino
Arduino sample code file for PCA9685
16-channel, 12-bit PWM Fm+ I2C-bus LED controller
See PCA9685.pdf datasheet for details

@author	Thomas Oppenhoff

https://github.com/TOppenhoff/PCA9685

Language: C++
License: GNU Public License

*/

#include <Wire.h>
#include <Serial.h>
#include <MCP23017.h>

// MCP23017 with default settings
MCP23017 expander;
uint8_t lastValue = LOW;

void setup()
{
  // initialize serial for debug output
  Serial.begin(9600);

  // initialize TwoWire communication
  Wire.begin();

  // set GPB0 to be an output (LED)
  expander.getPin(MCP23017_GPB0).setPinMode(OUTPUT);
  // set GPB0 to be inverted (pull to ground instead of up)
  expander.getPin(MCP23017_GPB0).setPolarityInvert(true);
  // set GPA0 to be an input (push button)
  expander.getPin(MCP23017_GPA0).setPinMode(INPUT);

  // setup the MCP23017 
  expander.setup();
}

void loop()
{
  // read the inputs from MCP23017
  expander.read();

  // get the value of our push button
  uint8_t buttonValue = expander.getPin(MCP23017_GPA0).getValue();

  // check if the button has gone HIGH
  if (buttonValue == HIGH && lastValue == LOW)
  {
    // someone is pushing the button, so toggle the LED
    lastValue = HIGH;
    if (expander.getPin(MCP23017_GPB0).getValue() == HIGH)
    {
      // the LED is on right now, so turn it off
      expander.getPin(MCP23017_GPB0).setValue(LOW);
    }
    else
    {
      // the LED is off right now, so turn it on
      expander.getPin(MCP23017_GPB0).setValue(HIGH);
    }
    
    // write to the expander to really turn on/off the LED
    expander.write();
  }

  // please read about hardware button debouncing. Doing it in code here really sucks ;-)
  delay(100);
}
