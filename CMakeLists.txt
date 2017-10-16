cmake_minimum_required(VERSION 2.6)
project(memstuff)

add_executable(memstuff main.c)

install(TARGETS memstuff RUNTIME DESTINATION bin)
