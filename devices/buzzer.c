#include <wiringPi.h>
#include <softTone.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define SPKR 	6 	/* GPIO25 */
#define TOTAL 	32 	/* 학교종의 전체 계이름의 수 */

static int buzzer_running = 0;   /* 루프 중단 신호 */
static pthread_mutex_t buzzer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t buzzer_thread;
static int initialized = 0;      /* 초기 값 세팅 */

int notes[] = {
    659, 622, 659, 622, 659, 494, 587, 523,
    440, 0,

    261, 329, 440, 494, 0,

    329, 415, 494, 523, 0,

    329, 659, 622, 659, 622, 659,
    494, 587, 523, 440, 0
};

/* 백그라운드에서 노래 재생하는 스레드 */
static void *buzzer_play(void *arg)
{
    int i;
    for(i = 0; i < TOTAL; i++) {
        pthread_mutex_lock(&buzzer_mutex);
        int running = buzzer_running;
        pthread_mutex_unlock(&buzzer_mutex);

        if(!running) break;

        softToneWrite(SPKR, notes[i]);

        if(notes[i] == 0)
            delay(300);      // 쉼표
        else
            delay(400);      // 음표
    }
    softToneWrite(SPKR, 0);

    pthread_mutex_lock(&buzzer_mutex);
    buzzer_running = 0;
    pthread_mutex_unlock(&buzzer_mutex);

    return NULL;
}


void *buzzer_control(void *arg)
{
    printf("Raspberry Pi Buzzer \n");
    
    if(!initialized) {
        wiringPiSetup();
        pinMode(SPKR, OUTPUT);
        softToneCreate(SPKR);
        initialized = 1;
    } 

    if(strcmp((char*)arg, "ON") == 0) {
        pthread_mutex_lock(&buzzer_mutex);
        if(buzzer_running) {    /* 이미 재생중일 때 무시 */
            pthread_mutex_unlock(&buzzer_mutex);
            return NULL;
        }        
        buzzer_running = 1;
        pthread_mutex_unlock(&buzzer_mutex);

        pthread_create(&buzzer_thread, NULL, buzzer_play, NULL);
        pthread_detach(buzzer_thread);
    }
    else if(strcmp((char*)arg, "OFF") == 0) {
        pthread_mutex_lock(&buzzer_mutex);
        buzzer_running = 0;
        pthread_mutex_unlock(&buzzer_mutex);
        softToneWrite(SPKR, 0);             /* 부저 끄기 */
    }
    return NULL;    
}