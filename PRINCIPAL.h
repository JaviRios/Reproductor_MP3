#include "cmsis_os2.h"
#include "stdio.h"
#include "string.h"

#ifndef __PRINCIPAL_H

#define MAX_MENSAJES 8

#define ARRIBA 1
#define DERECHA 2
#define ABAJO 3
#define IZQUIERDA 4
#define CENTRO 5

#define AZUL 1
#define VERDE 2
#define ROJO 4
#define APAGADOS 0

#define SEGUNDOS_FLG 0x01
#define TEMPERATURA_FLG 0x02
#define VOLUMEN_FLG 0x04
#define TREPR_FLG 0x08
#define CARPETA_CANCION_FLG 0x10
#define AVISO_CAMBIO_MODO 0x20

//MP3 COMANDOS
#define CMD_PLAY_W_INDEX 0X03
#define CMD_PLAY_W_FOLDER_N_FILE 0x0F
#define CMD_SET_VOLUME 0X06
#define CMD_SEL_DEV 0X09
#define CMD_DEV_TF 0X02
#define CMD_PLAY 0X0D
#define CMD_PAUSE 0X0E
#define CMD_SINGLE_CYCLE 0X19
#define CMD_SINGLE_CYCLE_ON 0X00
#define CMD_SINGLE_CYCLE_OFF 0X01
#define CMD_PLAY_W_VOL 0X22
#define CMD_WAKE_UP 0x0B
#define CMD_SLEEP_MODE 0x0A
#define CMD_RESET 0x0C

int Iniciar_PRINCIPAL(void);

#endif
