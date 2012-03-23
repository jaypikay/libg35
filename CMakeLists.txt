cmake_minimum_required(VERSION 2.6)
project(libg35)

set(CMAKE_C_FLAGS "-g -Wall")

add_library(g35 SHARED g35.c g35uinput.c)
target_link_libraries(g35 usb)
