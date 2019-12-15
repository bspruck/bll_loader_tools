
#include <stdio.h>

int main(int argc,char *argv[])
{
  if( argc<3){
      printf("usage: %s in out\n",argv[0]);
      return -1;
  }
  FILE *fin, *fout;
  fin=fopen(argv[1],"rb");
  fout=fopen(argv[2],"wb+");
  if( fin && fout){
    unsigned int rfl=0, count=0;
    int c, cc;
    c=fgetc(fin);
    rfl|=((c^0xFF)&0xFF)<<16;
    c=fgetc(fin);
    rfl|=((c^0xFF)&0xFF)<<8;
    c=fgetc(fin);
    rfl|=((c^0xFF)&0xFF);
    printf("Size: %u %X\n",rfl,rfl);
    while(count<rfl){
      int i;
      cc=0;
      for( i=0; i<16; i++){
	c=fgetc(fin);
	fputc(c,fout);
	cc^=c;
	count++;
	printf("%02X ",c&0xFF);
      }
      c=fgetc(fin);
      c&=0xFF;
      cc&=0xFF;
      printf("= %02X %02X\n",c,cc);
      if(c!=cc){
	printf("CRC failed!!\n");
	break;
      }
    }
    printf("Count: %d %X of %d %X",count,count,rfl,rfl);
  }
  if(fout) fclose(fout);
  if(fin) fclose(fin);
  return 0;
}