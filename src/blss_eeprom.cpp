#include <EEPROM.h>
#include "blss.h"


void clearEEPROM()
{
  EEPROM.begin(EEPSIZE);
  for (int i = 0; i < EEPSIZE; i++)
  {
    EEPROM.write(i, 0);
  }
  EEPROM.end();
}

void readStringFromEEPROM(uint16_t pos, char* str, uint16_t len)
{
  for (int i = 0; i < len; ++i)
  {
    str[i] = EEPROM.read(pos + i);
    if (str[i] == 0) return;
  }
  str[len] = 0; //make sure every string is properly terminated. str must be at least len +1 big.
}