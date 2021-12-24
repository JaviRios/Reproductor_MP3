
#include "lcd.h"
#include "stdio.h"
#include "string.h"
#include "Arial12x12.h"
#include "cmsis_os2.h" 
#include "stm32f4xx_hal.h"
#include "Driver_SPI.h"

#define PIN_AO GPIO_PIN_13
#define PORT_AO GPIOF
#define PIN_CS GPIO_PIN_14
#define PORT_CS GPIOD
#define PIN_RST GPIO_PIN_6
#define PORT_RST GPIOA

extern osMessageQueueId_t COLA_LCD;

typedef struct { 
  char info_mostrar[26];
  uint8_t linea;
} PAQUETE_LCD;
 
PAQUETE_LCD msg_received;

extern ARM_DRIVER_SPI Driver_SPI1;
ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

TIM_HandleTypeDef htim7_lcd;
osThreadId_t tid_ThColaLCD;

char buffer[512] = {0};
int positionL1 = 0, positionL2 = 0, position = 0, line = 2;

void ThColaLCD (void *argument);


void symbolToLocalBuffer_L1(uint8_t symbol){
	uint8_t i, value1, value2;
	uint16_t offset = 0;
	
	offset = 25 * (symbol - ' ');
	
	for(i = 0; i < 12; i++){
		value1 = Arial12x12[offset+i*2+1];
		value2 = Arial12x12[offset+i*2+2];
		
		buffer[i+positionL1] = value1;
		buffer[i+128+positionL1]=value2;
	}
	positionL1 += Arial12x12[offset];
	
}

void symbolToLocalBuffer_L2(uint8_t symbol){
	uint8_t i, value1, value2;
	uint16_t offset = 0;
	
	offset = 25 * (symbol - ' ');
	
	for(i = 0; i < 12; i++){
		value1 = Arial12x12[offset+i*2+1];
		value2 = Arial12x12[offset+i*2+2];
		
		buffer[i+256+positionL2] = value1;
		buffer[i+384+positionL2]=value2;
	}
	positionL2 += Arial12x12[offset];
	
}

void symbolToLocalBuffer(uint8_t linea, uint8_t symbol)
{
	
  if (linea == 1)
  {
    symbolToLocalBuffer_L1(symbol);
  }
  else if(linea == 2)
  {
    symbolToLocalBuffer_L2(symbol);
  }
}

void delay(uint32_t microsegundos){
		htim7_lcd.Instance = TIM7;
		__HAL_RCC_TIM7_CLK_ENABLE();
		htim7_lcd.Init.Prescaler = 83; //84MHz / 84 = 1 Mhz
		htim7_lcd.Init.Period = microsegundos - 1; //2Mhz / microsegundos = 1MhZ = 0.5s 
		
		HAL_TIM_Base_Init(&htim7_lcd);//CNT = 0
		__HAL_TIM_CLEAR_FLAG(&htim7_lcd, TIM_FLAG_UPDATE);
		HAL_TIM_Base_Start(&htim7_lcd);
	
		while (__HAL_TIM_GET_FLAG(&htim7_lcd, TIM_FLAG_UPDATE) == false);
	
		HAL_TIM_Base_Stop(&htim7_lcd);
}

void LCD_reset(void){
	static GPIO_InitTypeDef GPIO_InitStruct={0};
	
	SPIdrv->Initialize(NULL);
	SPIdrv->PowerControl(ARM_POWER_FULL);
	SPIdrv->Control(ARM_SPI_MODE_MASTER  | 
									ARM_SPI_CPOL1_CPHA1  | 
									ARM_SPI_MSB_LSB 		 | 
									ARM_SPI_DATA_BITS(8), 20000000 ); 
	
	//PIN PA6 [RESET]	| PIN PD14 [CHIP SELECT] | PIN PF13 [AO]
		__HAL_RCC_GPIOA_CLK_ENABLE();
	  GPIO_InitStruct.Pin = PIN_RST;
	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		HAL_GPIO_Init(PORT_RST, &GPIO_InitStruct);
		
		__HAL_RCC_GPIOD_CLK_ENABLE();
		GPIO_InitStruct.Pin = PIN_CS;
		HAL_GPIO_Init(PORT_CS, &GPIO_InitStruct);
		
		__HAL_RCC_GPIOF_CLK_ENABLE();
		GPIO_InitStruct.Pin = PIN_AO;
		HAL_GPIO_Init(PORT_AO, &GPIO_InitStruct);
	
	HAL_GPIO_WritePin(PORT_RST, PIN_RST, GPIO_PIN_RESET);
	delay(10);
	HAL_GPIO_WritePin(PORT_RST, PIN_RST	, GPIO_PIN_SET);
	delay(1000);
	
	memset(buffer, 0, 512);
}

void LCD_wr_data(unsigned char data)
{
	ARM_SPI_STATUS stat;
	
// Seleccionar CS = 0;
	HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_RESET);
// Seleccionar A0 = 1;
	HAL_GPIO_WritePin(PORT_AO, PIN_AO, GPIO_PIN_SET);
// Escribir un dato (data) usando la función SPIDrv->Send(…);
	SPIdrv->Send(&data, sizeof(data));
// Esperar a que se libere el bus SPI;
	do{
		stat = SPIdrv->GetStatus();
	}while(stat.busy);
// Seleccionar CS = 1;
	HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET);
}

void LCD_wr_cmd(unsigned char cmd)
{
	ARM_SPI_STATUS stat;
	
// Seleccionar CS = 0;
	HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_RESET);
// Seleccionar A0 = 0;
	HAL_GPIO_WritePin(PORT_AO, PIN_AO, GPIO_PIN_RESET);
// Escribir un comando (cmd) usando la función SPIDrv->Send(…);
	SPIdrv->Send(&cmd, sizeof(cmd));
// Esperar a que se libere el bus SPI;
	do{
		stat = SPIdrv->GetStatus();
	}while(stat.busy);
// Seleccionar CS = 1;
	HAL_GPIO_WritePin(PORT_CS, PIN_CS, GPIO_PIN_SET);
}

void LCD_init_cmd(){
	LCD_wr_cmd(0xAE);//DISPLAY OFF 
	LCD_wr_cmd(0xA2);//BIAS VOLTAGE
	
	LCD_wr_cmd(0xA0);
	LCD_wr_cmd(0xC8);//SCAN EN SALIDAS COM ES NORMAL
	
	LCD_wr_cmd(0x22);//VOLTAGE RESISTOR RATIO
	LCD_wr_cmd(0x2F);//POWER RATIO
	LCD_wr_cmd(0x40);//START LINE = 0
	LCD_wr_cmd(0xAF);//DISPLAY ON
	
	LCD_wr_cmd(0x81);
	LCD_wr_cmd(0x17);//SET CONTRAST
	
	LCD_wr_cmd(0xA4);//DISPLAY ALL POINTS NORMAL
	LCD_wr_cmd(0xA6);//LCD DISPLAY NORMAL
	
}

void LCD_update(uint8_t linea){
		int i;
	if(linea == 1){
			//--------------------------------------------------------------------
		LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
		LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
		LCD_wr_cmd(0xB0); // ASIGNA DIRECCION DE Página 0
			for(i=0;i<128;i++){
				LCD_wr_data(buffer[i]);
			}
		//--------------------------------------------------------------------
		LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
		LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
		LCD_wr_cmd(0xB1); //ASIGNA DIRECCION DE Página 1
			for(i=128;i<256;i++){
				LCD_wr_data(buffer[i]);
			}
		}else if(linea == 2){

		//--------------------------------------------------------------------
		LCD_wr_cmd(0x00);
		LCD_wr_cmd(0x10);
		LCD_wr_cmd(0xB2); //ASIGNA DIRECCION DE Página 2
			for(i=256;i<384;i++){
				LCD_wr_data(buffer[i]);
			}
		//--------------------------------------------------------------------
		LCD_wr_cmd(0x00);
		LCD_wr_cmd(0x10);
		LCD_wr_cmd(0xB3); //ASIGNA DIRECCION DE Pagina 3
			for(i=384;i<512;i++){
				LCD_wr_data(buffer[i]);
			}			
		}
}


void print_lineas(char linea1[], uint16_t L1_length, char linea2[], uint16_t L2_length){
  uint8_t i;
  
  positionL1 = 0;
  positionL2 = 0;
  
  for (i = 0; i < L1_length; i++){
    symbolToLocalBuffer(1, (uint8_t) linea1[i]);
  }
  
  for (i = 0; i < L2_length; i++){
    symbolToLocalBuffer(2, (uint8_t) linea2[i]);
  }
  
  LCD_update(1);
	LCD_update(2);
}

int Init_ThLCD (void) {
 
  tid_ThColaLCD = osThreadNew(ThColaLCD, NULL, NULL);
  if (tid_ThColaLCD == NULL) {
    return(-1);
  }
 
  return(0);
}


int LCD_Initialization(){
	int status = 0;
	LCD_reset();
	LCD_init_cmd();
	
	status = Init_ThLCD();
	if(status == -1)
		return -1;
	
	return 0;
}

void ThColaLCD (void *argument) {
  osStatus_t status;
	
	int linea_length = 0;
	
	memset(buffer, 0, 512);
	LCD_update(1);
	LCD_update(2);
	
  while (1) {
			
		status = osMessageQueueGet(COLA_LCD, &msg_received, NULL, osWaitForever);
			
		if (status == osOK) {
			linea_length = strlen(msg_received.info_mostrar);
			for (int i = 0; i < linea_length; i++){
				symbolToLocalBuffer(msg_received.linea, msg_received.info_mostrar[i]);
			}
			LCD_update(msg_received.linea);
			positionL1 = 0;
			positionL2 = 0;	
		}
		osThreadYield();
	}	
}
