#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <string.h>
#include <sys/socket.h>

#define CDS 0								// GPIO.17 - 11번핀
#define	LED	26								// GPIO.12 - 26번핀

/* 클라이언트 출력용 구조체 선언 */
typedef struct {
    int threshold;
    int csock;
} cds_arg_t;

void *cds_control(void *arg)
{
	cds_arg_t *cds_arg = (cds_arg_t *)arg;
    int threshold = cds_arg->threshold;
    int csock = cds_arg->csock;
	char msg[128];

	wiringPiSetup();
	pinMode(LED, OUTPUT);	

	int fd, a2dChannel = CDS;
	int a2dVal;	
    
	if((fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48))<0) {
		printf("wiringPiI2CSetupInterface failed : \n");
		return NULL;
	}	

	wiringPiI2CWrite(fd, 0x00 | a2dChannel);
	wiringPiI2CRead(fd); 
	a2dVal = wiringPiI2CRead(fd);

	if(a2dVal < threshold) {
        digitalWrite(LED, LOW);
        snprintf(msg, sizeof(msg), "[SERVER]: CDS SENSOR CHECK \n a2dVal: %d | Threshold: %d | Bright -> LED OFF\n", a2dVal, threshold);
    } else {
        digitalWrite(LED, HIGH);
        snprintf(msg, sizeof(msg), "[SERVER]: CDS SENSOR CHECK \n a2dVal: %d | Threshold: %d | Dark -> LED ON\n", a2dVal, threshold);
    }

    send(csock, msg, strlen(msg), 0);	
	return NULL;
}
