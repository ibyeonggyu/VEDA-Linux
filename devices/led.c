#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <string.h>

#define	LED	26								// GPIO.12 - 26번핀
#define PWM_MAX 100							// 밝기 조절

static int initialized = 0;                 /* 초기 값 세팅 */

void *led_control(void *arg)
{
    printf("Raspberry Pi LED %s\n", (char*)arg);
	
    if(!initialized) {
	    wiringPiSetup();
        pinMode(LED, PWM_OUTPUT);

        /* 밝기 조절 세팅 */
        pwmSetMode(PWM_MODE_MS);
        pwmSetRange(1024);
        pwmSetClock(32);
        initialized = 1;
    }

    if(strcmp((char*)arg, "ON") == 0) {		// LED ON
        pwmWrite(LED, 1023);       
    }

    else if(strcmp((char*)arg, "HIGH") == 0) {
        pwmWrite(LED, 1023);      		// 밝기 최대
    }
    else if(strcmp((char*)arg, "MID") == 0) {
        pwmWrite(LED, 700);       		// 밝기 중간
    }
    else if(strcmp((char*)arg, "LOW") == 0) {
        pwmWrite(LED, 300);        		// 밝기 최저
    }
    else if(strcmp((char*)arg, "OFF") == 0)	{ // LED OFF
        pwmWrite(LED, 0);     
    }
    else {                                  /* 예외 처리 */
        printf("Unknown Command!! \n");
    }     

    return NULL;
}