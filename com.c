#include "Driver_USART.h"
#include "cmsis_os2.h" 
#include "os_tick.h"
#include <stdio.h>
#include <string.h>
#include "com.h"

#define UART_FLG 0x02
 
void myUART_Thread_mp3(void const *argument);
void Thmp3 (void *argument);
void myUART_Thread_com(void* args);
	
		/* USART Driver */
extern ARM_DRIVER_USART Driver_USART3;
static ARM_DRIVER_USART * USARTdrv = &Driver_USART3;
	
extern osMessageQueueId_t COLA_TERA_TERM;
extern int segundos;

osThreadId_t tid_myUART_Thread_com, tid_Thcom;
 
void myUSART_callback_com(uint32_t event)
{
  uint32_t mask;
  mask = ARM_USART_EVENT_RECEIVE_COMPLETE  |
         ARM_USART_EVENT_TRANSFER_COMPLETE |
         ARM_USART_EVENT_SEND_COMPLETE     |
         ARM_USART_EVENT_TX_COMPLETE       ;
  if (event & mask) {
    /* Success: Wakeup Thread */
    osThreadFlagsSet(tid_myUART_Thread_com, UART_FLG);
  }
	if( event == ARM_USART_EVENT_TX_COMPLETE)
		osThreadFlagsSet(tid_myUART_Thread_com, UART_FLG);
}

int Com_Init(void){
	
	/*Initialize the USART driver */
    USARTdrv->Initialize(myUSART_callback_com);
    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 4800 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 9600);
     
    /* Enable Receiver and Transmitter lines */
    USARTdrv->Control (ARM_USART_CONTROL_TX, 1);
    USARTdrv->Control (ARM_USART_CONTROL_RX, 1);
	
	tid_myUART_Thread_com = osThreadNew(myUART_Thread_com, NULL, NULL);
  if (tid_myUART_Thread_com == NULL) {
    return(-1);
  }
	return 0;
}
 
/* CMSIS-RTOS Thread - UART command thread */
void myUART_Thread_com(void *argument){
	char info_mandar_tera[27];
	int horas_info = 0, minutos_info = 0, segundos_info = 0;
	char byte_cmd_tera[16]={0};
	 osStatus_t status;
	
    while (1){
				
				status = osMessageQueueGet(COLA_TERA_TERM, &byte_cmd_tera, NULL, osWaitForever); 
				if(status == osOK){
					segundos_info = segundos % 60;
					minutos_info = (segundos / 60) % 60;
					horas_info = (segundos / 60) / 60;
				
					sprintf(info_mandar_tera, "%.2d:%.2d:%.2d-->%s ",horas_info, minutos_info, segundos_info, byte_cmd_tera);
					USARTdrv->Send(info_mandar_tera, sizeof(info_mandar_tera));
					osThreadFlagsWait(UART_FLG, osFlagsWaitAny, osWaitForever);
			}

    }
}

