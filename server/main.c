#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

#define PORT 60000
#define BUFSIZE 256
static void *buzzer_handle = NULL;  /* 전역 buzzer handle */

/* 클라이언트 용 cds 구조체 선언*/
typedef struct {
    int threshold;
    int csock;
} cds_arg_t;
typedef struct {
    int num;
    int csock;
} seg_arg_t;

/* 서버 강제종료용 전역변수 */
pid_t main_pid;

/* 데몬화 처리 */
void daemonize()
{
    pid_t pid;
    switch(pid = fork()) {
        case -1:
            perror("fork");
            exit(1);
        case 0:
            setsid();
            umask(0);
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
            break;
        default:
            exit(0);
    }
}

/* 시그널 처리 */
void signal_handler(int sig) 
{
    if(buzzer_handle) dlclose(buzzer_handle);
    printf("\n -----서버종료----- \n");
    exit(0);
}

/* 클라이언트 장치 제어 */
void *device_client(void *arg)
{
    int csock = *((int*)arg);
    free(arg);
    char buf[BUFSIZE];
    char device[32], cmd[32];

    memset(buf, 0, BUFSIZE);
    memset(device, 0, sizeof(device));
    memset(cmd, 0, sizeof(cmd));

    /* 1. 클라이언트 메시지 수신 */
    recv(csock, buf, BUFSIZE, 0);
    printf("수신 : %s\n", buf);

    /* 2. 각종 명령어 파싱*/
    sscanf(buf, "%s %s", device, cmd);

    /* 3. dlopen 장치 제어 */
    void *handle;
    void*(*fptr)(void *);

    /* LED 제어 */
    if(strcmp(device, "LED") == 0) {
        handle = dlopen("./libdevice_led.so", RTLD_LAZY);
        if(!handle) {
            fprintf(stderr, "dlopen error: %s\n", dlerror());
            close(csock);
            return NULL;
        }
        dlerror();

        fptr = dlsym(handle, "led_control");
        char *err = dlerror();
        if(err != NULL) {
            fprintf(stderr, "dlsym error: %s\n", err);
            dlclose(handle);
            close(csock);
            return NULL;
        }
        fptr(cmd);
        dlclose(handle);

        /* 동작후 클라이언트가 수신하는 메세지 출력 */
        if(strcmp(cmd, "ON") == 0)
            send(csock, "[SERVER]: LED ON\n", 17, 0);
        else if(strcmp(cmd, "OFF") == 0)
            send(csock, "[SERVER]: LED OFF\n", 18, 0);
        else if(strcmp(cmd, "HIGH") == 0)
            send(csock, "[SERVER]: LED BRIGHTNESS HIGH\n", 30, 0);
        else if(strcmp(cmd, "MID") == 0)
            send(csock, "[SERVER]: LED BRIGHTNESS MID\n", 29, 0);
        else if(strcmp(cmd, "LOW") == 0)
            send(csock, "[SERVER]: LED BRIGHTNESS LOW\n", 29, 0);
    }

    /* BUZZER 제어 */
    else if(strcmp(device, "BUZZER") == 0) {
        if(!buzzer_handle)
            buzzer_handle = dlopen("./libdevice_buzzer.so", RTLD_LAZY);
        if(!buzzer_handle) {
            fprintf(stderr, "dlopen error: %s\n", dlerror());
            close(csock);
            return NULL;
        }
        dlerror();

        fptr = dlsym(buzzer_handle, "buzzer_control");
        char *err = dlerror();
        if(err != NULL) {
            fprintf(stderr, "dlsym error: %s\n", err);
            close(csock);
            return NULL;
        }
        fptr(cmd);

        /* 동작후 클라이언트가 수신하는 메세지 출력 */
        if(strcmp(cmd, "ON") == 0)
            send(csock, "[SERVER]: BUZZER ON\n", 20, 0);
        else if(strcmp(cmd, "OFF") == 0)
            send(csock, "[SERVER]: BUZZER OFF\n", 21, 0);

    }

    /* CDS 센서 제어 */
    else if(strcmp(device, "CDS") == 0) {
        handle = dlopen("./libdevice_cdssensor.so", RTLD_LAZY);
        if(!handle) {
            fprintf(stderr, "dlopen error: %s\n", dlerror());
            close(csock);
            return NULL;
        }
        dlerror();

        void *(*cds_fptr)(void *) = dlsym(handle, "cds_control");
        char *err = dlerror();
        if(err != NULL) {
            fprintf(stderr, "dlsym error: %s\n", err);
            dlclose(handle);
            close(csock);
            return NULL;
        }

        cds_arg_t *cds_arg = malloc(sizeof(cds_arg_t));
        cds_arg->threshold = atoi(cmd);
        cds_arg->csock = csock;
        cds_fptr(cds_arg);
        free(cds_arg);

        dlclose(handle);
    }

    /* SEGMENT 제어 */
    else if(strcmp(device, "SEG") == 0) {
        handle = dlopen("./libdevice_segment.so", RTLD_LAZY);
        if(!handle) {
            fprintf(stderr, "dlopen error: %s", dlerror());
            close(csock);
            return NULL;
        }
        dlerror();
        
        void *(*seg_fptr)(void *) = dlsym(handle, "segment_control");
        char *err = dlerror();
        if(err != NULL) {
            fprintf(stderr, "dlsym error: %s\n", err);
            dlclose(handle);
            close(csock);
            return NULL;
        }

        seg_arg_t *seg_arg = malloc(sizeof(seg_arg_t));
        seg_arg->num = atoi(cmd);
        seg_arg->csock = csock;
        seg_fptr(seg_arg);
        free(seg_arg);
        dlclose(handle);

        /* 세그먼트 끝나고 부저 시작 */
        if(!buzzer_handle)
            buzzer_handle = dlopen("./libdevice_buzzer.so", RTLD_LAZY);
        if(!buzzer_handle) {
            fprintf(stderr, "dlopen error: %s\n", dlerror());
            close(csock);
            return NULL;
        }
        dlerror();

        void *(*buzzer_fptr)(void *) = dlsym(buzzer_handle, "buzzer_control");
        char *buzzer_err = dlerror();
        if(buzzer_err != NULL) {
            fprintf(stderr, "dlsym error: %s\n", buzzer_err);
            close(csock);
            return NULL;
        }
        buzzer_fptr("ON");
        send(csock, "[SERVER]: BUZZER ON\n", strlen("[SERVER]: BUZZER ON\n"), 0);
    }
    /* 서버 종료 */
    else if(strcmp(device, "SHUTDOWN") == 0) {
        send(csock, "[SERVER]: SHUTDOWN\n", strlen("[SERVER]: SHUTDOWN\n"), 0);
        close(csock);
        kill(main_pid, SIGKILL);
        return NULL;
    }

    /* 4. 소켓 닫기 */
    close(csock);
    return NULL;
}

int main() 
{   /* 데몬화 */
    daemonize();
    main_pid = getpid();
    
    /* 서버 시그널 처리 */
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGKILL);    
    sigprocmask(SIG_SETMASK, &mask, NULL);
    signal(SIGINT, signal_handler);

    int ssock, csock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);
    pthread_t thread;

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if(ssock < 0) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(ssock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(1);
    }
    if(listen(ssock, 10) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Server Start - Port Num : %d\n", PORT);

    while(1) {
        csock = accept(ssock, (struct sockaddr*)&client_addr, &len);
        printf("Client Connected : %s\n", inet_ntoa(client_addr.sin_addr));
        
        int *csock_ptr = malloc(sizeof(int));
        *csock_ptr = csock;
        pthread_create(&thread, NULL, device_client, csock_ptr);  // ← 포인터 전달
        pthread_detach(thread);
        }

    return 0;
}