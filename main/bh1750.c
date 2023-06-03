#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "bh1750.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "i2c-example";

#define I2C_MASTER_SCL_IO GPIO_NUM_23 /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_18 /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM 1              /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ   /*!< I2C master read */
#define ACK_CHECK_EN 0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0          /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                /*!< I2C ack value */
#define NACK_VAL 0x1               /*!< I2C nack value */

#define BH1750_SLAVE_ADDR 0x23 // 从机地址
#define BH1750_PWR_DOWN 0x00   // 关闭模块
#define BH1750_PWR_ON 0x01     // 打开模块等待测量指令
#define BH1750_RST 0x07        // 重置数据寄存器值在PowerOn模式下有效
#define BH1750_CON_H 0x10      // 连续高分辨率模式，1lx，120ms
#define BH1750_CON_H2 0x11     // 连续高分辨率模式，0.5lx，120ms
#define BH1750_CON_L 0x13      // 连续低分辨率模式，4lx，16ms
#define BH1750_ONE_H 0x20      // 一次高分辨率模式，1lx，120ms，测量后模块转到PowerDown模式
#define BH1750_ONE_H2 0x21     // 一次高分辨率模式，0.5lx，120ms，测量后模块转到PowerDown模式
#define BH1750_ONE_L 0x23      // 一次低分辨率模式，4lx，16ms，测量后模块转到PowerDown模式

SemaphoreHandle_t data_mutex = NULL;
float data = 0;

/** 
 * @brief Read the data of BH1750
 * 
*/
float get_bh1750_lux()
{
    float lux = 0.0;

    xSemaphoreTake(data_mutex, portMAX_DELAY);
    lux = data;
    xSemaphoreGive(data_mutex);

    return lux;
}

/**
 @brief I2C驱动初始化
 @param 无
 @return 无
*/
int I2C_Init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 @brief I2C写数据函数
 @param slaveAddr -[in] 从设备地址
 @param regAddr -[in] 寄存器地址
 @param pData -[in] 写入数据
 @param dataLen -[in] 写入数据长度
 @return 错误码
*/
int I2C_WriteData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    if (NULL != regAddr)
    {
        i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    }
    i2c_master_write(cmd, pData, dataLen, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(1, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 @brief I2C读数据函数
 @param slaveAddr -[in] 从设备地址
 @param regAddr -[in] 寄存器地址
 @param pData -[in] 读出数据
 @param dataLen -[in] 读出数据长度
 @return 错误码
*/
int I2C_ReadData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | READ_BIT, ACK_CHECK_EN);
    if (NULL != regAddr)
    {
        i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    }
    i2c_master_read(cmd, pData, dataLen, ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(1, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 @brief BH1750初始化函数
 @param 无
 @return 无
*/
void BH1750_Init(void)
{
    uint8_t data;
    int ret = 0;

    // intialize i2c bus
    ret  = I2C_Init();
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C_Init error");
        return;
    }

    // intialize the mutex
    data_mutex = xSemaphoreCreateMutex();
    if(data_mutex == NULL)
    {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex error");
        return;
    }

    data = BH1750_PWR_ON; // 发送启动命令
    I2C_WriteData(BH1750_SLAVE_ADDR, NULL, &data, 1);
    data = BH1750_CON_H; // 设置连续高分辨率模式，1lx，120ms
    I2C_WriteData(BH1750_SLAVE_ADDR, NULL, &data, 1);
}

/**
 @brief BH1750获取光强度
 @param 无
 @return 光强度
*/
float BH1750_ReadLightIntensity(void)
{
    float lux = 0.0;
    uint8_t sensorData[2] = {0};
    I2C_ReadData(BH1750_SLAVE_ADDR, NULL, sensorData, 2);
    lux = (sensorData[0] << 8 | sensorData[1]) / 1.2;
    return lux;
}

void i2c_test_task(void *arg)
{
    int cnt = 0;
    float lux;

    BH1750_Init();
    vTaskDelay(180 / portTICK_PERIOD_MS); // 设置完成后要有一段延迟

    while (1)
    {
        ESP_LOGI(TAG, "test cnt: %d", cnt++);
        lux = BH1750_ReadLightIntensity();
        xSemaphoreTake(data_mutex, portMAX_DELAY);
        data = lux;
        printf("sensor val: %.02f [Lux]\n", lux);
        xSemaphoreGive(data_mutex);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vSemaphoreDelete(data_mutex);
    vTaskDelete(NULL);
}

// void app_main(void)
// {
//     print_mux = xSemaphoreCreateMutex();
//     ESP_ERROR_CHECK(I2C_Init());
//     xTaskCreate(i2c_test_task, "i2c_test_task_0", 1024 * 2, NULL, 10, NULL);
// }
