#include "Driver_USART.h"
#include "cmsis_os2.h" 
#include "os_tick.h"
#include "mp3.h"
#include <stdio.h>
#include <string.h>
 
void myUART_Thread_mp3(void *argument);
	
osThreadId_t tid_myUART_Thread_mp3, tid_Thmp3;
extern osMessageQueueId_t COLA_MP3;
 
/* USART Driver */
extern ARM_DRIVER_USART Driver_USART6;
static ARM_DRIVER_USART * USARTdrv = &Driver_USART6;
char byte_cmd[8]={0};
int primera_vez = 0;
 
void myUSART_callback_mp3(uint32_t event)
{
  uint32_t mask;
  mask = ARM_USART_EVENT_RECEIVE_COMPLETE  |
         ARM_USART_EVENT_TRANSFER_COMPLETE |
         ARM_USART_EVENT_SEND_COMPLETE     |
         ARM_USART_EVENT_TX_COMPLETE       ;
	
  if (event & mask) {
    /* Success: Wakeup Thread */
    osThreadFlagsSet(tid_myUART_Thread_mp3, 0x01);
  }
	if( event == ARM_USART_EVENT_TX_COMPLETE)
		osThreadFlagsSet(tid_myUART_Thread_mp3, 0x01);
}
 
int Mp3_Init(void){
	/*Initialize the USART driver */
    USARTdrv->Initialize(myUSART_callback_mp3);
    /*Power up the USART peripheral */
    USARTdrv->PowerControl(ARM_POWER_FULL);
    /*Configure the USART to 9600 Bits/sec */
    USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS |
                      ARM_USART_DATA_BITS_8 |
                      ARM_USART_PARITY_NONE |
                      ARM_USART_STOP_BITS_1 |
                      ARM_USART_FLOW_CONTROL_NONE, 9600);
     
    /* Enable Receiver and Transmitter lines */
    USARTdrv->Control (ARM_USART_CONTROL_TX, 1);
    USARTdrv->Control (ARM_USART_CONTROL_RX, 1);
	
	tid_myUART_Thread_mp3 = osThreadNew(myUART_Thread_mp3, NULL, NULL);
  if (tid_myUART_Thread_mp3 == NULL) {
    return(-1);
  }
	return 0;
}

/* CMSIS-RTOS Thread - UART command thread */
void myUART_Thread_mp3( void* args){
	osStatus_t status;
		
    while (1){
			status = osMessageQueueGet(COLA_MP3, &byte_cmd, NULL, osWaitForever);
			if(status == osOK){
				USARTdrv->Send(byte_cmd, 8);
				osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
			}
			
			osThreadYield(); 
		}
}
