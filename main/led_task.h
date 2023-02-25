#ifndef __LED_TASK_H__
#define __LED_TASK_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

#define LED_TAG "LED"

#define POWER_ON 1
#define POWER_OFF 0

#define LED_PIN CONFIG_BLINK_GPIO

typedef struct
{
    uint8_t led_pin;
} led_info_t;

typedef struct
{
    uint8_t led_status;
    SemaphoreHandle_t led_mux;
} led_status_t;

// Declare the led control function
void Configure_LED(void);
int Get_LED_Status(void);
int Power_LED_ON(void);
int Power_LED_OFF(void);
led_info_t Get_LED_Info(void);
int Get_LED_PIN_Number(void);

// Declare the led task entry function
void led_task_handler(void *pvParameter);

#endif // __LED_TASK_H__