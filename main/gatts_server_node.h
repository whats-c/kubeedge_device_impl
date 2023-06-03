#ifndef __GATTS_SERVER_FOUR_H__
#define __GATTS_SERVER_FOUR_H__

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

#define GATTS_TAG "NODE_LED_DTH11_BUZZER"

// Define profile service and characteristic UUID
#define GATTS_SERVICE_UUID_NODE_LED_DTH11_BUZZER 0x00FD
#define GATTS_CHAR_POWER_STATUS_UUID_NODE_LED_DTH11_BUZZER 0xFD01
#define GATTS_DESCR_POWER_STATUS_UUID_NODE_LED_DTH11_BUZZER 0xFD10
#define GATTS_CHAR_TEMPERATURE_UUID_NODE_LED_DTH11_BUZZER 0xFD02
#define GATTS_DESCR_TEMPERATURE_UUID_NODE_LED_DTH11_BUZZER 0xFD20
#define GATTS_CHAR_HUMIDITY_UUID_NODE_LED_DTH11_BUZZER 0xFD03
#define GATTS_DESCR_HUMIDITY_UUID_NODE_LED_DTH11_BUZZER 0xFD30
#define GATTS_CHAR_BUZZER_UUID_NODE_LED_DTH11_BUZZER 0xFD04
#define GATTS_DESCR_BUZZER_UUID_NODE_LED_DTH11_BUZZER 0xFD40
#define GATTS_NUM_HANDLE_NODE_LED_DTH11_BUZZER 10

// Define LED, SMOKE, DTH11 device name
#define NODE_LED_DTH11_BUZZER_DEVICE_NAME "NODE_LED_DTH11_BUZZER"

#define NODE_LED_DTH11_BUZZER_MANUFACTURER_DATA_LEN 17

#define GATTS_CHAR_VAL_LEN_MAX 40
#define PREPARE_BUF_MAX_SIZE 1024

#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#define PROFILE_NUM 1
#define PROFILE_NODE_LED_DTH11_BUZZER_APP_ID 0

// Define general profile structure for LED, SMOKE, DTH11
struct gatts_profile_intst_four
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
    uint16_t char3_handle;
    esp_bt_uuid_t char3_uuid;
    esp_gatt_perm_t perm3;
    esp_gatt_char_prop_t property3;
    uint16_t descr3_handle;
    esp_bt_uuid_t descr3_uuid;
    uint16_t char4_handle;
    esp_bt_uuid_t char4_uuid;
    esp_gatt_perm_t perm4;
    esp_gatt_char_prop_t property4;
    uint16_t descr4_handle;
    esp_bt_uuid_t descr4_uuid;
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
typedef uint8_t buzzer_status_t;

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

void gatts_server_node_task_handler(void *);


#endif /* __GATTS_SERVER_H__ */