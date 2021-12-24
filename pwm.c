/*
--Módulo encargado de generar un pitido en el zumbador mediante la generación de una señal PWM
--Thread a la espera de un flag que indique que hay que generar un pitido */

#include "cmsis_os2.h" 
#include "stm32f4xx_hal.h"
#include "pwm.h"

void ThPWM (void *argument);
int Init_ThPWM (void);
void TIM2_Init(void);

osThreadId_t tid_ThPWM;  

TIM_HandleTypeDef htim2;
TIM_OC_InitTypeDef Config;

int PWM_Init(){//NECESITA INCLUIRSE EN EL MAIN
	GPIO_InitTypeDef GPIO_InitStruct={0};
	int status = 0;
	
	__HAL_RCC_GPIOA_CLK_ENABLE();
	
		  //ZUMBADOR 									
	GPIO_InitStruct.Pin = GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); 

	TIM2_Init();
	status = Init_ThPWM();
	if(status == -1)
		return -1;
	
	return 0;
}

void TIM2_Init(){
		
		htim2.Instance = TIM2;
		htim2.Init.Prescaler = 83; //(APB1)84MHz / 84 = 1Mhz
		htim2.Init.Period = 999; //1Mhz / period = 1Mhz / 1000 = 1khz => 1ms 
	
		Config.OCMode = TIM_OCMODE_PWM1;
		Config.Pulse = 9;//10
	
		__HAL_RCC_TIM2_CLK_ENABLE();
		HAL_TIM_PWM_Init(&htim2);
		HAL_TIM_PWM_ConfigChannel(&htim2,&Config,TIM_CHANNEL_4);

	}

int Init_ThPWM (void) {//NECESITA INCLUIRSE EN EL MAIN
 
  tid_ThPWM = osThreadNew(ThPWM, NULL, NULL);
  if (tid_ThPWM == NULL) {
    return(-1);
  }
 
  return(0);
}

void ThPWM (void *argument){
	
	while (1) {
		osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
		HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_4);
		osDelay(100);
		HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
    osThreadYield();
  }
}
