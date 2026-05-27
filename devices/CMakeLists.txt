add_library(device_led SHARED led.c)
target_link_libraries(device_led wiringPi)

add_library(device_cdssensor SHARED cds_sensor.c)
target_link_libraries(device_cdssensor wiringPi)

add_library(device_buzzer SHARED buzzer.c)
target_link_libraries(device_buzzer wiringPi)

add_library(device_segment SHARED segment.c)
target_link_libraries(device_segment wiringPi)