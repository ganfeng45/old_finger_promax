set(COMPONENT_REQUIRES "Adafruit_Fingerprint" "MFRC522_I2C" "ESP8266Audio" "RTClib" "Adafruit_BusIO""DFRobot_LIS""arduino")

idf_component_register(SRCS
                "blackboard.cpp" 
                "blackboard_c.c" 
                "myutil.cpp"
                "access_task.cpp"
                "task_ble.cpp"
                "audio_task.c"
                "safeblet_task.cpp"
                "main.cpp"
                INCLUDE_DIRS ".")

# spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
                


