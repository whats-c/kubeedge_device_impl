idf_component_register(SRCS "bh1750.c" "led_task.c" "mq2_task.c" "dth11_task.c" "gatts_server.c" "main.c"
                    INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
