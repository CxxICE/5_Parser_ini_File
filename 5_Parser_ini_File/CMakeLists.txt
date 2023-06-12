cmake_minimum_required(VERSION 3.22.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(PROJ "Parser_ini_File")
set(SRC "Parser_ini_File.cpp")
set(OUT "Parser_ini_File")


project (${PROJ})
add_executable(Parser_ini_File ${SRC})

set_property(
TARGET Parser_ini_File 
PROPERTY OUTPUT_NAME ${OUT})