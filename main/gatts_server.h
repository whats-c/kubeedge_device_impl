#ifndef __GATTS_SERVER_H__
#define __GATTS_SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_mac.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#include "led_task.h"
#include "mq2_task.h"
#include "dth11_task.h"
#include "bh1750.h"

#define GATTS_TAG "BLE_LED_SMOKE_DTH11"

// Define profile service and characteristic UUID
#define GATTS_SERVICE_UUID_LED 0x00FF
#define GATTS_CHAR_POWER_STATUS_UUID_LED 0xFF01
#define GATTS_DESCR_POWER_STATUS_UUID_LED 0xFF10
#define GATTS_CHAR_PIN_NUMBER_UUID_LED 0xFF02
#define GATTS_DESCR_PIN_NUMBER_UUID_LED 0xFF20
#define GATTS_NUM_HANDLE_LED 6

#define GATTS_SERVICE_UUID_SMOKE 0x00FE
#define GATTS_CHAR_SOMKE_UUID_SMOKE 0xFE01
#define GATTS_DESCR_SOMKE_UUID_SMOKE 0xFE10
#define GATTS_CHAR_ALARM_UUID_SMOKE 0xFE02
#define GATTS_DESCR_ALARM_UUID_SMOKE 0xFE20
#define GATTS_NUM_HANDLE_SMOKE 6

#define GATTS_SERVICE_UUID_DTH11 0x00FD
#define GATTS_CHAR_TEMPERATURE_UUID_DTH11 0xFD01
#define GATTS_DESCR_TEMPERATURE_UUID_DTH11 0xFD10
#define GATTS_CHAR_HUMIDITY_UUID_DTH11 0xFD02
#define GATTS_DESCR_HUMIDITY_UUID_DTH11 0xFD20
#define GATTS_NUM_HANDLE_DTH11 6

#define GATTS_SERVICE_UUID_BH1750 0x00FC
#define GATTS_CHAR_LUX_UUID_BH1750 0xFD01
#define GATTS_DESCR_LUX_UUID_BH1750 0xFD10
#define GATTS_NUM_HANDLE_BH1750 6

// Define LED, SMOKE, DTH11 device name
#define LED_DEVICE_NAME "ESP_LED_SOMKE_DTH11"
#define SMOKE_DEVICE_NAME "ESP_LED_SOMKE_DTH11"
#define DTH11_DEVICE_NAME "ESP_LED_SOMKE_DTH11"
#define BH1750_DEVICE_NAME "ESP_LED_SOMKE_DTH11"

#define LED_MANUFACTURER_DATA_LEN 17
#define SOMKE_MANUFACTURER_DATA_LEN 17
#define DTH11_MANUFACTURER_DATA_LEN 17
#define BH1750_MANUFACTURER_DATA_LEN 17

#define GATTS_CHAR_VAL_LEN_MAX 40
#define PREPARE_BUF_MAX_SIZE 1024

#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#define PROFILE_NUM 4
#define PROFILE_LED_APP_ID 0
#define PROFILE_SMOKE_APP_ID 1
#define PROFILE_DTH11_APP_ID 2
#define PROFILE_BH1750_APP_ID 3

// Define general profile structure for LED, SMOKE, DTH11
struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char1_handle;
    esp_bt_uuid_t char1_uuid;
    esp_gatt_perm_t perm1;
    esp_gatt_char_prop_t property1;
    uint16_t descr1_handle;
    esp_bt_uuid_t descr1_uuid;
    uint16_t char2_handle;
    esp_bt_uuid_t char2_uuid;
    esp_gatt_perm_t perm2;
    esp_gatt_char_prop_t property2;
    uint16_t descr2_handle;
    esp_bt_uuid_t descr2_uuid;
};

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

typedef uint8_t led_power_status_t;
typedef uint8_t led_pin_number_t;
typedef uint8_t dth11_temperature_t;
typedef uint8_t dth11_humidity_t;
typedef float smoke_smoke_t;
typedef uint8_t smoke_alarm_t;
typedef float bh1750_lux_t;

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

void gatts_server_task_handler(void *);
#endif /* __GATTS_SERVER_H__ */