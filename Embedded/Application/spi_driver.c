#include <LPC21xx.H>
#include"header.h"
#define SPIF ((S0SPSR>>7)&1)
void spi_init(){
 IODIR0|=(1<<7);//po.7->output dir
 IOSET0=1<<7; //slave diselect
 PINSEL0|=0x1500;//P0.5->MISO,P0.4->SCK0,P0.6->MOSI
 S0SPCR=0x20;//CPOL=CPHA=0,MSTR,MSB first
 S0SPCCR=16; //SPI freq is 1MHz;
//	S0SPCCR= 120;
}
//Transfer function
unsigned char spi_data(unsigned char data){
 S0SPDR=data; //data from M->S
 while(SPIF==0); //Waiting for transmission complete
return S0SPDR; //return resived data
}
	unsigned short int spi_read(unsigned char ch){
 char bH=0,bL=0;
 unsigned short int temp;
 IOCLR0=1<<7; //chip select slave selected
 ch<<=6;
 spi_data(0x06); //start bit ,single ended mode
 bH=spi_data(ch); //M->S: select channel
 //S->M: B11-B8
 bL=spi_data(0x00); //M->S dont care value
 //S->M B7-B0
 IOSET0=1<<7; //slave diselect
 temp=((bH&0x0f)<<8)|bL; //12bit value
 return temp;
}
