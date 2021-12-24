#include "stm32f4xx_hal.h"
#include "cmsis_os2.h" // CMSIS RTOS header file
#include "joystick.h"
#include "os_tick.h"

#define MAX_MENSAJES 8

typedef struct {
	uint8_t pulsacion; //pulsacion corta -> pulsacion = 0 || pulsacion larga -> pulsacion = 1
	uint8_t tecla; 
} mensaje;

mensaje msg;

int Init_Thjoy_Rebotes (void);
int Init_Timer_Joy (void);//inicializa 2 timers, 50ms(rebotes) y 1s (pulsaciones)
int Crear_Cola_JOY(void);
void Thjoy_Rebotes (void *argument);

static uint32_t exec50ms, exec1s;

osTimerId_t tim_50ms, tim_1s;
osThreadId_t tid_Thjoy;
extern osThreadId_t tid_ThPWM;

osMessageQueueId_t COLA_JOY;

int Joystick_Init(){
	GPIO_InitTypeDef GPIO_InitStruct={0};
	int status = 0;
	
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	
			//PULSADOR 					ARRIBA				DERECHA
	GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
			//PULSADOR 					ABAJO					IZQUIERDA			CENTRO
	GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
	
	status = Init_Thjoy_Rebotes() | Init_Timer_Joy() | Crear_Cola_JOY();
	if(status == -1)
		return -1;
	
	return 0;
}

int Init_Thjoy_Rebotes (void) {
 
  tid_Thjoy = osThreadNew(Thjoy_Rebotes, NULL, NULL);
  if (tid_Thjoy == NULL) {
    return(-1);
  }
 
  return(0);
}
 
void Thjoy_Rebotes (void *argument) {
	
  while (1) {
		osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
    osTimerStart(tim_50ms, 50); 
    osThreadYield();
  }
}

int Crear_Cola_JOY(){
	int retorno = 0;
	
	COLA_JOY = osMessageQueueNew(MAX_MENSAJES, sizeof(msg), NULL);
  if (COLA_JOY == NULL) {
		retorno = -1;
  }
	
	return retorno;
}

static void Timer50ms_Callback (void const *arg) {
	
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == 1){
		osThreadFlagsSet(tid_ThPWM, 0x01);
		osTimerStart(tim_1s, 1000U);
		msg.tecla = 1;
	}else if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == 1){
		osThreadFlagsSet(tid_ThPWM, 0x01);
		osTimerStart(tim_1s, 1000U);
		msg.tecla = 2;
	}else if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_12) == 1){
		osThreadFlagsSet(tid_ThPWM, 0x01);
		osTimerStart(tim_1s, 1000U);
		msg.tecla = 3;
	}else if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_14) == 1){
		osThreadFlagsSet(tid_ThPWM, 0x01);
		osTimerStart(tim_1s, 1000U);
		msg.tecla = 4;
	}else if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15) == 1){
		osThreadFlagsSet(tid_ThPWM, 0x01);
		osTimerStart(tim_1s, 1000U);
		msg.tecla = 5;
	}
}

static void Timer1s_Callback (void const *arg) {
	
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == 1){
		msg.pulsacion = 1;
		msg.tecla = 1;
	}else if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == 1){
		msg.pulsacion = 1;
		msg.tecla = 2;
	}else if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_12) == 1){
		msg.pulsacion = 1;
		msg.tecla = 3;
	}else if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_14) == 1){
		msg.pulsacion = 1;
		msg.tecla = 4;
	}else if(HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15) == 1){
		msg.pulsacion = 1;
		msg.tecla = 5;
	}

	osMessageQueuePut(COLA_JOY, &msg, 0U, 500U);
	msg.pulsacion = 0;
}
 

int Init_Timer_Joy (void) {
 
  exec50ms = 1U;
	exec1s = 1U;
  tim_50ms = osTimerNew((osTimerFunc_t)&Timer50ms_Callback, osTimerOnce, &exec50ms, NULL);
	tim_1s = osTimerNew((osTimerFunc_t)&Timer1s_Callback, osTimerOnce, &exec1s, NULL);
  	
  return NULL;
}
