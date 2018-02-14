/************** LIBRARY TO CONTROL MAX7219 DOT MATRIX DISPLAYS ****************/
/*
/* configurations:
 uint8_t din = 11;           //DIN pin, any or MOSI for SPI mode
 uint8_t cs = 7;             //CS pin, any or NOT(!!) SS for SPI mode
 uint8_t clk = 13;           //CLK pin, any or CLK for SPI mode
 uint8_t nr_of_displays = 8; //number of MAX7219 chips in series
 
 #define MATRIX_SPI_MODE     //for using SPI
 
/* Author: (c) Meinhard Kissich, www.meinhard-kissich.at
/* Version: 0.9
*/

#include "Arduino.h"
#include "matrix7219.h"
#include "font.h"
#include <SPI.h>


matrix7219::matrix7219(uint8_t pin_din, uint8_t pin_cs, uint8_t pin_clk,
                       uint8_t nr_of_displays)
{
  din = pin_din;
  cs  = pin_cs;
  clk = pin_clk;
  cnt_disp = nr_of_displays;
  
  //malloc buffer
  buffer_byte_size = 8 * cnt_disp;
  buffer = (uint8_t*) malloc(sizeof(uint8_t) * buffer_byte_size);
}

/*******************************************************************************
 initialize array of displays: set pinmode, write basic settings, clear old data
 *******************************************************************************/
void matrix7219::init()
{
  pinMode(din,  OUTPUT);
  pinMode(clk,  OUTPUT);
  pinMode(cs,   OUTPUT);
  
  digitalWrite(clk, LOW);
  
#ifdef MATRIX_SPI_MODE
  SPI.begin();
#endif
  
  sendPacketToAll(max7219_reg_displayTest,  0x00);  //test mode off
  sendPacketToAll(max7219_reg_shutdown,     0x01);  //shut down off
  sendPacketToAll(max7219_reg_scanLimit,    0x07);  //no limit
  
  clearRegisters();
}

/*******************************************************************************
 select the MAX7219 chips by clearing the CS signal
 *******************************************************************************/
void matrix7219::selectChip()
{
  digitalWrite(cs, LOW);
}

/*******************************************************************************
 unselect the MAX7219 chips by setting the CS signal
 *******************************************************************************/
void matrix7219::unselectChip()
{
  digitalWrite(cs, HIGH);
}

/*******************************************************************************
 send an 16-bit data packet to the DIN of the first MAX7219 without latching
 it into the decoder
 *******************************************************************************/
void matrix7219::sendPacket(uint8_t command, uint8_t data)
{
  selectChip();
  
#ifndef MATRIX_SPI_MODE
  shiftOut(din, clk, MSBFIRST, command);
  shiftOut(din, clk, MSBFIRST, data);
#else
  SPI.beginTransaction(settingsMSBFIRST);
  SPI.transfer(command);
  SPI.transfer(data);
  SPI.endTransaction();
#endif
}

/*******************************************************************************
 send an 16-bit data packet to the DIN of the first MAX7219 without latching
 it into the decoder: data byte is rotated 180° LSB <-> MSB
 *******************************************************************************/
void matrix7219::sendPacketOfBuffer(uint8_t command, uint8_t data)
{
  selectChip();
  
#ifndef MATRIX_SPI_MODE
  shiftOut(din, clk, MSBFIRST, command);
  shiftOut(din, clk, LSBFIRST, data);
#else
  SPI.beginTransaction(settingsMSBFIRST);
  SPI.transfer(command);
  SPI.beginTransaction(settingsLSBFIRST);
  SPI.transfer(data);
  SPI.endTransaction();
#endif
}

/*******************************************************************************
 rising edge on CS to latch data from the shift register into the decoder
 *******************************************************************************/
void matrix7219::ackData()
{
  selectChip();
  unselectChip();
}

/*******************************************************************************
 send a data packet to a specific MAX7219 chip in the string. filling the others
 with NOOP commands to just access this specific one
 *******************************************************************************/
void matrix7219::sendPacketToNr(uint8_t nr, uint8_t command, uint8_t data)
{
  //check nr
  if (nr > (cnt_disp - 1))
    return;
  
  //1. shift out old data
  for (uint8_t i = 0; i < (cnt_disp - nr - 1); i++)
    sendPacket(max7219_reg_noop, 0x00);
  
  //shift current data
  sendPacket(command, data);
  
  //shift this data to display nr xx
  for (uint8_t i = 0; i < nr; i++)
    sendPacket(max7219_reg_noop, 0x00);
  
  ackData();
}

/*******************************************************************************
 send a data packet to all MAX7219 in the string
 *******************************************************************************/
void matrix7219::sendPacketToAll(uint8_t command, uint8_t data)
{
  for (uint8_t i = 0; i < cnt_disp; i++)
    sendPacket(command, data);
  
  ackData();
}

/*******************************************************************************
 clear old data from all MAX7219 in the string
 *******************************************************************************/
void matrix7219::clearRegisters()
{
  for (uint8_t i = 1; i <= 8; i++)
  {
    sendPacketToAll(i, 0x00);
  }
}

/*******************************************************************************
 efficiently writing the buffer to the string of displays
 *******************************************************************************/
void matrix7219::writeBufferToDisplay()
{
  //for each row: "dig 7 (0x08) ... dig 0 (0x01)"
  uint8_t row = 0;
  for (uint8_t dig = 8; dig > 0; dig--)
  {
    //for each display: n ... 0
    for (uint8_t disp = cnt_disp; disp > 0; disp--)
    {
      sendPacketOfBuffer(dig, buffer[(disp - 1) + row * cnt_disp]);
    }
    ackData();
    row++;
  }
}

/*******************************************************************************
 clear the whole buffer (0 = black), not written to display yet
 *******************************************************************************/
void matrix7219::clearBuffer()
{
  for (uint16_t i = 0; i < buffer_byte_size; i++)
  {
    buffer[i] = 0x00;
  }
}

/*******************************************************************************
 set a single dot in the buffer by cardesian coordinates: (0,0) = top left
 ----------------------
 | (0,0)  -->
 |   |
 |   v
 |
 *******************************************************************************/
void matrix7219::setDotInBuffer(uint16_t x, uint16_t y) //(0,0) top left
{
  if ((x > 7 * cnt_disp) || (y > 7))
    return;
  
  uint8_t in_disp = x / 8;
  uint8_t in_byte = in_disp + cnt_disp * y;
  
  buffer[in_byte] |= (1 << (7 - (x % 8)));
}

/*******************************************************************************
 clear a single dot in the buffer by cardesian coordinates: (0,0) = top left
 ----------------------
 | (0,0)  -->
 |   |
 |   v
 |
 *******************************************************************************/
void matrix7219::clearDotInBuffer(uint16_t x, uint16_t y) //(0,0) top left
{
  if ((x > 7 * cnt_disp) || (y > 7))
    return;
  
  uint8_t in_disp = x / 8;
  uint8_t in_byte = in_disp + cnt_disp * y;
  
  buffer[in_byte] &= ~(1 << (7 - (x % 8)));
}

/*******************************************************************************
 write an ASCII character to the buffer in font8x8.
 x ... offset from left | ->>> left in pixel
 *******************************************************************************/
void matrix7219::writeCharToBuffer(char ascii, uint16_t x)
{
  uint8_t char8x8[6];
  
  //copy character
  for (uint8_t i = 0; i < 6; i++)
    char8x8[i] = font8x8[ascii - 32][i];
  
  //for each character row
  for (uint16_t col = 0; col < char8x8[0]; col++)
  {
    for (uint8_t row = 0; row < 8; row++)
    {
      if (char8x8[col + 1] & (1 << row))
        setDotInBuffer(x + col, row);
      else
        clearDotInBuffer(x + col, row);
    }
  }
}

/*******************************************************************************
 write a string to the buffer in font8x8.
 x ... offset from left | ->>> left in pixel
 *******************************************************************************/
void matrix7219::writeStringToBuffer(const char str[], uint16_t x)
{
  uint16_t x_offset = 0;
  
  for (uint8_t i = 0; i < strlen(str) ; i++)
  {
    char c = str[i];
    writeCharToBuffer(c, x + x_offset);
    x_offset += font8x8[c - 32][0] + 1;
    
    if ((x_offset + x) > (cnt_disp * 8))
      break;
  }
}

/*******************************************************************************
 shift condent in the memory 1 pixel to the right, most right pixels are lost,
 most left pixels are blank after shifting, can be used for scrolling text
 *******************************************************************************/
void matrix7219::memShiftRight()
{
  for (int i = buffer_byte_size - 1; i >= 0; i--)
  {
    buffer[i] = (buffer[i] >> 1);
    
    //not the first byte in new row and not byte 0 and lost bit = 1
    if ((i % cnt_disp != 0) && (buffer[i - 1] & 0x01))
      buffer[i] |= (1 << 7);
    
  }
}

/*******************************************************************************
 shift condent in the memory 1 pixel to the left, most left pixels are lost,
 most right pixels are blank after shifting, can be used for scrolling text
 *******************************************************************************/
void matrix7219::memShiftLeft()
{
  for (uint16_t i = 0; i < buffer_byte_size; i++)
  {
    buffer[i] = (buffer[i] << 1);
    
    //not last in row and MSB of next byte == 1
    if (((i + 1) % cnt_disp != 0) && (buffer[i + 1] & 0x80))
      buffer[i] |= 0x01;
  }
}

/*******************************************************************************
 set the brightness of the LED pixels. 0 - 15 are valid settings.
 *******************************************************************************/
void matrix7219::setIntensity(uint8_t value)
{
  if (value > 15)
    return;
  
  sendPacketToAll(max7219_reg_intensity,value);
}

//end

