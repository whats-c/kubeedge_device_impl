#include "dth11_task.h"

uint8_t DHT11Data[4] = {0};

static dth11_value_t dth11_value;

// us延时函数
void DelayUs(uint32_t nCount)
{
    ets_delay_us(nCount);
}

// configure the dth11 and start it
void DHT11_Start(void)
{
    if (dth11_value.dth11_mux == NULL)
    {
        dth11_value.dth11_mux = xSemaphoreCreateMutex();
    }

    DHT11_OUT;          // 设置端口方向
    DHT11_CLR;          // 拉低端口
    DelayUs(19 * 1000); // 持续最低18ms;

    DHT11_SET;   // 释放总线
    DelayUs(30); // 总线由上拉电阻拉高，主机延时30uS;
    DHT11_IN;    // 设置端口方向

    while (!gpio_get_level(DHT11_PIN))
        ; // 等待80us低电平响应信号结束
    while (gpio_get_level(DHT11_PIN))
        ; // 将总线拉高80us
}
//----------读数据----------//
uint8_t DHT11_ReadValue(void)
{
    uint8_t i, sbuf = 0;
    for (i = 8; i > 0; i--)
    {
        sbuf <<= 1;
        while (!gpio_get_level(DHT11_PIN))
            ;
        DelayUs(30); // 延时 30us 后检测数据线是否还是高电平
        if (gpio_get_level(DHT11_PIN))
        {
            sbuf |= 1;
        }
        else
        {
            sbuf |= 0;
        }
        while (gpio_get_level(DHT11_PIN))
            ;
    }
    return sbuf;
}
//--------读温湿度-----------//
uint8_t DHT11_ReadTemHum(uint8_t *buf)
{
    uint8_t check;
    buf[0] = DHT11_ReadValue();
    buf[1] = DHT11_ReadValue();
    buf[2] = DHT11_ReadValue();
    buf[3] = DHT11_ReadValue();

    check = DHT11_ReadValue();

    if (check == buf[0] + buf[1] + buf[2] + buf[3])
        return 1;
    else
        return 0;
}

// read the temperature value
uint8_t DHT11_ReadTemp(void)
{
    uint8_t temp;
    xSemaphoreTake(dth11_value.dth11_mux, 1);
    temp = dth11_value.temp_value;
    xSemaphoreGive(dth11_value.dth11_mux);
    return temp;
}

// read the humidity value
uint8_t DHT11_ReadHumi(void)
{
    uint8_t humi;
    xSemaphoreTake(dth11_value.dth11_mux, 1);
    humi = dth11_value.humi_value;
    xSemaphoreGive(dth11_value.dth11_mux);
    return humi;
}

// if the dth11 task design is not understand, you can look the blog
// https://blog.csdn.net/u011594289/article/details/122182736
//-------DHT11任务---------//
void DHT11_task_handler(void *pvParam)
{

    ESP_LOGI(DTH11_TAG, "DHT11_task_handler start");
    gpio_pad_select_gpio(DHT11_PIN);
    while (1)
    {

        //--------此行可插入挂起其他任务代码,以免这个任务收到干扰-------//

        DHT11_Start();
        if (DHT11_ReadTemHum(DHT11Data))
        {
            xSemaphoreTake(dth11_value.dth11_mux, 1);
            dth11_value.temp_value = DHT11Data[2];
            dth11_value.humi_value = DHT11Data[0];
            printf("Temp=%d, Humi=%d\r\n", dth11_value.temp_value, dth11_value.humi_value);
            xSemaphoreGive(dth11_value.dth11_mux);
        }
        else
        {
            printf("DHT11 Error!\r\n");
        }

        //--------------然后可在此行插入恢复其他任务代码----------------//
        vTaskSuspend(NULL); // 这里是把自己挂起，挂起后该任务被暂停，不恢复是不运行的
    }
}

// void app_main(void)
// {

//     //-----------配置定时器-------------//
//     esp_timer_create_args_t start_dht = {
//         .callback = &timer_periodic,
//         .arg = NULL,
//         .name = "PeriodicTimer"};
//     //----------定时器初始化-----------//
//     esp_timer_init();
//     esp_timer_create(&start_dht, &TempHumi_timer);         // 创建定时器
//     esp_timer_start_periodic(TempHumi_timer, 3000 * 1000); // 定时器每300万微秒（3秒）中断一次

//     dth11_value.temp_value = 0;
//     dth11_value.humi_value = 0;
//     dth11_value.dth11_mux = xSemaphoreCreateMutex();

//     // 创建任务，并且使用一个核心，毕竟ESP32是双核，0是第一个核心，1是第二个。
//     // 任务栈的大小由自己来测试，代码这里有空余多的空间。
//     // 任务级别尽量高点，这里为2；再放入任务句柄，以便用于定时中断来恢复任务
//     xTaskCreatePinnedToCore(DHT11_task_handler, "DHT11", 4000, NULL, 2, &dhtTask, 1);
// }