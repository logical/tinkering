#ifndef SOFTSCCB_H
#define SOFTSCCB_H

#define SCCB_READ 2
#define SCCB_WRITE 1
#define SCCB_DONE 0

typedef struct{
	unsigned int byteRegister;
	unsigned char currentBit;
	unsigned long GpioBase;
	unsigned char sclGpio;
	unsigned char sdaGpio;
	unsigned char state;
}SCCBStruct;

void SCCBInit(unsigned long setbase,unsigned char setscl,unsigned char setsda);
void SCCBStart(void);
void SCCBStop(void);
unsigned char SCCBRead(void);
void SCCBWrite( unsigned char DataByte,unsigned char rw);
tBoolean SCCBBusy(void);
void SCCBTick(void);
void SCCBTimerIntHandler( void);

#endif
