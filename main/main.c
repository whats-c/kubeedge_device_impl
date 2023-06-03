#include "led_task.h"
#include "mq2_task.h"
#include "dth11_task.h"
// #include "gatts_server.h"
#include "esp_system.h"
#include "bh1750.h"
#include "gatts_server_node.h"

#define MAIN_TAG "MAIN"

static TaskHandle_t led_task_handle = NULL;
// static TaskHandle_t mq2_handler = NULL;
// static TaskHandle_t bh1750_handler = NULL;

//-------------任务句柄----------------//
TaskHandle_t dhtTask;
//-------------定时器句柄定义和回调函数-----------//
esp_timer_handle_t TempHumi_timer = 0;

// 定时器的回调函数
void timer_periodic(void *arg)
{
    vTaskResume(dhtTask); // 恢复任务
}

void app_main(void)
{
    // create led task and start it
    ESP_LOGI(LED_TAG, "Starting LED Task\n\r");
    xTaskCreate(led_task_handler, "led_task_handler", 2048, NULL, 5, &led_task_handle);

    // // creat mq2 task and start it
    // ESP_LOGI(MQ2_TAG, "Starting MQ2 Task\n\r");
    // xTaskCreate(mq2_task_handler, "mq2_task_entry", 4096, NULL, 5, &mq2_handler);

    // create dth11 task and start it
    // all the operation of below you can read the comments in dth11_task.c
    esp_timer_create_args_t start_dht = {
        .callback = &timer_periodic,
        .arg = NULL,
        .name = "PeriodicTimer"};
    esp_timer_init();
    esp_timer_create(&start_dht, &TempHumi_timer);
    esp_timer_start_periodic(TempHumi_timer, 3000 * 1000);
    xTaskCreate(DHT11_task_handler, "DHT11", 4096, NULL, 2, &dhtTask);

    // xTaskCreate(i2c_test_task, "bh1750", 4096, NULL, 2, &bh1750_handler);

    // create gatts server task and start it
    xTaskCreate(gatts_server_node_task_handler, "gatts_server", 4096, NULL, 5, NULL);
}
