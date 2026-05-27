add_executable(server_exe main.c)
target_link_libraries(
	server_exe
	dl
	pthread
	wiringPi
	crypt
)
