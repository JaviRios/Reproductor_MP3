#include "clock.h"
#include "cmsis_os2.h"

#define SEGUND_FLG 0X01

extern int segundos;

osTimerId_t tid_timer_1s;
osThreadId_t thTimer_1s;
static int exec_tim1s = 1U;
extern osThreadId_t Id_Thread_Principal;

void timer_1s_callback(void);
void thTimer_1s_callback(void *argument);
		
int Init_Timer_1s (void) {
  
		thTimer_1s = osThreadNew(thTimer_1s_callback, NULL, NULL);
		
		tid_timer_1s = osTimerNew((osTimerFunc_t)&timer_1s_callback, osTimerPeriodic, &exec_tim1s, NULL);
		if(thTimer_1s == NULL | tid_timer_1s == NULL)
			return (-1);
		
  return(0);
}
		
	void timer_1s_callback(void){
		segundos++;
		osThreadFlagsSet(Id_Thread_Principal, SEGUND_FLG);
		osTimerStart(tid_timer_1s, 1000U);
	}
	
	void thTimer_1s_callback(void *argument){
		osTimerStart(tid_timer_1s, 1000U);
		
		while(1){
			
			osThreadYield();
		}
		
	}
	