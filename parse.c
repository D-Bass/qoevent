#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

FILE* fo, *fi, *fs;
uint16_t jl[8192],jl_temp[8192];
char sram[65536];

int main()
{
  int size;
  const char* fname;
  uint16_t* base16;
  uint16_t rec_len;
  uint16_t* j;
  uint16_t offset;
  int i,err_code;

  size=384;
  if((fi=fopen("cshwlog.bin","rb+"))==NULL) return -1;


  while(fgetc(fi)!=0x55) if(feof(fi)) {fclose(fi);return -1;}

  if((fs=fopen("sram.bin","wb+"))==NULL) return -1;
  fread(jl,1,2+size,fi);
  size>>=1;
  fwrite(sram,1,fread(sram,1,65536,fi),fs);
  fclose(fi);
  fclose(fs);

  if((fo=fopen("log.txt","wb+"))==NULL) return -1;

  base16=jl+1;
  j=base16;
  j+=((*(base16-1)&0xffff)>>1)-1;

  offset=j-base16;
  //fprintf(fo,"Log size %i last record %u\xaa\xaa\x55\x55",size,offset);
  for(i=0;i<=offset;i++) jl_temp[i]=*(j-i);
  for(i=0;i<size-offset-1;i++) jl_temp[i+offset+1]=*(j-i-offset+size-1);
  memcpy(base16,jl_temp,size<<1);
  j=base16;

  for(i=0;i<size;i++)
  {
    rec_len=*j>>8;err_code=*j&0xff;

    fprintf(fo,"tail : %04x\n\r",*j++);

    if(!rec_len) break;
    fprintf(fo,"ErrCode %i\n\r",err_code);
    for(i=0;i<rec_len-4;i++)
      fprintf(fo,"rstack%i : %04x\n\r",i,*j++);
    fprintf(fo,"Card in pool : %02x%02x\n\r",(uint16_t)(j[1]>>8|j[1]<<8),(uint16_t)(j[0]>>8|j[0]<<8));j+=2;
    fprintf(fo,"Active card  : %02x%02x\n\r",(uint16_t)(j[1]>>8|j[1]<<8),(uint16_t)(j[0]>>8|j[0]<<8));j+=2;
  }
  fclose(fo);
  return 0;
}
