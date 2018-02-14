
/*

*/

#ifndef _matrix7219_H_
#define _matrix7219_H_

#include "Arduino.h"

#define CLK_HIGH() digitalWrite(clk, HIGH)
#define CLK_LOW()  digitalWrite(clk, LOW)

#define settingsLSBFIRST SPISettings(16000000, LSBFIRST, SPI_MODE0)
#define settingsMSBFIRST SPISettings(16000000, MSBFIRST, SPI_MODE0)

#define MATRIX_SPI_MODE

//upper bytes for commands
#define max7219_reg_noop        0x00
#define max7219_reg_digit0      0x01
#define max7219_reg_digit1      0x02
#define max7219_reg_digit2      0x03
#define max7219_reg_digit3      0x04
#define max7219_reg_digit4      0x05
#define max7219_reg_digit5      0x06
#define max7219_reg_digit6      0x07
#define max7219_reg_digit7      0x08
#define max7219_reg_decodeMode  0x09
#define max7219_reg_intensity   0x0a
#define max7219_reg_scanLimit   0x0b
#define max7219_reg_shutdown    0x0c
#define max7219_reg_displayTest 0x0f

class matrix7219
{
private:
  uint8_t din;
  uint8_t cs;
  uint8_t clk;
  uint8_t cnt_disp;

  /****************************** BUFFER - MAP *********************************

DISPLAY NR:  0          1          2        ....        n-1         n
        ----------+----------+----------+----------+----------+----------+
       |  bfr[0]  | ........ | ........ | ........ | ........ |  bfr[n]  |
       | ........ | ........ | ........ | ........ | ........ | ........ |
       | ........ | ........ | ........ | ........ | ........ | ........ |
DIN -> | ........ | ........ | ........ | ........ | ........ | ........ |
       | ........ | ........ | ........ | ........ | ........ | ........ |
       | ........ | ........ | ........ | ........ | ........ | ........ |
       | ........ | ........ | ........ | ........ | ........ | ........ |
       | ........ | ........ | ........ | ........ | ........ |  bfr[8n] |
        ----------+----------+----------+----------+----------+----------+

  *****************************************************************************/


  void selectChip();
  void unselectChip();
  void sendPacket(uint8_t command, uint8_t data);
  void sendPacketOfBuffer(uint8_t command, uint8_t data);
  void ackData();


public:
  uint8_t *buffer;
  uint8_t buffer_byte_size;


  matrix7219(uint8_t pin_din, uint8_t pin_cs, uint8_t pin_clk,
    uint8_t nr_of_displays);

  void init();
  void sendPacketToNr(uint8_t nr, uint8_t command, uint8_t data); //start: 0
  void sendPacketToAll(uint8_t command, uint8_t data);
  void clearRegisters();
  void writeBufferToDisplay();
  void setDotInBuffer(uint16_t x, uint16_t y); //(0,0) top left
  void clearDotInBuffer(uint16_t x, uint16_t y); //(0,0) top left
  void clearBuffer();
  void writeCharToBuffer(char ascii, uint16_t x); //(x = 0) left
  void writeStringToBuffer(const char str[], uint16_t x); //(x = 0) left
  void memShiftRight();
  void memShiftLeft();
  void setIntensity(uint8_t value);
};

#endif
