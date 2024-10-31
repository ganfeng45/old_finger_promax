# CMakeFiles.txt inside "SparkFun_u-blox_GNSS" folder
cmake_minimum_required(VERSION 3.5)
idf_component_register(SRCS "src/esp32_can_builtin.cpp" "src/esp32_can.cpp" "src/mcp2515.cpp" "src/mcp2517fd.cpp"
                    INCLUDE_DIRS "src/."
                    REQUIRES "espressif__arduino-esp32" "can_common"# Library requires Arduino
                    )
set_source_files_properties(
    "src/mcp2517fd.cpp"
    PROPERTIES COMPILE_FLAGS -Wno-format)
project(esp32_can)