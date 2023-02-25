#ifndef __DTH11_TASK_H__
#define __DTH11_TASK_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"
#include "esp_log.h"

#define DTH11_TAG "DTH11"
#define DEBUG_TAG "DEBUG"

#define DHT11_PIN 21 // 可通过宏定义，修改引脚

#define DHT11_CLR gpio_set_level(DHT11_PIN, 0)
#define DHT11_SET gpio_set_level(DHT11_PIN, 1)
#define DHT11_IN gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT)
#define DHT11_OUT gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT)

typedef struct
{
    uint8_t humi_value;
    uint8_t temp_value;
    SemaphoreHandle_t dth11_mux;
} dth11_value_t;

// Declare the function
void DHT11_task_handler(void *pvParam);
uint8_t DHT11_ReadHumi(void);
uint8_t DHT11_ReadTemp(void);

#endif /* __DTH11_TASK_H__ */