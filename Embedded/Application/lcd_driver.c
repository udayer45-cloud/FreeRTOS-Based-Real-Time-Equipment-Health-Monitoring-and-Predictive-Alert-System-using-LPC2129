#include<LPC21xx.H>
#include"header.h"
#include "FreeRTOS.h"
#include "task.h"

void lcd_data(unsigned char data){
 unsigned int temp;
 IOCLR1=0xFE<<16;
 temp=(data&0xf0)<<16;
 IOSET1=temp;
 IOSET1=1<<17;
 IOCLR1=1<<18;
 IOSET1=1<<19;
 //delay_ms(2);
 vTaskDelay(pdMS_TO_TICKS(5));
 IOCLR1=1<<19;
 IOCLR1=0xFE<<16;
 temp=(data&0x0f)<<20;
 IOSET1=temp;
 IOSET1=1<<17;
 IOCLR1=1<<18;
 IOSET1=1<<19;
 vTaskDelay(pdMS_TO_TICKS(5));
	//delay_ms(2);
 IOCLR1=1<<19;
 }
 void lcd_cmd(unsigned char cmd){
 unsigned int temp;
 IOCLR1=0xFE<<16;
 temp=(cmd&0xf0)<<16;
 IOSET1=temp;
 IOCLR1=1<<17;
 IOCLR1=1<<18;
 IOSET1=1<<19;
	 vTaskDelay(pdMS_TO_TICKS(5));
 //delay_ms(2);
 IOCLR1=1<<19;
 IOCLR1=0xFE<<16;
 temp=(cmd&0x0f)<<20;
 IOSET1=temp;
 IOCLR1=1<<17;
 IOCLR1=1<<18;
 IOSET1=1<<19;
	 vTaskDelay(pdMS_TO_TICKS(5));
 //delay_ms(2);
 IOCLR1=1<<19;
 }
 void lcd_string(char *p){
 while(*p){
 lcd_data(*p);
 p++;
 }
 }
 void lcd_int(int n){
 int a[10],i=0;
 if(n==0){
lcd_data('0');
 }
 if(n<0){
 n=-n;
 lcd_cmd(0x80);
 lcd_data('-');
 }
 while(n){
 a[i]=(n%10)+48;
 n=n/10;
 i++;
 }
 for(i=i-1;i>=0;i--){
 lcd_data(a[i]);
 }
 }
void lcd_float(float f)
{
int n1;
n1=f;
lcd_int(n1);
lcd_data('.');
n1=(f-n1)*10;
lcd_int(n1);
}
 void lcd_init(){
 IODIR1|=0xFE<<16;
 lcd_cmd(0x02);
 lcd_cmd(0x28);
 lcd_cmd(0x0e);
 lcd_cmd(0x01);
 lcd_cmd(0x0c);
}
 
