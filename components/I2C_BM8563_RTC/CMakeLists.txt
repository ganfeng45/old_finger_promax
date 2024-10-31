# CMakeFiles.txt inside "SparkFun_u-blox_GNSS" folder
cmake_minimum_required(VERSION 3.5)
file(GLOB_RECURSE SOURCES_CPP "src/*.cpp")

idf_component_register(SRCS ${SOURCES_CPP}
                    INCLUDE_DIRS "src/."
                    REQUIRES "espressif__arduino-esp32"  # Library requires Arduino
                    )

project(RTClib)