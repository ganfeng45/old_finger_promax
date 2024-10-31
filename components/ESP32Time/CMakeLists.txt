# CMakeFiles.txt inside "SparkFun_u-blox_GNSS" folder
cmake_minimum_required(VERSION 3.5)
idf_component_register(SRCS "src/ESP32Time.cpp"
                    INCLUDE_DIRS "src/."
                    REQUIRES "espressif__arduino-esp32" # Library requires Arduino
                    )

project(ESP32Time)