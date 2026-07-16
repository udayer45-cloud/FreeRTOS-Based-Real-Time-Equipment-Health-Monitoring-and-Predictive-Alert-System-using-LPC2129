#include <LPC21xx.H>
#include"header.h"
void adc_init(void){
 PINSEL1|=0x01000000;
// ADCR=0x01000000;
	ADCR = 0x00200D00;  // CLKDIV=13, gives 60/14 = 4.28MHz  4.5MHz
}
#define DON ((ADDR>>31)&1)
unsigned int adc_read(unsigned char ch){
 int res;
 ADCR|=1<<ch;
 ADCR|=1<<24;
 while(DON==0);
 res=((ADDR>>6)&0x3ff);
 ADCR&=~(1<<24);
 ADCR&=~(1<<ch);
 return res;
}
