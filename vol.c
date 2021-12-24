#include "cmsis_os2.h" 
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "adc.h"
#include "vol.h"

#define MSGQUEUE_OBJECTS 8

void Thread_Volumen (void *argument);
int Init_MsgQueue_vol (void);
void timer_200ms_callback(void);

osMessageQueueId_t COLA_VOL;

osThreadId_t Id_Thread_Vol, tid__200ms; 

osTimerId_t tid_timer_200ms;

int volumen_mandar = 0;

int volumen = 50, volumen_anterior = 50;
static int exec_tim200ms = 1U;

int Ini_Volumen (void) {
	
	//THREADS
  Id_Thread_Vol = osThreadNew(Thread_Volumen, NULL, NULL);
  if (Id_Thread_Vol == NULL) {
    return(-1);
  }
	
	//COLA MENSAJES
	COLA_VOL = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(volumen), NULL);
  if (COLA_VOL == NULL) {
		return (-1);
  }
	
	//TIMER
	tid_timer_200ms = osTimerNew((osTimerFunc_t)&timer_200ms_callback, osTimerPeriodic, &exec_tim200ms, NULL);
		if(tid_timer_200ms == NULL)
			return (-1);
 
  return(0);
}

void timer_200ms_callback(void){
	osThreadFlagsSet(Id_Thread_Vol,0x01);
	osTimerStart(tid_timer_200ms, 200U);
}
	
void Thread_Volumen (void *argument) {
	float value;
	osStatus_t status;
	ADC_HandleTypeDef adchandle;
	uint32_t flags = 0;
	
	osTimerStart(tid_timer_200ms, 2000U);
  
	ADC1_pins_F429ZI_config();
	ADC_Init_Single_Conversion(&adchandle,ADC1);
	
  while (1) {
		flags = osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
			if(flags == osFlagsErrorTimeout){
				flags = 0;
			}
			if(flags == 0x01){
				value = ADC_getVoltage(&adchandle, 10);
				//Convertir value en valores Hexadecimales
				volumen = (int)((value *100) / 3.3f);

				if(volumen != volumen_anterior){
					
					volumen_mandar = (volumen * 30) / 100;
					if(volumen_mandar == 29)
						volumen_mandar = 30;
					status = osMessageQueuePut(COLA_VOL, &volumen_mandar, 0U, 0U);
					if(status == osOK)
						volumen_anterior = volumen_mandar;
				}
			}
		
    osDelay(1000);
		osThreadYield(); 
  }
}

