#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFSIZE 256

/* 시그널 처리 */
void signal_handler(int sig) 
{
    printf("\n프로그램을 종료합니다. \n");
    exit(0);
}

/* 메뉴판 설정 */
void print_menu() 
{
    printf("\n========================================\n");
    printf("            Device Control Menu\n");
    printf("----------------------------------------\n");
    printf("  1. LED ON\n");
    printf("  2. LED OFF\n");
    printf("  3. LED BRIGHTNESS\n");
    printf("  4. BUZZER ON\n");
    printf("  5. BUZZER OFF\n");
    printf("  6. CDS SENSOR CHECK\n");
    printf("  7. 7SEGMENT COUNTDOWN\n");
    printf("----------------------------------------\n");
    printf("  0. 프로그램 종료\n");
    printf("  99. 서버 강제 종료\n");
    printf("========================================\n");
}

/* LED 밝기 조절 추가 메뉴 */
void print_brightness_menu() 
{
    printf("\n  3-1. HIGH\n");
    printf("  3-2. MID\n");
    printf("  3-3. LOW\n");
    printf("Select: ");
}

/* 서버 연결 관련 */
int connect_server(const char *ip, int port)
{
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if(connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }
    return sock;
}

/* 명령어 전달 */
void send_command(const char *ip, int port, const char *cmd)
{
    int sock;
    char buf[BUFSIZE];
    
    sock = connect_server(ip, port);
    if(sock < 0) {
        printf("[ERROR]: 서버 연결 실패\n");
        return;
    }
    send(sock, cmd, strlen(cmd), 0);
    memset(buf, 0, BUFSIZE);

    int server_exit = recv(sock, buf, BUFSIZE, 0);
    
    if(server_exit <= 0) {
        printf("[SERVER 연결 종료]\n");
        close(sock);
        return;
    }       
    printf("%s\n", buf);

    close(sock);
}

int main(int argc, char *argv[])
{
    /* 시그널 처리 */
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigprocmask(SIG_SETMASK, &mask, NULL);
    signal(SIGINT, signal_handler);

    /* 서버 연결시 인자 잘못 작성시 */
    if(argc != 3) {
        printf("Usage: %s <server_ip> <port> \n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    /* 연결 정보 출력 */
    int test_sock = connect_server(server_ip, port);
    if(test_sock < 0) {
        printf("[ERROR]: 서버 연결 실패\n");
        exit(1);
    }

    /* 내 IP 가져오기 */
    struct sockaddr_in my_addr;
    socklen_t my_len = sizeof(my_addr);
    getsockname(test_sock, (struct sockaddr*)&my_addr, &my_len);

    printf("\n****************************************\n");
    printf("  Client  : %s\n", inet_ntoa(my_addr.sin_addr));
    printf("  Server  : %s\n", server_ip);
    printf("  Port    : %d\n", port);
    printf("  Connected!\n");
    printf("****************************************\n");
    close(test_sock);

    int select;
    char cmd[BUFSIZE];
    char sub_select[8];

    /* 기능 구현 */
    while(1) {
        print_menu();
        printf("Select: ");
        if(scanf("%d", &select) != 1) {
            while(getchar() != '\n');
            select = -1;
        }

        switch(select) {
            case 0: 
                printf("프로그램을 종료합니다.\n");
                exit(0);

            case 1:
                send_command(server_ip, port, "LED ON");
                break;
            case 2:
                send_command(server_ip, port, "LED OFF");
                break;

            case 3:
                print_brightness_menu();
                scanf("%s", sub_select);
                if(strcmp(sub_select, "3-1") == 0)
                    send_command(server_ip, port, "LED HIGH");
                else if(strcmp(sub_select, "3-2") == 0)
                    send_command(server_ip, port, "LED MID");
                else if(strcmp(sub_select, "3-3") == 0)
                    send_command(server_ip, port, "LED LOW");
                else
                    printf("[ERROR]: 3-1, 3-2, 3-3 중에서 입력하세요.\n");
                break;

            case 4:
                send_command(server_ip, port, "BUZZER ON");
                break;

            case 5:
                send_command(server_ip, port, "BUZZER OFF");
                break;

            case 6: {
                int threshold;
                printf("  Threshold 입력 (0~255): ");
                scanf("%d", &threshold);
                if(threshold < 0 || threshold > 255) {
                    printf("[ERROR]: 0~255 사이의 숫자를 입력하세요.\n");
                    break;
                }
                snprintf(cmd, BUFSIZE, "CDS %d", threshold);
                send_command(server_ip, port, cmd);
                break;
            }

            case 7: {
                int num;
                printf("  숫자 입력 (0~9): ");
                scanf("%d", &num);
                if(num < 0 || num > 9) {
                    printf("[ERROR]: 0~9 사이의 숫자를 입력하세요.\n");
                    break;
                }
                snprintf(cmd, BUFSIZE, "SEG %d", num);

                int sock = connect_server(server_ip, port);
                if(sock < 0) {
                    printf("[SERVER 연결 종료]\n");
                    break;
                }

                send(sock, cmd, strlen(cmd), 0);

                /* 카운트다운 끝날때까지 반복 recv */
                char buf[BUFSIZE];
                while(1) {
                    memset(buf, 0, BUFSIZE);
                    int server_exit = recv(sock, buf, BUFSIZE, 0);
                    if(server_exit <= 0) {
                        printf("[SERVER 연결 종료]\n");
                        close(sock);
                        break;
                    }
                    printf("%s", buf);

                    /* [BUZZER ON] 받으면 종료 */
                    if(strstr(buf, "[SERVER]: BUZZER ON") != NULL)
                        break;
                }

                close(sock);
                break;
            }
            /* 서버 강제 종료 */
            case 99: {
                send_command(server_ip, port, "SHUTDOWN");
                exit(0);

            }
            default:
                printf("[ERROR]: 올바른 번호를 입력하세요.\n");
                break;
        }
    }

    return 0;
}
