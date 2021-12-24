/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"                          // CMSIS RTOS header file
#include "temp.h"
#include "stm32f4xx_hal.h"
#include "Driver_I2C.h"
#include <stdio.h>

/* Macros --------------------------------------------------------------------*/

#define I2C_ADDR_TEMP       0x48

/* Public variables ----------------------------------------------------------*/

extern ARM_DRIVER_I2C Driver_I2C1;
float temperatura = 0;
int temp_decimal = 0, temp_entero = 0;
extern osThreadId_t Id_Thread_Principal;

/* Private variables ---------------------------------------------------------*/

static osThreadId_t tid_Thtemp;                        // thread id
static ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C1;

/* Function prototypes -------------------------------------------------------*/

static void Thtemp (void *argument);                   // thread function

/* Public functions --------------------------------------------------------- */

int Init_Thtemp (void) {
 
  tid_Thtemp = osThreadNew(Thtemp, NULL, NULL);
  if (tid_Thtemp == NULL) {
    return(-1);
  }
  
  return(0);
}

/* Private functions -------------------------------------------------------- */
 
/* Initialize I2C connected EEPROM */
static void I2C_Initialize_temp (void) {
  I2Cdrv->Initialize (NULL);
  I2Cdrv->PowerControl (ARM_POWER_FULL);
  I2Cdrv->Control      (ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
  I2Cdrv->Control      (ARM_I2C_BUS_CLEAR, 0);
}

/* Read I2C connected temperature device */
static int32_t I2C_read_temp (uint8_t buf [2]) {
  
  I2Cdrv->MasterReceive (I2C_ADDR_TEMP, buf, 2, false);
 
  /* Wait until transfer completed */
  while (I2Cdrv->GetStatus().busy);
  
  /* Check if all data transferred */
  if (I2Cdrv->GetDataCount () != 2)
  {
    return -1;
  }
 
  return 0;
}

static void convertir_temperatura (uint8_t buf [2])
{
  int16_t temperatura_hex;
	float temperatura_antes_enviar;
	
  
  temperatura_hex = (int16_t) (buf[1] >> 5);
  temperatura_hex |= ((int16_t) (buf[0] << 8)) >> 5;
  
  temperatura_antes_enviar = temperatura_hex * 0.125f;
	
	if( (temperatura_antes_enviar *1000) != (temperatura * 1000)){
			temperatura = temperatura_antes_enviar;
			osThreadFlagsSet(Id_Thread_Principal, 0x02);
	}
	
}

static void Thtemp (void *argument) {
  int32_t status;
  uint8_t buf [2];
  
  I2C_Initialize_temp();
  
  while (1)
  {
    status = I2C_read_temp(buf);
    
    if (status != -1){
    convertir_temperatura(buf);
    }
    
    osDelay(1000U);
  }
}
