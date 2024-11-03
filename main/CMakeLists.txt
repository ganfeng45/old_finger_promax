# Hal
# file(GLOB_RECURSE HAL_SRCS
#     hal/*.c
#     hal/*.cpp
# )
# set(HAL_INCS
#     hal/
# )

# Apps
file(GLOB_RECURSE USER_APPS_SRCS
    app/*.c
    app/*.cpp
)
set(USER_APPS_INCS
    app/
)

# Hardware manager
# file(GLOB_RECURSE HM_SRCS
#     hardware_manager/*.c
#     hardware_manager/*.cpp
# )
# set(HM_INCS
#     hardware_manager/
# )

#set(NETWORK_DCE "app/network_dce.c")
# file(GLOB_RECURSE APP_SRCS ${APP_DIR}/*.c ${APP_DIR}/*.cpp  ${CORE_DIR}/*.c ${CORE_DIR}/*.cpp "ui/*.c"  ${CMAKE_SOURCE_DIR})
# file(GLOB_RECURSE APP_SRCS_CPP ${APP_DIR}/*.cpp)
# file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c")
# set(COMMON_HEADER_DIR ${APP_DIR}/blackboard_c.h)
# idf_component_register(SRCS  "main.cpp" ${APP_SRCS} 

idf_component_register(SRCS "main.cpp" ${USER_APPS_SRCS} 
                        INCLUDE_DIRS "."  ${USER_APPS_INCS})



# set_source_files_properties(
#     "app/app_aliyun.c"
#     PROPERTIES COMPILE_FLAGS -Wno-format)

# idf_component_register(SRCS "main.cpp"
#                     INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
