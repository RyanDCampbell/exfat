cmake_minimum_required(VERSION 3.19)
project(fsreader C)

set(CMAKE_C_STANDARD 99)

add_library(fsreader library.c library.h)