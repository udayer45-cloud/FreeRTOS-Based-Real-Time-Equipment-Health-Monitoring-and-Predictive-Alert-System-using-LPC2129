#include<lpc21xx.h>
#include"header.h"
void uart0_init(int baud){
 int pclk,res;
 int a[]={15,60,30,15};
 pclk = a[VPBDIV & 0x03] * 1000000;
 res=pclk/(16*baud);
 PINSEL0|=0x5;
 U0LCR=0x83;
 U0DLL=res&0xff;
 U0DLM=(res>>8)&0xff;
 U0LCR=0x3;
}

#define THRE ((U0LSR>>5)&1)
void uart0_tx(unsigned char data){
 U0THR=data;
 while(THRE==0);
}
#define RDR (U0LSR&1)
unsigned char uart0_rx(void){
 while(RDR==0);
 return U0RBR;
}
void uart0_tx_string(char *p){
 while(*p){
 uart0_tx(*p);
 p++;
 }
}
void uart0_integer(int num){
int i=0;
char a[15];
 if(num==0){
 uart0_tx('0');
 return;
 }
 else if(num<0){
 uart0_tx('-');
 num=-num;
 }
 while(num){
 a[i]=num%10+48;
 num=num/10;
 i++;
 }
 for(i=i-1;i>=0;i--)
 uart0_tx(a[i]);
}
void uart0_float(float f){
int n1;
n1=f;
uart0_integer(n1);
uart0_tx('.');
n1=(f-n1)*10;
uart0_integer(n1);
}
