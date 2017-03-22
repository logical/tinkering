/*
  STM32 based OV7670 handling
  ================================
  Pin connections:
  ----------------
    OV7670  STM32F103C8T6 generic
  1.  Vcc   3.3V
  2.  GND   GND
  3.  SCL   PB6
  4.  SDA   PB7
  5.  Vsync   PB4 (in)
  6.  Href    PB5 (in)
  7.  Pclk    PB3 (in)
  8.  Xclk    PB0 (out)
  9.  D7..D0  PA7..PA0  (in)

*/

#include <Wire.h>
#include "ov7670.h"

// pin definitions
#define SCL_PIN    PB6
#define SDA_PIN   PB7
#define VSYNC_PIN PB4
#define HREF_PIN  PB5
//b3 is mapped to jtag so I switched to b8
#define PCLK_PIN  PB8
#define XCLK_PIN  PB0 // timer 3 channel 3

#define LED_PIN   PC13
#define LED_ON    ( (GPIOC_BASE)->BRR = BIT13 )
#define LED_OFF   ( (GPIOC_BASE)->BSRR = BIT13 )

#define IDR_P ( (GPIOB_BASE)->IDR )
#define VSYNC (IDR_P & BIT4)  // PB4
#define HREF  (IDR_P & 0b100000)  // PB5
#define PCLK  (IDR_P & 0b100000000)  // PB8

#define SCCB Wire

char str[150];  // for sprintf
int debug;

USBSerial USBS;
//#define USBS USBS
/*****************************************************************************/
/* Configuration: this lets you easily change between different resolutions
   You must uncomment only one, no more no less */

//#define USE_VGA
//#define USE_QVGA
#define USE_QQVGA // 160x120

/*****************************************************************************/
// Timer 3 channel 3 is mapped on PB0 (see Reference Manual table 44)
// Set Timer 3 channel 3 to generate PWM with 8MHz
/*****************************************************************************/
#define TIMER_RELOAD       9 // to divide the system clock
#define TIMER_FREQUENCY     ( 72000000/TIMER_RELOAD )
/*****************************************************************************/
void Timer_SetupPWM()
{
  // set timer 3 channel 3 in PWM 1 mode
  timer_dev *timerDevice  = TIMER3;
  uint8 timerChannel      = TIMER_CH3;
  timer_init(timerDevice);  // feed timer 3 registers with system clock
  pinMode(XCLK_PIN, PWM);
  timer_pause(timerDevice); // stop timer
  timer_set_reload(timerDevice, (TIMER_RELOAD - 1));
  timer_set_compare(timerDevice, timerChannel, TIMER_RELOAD / 2); // duty cycle = 50%
  timer_set_mode(timerDevice, timerChannel, TIMER_PWM);
  timer_resume(timerDevice);  // let timer run
}
/*****************************************************************************/
void setup()
{
  debug = 1;  // let USBS output print debug messages
  USBS.begin(500000); // USB is always 12 Mbit/sec
  USBS.println();
  delay(5000);
  USBS.println("*** STM32 application for OV7670 camera ***");
  // initialize digital pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(VSYNC_PIN, INPUT);
  pinMode(HREF_PIN, INPUT);
  /* Disable the USBS Wire Jtag Debug Port SWJ-DP */
  pinMode(PCLK_PIN, INPUT);
  //pinMode(XCLK_PIN, OUTPUT); - done in timer setup

  // setup data pins
  USBS.print("Configuring ports...");
  for (int p = PA0; p < PA0 + 8; p++) {
    pinMode(p, INPUT);
  }
  USBS.println("done."); delay(10);
  //Check_SDA_SCL();
  USBS.print("Configuring camera...");
  Timer_SetupPWM(); // start generating XCLK
  //SCCB.begin();
  Cam_Init();
  USBS.println("done."); delay(10);
  // read pixels
  //  CountLinePixels();
}
/*****************************************************************************/
/*****************************************************************************/
#define NR_LINES      120 // QQVGA resolution
#define MISSING_LINES 40  // due to RAM limitations we cannot store all lines
#define BUF_SIZE_Y  (NR_LINES-MISSING_LINES)
#define BUF_SIZE_X  (160-MISSING_LINES)
uint8_t img_buf[BUF_SIZE_Y][BUF_SIZE_X] __attribute__((packed, aligned(1))); // store only 100 lines a 156 bytes, packed in 32 bit wide data - to easy access
//uint16_t pxl_buf[640];
uint8_t line_buf[BUF_SIZE_X] __attribute__((packed, aligned(1)));
int hist[256];
//int hist_pixels;
#define CAM_DATA  ( ((GPIOA_BASE)->IDR) & 0x000000FF )  // store only the lower 8 bits
/*****************************************************************************/
static void CountLinePixels(void)
{
  uint16_t pck, line, index;

  int wa = 0; // do warm-up frames, optional, set it to 0 if not used
  while ( (wa--) > 0) {
    //Wait for vsync
    USBS.print("wait for Vsync high...\n");
    while ( !VSYNC ); //wait for high
    USBS.print("wait for Vsync low...\n");
    while ( VSYNC ); //wait for low
  }
  /*
    for (int j=0; j<BUF_SIZE_Y; j++) {
    memset(img_buf[j],100,BUF_SIZE_X);
    }
  */

  line = 0;
  LED_ON; // debug
  noInterrupts();

  while ( VSYNC ); //wait for low
  while ( !VSYNC) // don't process all lines - not enough RAM
  {
    if (line < BUF_SIZE_Y) {
      pck = 0;
      index = 0;
      // wait for HREF
      while ( !HREF ); //wait for high

      while ( HREF )  // process while high
      {

        while ( !PCLK ); // wait for high

        if ( pck & 0x01 && (index < BUF_SIZE_X) ) { // store only the Y component, the odd bytes, in case of YUV422
          img_buf[line][index++] = CAM_DATA;
        }

        pck++;
        //      while( PCLK ); // wait for low
      }

      //    pxl_buf[line++] = pck;  // store nr. of detected pixel clocks for each row - debug only
      line ++;
      // end of line, check for next line or vertical sync
    }
  }

  interrupts();
  LED_OFF;

  //  USBS.println("> image acquisition done."); //delay(10);
  /*
    for (int i=0; i<line; i++) {
      sprintf(str, "line %02u\: %u\n", i, pxl_buf[i]); USBS.print(str);
    }
  */
  // IMG_Parse_chars();
}
/*****************************************************************************/
/*****************************************************************************/
void BlinkError(int code)
{
  while (1) {
    while (code--) {
      LED_ON;
      delay(200);
      LED_OFF;
      delay(200);
    }
    delay(500);
  }

}

/*****************************************************************************/
/*****************************************************************************/
void SendImgData()
{
#define BUF_SIZE (BUF_SIZE_X * BUF_SIZE_Y)

  // print out the data to USBS
  for (int j = 0; j < BUF_SIZE_Y; j++) {
    USBS.write(img_buf[j], BUF_SIZE_X);
    delay(1);
  }

  //  delay(250); // leave time for host to process previous data
}
/*****************************************************************************/
// the loop function runs over and over again forever
/*****************************************************************************/
void loop()
{ // restart image data reading if USBS character detected
  if ( USBS.available() > 0 ) {
    //while ( USBS.available()>0 )
    char c = USBS.read(); // read all dummy bytes
    if (c == 'a') {
      CountLinePixels();
      SendImgData();
    }
  }
}



/*****************************************************************************/
/*****************************************************************************/
void Cam_Init()
{
  SCCB.begin(); // start I2C bus
  LED_ON;
  uint8_t pid, ver;
  do {  // read PID and VER
    pid = rdReg(REG_PID);
    ver = rdReg(REG_VER);
    sprintf(str, "PID: %02X, VER: %02X", pid, ver); USBS.println(str);
  } while (pid != 0x76 || ver < 70);

  wrReg(REG_COM7, COM7_RESET);  // reset all internal registers to default values
  delay(100);

  wrRegs(ov7670_default); // load camera default settings
  // customize settings
  wrRegs(yuv422_ov7670);  // set color
  wrRegs(qcif_ov7670); // scaling
  /*
    // extra scaling
    wrReg(REG_SCALING_DCWCTR, 0x22); //Down sample by 4
    wrReg(REG_SCALING_PCLK_DIV, 0x02); //Clock div8
  */
  /* optional settings */
  //
  //  wrReg(REG_COM14, (BIT4|BIT0) ); // PCLK divider
  //  wrReg(0x73, 0x04); // PCLK divider
  //  wrReg(REG_ABLC1, BIT2); // automatic black level correction
  wrReg(REG_BRIGHT, 0xb0);  // brightness -2
  wrReg(REG_CONTRAS, 0x60); // contrast +2
  //wrReg(REG_COM8, 0xe7);  // AWB on
  //wrReg(AWBCTR0, 0x9f); // Simple AWB
  //  wrReg(REG_MVFP, MVFP_MIRROR|MVFP_FLIP); // mirror&flip image
  wrReg(REG_COM10, COM10_PCLK_HB); //pclk does not toggle on HBLANK
  //  wrReg(REG_COM7,0x02);// enable color bar - for testing only !!!
  //wrReg(REG_CLKRC,0x3); // clock divider
  LED_OFF;
}
/*****************************************************************************/
/*****************************************************************************/
void Cam_DumpRegs(uint8_t nr)
{
  USBS.print("ADDR\tVALUE\n");
  for (int i = 0; i < 0xC9; i++) {
    sprintf(str, "0x%02X,\t0x%02X\n", i, (rdReg(i) & 0x00FF));
    USBS.print(str);
  }
}
/*****************************************************************************/
/*****************************************************************************/
int rdReg(uint8_t regAddr)
{
  SCCB.i2c_start();
  /* check the I2C lines, optional
    if ( digitalRead(SCL_PIN)==1 || digitalRead(SDA_PIN)==1 ) {
    USBS.print("Wire read start error! System halted!");
    while(1);
    } */
  SCCB.i2c_shift_out( camAddr_WR );
  if (!SCCB.i2c_get_ack())
  {
    USBS.println("-NACK10-"); // debug
    SCCB.i2c_stop();
    return -1;
  }
  SCCB.i2c_shift_out( regAddr );
  if (!SCCB.i2c_get_ack())
  {
    USBS.println("-NACK11-"); // debug
    SCCB.i2c_stop();
    return -1;
  }
  SCCB.i2c_stop();
  // restart for read access
  SCCB.i2c_start();
  SCCB.i2c_shift_out( camAddr_RD );
  if (!SCCB.i2c_get_ack())
  {
    USBS.println("-NACK12-"); // debug
    SCCB.i2c_stop();
    return -1;
  }
  // read here
  uint8_t dat = SCCB.i2c_shift_in();
  SCCB.i2c_send_nack();
  SCCB.i2c_stop();
  return dat;
}
/*****************************************************************************/
/*****************************************************************************/
int wrReg(uint8_t regAddr, uint8_t regVal)
{
  //sprintf(str, "addr: %02X, val: %02X", regAddr, regVal); USBS.println(str);
  SCCB.i2c_start();
  /* check the I2C lines, optional
    if ( digitalRead(SCL_PIN)==1 || digitalRead(SDA_PIN)==1 ) {
    USBS.println("Wire write start error! System halted!");
    while(1);
    } */
  SCCB.i2c_shift_out( camAddr_WR );
  if (!SCCB.i2c_get_ack()) {
    USBS.println("-NACK00-"); // debug
    SCCB.i2c_stop();
    return -1;
  }
  SCCB.i2c_shift_out( regAddr );
  if (!SCCB.i2c_get_ack()) {
    USBS.println("-NACK01-"); // debug
    SCCB.i2c_stop();
    return -1;
  }
  SCCB.i2c_shift_out(regVal);
  if (!SCCB.i2c_get_ack()) {
    USBS.println("-NACK02-"); // debug
    SCCB.i2c_stop();
    return -1;
  }
  SCCB.i2c_stop();
  return 0;
}
/*****************************************************************************/
/*****************************************************************************/
void wrRegs(const struct regval_list reglist[])
{
  uint8_t reg_addr, reg_val;
  const struct regval_list *next = reglist;
  while (1) {
    reg_addr = next->reg_num;
    reg_val = next->value;
    if ( (reg_addr == 0xff) & (reg_val == 0xff) ) break;
    while (wrReg(reg_addr, reg_val) == -1);
    next++;
  }
}
