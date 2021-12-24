//Modulo encargado de la gestion de los LED RGB para informar del estado del sistema

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h" 
#include "led.h"

void Thled_read (void *argument);
void Encender_led(uint8_t led, uint8_t frecuencia);

typedef struct {                                // object data type
  uint8_t led_a_encender;
  uint8_t frec;//En Hz
} PAQUETE_LED;

PAQUETE_LED msg_led_read;

int tiempo = 0;

extern osMessageQueueId_t COLA_RGB;

osThreadId_t tid_Thled_read;

int Led_Init(){//NECESITA INCLUIRSE EN EL MAIN
	GPIO_InitTypeDef GPIO_InitStruct={0};
	
	__HAL_RCC_GPIOD_CLK_ENABLE();
	
			//LED RGB 						ROJO				 VERDE				 AZUL
	GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_12 | GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	//GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	tid_Thled_read = osThreadNew(Thled_read, NULL, NULL);
  if (tid_Thled_read == NULL) {
    return(-1);
  }
	
	return 0;
}

void Thled_read (void *argument){
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET);

	while(1){
		osMessageQueueGet(COLA_RGB, &msg_led_read, NULL, 0);
		
		Encender_led(msg_led_read.led_a_encender, msg_led_read.frec);
		
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET);
		
		osThreadYield();   
	}
}

//Le llegan mensajes de:
//			-Encender led Rojo frecuencia x
//   leds = ROJO VERDE AZUL; 
//			ROJO -> leds = 4;  0X2000
//			VERDE -> leds = 2; 0X1000
//			AZUL -> leds = 1;  0X0800
void Encender_led(uint8_t led, uint8_t frecuencia){
	
	if(frecuencia == 1){
		tiempo = 1000;
	}else if(frecuencia == 4){
		tiempo = 125;
	}
	
		HAL_GPIO_WritePin(GPIOD, ((uint16_t)led << 11), GPIO_PIN_RESET);
		osDelay( tiempo );
		HAL_GPIO_WritePin(GPIOD, ((uint16_t)led << 11), GPIO_PIN_SET);
		osDelay( tiempo );
	
}
