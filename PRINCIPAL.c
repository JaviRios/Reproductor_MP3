#include "PRINCIPAL.h"

//PAQUETES A MANDAR A LAS COLAS MENSAJES
typedef struct {
  char info_mostrar[26];
  uint8_t linea;
} PAQUETE_LCD;
PAQUETE_LCD paquete_lcd;

typedef struct {
  uint8_t led_a_encender;
  uint8_t frec;//En Hz
} PAQUETE_LED;
PAQUETE_LED paquete_led;

typedef struct {
	uint8_t pulsacion; //pulsacion corta -> pulsacion = 0 || pulsacion larga -> pulsacion = 1
	uint8_t tecla; 	
} PAQUETE_JOY;
PAQUETE_JOY paquete_joy;

int modo = 0; //REPOSO 0 REPRODUCCION 1
uint32_t flags_reposo = 0, flags_reprod = 0, flags_modo = 0;

int vol = 30, Trepr = 0;
//extern int temp_decimal, temp_entero;
extern float temperatura;
extern int volumen_mandar;
int segundos;
static char byte_cmd_msg_mp3[8];
static char byte_cmd_msg_tera[16];

//ID COLAS DE MENSAJES
osMessageQueueId_t COLA_LCD, COLA_RGB, COLA_TERA_TERM, COLA_MP3;
extern osMessageQueueId_t COLA_JOY, COLA_VOL;

//ID THREAD
osTimerId_t Id_Timer_1s_TRepr;
osThreadId_t Id_Thread_Principal;
extern osThreadId_t tid_myUART_Thread_com;

//DECLARACION FUNCIONES
int Crear_Colas_A_Leer(void);
int Crear_Thread_Principal(void);
void Thread_Principal (void *argument);
void Timer_1s_TRepr_callback(void);
void EnviarComando(int8_t comando, int16_t dato, uint32_t timeout);
void Enviar_TERATERM (int8_t comando, int8_t dato1, int8_t dato2, uint32_t timeout);

int Iniciar_PRINCIPAL(){
	int retorno = 0;
	
	retorno = Crear_Colas_A_Leer();
	retorno = Crear_Thread_Principal();
	
	return retorno;
}

int Crear_Colas_A_Leer(){
	int retorno = 0;
	
	COLA_LCD = osMessageQueueNew(MAX_MENSAJES, sizeof(PAQUETE_LCD), NULL);
	COLA_RGB = osMessageQueueNew(MAX_MENSAJES, sizeof(PAQUETE_LED), NULL);
	COLA_TERA_TERM = osMessageQueueNew(MAX_MENSAJES, 16, NULL);
	COLA_MP3 = osMessageQueueNew(MAX_MENSAJES, 8, NULL);
  if (COLA_LCD == NULL | COLA_RGB == NULL | COLA_TERA_TERM == NULL) {
		retorno = -1;
  }
	
	return retorno;
}

int Crear_Thread_Principal(){
	int retorno = 0;
	int exec_tim1s = 1U;
	
	Id_Thread_Principal = osThreadNew(Thread_Principal, NULL, NULL);
  if (Id_Thread_Principal == NULL) {
    retorno = -1;;
  }
	
	Id_Timer_1s_TRepr = osTimerNew((osTimerFunc_t)&Timer_1s_TRepr_callback, osTimerPeriodic, &exec_tim1s, NULL);
		if(Id_Timer_1s_TRepr == NULL)
			retorno = -1;
		
	return retorno;
}

void Timer_1s_TRepr_callback(void){
	Trepr++;
	osThreadFlagsSet(Id_Thread_Principal, TREPR_FLG);
	osTimerStart(Id_Timer_1s_TRepr, 1000U);
	
}

void Thread_Principal (void *argument){
	osStatus_t status;
	int carpeta = 0x01;//1 - 1ªcarpeta  2 - 2ªcarpea  3 - 3ªcarpeta    INICIALMENTE carpeta 1, asi que si se pulsa ABAJO se cambia a carpeta 2
	int cancion = 0, cancion1 = 0, cancion2 = 0;
	int primera_vez = 0;
	int pausa = 0; //0 PLAY  -  1 PAUSA
	int horas_info = 0, minutos_info = 0, segundos_info = 0;
	int Trepr_min = 0, Trepr_seg = 0;
	int primera = 0;
	int first = 0;	
	
	while(1){
		
		//LECTURA DE LAS COLAS
		status = osMessageQueueGet(COLA_JOY, &paquete_joy, NULL, 200);//Cada 200ms checkea si hay un nuevo mensaje
		
		status = osMessageQueueGet(COLA_VOL, &vol, NULL, 200);
		if(status == osOK){
			osThreadFlagsSet(Id_Thread_Principal, VOLUMEN_FLG);
			EnviarComando(CMD_SET_VOLUME, (0x00 << 8) | volumen_mandar, 0);
		}
		
		//ENVIO DE COMANDO AL MP3 Y LA TRAMA AL TERATERM
			//RESETEO SELECCION DE DISPOSITIVO Y SLEEP MODE 
				if(!first){
					first = 1;
					EnviarComando(CMD_RESET, 0x0000, 0);
					osDelay(500);
					Enviar_TERATERM(CMD_RESET, 0x00, 0x00, 0);
					
					EnviarComando(CMD_SEL_DEV, 0x0002, 0);
					Enviar_TERATERM(CMD_SEL_DEV, 0x00, 0x02, 0);
					
					EnviarComando(CMD_SET_VOLUME,0X001E, 0);
					Enviar_TERATERM(CMD_SET_VOLUME, 0x00, 0x1E, 0);
					
					EnviarComando(CMD_SLEEP_MODE, 0x0000, 0);
					Enviar_TERATERM(CMD_SLEEP_MODE, 0x00, 0x00, 0);						
				}
		
//-----------------------------ENTRAR EN MODO REPRODUCCION-----------------------------//
/*-*/if(modo){
	
			//ARRANCAMOS TIMER TREPR
		if(!primera){
			primera = 1;
			Trepr = 0;
			osTimerStart(Id_Timer_1s_TRepr, 1000U);

			EnviarComando(CMD_WAKE_UP,0x0000,0);
			Enviar_TERATERM(CMD_WAKE_UP, 0x00, 0x00, 0);
			
		//PLAY WITH FOLDER SONG
			EnviarComando( CMD_PLAY_W_FOLDER_N_FILE, 0x0100, 0);
			Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x01, 0x00, 0);
		}
		
			//CONTROL DEL JOYSTICK
			if(paquete_joy.tecla == DERECHA){
				
				//ARRANCAMOS TIMER TREPR
				Trepr = 0;
				osTimerStart(Id_Timer_1s_TRepr, 1000U);
				if(carpeta == 1){
					if(cancion1){
						cancion =0;
						cancion1=0;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);

						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0100, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x01, 0x00, 0);
					}else if(!cancion1){
						cancion1=1;
						cancion = 1;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);

						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0101, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x01, 0x01, 0);
					}
				}else if(carpeta == 2){
					if(cancion2 == 0){
						cancion2 = 1;
						cancion = 1;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);

						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0201, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x02, 0x01, 0);
					}else if(cancion2 == 1){
						cancion2 = 2;
						cancion = 2;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);

						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0202, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x02, 0x02, 0);
					}else if(cancion2 == 2){
						cancion2 = 0;
						cancion = 0;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);

						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0200, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x02, 0x00, 0);
					}				
				}else if(carpeta == 3){
					cancion = 0;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);

					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0300, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x03, 0x00, 0);
				}			
			}else if(paquete_joy.tecla == IZQUIERDA){
				
				if(carpeta == 1){
					if(cancion1){
						cancion1=0;
						cancion = 0;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
						
						//ARRANCAMOS TIMER TREPR
							Trepr = 0;
							osTimerStart(Id_Timer_1s_TRepr, 1000U);
						
						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0100, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x01, 0x00, 0);
					}else if(!cancion1){
						cancion1=1;
						cancion = 1;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
						
						//ARRANCAMOS TIMER TREPR
							Trepr = 0;
							osTimerStart(Id_Timer_1s_TRepr, 1000U);

						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0101, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x01, 0x01, 0);
					}
				}else if(carpeta == 2){
					if(cancion2 == 0){
						cancion2 = 2;
						cancion = 2;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
						
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);
						
						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0202, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x02, 0x02, 0);
					}else if(cancion2 == 1){
						cancion2 = 0;
						cancion = 0;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
						
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);

						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0200, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x02, 0x00, 0);
					}else if(cancion2 == 2){
						cancion2 = 1;
						cancion = 1;
						osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
						
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);						
						
						EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0201, 0);
						Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x02, 0x01, 0);
					}				
				}else if(carpeta == 3){
					cancion = 0;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
					
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);					
					
					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, 0x0300, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x03, 0x00, 0);
				}	
			}else if(paquete_joy.tecla == ABAJO){//SIGUIENTE CARPETA  01 -> 02 -> 03 -> 01 ...
				if(carpeta == 1){
					carpeta = 2;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
					
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);					
					
					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, (carpeta << 8) | 0x00, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, carpeta, 0x00, 0);
				}else if(carpeta == 2){
					carpeta = 3;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
					
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);					
					
					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, (carpeta << 8) | 0x00, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, carpeta, 0x00, 0);
				}else if(carpeta == 3){
					carpeta = 1;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
					
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);					
					
					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, (carpeta << 8) | 0x00, 0);			
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, carpeta, 0x00, 0);
				}
			}else if(paquete_joy.tecla == ARRIBA){//CARPETA ANTERIOR 01 -> 03 -> 02 -> 01 ...
				if(carpeta == 1){
					carpeta = 3;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
					
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);					
					
					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, (carpeta << 8) | 0x00, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, carpeta, 0x00, 0);
				}else if(carpeta == 2){
					carpeta = 1;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
					
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);					
						
					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, (carpeta << 8) | 0x00, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, carpeta, 0x00, 0);
				}else if(carpeta == 3){
					carpeta = 2;
					osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
					
					//ARRANCAMOS TIMER TREPR
						Trepr = 0;
						osTimerStart(Id_Timer_1s_TRepr, 1000U);					
					
					EnviarComando(CMD_PLAY_W_FOLDER_N_FILE, (carpeta << 8) | 0x00, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, carpeta, 0x00, 0);
				}
			}else if(paquete_joy.tecla == CENTRO && paquete_joy.pulsacion == 0){
				if(pausa){//QUEREMOS DARLE AL PLAY
					pausa = 0;
					
					//ENCENDER LED AZUL 
					paquete_led.led_a_encender = VERDE;
					paquete_led.frec=4;
					osMessageQueuePut(COLA_RGB, &paquete_led, 0U, 0U);
					
				//PLAY
					EnviarComando(CMD_PLAY,0x0000,0);
					Enviar_TERATERM(CMD_PLAY, 0x00, 0x00, 0);
					
				//EMPEZAR TIMER TREPR
					osTimerStart(Id_Timer_1s_TRepr, 1000U);
				
				}else{//QUEREMOS PAUSAR LA CANCION
					pausa = 1;

				//ENCENDER LED VERDE 
					paquete_led.led_a_encender = AZUL;
					paquete_led.frec=1;
					osMessageQueuePut(COLA_RGB, &paquete_led, 0U, 0U);

				//PAUSA
					EnviarComando(CMD_PAUSE, 0x0000, 0);
					Enviar_TERATERM(CMD_PAUSE, 0x00, 0x00, 0);
					
				//PAUSAMOS EL TIMER TREPR
					osTimerStop(Id_Timer_1s_TRepr);
				
				}
			}else if(paquete_joy.tecla == CENTRO && paquete_joy.pulsacion == 1){
				modo = 0;
				Trepr = 0;
				
				sprintf(paquete_lcd.info_mostrar,"                 ");
				paquete_lcd.linea = 1;
				status =  osMessageQueuePut(COLA_LCD, &paquete_lcd, 0U, 0U);
				
				sprintf(paquete_lcd.info_mostrar,"                 ");
				paquete_lcd.linea = 2;
				status =  osMessageQueuePut(COLA_LCD, &paquete_lcd, 0U, 0U);
				
				//APAGAR LEDS
					paquete_led.led_a_encender = APAGADOS;
					osMessageQueuePut(COLA_RGB, &paquete_led, 0U, 0U);
				
				//SLEEP MODE
					EnviarComando(CMD_SLEEP_MODE,0X0000, 0);
					Enviar_TERATERM(CMD_SLEEP_MODE, 0x00, 0x00, 0);
			}
			
			flags_modo = osThreadFlagsWait( AVISO_CAMBIO_MODO , osFlagsWaitAny, 1000);
				if(flags_modo == osFlagsErrorTimeout){
					flags_modo = 0;
				}
				if( flags_modo & AVISO_CAMBIO_MODO ){
					osThreadFlagsClear(AVISO_CAMBIO_MODO);
			
					EnviarComando(CMD_WAKE_UP,0x0000,0);
					Enviar_TERATERM(CMD_WAKE_UP, 0x00, 0x00, 0);
			
					EnviarComando( CMD_PLAY_W_FOLDER_N_FILE, 0x0100, 0);
					Enviar_TERATERM(CMD_PLAY_W_FOLDER_N_FILE, 0x01, 0x00, 0);
				}
			
			//REPRESENTACION LCD
				flags_reprod = osThreadFlagsWait( VOLUMEN_FLG | CARPETA_CANCION_FLG | TREPR_FLG, osFlagsWaitAny, 100);
				
				if(flags_reprod == osFlagsErrorTimeout){
					flags_reprod = 0;
				}
				if( ((flags_reprod & CARPETA_CANCION_FLG) || (flags_reprod & VOLUMEN_FLG)) && (!primera_vez) ){
						sprintf(paquete_lcd.info_mostrar,"   F:%.2d C:%.2d Vol:%d",carpeta,cancion, volumen_mandar);
						paquete_lcd.linea = 1;
						status =  osMessageQueuePut(COLA_LCD, &paquete_lcd, 0U, 0U);
				}
				if( flags_reprod & TREPR_FLG ){
						Trepr_seg = Trepr % 60;
						Trepr_min = (Trepr / 60) % 60;
					
						sprintf(paquete_lcd.info_mostrar,"    Trep: %.2d:%.2d",Trepr_min, Trepr_seg);
						paquete_lcd.linea = 2;
						status = osMessageQueuePut(COLA_LCD, &paquete_lcd, 0U, 0U);
					}
			
//--------------------------------ENTRAR EN MODO REPOSO--------------------------------//			
		}else if(!modo){
			Trepr = 0;
			primera = 0;
			
			flags_reposo = osThreadFlagsWait(SEGUNDOS_FLG | TEMPERATURA_FLG, osFlagsWaitAny, 100);
				if( flags_reposo == osFlagsErrorTimeout){
					flags_reposo = 0;
				}
				if(flags_reposo & TEMPERATURA_FLG) {
					sprintf(paquete_lcd.info_mostrar,"  %s%.1f%s","JGR 2021 T:",temperatura,"C");
					paquete_lcd.linea = 1;
					status  = osMessageQueuePut(COLA_LCD, &paquete_lcd, 0U, 0U);
				}
				if(flags_reposo & SEGUNDOS_FLG){
					segundos_info = segundos % 60;
					minutos_info = (segundos / 60) % 60;
					horas_info = (segundos / 60) / 60;
						
					sprintf(paquete_lcd.info_mostrar, "      %.2d:%.2d:%.2d",horas_info, minutos_info, segundos_info);
					paquete_lcd.linea = 2;
					osMessageQueuePut(COLA_LCD, &paquete_lcd, 0U, 0U);
				}
			
			if(paquete_joy.tecla == CENTRO && paquete_joy.pulsacion == 1){
				modo = 1;
				
				osThreadFlagsSet(Id_Thread_Principal, CARPETA_CANCION_FLG);
				
				//APAGAR LEDS
					paquete_led.led_a_encender = APAGADOS;
					osMessageQueuePut(COLA_RGB, &paquete_led, 0U, 0U);
				
				osThreadFlagsSet(Id_Thread_Principal, AVISO_CAMBIO_MODO);
					
		//ENCENDER LED VERDE 
					paquete_led.led_a_encender = VERDE;
					paquete_led.frec=4;
					osMessageQueuePut(COLA_RGB, &paquete_led, 0U, 0U);
				
			}
		}

		paquete_joy.pulsacion = 0;
		paquete_joy.tecla = 0;
		memset(byte_cmd_msg_mp3, 0, 8);
		
		osThreadYield(); 
	}
}

void EnviarComando(int8_t comando, int16_t dato, uint32_t timeout){
	byte_cmd_msg_mp3[0] = 0x7E;
	byte_cmd_msg_mp3[1] = 0xFF;
	byte_cmd_msg_mp3[2] = 0x06;
	byte_cmd_msg_mp3[3] = comando;
	byte_cmd_msg_mp3[4] = 0x00;
	byte_cmd_msg_mp3[5] = (int8_t)(dato >> 8);;
	byte_cmd_msg_mp3[6] = (int8_t)(dato);
	byte_cmd_msg_mp3[7] = 0xEF;
	
	osMessageQueuePut(COLA_MP3, &byte_cmd_msg_mp3, 0U, timeout);
}

void Enviar_TERATERM (int8_t comando, int8_t dato1, int8_t dato2, uint32_t timeout){
	sprintf(byte_cmd_msg_tera,"%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X", 0x7E, 0xFF, 0x06, comando, 0x00, dato1, dato2, 0xEF);
	osMessageQueuePut(COLA_TERA_TERM, &byte_cmd_msg_tera, 0U, timeout);
	//osThreadFlagsSet(tid_myUART_Thread_com, 0x01);
}
