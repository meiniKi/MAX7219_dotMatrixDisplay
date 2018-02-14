

#include "Arduino.h"
#include "../lib/matrix7219.h"

uint8_t din = 11;             // DIN pin of MAX7219 module
uint8_t cs = 7;               // CS pin of MAX7219 module
uint8_t clk = 13;             // CLK: NOT SS of ARDUINO when using SPI mode!!
uint8_t nr_of_displays = 8;   // nr of displays in series

matrix7219 m(din, cs, clk, nr_of_displays);

void setup(){
  Serial.begin(9600);

  m.init();
  m.clearBuffer();

  m.writeStringToBuffer("Hi!", 4);

  m.writeBufferToDisplay();
  delay(1000);
  m.setIntensity(0);
}

void loop()
{
  for (uint8_t i = 0; i < 25; i++)
  {
    m.memShiftRight();
    m.writeBufferToDisplay();
    delay(75);
  }
  for (uint8_t i = 0; i < 25; i++)
  {
    m.memShiftLeft();
    m.writeBufferToDisplay();
    delay(75);
  }
}
