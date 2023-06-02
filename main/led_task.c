/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "led_task.h"

static led_status_t led_status;

// static TaskHandle_t led_task_handle = NULL;

void led_task_handler(void *pvParameter)
{
    ESP_LOGI(LED_TAG, "Starting LED Task\n\r");
    // intialize led status
    led_status.led_mux = xSemaphoreCreateMutex();
    // configuration of led
    Configure_LED();

    while (1)
    {
        switch (Get_LED_Status())
        {
        case POWER_ON:
            Power_LED_OFF();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            break;
        case POWER_OFF:
            Power_LED_ON();
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            break;
        default:
            break;
        }
    }
    Power_LED_ON();

    vTaskDelete(NULL);
}

// configure led 
void Configure_LED(void)
{
    ESP_LOGI(LED_TAG, "Configuring LED\n\r");
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

// get led status
int Get_LED_Status(void)
{
    int status;
    xSemaphoreTake(led_status.led_mux, 1);
    status = (int)led_status.led_status;
    xSemaphoreGive(led_status.led_mux);
    return status;
}

// set led status
int Power_LED_ON(void)
{
    ESP_LOGI(LED_TAG, "Turning LED ON\n\r");
    gpio_set_level(LED_PIN, POWER_ON);
    xSemaphoreTake(led_status.led_mux, 1);
    led_status.led_status = POWER_ON;
    xSemaphoreGive(led_status.led_mux);
    return 0;
}

// reset the led status
int Power_LED_OFF(void)
{
    ESP_LOGI(LED_TAG, "Turning LED OFF\n\r");
    gpio_set_level(LED_PIN, POWER_OFF);
    xSemaphoreTake(led_status.led_mux, 1);
    led_status.led_status = POWER_OFF;
    xSemaphoreGive(led_status.led_mux);
    return 0;
}

// get the led information. now the information only have the led pin number
led_info_t Get_LED_Info(void)
{
    led_info_t led_info;
    led_info.led_pin = LED_PIN;
    return led_info;
}

// get the led pin number
int Get_LED_PIN_Number(void)
{
    return LED_PIN;
}

// void app_main(void)
// {
//     ESP_LOGI(LED_TAG, "Starting LED Task\n\r");
//     xTaskCreate(&led_task_handler, "led_task_handler", 2048, NULL, 5, &led_task_handle);
//     return;
// }