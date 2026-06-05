# VEDA-Linux : TCP 기반 원격 장치 제어 프로그램

## 프로젝트 구조

```
Project/
├── build.sh                        # 빌드 및 Raspberry Pi4 전송 스크립트
├── CMakeLists.txt                  # 루트 CMake 설정
├── raspberrypi4_toolchain.cmake    # 크로스컴파일 툴체인 설정
├── client/
│   └── main.c                      # 클라이언트 메인 소스
├── devices/
│   ├── CMakeLists.txt              # 장치 라이브러리 빌드 설정
│   ├── led.c                       # LED 제어 (PWM 밝기 조절)
│   ├── buzzer.c                    # 부저 제어 (softTone 멜로디 재생)
│   ├── cds_sensor.c                # 조도센서 제어 (I2C ADC)
│   └── segment.c                   # 7세그먼트 제어 (카운트다운)
└── server/
    ├── CMakeLists.txt              # 서버 빌드 설정
    └── main.c                      # 서버 메인 소스 (데몬, dlopen, pthread)
```

## 빌드 방법
### 사전 준비
```bash
sudo apt install gcc-aarch64-linux-gnu
```
### Raspberry Pi4 연결 설정
`build.sh` 상단의 아래 값을 환경에 맞게 수정:
```bash
RPI_USER=aposeseco       # Raspberry Pi4 계정명
RPI_IP=100.65.223.14     # Raspberry Pi4 IP 주소
RPI_DEST=/home/aposeseco/Project  # 전송 경로
```

### 빌드 및 전송
```bash
./build.sh          # 전체 빌드 + Raspberry Pi4 자동 전송
./build.sh clean    # 빌드 결과물 정리
```

빌드 시 자동으로:
1. 클라이언트 (`client_exe`) — Ubuntu에서 gcc 빌드
2. 서버 + 장치 라이브러리 — CMake 크로스컴파일 (aarch64)
3. `server_exe`, `*.so` — Raspberry Pi4로 scp 전송


## 실행 방법
### 서버 (Raspberry Pi4)
```bash
cd ~/Project
./server_exe
```

### 클라이언트 (Ubuntu)
```bash
./client_exe <Raspberry Pi4 IP> 60000
```

## 서버 구조 (server/main.c)
### 주요 기능
- `daemonize()` 로 백그라운드 데몬 프로세스 실행
- `pthread` 멀티스레드로 다중 클라이언트 동시 처리
- `dlopen` / `dlsym` 으로 런타임에 장치별 `.so` 동적 로드
- `SIGINT` / `SIGTERM` 시그널 처리로 안전 종료 (buzzer dlclose)

### 지원 명령어
| 명령어 | 동작 |
|--------|------|
| `LED ON/OFF` | LED 켜기/끄기 |
| `LED HIGH/MID/LOW` | LED 밝기 조절 |
| `BUZZER ON/OFF` | 멜로디 재생/정지 |
| `CDS <threshold>` | 조도 측정 후 LED 자동 제어 |
| `SEG <num>` | 카운트다운 후 부저 울림 |
| `SHUTDOWN` | 서버 종료 |

## 클라이언트 구조 (client/main.c)

- 실행 인자: `<서버 IP> <포트>`
- 메뉴 기반 UI로 장치 선택 및 명령 전송
- 명령마다 새 TCP 연결 생성 후 응답 수신
- `SIGINT` (Ctrl+C) 처리로 안전 종료
- 7세그먼트 카운트다운은 소켓 유지하며 실시간 출력

## 장치 라이브러리 (devices/)
### LED (libdevice_led.so)
- wiringPi PWM 밝기 제어
- HIGH(1023) / MID(700) / LOW(300) / OFF(0)

### 부저 (libdevice_buzzer.so)
- softTone으로 멜로디 재생
- 별도 스레드 백그라운드 재생
- mutex로 중복 재생 방지

### 조도센서 (libdevice_cdssensor.so)
- I2C (ADS7830 ADC) 조도값 측정
- Threshold 비교 → LED 자동 ON/OFF

### 7세그먼트 (libdevice_segment.so)
- 7개 GPIO 핀 직접 제어
- 입력 숫자부터 0까지 1초 간격 카운트다운
- 완료 후 서버에서 부저 자동 실행

## 사용 방법
### 메뉴 구성
```
| 번호 | 기능 | 설명 |
| 1 | LED ON | LED 켜기 |
| 2 | LED OFF | LED 끄기 |
| 3 | LED BRIGHTNESS | 밝기 조절 (3-1: HIGH / 3-2: MID / 3-3: LOW) |
| 4 | BUZZER ON | 멜로디 재생 시작 (백그라운드 실행) |
| 5 | BUZZER OFF | 멜로디 재생 중지 |
| 6 | CDS SENSOR CHECK | Threshold 입력 후 조도 측정 및 LED 자동 제어 |
| 7 | 7SEGMENT COUNTDOWN | 숫자(0~9) 입력 후 카운트다운, 완료 시 부저 자동 실행 |
| 0 | 프로그램 종료 | 클라이언트 정상 종료 및 클라이언트 재접속 가능|
| 99 | 서버 강제 종료 | 서버 프로세스 원격 강제 종료 |
```
### 시그널 처리
```
| 시그널 | 동작 |
|--------|------|
| Ctrl+C (SIGINT) | 클라이언트 정상 종료 |
| Ctrl+Z (SIGTSTP) | 무시 (차단됨) |
| Ctrl+\ (SIGQUIT) | 무시 (차단됨) |
| 그 외 시그널 | 전부 차단 (SIGINT만 허용) |
```
## 종료 방법
```bash
0   # 클라이언트 정상 종료
99  # 서버 강제 종료
Ctrl+C  # 클라이언트 종료
```
