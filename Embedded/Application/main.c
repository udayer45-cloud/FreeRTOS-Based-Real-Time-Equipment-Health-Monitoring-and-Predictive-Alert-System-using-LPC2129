/* Standard includes. */
#include <stdlib.h>
#include "header.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h" 
typedef struct sensorData
{
	float temp;
	int ldr;
	int poten;
	int sec;
	int min;
	int hrs;
	int date;
	int month;
	int year;
}SD;

SemaphoreHandle_t alert;
QueueHandle_t senpro, prour,proalr;
TaskHandle_t sensor,alarm,uart,process,watchd;
char str[100];

volatile int alertActive=0;
volatile unsigned int sensorHB  = 0;
volatile unsigned int processHB = 0;
volatile unsigned int uartHB    = 0;
volatile unsigned char faultDetected = 0;
TickType_t faultTime;

#define mainBUS_CLK_FULL	((unsigned char)0x01)
#define mainCHECK_TASK_PRIORITY	 (tskIDLE_PRIORITY+3)

void watchdog_init(int tout)
{
	int a[]={15,60,30,15,15};
	unsigned int pclk=a[VPBDIV%4]*1000000;
	WDTC=(tout*pclk)/4; 
	WDMOD=0x3; //en wdg and reset
	WDFEED = 0xAA; // Feed sequence
  WDFEED = 0x55;
}
		
static void sensorTask(void* pvParameters){
		SD s;
		float vout;
	  unsigned int res;
    i2c_write(0xD0,0x00,0x00);
    i2c_write(0xD0,0x01,0x27);
    i2c_write(0xD0,0x02,0x6);
    i2c_write(0xD0,0x03,0x05);
    i2c_write(0xD0,0x04,0x24);
    i2c_write(0xD0,0x05,0x06);
    i2c_write(0xD0,0x06,0x26);
    lcd_init();
    while(1){
			///ADC READ
			res = adc_read(1);           // 0-1023 for 0-3.3V
			vout=(res*3.3)/1023;
			s.temp=(vout-0.5)/0.01;
//			s.temp+=42;
		///SPI READ
			s.poten = (spi_read(1) * 100) / 4095;
			s.ldr   = (spi_read(2) * 100) / 4095;
		///I2C READ
			s.sec = i2c_read(0xd1, 0x00);
			s.min = i2c_read(0xd1, 0x01);
			s.hrs = i2c_read(0xd1, 0x02);
			s.date = i2c_read(0xd1, 0x04);
			s.month = i2c_read(0xd1, 0x05);
			s.year  = i2c_read(0xd1, 0x06);
		/// SENDING QUEUE
			xQueueSend(senpro, &s,pdMS_TO_TICKS(10));
			sensorHB++;
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void processingTask(void* pvParameters){		
 SD now;
    while(1){
        int i = xQueueReceive(senpro, &now, portMAX_DELAY);			
        if(i == pdTRUE){
            if(now.temp>=80||now.poten>=80){
                alertActive = 1;
                xQueueSend(proalr, &now,pdMS_TO_TICKS(10));
                xSemaphoreGive(alert);
            }
             else if((now.temp<80)&&(now.poten<80)&&(alertActive==1)){
                    alertActive = 0;
            }
            xQueueSend(prour, &now,pdMS_TO_TICKS(10));
					
					/// LCD						
					lcd_cmd(0x80);
					//date month year					
					lcd_int(now.date);
					lcd_data('-');
					lcd_int(now.month);
					lcd_data('-');
					lcd_int(now.year);
					lcd_data(' ');
					//hrs mins
					lcd_cmd(0x89);
					lcd_int(now.hrs);
					lcd_data(':');
					lcd_int(now.min);
					// temp load light
					lcd_cmd(0xc0);
					lcd_float(now.temp);
					lcd_data(' ');
//					lcd_cmd(0xc7);
					lcd_int(now.poten);
					lcd_data(' ');
//					lcd_cmd(0xcc);
					lcd_int(now.ldr);
					lcd_data(' ');
					lcd_data(' ');
        }
			 processHB++;
    }
}
static void alertTask(void * pvParameters){
	 SD a;
    long int t = 500;
    while(1){
        xSemaphoreTake(alert, portMAX_DELAY);
        while(alertActive){
            if(xQueueReceive(proalr, &a, 0)==pdTRUE)
						{
							if((a.temp>=95) || (a.poten>=95))
									t = 50;
							else if((a.temp>=90) ||  (a.poten>=90))
									t = 100;
							else if((a.temp>=85) || (a.poten>=85))
									t = 250;
							else
									t = 500;
						}
            IOCLR0 = 1<<17|1<<18|1<<19;
						IOSET0=1<<21;
						vTaskDelay(pdMS_TO_TICKS(t));
            IOSET0 = 1<<17|1<<18|1<<19;
						IOCLR0=1<<21;
						vTaskDelay(pdMS_TO_TICKS(t));
        }
        IOSET0 = 1<<17|1<<18|1<<19;
				IOCLR0=1<<21;
    }
}
static void uartTask(void * pvParameters)
{
SD s;
	while(1)
	{
		int i=xQueueReceive(prour,&s,portMAX_DELAY);
		if(i==pdTRUE)
		{
			uart0_integer(s.date);
			uart0_tx('-');
			uart0_integer(s.month);
			uart0_tx('-');
			uart0_integer(s.year);
			uart0_tx(' ');
			uart0_integer(s.hrs);
			uart0_tx(':');
			uart0_integer(s.min);
			uart0_tx(':');
			uart0_integer(s.sec);
			uart0_tx(' ');
			uart0_float(s.temp);
			uart0_tx(' ');
			uart0_integer(s.poten);
			uart0_tx(' ');
			uart0_integer(s.ldr);
			uart0_tx_string("\r\n");
			uartHB++;
		}
	}
}

static void watchdog(void *pvParameters)
{
    unsigned int oldSensorHB  = 0;
    unsigned int oldProcessHB = 0;
    unsigned int oldUartHB    = 0;

    unsigned char sensorMiss  = 0;
    unsigned char processMiss = 0;
    unsigned char uartMiss    = 0;

    watchdog_init(5);     // Hardware timeout = 5 sec

    while(1)
    {
        /* Sensor Health Check */
        if(sensorHB == oldSensorHB)
            sensorMiss++;
        else
            sensorMiss = 0;

        oldSensorHB = sensorHB;

        /* Process Health Check */
        if(processHB == oldProcessHB)
            processMiss++;
        else
            processMiss = 0;

        oldProcessHB = processHB;

        /* UART Health Check */
        if(uartHB == oldUartHB)
            uartMiss++;
        else
            uartMiss = 0;

        oldUartHB = uartHB;

        /* Detect Fault Only Once */
        if((sensorMiss >= 3) && (faultDetected == 0))
        {
//            uart0_tx_string("FAULT: SENSOR TASK FAILURE\r\n");

            faultDetected = 1;
            faultTime = xTaskGetTickCount();
        }

        if((processMiss >= 3) && (faultDetected == 0))
        {
//            uart0_tx_string("FAULT: PROCESS TASK FAILURE\r\n");

            faultDetected = 1;
            faultTime = xTaskGetTickCount();
        }

        if((uartMiss >= 3) && (faultDetected == 0))
        {
//            uart0_tx_string("FAULT: UART TASK FAILURE\r\n");

            faultDetected = 1;
            faultTime = xTaskGetTickCount();
        }

        /* Healthy System */
        if(faultDetected == 0)
        {
            WDFEED = 0xAA;
            WDFEED = 0x55;
        }
        else
        {
            /* Give UART 3 seconds to finish */

            if((xTaskGetTickCount() - faultTime) < pdMS_TO_TICKS(3000))
            {
                WDFEED = 0xAA;
                WDFEED = 0x55;
            }

            /* After 3 sec: no feeding -> watchdog will reset MCU */
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
static void prvSetupHardware( void );

int main( void ){
	prvSetupHardware();
	uart0_init(9600);
	spi_init();
	i2c_init();
	adc_init();
	
	IODIR0|=1<<21|1<<17|1<<18|1<<19;
	IOSET0|=1<<17|1<<18|1<<19;
	
	prour=xQueueCreate(10,sizeof(SD));
	proalr=xQueueCreate(10,sizeof(SD));
	senpro=xQueueCreate(10,sizeof(SD));

	alert=xSemaphoreCreateBinary();	

	xTaskCreate(sensorTask,"sensor",256,NULL,3,&sensor);
  xTaskCreate(processingTask,"process",256,NULL,2,&process);
	xTaskCreate(alertTask,"alarm",256,NULL,2,&alarm);
	xTaskCreate(uartTask,"uart",256,NULL,2,&uart);
	xTaskCreate(watchdog,"watchdog",256,NULL,1,&watchd);
	vTaskStartScheduler();
	while(1);
}

static void prvSetupHardware(void){
	VPBDIV = mainBUS_CLK_FULL;//PCLK=60MHz
}




