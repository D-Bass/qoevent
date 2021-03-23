#include <stdint.h>

uint16_t crc16(const void* data, uint8_t len)
{
  register uint_fast8_t i;
  char* d=(char*) data;
  uint16_t crc=0xffff;
  while (len--) {
    crc^=*d++ <<8;
    for (i=0; i<8; i++) {
      crc=crc<<1 ^ ( crc&0x8000 ? 0x5935 : 0);
    }
  }
  return crc>>8&0xff | crc<<8&0xff00; //for auto zeroing
}

uint16_t crc8(void* data,uint8_t len)
{
  char* d=(char*) data;
  register uint_fast8_t i;
  uint8_t crc=0xff;
  while (len--) {
    crc^=*d++;
    for (i=0; i<8; i++) {
      crc=crc<<1 ^ ( crc&0x80 ? 0x07 : 0);
    }
  }
  return crc;
}
