#include <LPC21xx.H>
#include"header.h"
#define SI ((I2CONSET>>3)&1)
void i2c_init(){
PINSEL0|=0x50;
I2CONSET=1<<6;
I2SCLL=I2SCLH=75;
}
void i2c_write(unsigned char sa,unsigned char mr,unsigned char data){
I2CONSET=1<<5;
I2CONCLR=1<<3;
while(SI==0);
 I2CONCLR=1<<5;
// SA+W	
I2DAT=sa;
I2CONCLR=1<<3;
while(SI==0);
if(I2STAT==0x20){
uart0_tx_string("Error: SEND sa+w\r\n");
uart0_integer(I2STAT);
uart0_tx_string("\r\n");
goto L;
}
//MR
I2DAT=mr;
I2CONCLR=1<<3;
while(SI==0);
if(I2STAT==0x30){
uart0_tx_string("Error: SEND memory\r\n");
uart0_integer(I2STAT);
uart0_tx_string("\r\n");
goto L;
}
//DATA
I2DAT=data;
I2CONCLR=1<<3;
while(SI==0);
if(I2STAT==0x30){
uart0_tx_string("Error: SEND data\r\n");
uart0_integer(I2STAT);
uart0_tx_string("\r\n");
goto L;
}

L:
I2CONSET=1<<4;
I2CONCLR=1<<3;
}
unsigned char i2c_read(unsigned char sa,unsigned char mr){
 uc u,v;
I2CONSET=1<<5;
I2CONCLR=1<<3;
while(SI==0);
 I2CONCLR=1<<5;
//SA+W
I2DAT=sa^1;
I2CONCLR=1<<3;
while(SI==0);
if(I2STAT==0x20){
uart0_tx_string("Error: RECEIVER sa+w not match\r\n");
goto L;
}
//MR
I2DAT=mr;
I2CONCLR=1<<3;
while(SI==0);
if(I2STAT==0x30){
uart0_tx_string("Error: RECEIVER memory address not match\r\n");
goto L;
}
//RESTART
I2CONSET=1<<5;
I2CONCLR=1<<3;
while(SI==0);
 I2CONCLR=1<<5;
//SW+R
I2DAT=sa;
I2CONCLR=1<<3;
while(SI==0);
if(I2STAT==0x48){
uart0_tx_string("Error: RECEIVER sa+r not match\r\n");
goto L;
}
I2CONCLR=1<<3;
//WAITING FOR DATA
while(SI==0);
//COLLECTING THE DATA
v=I2DAT;
// BCD -> DECIMAL CONCERTION
u =((v>>4)*10)+(v&0x0f);
L:
//STOP BIT
 I2CONSET=1<<4;
//CLEARING SI FLAG
 I2CONCLR=1<<3;
//RETURNING DATA
return u;
}
