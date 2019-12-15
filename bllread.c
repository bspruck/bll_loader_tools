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

    if (argc < 1) {
        printf("%s device speed\n%s /dev/ttyUSB0 9600\n", argv[0], argv[0]);
        return -1;
    }
    
    int baudrate=9600;
    if(argc>=3) baudrate=atoi(argv[2]);
    char *serialdevice="/dev/ttyUSB0";
    char *filename="dump.dat";
    
    if( argc>=2) serialdevice=argv[1];

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

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd<0){
		perror(filename); 
		exit(-1); 
	}
	
    printf("\nRecv Data \n");
    unsigned char buffer[64*1024];
    int offr=0;
     while(1){
        int r;
         r=read(serial,buffer,64*1024);
         if(r>0){
             offr+=r;
             write(fd,buffer,r);
            printf (" Rd %d (%d)\n",r,offr);
            if(r<1024) sleep(1);
         }
     }
    
	tcflush(serial, TCIFLUSH);
	/* restore the old port settings */
	tcsetattr(serial, TCSANOW, &oldtio);
    close(serial);
	printf("\ndone!\n");
}

