#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#define SEG_G  22   // GPIO.6
#define SEG_F  23   // GPIO.13
#define SEG_A  24   // GPIO.19
#define SEG_B  25   // GPIO.26
#define SEG_E  27   // GPIO.16
#define SEG_D  28   // GPIO.20
#define SEG_C  29   // GPIO.21

/* 클라이언트 출력용 구조체 선언 */
typedef struct {
    int num;
    int csock;
} seg_arg_t;

// A,B,C,D,E,F 순서 (0=켜짐, 1=꺼짐)
int digits[10][7] = {
    {0,0,0,0,0,0,1},  // 0
    {1,0,0,1,1,1,1},  // 1
    {0,0,1,0,0,1,0},  // 2
    {0,0,0,0,1,1,0},  // 3
    {1,0,0,1,1,0,0},  // 4
    {0,1,0,0,1,0,0},  // 5
    {0,1,0,0,0,0,0},  // 6
    {0,0,0,1,1,1,1},  // 7
    {0,0,0,0,0,0,0},  // 8
    {0,0,0,0,1,0,0},  // 9
};

int gpiopins[7] = {SEG_A, SEG_B, SEG_C, SEG_D, SEG_E, SEG_F, SEG_G};

void fndInit() {
    int i;
    for (i = 0; i < 7; i++) {
        pinMode(gpiopins[i], OUTPUT);
        digitalWrite(gpiopins[i], HIGH);  // 전부 꺼짐
    }
}

void fndControl(int num) {
    int i;
    for (i = 0; i < 7; i++) {
        digitalWrite(gpiopins[i], digits[num][i] ? HIGH : LOW);
    }
}

void *segment_control(void* arg) {
    seg_arg_t *seg_arg = (seg_arg_t *)arg;
    int no = seg_arg->num;
    int csock = seg_arg->csock;

    char msg[64];

    if(no < 0 || no > 9) {
        send(csock, "[SEG ERROR] 0 ~ 9 사이의 번호를 입력하세요.\n",
            strlen("[SEG ERROR] 0 ~ 9 사이의 번호를 입력하세요.\n"), 0);
        return NULL;
    }

    wiringPiSetup();
    fndInit();                        
    
    while(no >= 0) {
        snprintf(msg, sizeof(msg), "[SERVER]: Countdown -%d-\n", no);
        send(csock, msg, strlen(msg), 0);
        fndControl(no);
        sleep(1);
        no--;
    }
    fndInit();

    return NULL;
}