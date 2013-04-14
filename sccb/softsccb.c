#include <stdlib.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "softsccb.h"

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

static SCCBStruct SCCBMaster;

void SCCBWrite(unsigned char DataByte,unsigned char rw){
	SCCBMaster.currentBit=10;
	if(SCCBMaster.state!=SCCB_DONE){
		SCCBMaster.byteRegister=((DataByte<<1)+rw)<<1;
	}
	else {
		SCCBMaster.byteRegister=(DataByte<<1)+rw;
	}
	SCCBMaster.state=SCCB_WRITE;
	while(SCCBMaster.currentBit>0){}
}

unsigned char SCCBRead(void){
	SCCBMaster.byteRegister=0;
	SCCBMaster.currentBit=8;
	SCCBMaster.state=SCCB_READ;
	GPIODirModeSet(SCCBMaster.GpioBase,SCCBMaster.sdaGpio,GPIO_DIR_MODE_IN);
	while(SCCBMaster.currentBit>0){}
	return SCCBMaster.byteRegister;
}

void SCCBTick(void){
	static unsigned char quarter=0;
 	if(SCCBMaster.state==SCCB_READ){
		switch(quarter){
			case 0:

				break;
			case 1:
				GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sclGpio,0);
				break;
			case 2:
//acknowledge byte
				if(SCCBMaster.currentBit<2){
					GPIODirModeSet(SCCBMaster.GpioBase,SCCBMaster.sdaGpio,GPIO_DIR_MODE_OUT);
					GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sdaGpio,0);
				}
				else{
					SCCBMaster.byteRegister=SCCBMaster.byteRegister<<1;
					if(GPIOPinRead(SCCBMaster.GpioBase,SCCBMaster.sdaGpio)){
						SCCBMaster.byteRegister++;
					}
				}
				break;
			case 3:
				GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sclGpio,SCCBMaster.sclGpio);
				if(SCCBMaster.currentBit==1)quarter=0;
				SCCBMaster.currentBit--;
				break;
		}

	}

	else{

		switch(quarter){
			case 0:
				//first bit sda goes low
				//
				break;
			case 1:
				//clock goes low
				GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sclGpio,0);
				break;
			case 2:
				//read/write bit to write
				if(SCCBMaster.currentBit==1){
					GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sdaGpio,0);
				}
				else{
					GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sdaGpio,SCCBMaster.byteRegister&256?SCCBMaster.sdaGpio:0);
				}
				break;
			case 3:
				//clock goes high
				GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sclGpio,255);
				//last bit is finished
				SCCBMaster.byteRegister=SCCBMaster.byteRegister<<1;
				if(SCCBMaster.currentBit==1)quarter=0;
				SCCBMaster.currentBit--;
				break;
		}
	}
	quarter++;
	if(quarter>3)quarter=0;
}

void
SCCBTimerIntHandler(void)
{
	//TODO
	//Change to whatever timer you are using
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	if(SCCBMaster.currentBit>0)SCCBTick();
}

void SCCBInit(unsigned long setbase,
			unsigned char setscl,
			unsigned char setsda
			){
//	memset(&SCCBMaster, 0, sizeof(SCCBMaster));
    SCCBMaster.currentBit=0;
    SCCBMaster.GpioBase=setbase;
    SCCBMaster.sclGpio=setscl;
    SCCBMaster.sdaGpio=setsda;
    SCCBMaster.state=SCCB_DONE;

    //MAP_GPIOPinTypeI2C(GPIO_PORTA_BASE,GPIO_PIN_6|GPIO_PIN_7);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);



	GPIODirModeSet(SCCBMaster.GpioBase,SCCBMaster.sclGpio, GPIO_DIR_MODE_OUT);
    GPIOPadConfigSet(SCCBMaster.GpioBase,SCCBMaster.sclGpio,GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_OD);

    //
    // Set the SCL pin high.
    //
    GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sclGpio,255);

    //
    // Configure the SDA pin.
    //
    GPIODirModeSet(SCCBMaster.GpioBase,SCCBMaster.sdaGpio,GPIO_DIR_MODE_OUT);
    GPIOPadConfigSet(SCCBMaster.GpioBase ,SCCBMaster.sdaGpio,GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_OD);

    //
    // Set the SDA pin high.
    //
    GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sdaGpio,255);
    //
    // Configure the timer to generate an interrupt at a rate of 40KHz.  This
    // will result in a I2C rate of 100 KHz.
    // TODO: change this to whichever timer you are using.
    // TODO: change this to whichever I2C rate you require.
    //
    TimerConfigure(TIMER0_BASE, TIMER_CFG_32_BIT_PER);
    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 400000);
//    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
//    TimerEnable(TIMER0_BASE, TIMER_A);
//    IntEnable(INT_TIMER0A);
}
void SCCBStart(void){
    //
    // Enable the timer interrupt.
    // TODO: change this to whichever timer interrupt you are using.
    //

	IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerEnable(TIMER0_BASE, TIMER_A);
	GPIODirModeSet(SCCBMaster.GpioBase,SCCBMaster.sdaGpio,GPIO_DIR_MODE_OUT);
	GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sdaGpio,0);
//	SysCtlDelay(250);
}
void SCCBStop(void){
    //
    // Enable the timer interrupt.
    // TODO: change this to whichever timer interrupt you are using.
    //
	GPIOPinWrite(SCCBMaster.GpioBase, SCCBMaster.sdaGpio,SCCBMaster.sdaGpio);
	SCCBMaster.state=SCCB_DONE;
	IntDisable(INT_TIMER0A);
    TimerIntDisable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
}


tBoolean SCCBBusy(void){
	return SCCBMaster.state!=SCCB_DONE;
}
