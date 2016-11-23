// TEST PROGRAM (REV02m) FOR PROGRAMING KURUMI
// MADE BY HIROFUMI INOMATA, 26TH, JAN. 2016 (C)
// AND PRESENTED WITH BSD LICENSE,
// WITH TESTING ONLY ONE CASE
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
//#include <sys/ioctl_compat.h>
#include <termios.h>

#include "../ftd2xx.h"

int ftdi_open( const char *, int );
ssize_t ftdi_write( int , const void *, size_t );
ssize_t ftdi_read( int , void *, size_t );

FT_HANDLE ftHandle = NULL;

unsigned char buf[ 256*1024 ]; // 256KB
unsigned long bufLen;

unsigned char res[256];

unsigned char cmd[16];
unsigned char dat[256+4];

void setChksum( unsigned char *pkt )
{
	unsigned int len;
	int chksum;
	int i;

	len = pkt[1];
	if (len == 0) len = 256;
	
	for ( i=0, chksum=0; i<(len+1); i++ ) {
		chksum -= pkt[i+1];
	}
	pkt[len+2] = chksum & 0xff;
}

int main( int argc, char **argv )
{
	int ret;
	int i;
	int pathCom;
	struct termios termios;
	unsigned int stat_org, stat_cur;
	int cLen;
	FT_STATUS ftStatus;
	DWORD len_rx, len_tx, event;

	if ( argc < 2 ) {
		fprintf( stderr, "less arg (%d)\n", argc );
		exit(-1);
	}

	for ( bufLen=0; bufLen<sizeof(buf); bufLen++ ) {
		ret = read ( STDIN_FILENO, &(buf[bufLen]), sizeof(char) );
		if (ret < sizeof(char) ) break;
	}
	fprintf( stderr, "Program Size = %08ld Byte\n", bufLen );

	// open com port
	pathCom = ftdi_open ( argv[1], O_RDWR | O_NONBLOCK |  O_NOCTTY );
	if (pathCom == -1) {
		fprintf( stderr, "ERROR(%d): can't open com port\n", __LINE__ );
	}

printf( "GO 1\n" );
	// tcgetattr( pathCom, &termios );
	// cfsetspeed( &termios, B115200 );
	// termios.c_cflag &= (~CSIZE);
	// termios.c_cflag |= (CS8);
	// termios.c_cflag |= (CSTOPB);
	// termios.c_cflag &= (~PARENB);
	// termios.c_iflag |= IGNPAR;
	// tcsetattr( pathCom, TCSANOW, &termios );
	{
		ftStatus = FT_SetBaudRate( ftHandle, 115200 );
		ftStatus = FT_SetDataCharacteristics(ftHandle, 
		     FT_BITS_8,
		     FT_STOP_BITS_2,
		     // FT_STOP_BITS_1,
		     FT_PARITY_NONE);
		ftStatus = FT_SetFlowControl( ftHandle, FT_FLOW_NONE, 
			0x11, 0x13 );
	}

	ftStatus = FT_SetDtr( ftHandle );	// Set DTR

	ftStatus = FT_SetBreakOn( ftHandle );	// start break

	// tcflush( pathCom, TCIOFLUSH );	// flush i/0 buffer
	ftStatus = FT_Purge( ftHandle, FT_PURGE_TX | FT_PURGE_RX );

	// stat_cur = stat_org & (~TIOCM_DTR);

	ftStatus = FT_ClrDtr( ftHandle );	// Clear DTR

	// usleep( 730 );
	usleep( 1000 );

	ftStatus = FT_SetBreakOff( ftHandle ); // finish break

	// tcflush( pathCom, TCIOFLUSH );	// flush i/o buffer
	ftStatus = FT_Purge( ftHandle, FT_PURGE_TX | FT_PURGE_RX );

	//usleep( 20 );
	usleep( 50 );


	cLen = 0;
	cmd[cLen++] = 0x00;	// 2-wire
	ret = ftdi_write( pathCom, cmd, cLen );
	if ( ret < cLen ) {
		fprintf( stderr, "ERROR(%d): can't write 2-wire comand!\n", 
			__LINE__ );
		exit(-1);
	}

	//usleep( 65 );
	usleep( 100 );

	// set bps
	cLen = 0;
	cmd[cLen++] = 0x01;
	cmd[cLen++] = 0x03;
	cmd[cLen++] = 0x9a;
	cmd[cLen++] = 0x00;	// 115200 bps
	cmd[cLen++] = 0x21;	// 3.3V
	cmd[cLen++] = 0x00;
	cmd[cLen++] = 0x03;
	setChksum( cmd );
	ret = ftdi_write ( pathCom , cmd, cLen );
	if ( ret < cLen ) {
		fprintf( stderr, "ERROR(%d): can't write bps setup comand!\n", 
			__LINE__ );
		exit(-1);
	}

	for (;;) {
		ret = ftdi_read ( pathCom, res, 1 );
		if ( ret <= 0) {
			printf( "wate for ...\n" );
			continue;
		}
		if ( res[0] == 0x03 ) break;
		fprintf( stderr, "%02x:", res[0] );
	}
	fprintf( stderr, "%02x:", res[0] );
	for (;;) {
		ret = ftdi_read ( pathCom, res, 1 );
		if ( ret <= 0) {
			printf( "wate for ...\n" );
			continue;
		}
		if ( res[0] == 0x03 ) break;
		fprintf( stderr, "%02x:", res[0] );
	}
	fprintf( stderr, "%02x:", res[0] );
	fprintf( stderr, "\n\n" );

	// tcflush( pathCom, TCIOFLUSH );	// flush i/0 buffer
	ftStatus = FT_Purge( ftHandle, FT_PURGE_TX | FT_PURGE_RX );

	for (i=0;i<256;i++ ) {
		long adr = i * 1024;
		// Block Erase
		cLen = 0;
		cmd[cLen++] = 0x01;
		cmd[cLen++] = 0x04;
		cmd[cLen++] = 0x22;
		cmd[cLen++] = (adr & 0xff);	// L
		cmd[cLen++] = ((adr >> 8 ) & 0xff);	// M
		cmd[cLen++] = ((adr >> 16 ) & 0xff);	// H
		cmd[cLen++] = 0x00; // SUM
		cmd[cLen++] = 0x03; // TEX
		setChksum( cmd );
		ret = ftdi_write ( pathCom , cmd, cLen );
		if ( ret < cLen ) {
			fprintf( stderr, 
			"ERROR(%d): can't write Block Erase Command!\n", 
			__LINE__ );
			exit(-1);
		} else {
			fprintf( stderr, 
			"Addr: %ld, write Block Erase command\n", adr );
		}

		for (;;) {
			ret = ftdi_read ( pathCom, res, 1 );
			if ( ret <= 0) {
				printf( "wate for ...\n" );
				continue;
			}
			if ( res[0] == 0x03 ) break;
			fprintf( stderr, "%02x:", res[0] );
		}
		fprintf( stderr, "%02x:", res[0] );
		fprintf( stderr, "\n\n" );
	}

	usleep(10);

	// send programming command
	cLen = 0;
	cmd[cLen++] = 0x01;
	cmd[cLen++] = 0x07;
	cmd[cLen++] = 0x40;
	cmd[cLen++] = 0x00; // start L = 0
	cmd[cLen++] = 0x00; // start M = 0
	cmd[cLen++] = 0x00; // start H = 0
	cmd[cLen++] = 0xff; // ((bufLen-1) & 0xff); // end L
	cmd[cLen++] = (((bufLen-1) >> 8) & 0xff); // end M
	cmd[cLen++] = (((bufLen-1) >> 16) & 0xff); // end H
	cmd[cLen++] = 0x00; // Chksum
	cmd[cLen++] = 0x03; // ETX
	setChksum( cmd );
	fprintf( stderr, "HML = %02x%02x%02x\n", cmd[8],cmd[7],cmd[6] );
	ret = ftdi_write ( pathCom , cmd, cLen );
	if ( ret < cLen ) {
		fprintf( stderr, 
		"ERROR(%d): can't write Programming Command\n", 
		__LINE__ );
		exit(-1);
	} else {
		fprintf( stderr, "write programming command\n" );
	}

	for (;;) {
		ret = ftdi_read ( pathCom, res, 1 );
		if ( ret <= 0) {
			printf( "wate for ...\n" );
			continue;
		}
		if ( res[0] == 0x03 ) break;
		fprintf( stderr, "%02x:", res[0] );
	}
	fprintf( stderr, "%02x:", res[0] );

	ftStatus = FT_Purge( ftHandle, FT_PURGE_TX | FT_PURGE_RX ); // flush
	fprintf( stderr, "\n\n" );

	// send data
	for ( i=0; i<bufLen; i+=256 ) {
		int len;
		int j;

		usleep( 5 );
		//sleep( 5 );

		dat[0] = 0x02;
		dat[1] = 0;

		len = 256;
		// fprintf( stderr, "Addr:%d, Size:%d goes\n", i, dat[1] );
		// fprintf( stderr, "%d - %d\n", i, dat[1] );

		for (j=0; j<len; j++ ) {
			dat[2+j] = buf[i+j];
		}
		setChksum( dat );
		if ((i+j) >= (bufLen) ) {
			dat[3+len] = 0x03;	// end
			fprintf( stderr, " end<- " );
		} else {
			dat[3+len] = 0x17;	// not end
			fprintf( stderr, " cnt<- " );
		}
		ret = ftdi_write ( pathCom , dat, (4+len) );
		if (ret < (4+len) ) {
			fprintf( stderr, "ERROR(%d): can't write data!\n", 
			__LINE__ );
		} else {
			fprintf( stderr, "Done data write (ret=%d)\n", ret );
		}

		for (;;) {
			ftStatus = FT_GetStatus( ftHandle, &len_rx, 
				&len_tx, &event );
			if ( ftStatus != FT_OK ) {
				printf( "FT_GetStatus erroe\n" );
			}
			if ( len_rx > 0 ) break;
			// printf( "ev = %d\n", event );
		}

		for (;;) {
			ret = ftdi_read ( pathCom, res, 1 );
			if ( ret <= 0) {
				printf( "wate for ...\n" );
				continue;
			}
			if ( res[0] == 0x03 ) break;
			fprintf( stderr, "%02x:", res[0] );
		}
		fprintf( stderr, "%02x:", res[0] );
		fprintf( stderr, "\n\n" );
	}

	for (;;) {
		ret = ftdi_read ( pathCom, res, 1 );
		if ( ret <= 0) {
			printf( "wate for ...\n" );
			continue;
		}
		if ( res[0] == 0x03 ) break;
		fprintf( stderr, "%02x:", res[0] );
	}
	fprintf( stderr, "%02x:", res[0] );
	fprintf( stderr, "\n\n" );

	ftStatus = FT_SetDtr( ftHandle );	// set DTR
	
	usleep(1000);

	ftStatus = FT_ClrDtr( ftHandle );	// clear DTR

	ftStatus =  FT_Close( ftHandle );
}


int ftdi_open( const char *path, int option ) {
	FT_STATUS ftStatus;
	int iNumDevs = 0;
	char *pcBufLD[2];
	char cBufLD[64];
	pcBufLD[0] = cBufLD;
	pcBufLD[1] = NULL;

	ftStatus = FT_ListDevices( pcBufLD, &iNumDevs, 
		FT_LIST_ALL | FT_OPEN_BY_SERIAL_NUMBER );
        if(ftStatus != FT_OK) {
                printf( "Error: FT_ListDevices(%d)\n", (int)ftStatus );
                return -1;
        }
	printf( "iNumDevs= %d\n", iNumDevs );
	printf( "cBufLD = %s\n", cBufLD );
	if( (ftStatus = FT_OpenEx( cBufLD, 
		FT_OPEN_BY_SERIAL_NUMBER, &ftHandle)) != FT_OK ) {
		/*
			This can fail if the ftdi_sio driver is loaded
			use lsmod to check this and rmmod ftdi_sio to remove
			also rmmod usbserial
		*/
		printf("Error FT_OpenEx(%d), device %d\n", (int)ftStatus, 0 );
		printf("Use lsmod to check if ftdi_sio (and usbserial) "
			"are present.\n");
		printf("If so, unload them using rmmod, as they conflict "
			"with ftd2xx.\n");
		return -1;
	}
	return 0;
}

ssize_t ftdi_write( int fd, const void *buf, size_t len ) {
	FT_STATUS ftStatus;
	DWORD dwBytesWritten;
	dwBytesWritten = 0;

	ftStatus = FT_Write( ftHandle, buf, len, &dwBytesWritten );
	if ( ftStatus != FT_OK ) {
		printf( "write error\n" );
	}

	return dwBytesWritten;
}

ssize_t ftdi_read( int fd, void *buf, size_t len ) {
	FT_STATUS ftStatus;
	DWORD dwBytesRead;
	dwBytesRead = 0;

	ftStatus = FT_Read( ftHandle, buf, len, &dwBytesRead );
	if ( ftStatus != FT_OK ) {
		printf( "read error\n" );
	}

	return dwBytesRead;
}

