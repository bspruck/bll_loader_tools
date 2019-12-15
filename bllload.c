/*
 * Allows to set arbitrary speed for the serial device on Linux.
 * stty allows to set only predefined values: 9600, 19200, 38400, 57600, 115200, 230400, 460800.
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
//#include <termios.h>
#include <fcntl.h>
// #include <sys/ioctl.h>
#include <asm/termios.h>

int main(int argc, char* argv[]) 
{
    int serial=0, fd=0;
	struct termios oldtio, newtio;

    if (argc < 2) {
        printf("%s filename device speed\n%s file.o /dev/ttyUSB0 9600\n", argv[0], argv[0]);
        return -1;
    }
    
    int baudrate=9600;
    if(argc>=4) baudrate=atoi(argv[3]);
    char *serialdevice="/dev/ttyUSB0";
    char *filename="test.o";
    
    if( argc>=3) serialdevice=argv[2];
    if( argc>=2) filename=argv[1];

	serial = open(serialdevice, O_RDWR | O_NOCTTY ); 

	if (serial < 0) 
	{
		perror(serialdevice); 
		exit(-1); 
	}

// 	/* save current serial port settings */
	tcgetattr(serial, &oldtio); 

	/* clear struct for new port settings */
	bzero(&newtio, sizeof(newtio)); 

	/* 
	 * BAUDRATE : Set bps rate. You could also use cfsetispeed and cfsetospeed.
	 * CRTSCTS  : output hardware flow control (only used if the cable has
	 * all necessary lines. See sect. 7 of Serial-HOWTO)
	 * CS8      : 8n1 (8bit, no parity, 1 stopbit)
	 * CLOCAL   : local connection, no modem contol
	 * CREAD    : enable receiving characters
	 */
	newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD | PARENB; // CRTSCTS removed

	/*
	 * IGNPAR  : ignore bytes with parity errors
	 * ICRNL   : map CR to NL (otherwise a CR input on the other computer
	 * will not terminate input)
	 * otherwise make device raw (no other input processing)
	 */
	newtio.c_iflag = IGNPAR;

	/* Raw output , Lynx want even parity */
	newtio.c_oflag = 0;

	/*
	 * ICANON  : enable canonical input
	 * disable all echo functionality, and don't send signals to calling program
	 */
	newtio.c_lflag = 0; // non-canonical

	/* 
	 * set up: we'll be reading 4 bytes at a time.
	 */
	newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;     /* blocking read until n character arrives */

	/* 
	 * now clean the modem line and activate the settings for the port
	 */
	tcflush(serial, TCIFLUSH);
	tcsetattr(serial, TCSANOW, &newtio);

	// Linux-specific: enable low latency mode (FTDI "nagling off")
// 	ioctl(serial, TIOCGSERIAL, &ser_info);
// 	ser_info.flags |= ASYNC_LOW_LATENCY;
// 	ioctl(serial, TIOCSSERIAL, &ser_info);

    struct termios2 tio;
    ioctl(serial, TCGETS2, &tio);
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = baudrate;
    tio.c_ospeed = baudrate;
    ioctl(serial, TCSETS2, &tio);
    

    if(serial){
        struct termios2 tio2;
        ioctl(serial, TCGETS2, &tio2);
        printf("Check: Baudrate %d %d\n",tio2.c_ispeed,tio2.c_ospeed);
    }

    fd = open(filename, O_RDONLY);
	if(fd<0){
		perror(filename); 
		exit(-1); 
	}
	
	unsigned char header[10];
    int r;
    r = read(fd,header,10);
    if(r!=10) printf("len_fail\n");
//                     dc.w $8008      ; BRA *+8
//                 dc.w Start
//                 dc.w Len        ; _with_ Header
//                 dc.l "BS93"
// 
    header[0]=0x81;
    header[1]='P';
    unsigned int len;
    len=header[5]+(header[4]<<8);
    len-=10;
    if(len>64*1024){
        printf("Invalid size!\n");
        exit(-1);
    }
    printf("Len: %d\n",len);
    unsigned int xlen=len^0xFFFF;
    header[4]=(xlen>>8)&0xFF;
    header[5]=xlen&0xFF;
    
    
// - The ComLynx-Loader wants :
// 
// start-sequence : $81,"P"                        ; command : load program
// init-sequence  : LO(Start),HI(Start)            ; start address = dest.
//                  LO((Len-10) XOR $FFFF),HI((Len-10) XOR $FFFF)
//                                                 ; and len of data
// Xmit-sequence  : ....                           ; data
// checksum       : none at all !!
// 
    unsigned char buffer[64*1024];
    unsigned char buffer2[64*1024];
    r=read(fd,buffer,len);
    if(r!=len) printf("len_fail\n");
    close(fd);

    printf("\nSend Header\n");

    int i;
    for( i=0; i<6; i++){
        unsigned char x;
        printf("H%d $%X ",i,header[i]);
        write(serial,&header[i],1);
        x=0;
        read(serial,&x,1);
        if(x!=header[i]){ printf("$%X $%X\n",header[i],x); exit(-1);}
    }
    printf("\nSend Data \n");
//    write(serial,buffer,len);
    int offw=0, offr=0;
     while(offw<len){
         r=write(serial,&buffer[offw],1024<len-offw?1024:len-offw);
         if(r>0) offw+=r;
         printf (" Wr %d \n",r);

         r=read(serial,&buffer2[offr],len-offr);
         if(r>0) offr+=r;
         printf (" Rd %d \n",r);
     }

//     if (r == 0) {
//         printf("Changed successfully.\n");
//     } else {
//         perror("ioctl");
//     }

     while(offr<len){
        r=read(serial,&buffer2[offr],len-offr);
         if(r>0){
             offr+=r;
            printf (" Rd %d \n",r);
            sleep(1);
         }
     }
    if(memcmp(buffer,buffer2,len)!=0){
        printf("read back compare failed\n");
        for(int i=0; i<len; i++){
            if(buffer[i]!=buffer2[i]){
                printf("\n%d: %d %d\n",i,buffer[i],buffer2[i]);
                break;
            }
        }
    }else{
        printf("Readback O.k.\n");
    }
    
	tcflush(serial, TCIFLUSH);
	/* restore the old port settings */
	tcsetattr(serial, TCSANOW, &oldtio);
    close(serial);
	printf("\ndone!\n");
}

