cmake_minimum_required(VERSION 3.26.4)
project(Tema1 C)
set(CMAKE_C_STANDARD 23)

add_executable(app1 app1.c)

add_executable(app2 app2.c)

#include_directories(/usr/include)
add_executable(app3 app3.c)
#target_link_libraries(app3 argp)
# Nu merge nici cu 
#target_link_libraries(app3 argparse)

# Nu merge cu target_link_libraries, deoarece acesta pe linux este un header, nu un binar, programele compileaza pe environment-ul meu de Linux, probabil deoarece pe MacOS argparse e un binar, dar pe linux este header file
# Am vorbit cu alt coleg si el a trebuit sa dea link manual (gcc -o app3 ./../app3.c -L/usr/include/argp.h) dar nici lui nu i-a mers cu target_link_libraries, deci nu stiu ce poate sa fie