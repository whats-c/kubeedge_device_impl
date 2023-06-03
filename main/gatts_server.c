/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/****************************************************************************
 *
 * This demo showcases BLE GATT server. It can send adv data, be connected by client.
 * Run the gatt_client demo, the client demo will automatically connect to the gatt_server demo.
 * Client demo will enable gatt_server's notify after connection. The two devices will then exchange
 * data.
 *
 ****************************************************************************/
#include "gatts_server.h"
// Declare the static function
static void gatts_profile_led_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_smoke_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_dth11_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_bh1750_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static uint8_t adv_config_done = 0;

static esp_gatt_char_prop_t led_power_status_property = 0;
static esp_gatt_char_prop_t led_pin_number_property = 0;
static esp_gatt_char_prop_t smoke_smoke_property = 0;
static esp_gatt_char_prop_t smoke_alarm_property = 0;
static esp_gatt_char_prop_t dth11_temperature_property = 0;
static esp_gatt_char_prop_t dth11_humidity_property = 0;
static esp_gatt_char_prop_t bh1750_lux_property = 0;

static prepare_type_env_t led_prepare_write_env;
static prepare_type_env_t smoke_prepare_write_env;
static prepare_type_env_t dth11_prepare_write_env;
static prepare_type_env_t bh1750_prepare_write_env;

led_power_status_t *led_power_status_value_value;
led_pin_number_t *led_pin_number_value_value;
smoke_smoke_t *smoke_smoke_value_value;
smoke_alarm_t *smoke_alarm_value_value;
dth11_temperature_t *dth11_temperature_value_value;
dth11_humidity_t *dth11_humidity_value_value;
bh1750_lux_t *bh1750_lux_value_value;

static esp_attr_value_t gatts_led_power_status_val = {
    .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(led_power_status_t),
    .attr_value = NULL,
};

static esp_attr_value_t gatts_led_pin_number_val = {
    .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(led_pin_number_t),
    .attr_value = NULL,
};

static esp_attr_value_t gatts_smoke_smoke_val = {
    .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(smoke_smoke_t),
    .attr_value = NULL,
};

static esp_attr_value_t gatts_smoke_alarm_val = {
    .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(smoke_alarm_t),
    .attr_value = NULL,
};

static esp_attr_value_t gatts_dth11_temperature_val = {
    .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(dth11_temperature_t),
    .attr_value = NULL,
};

static esp_attr_value_t gatts_dth11_humidity_val = {
    .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(dth11_humidity_t),
    .attr_value = NULL,
};

static esp_attr_value_t gatts_bh1750_lux_val = {
    .attr_max_len = GATTS_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(bh1750_lux_t),
    .attr_value = NULL,
};

static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xEE,
    0x00,
    0x00,
    0x00,
    // second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xFF,
    0x00,
    0x00,
    0x00,
};

// The length of adv data must be less than 31 bytes
// static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
// adv data
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, // slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, // slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_LED_APP_ID] = {
        .gatts_cb = gatts_profile_led_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [PROFILE_DTH11_APP_ID] = {
        .gatts_cb = gatts_profile_smoke_event_handler, .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [PROFILE_SMOKE_APP_ID] = {
        .gatts_cb = gatts_profile_dth11_event_handler, .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [PROFILE_BH1750_APP_ID] = {
        .gatts_cb = gatts_profile_bh1750_event_handler, .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }
};

void write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        // advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(GATTS_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

void write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp)
    {
        if (param->write.is_prep)
        {
            if (prepare_write_env->prepare_buf == NULL)
            {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL)
                {
                    ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }
            else
            {
                if (param->write.offset > PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_OFFSET;
                }
                else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK)
            {
                ESP_LOGE(GATTS_TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK)
            {
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;
        }
        else
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
    {
        esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }
    else
    {
        ESP_LOGI(GATTS_TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf)
    {

        if (param->write.handle == gl_profile_tab[PROFILE_LED_APP_ID].char1_handle)
        {
            memcpy(gatts_led_power_status_val.attr_value, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
            if (gatts_led_power_status_val.attr_value[0] == 0x01)
            {
                Power_LED_ON();
            }
            else if (gatts_led_power_status_val.attr_value[0] == 0x00)
            {
                Power_LED_OFF();
            }
            else
            {
                ESP_LOGE(GATTS_TAG, "LED power status error");
            }
            // case gl_profile_tab[PROFILE_LED_APP_ID].char2_handle:
            //     memset(gatts_led_pin_number_val, prepare_write_env.prepare_buf, prepare_write_env.prepare_len);
            //     break;
            // case gl_profile_tab[PROFILE_SMOKE_APP_ID].char1_handle:
            //     memset(gatts_smoke_smoke_val, prepare_write_env.prepare_buf, prepare_write_env.prepare_len);
            //     break;
            // case gl_profile_tab[PROFILE_SMOKE_APP_ID].char2_handle:
            //     memset(gatts_smoke_alarm_val, prepare_write_env.prepare_buf, prepare_write_env.prepare_len);
            //     break;
            // case gl_profile_tab[PROFILE_DTH11_APP_ID].char1_handle:
            //     memset(gatts_dth11_temperature_val, prepare_write_env.prepare_buf, prepare_write_env.prepare_len);
            //     break;
            // case gl_profile_tab[PROFILE_DTH11_APP_ID].char2_handle:
            //     memset(gatts_dth11_humidity_val, prepare_write_env.prepare_buf, prepare_write_env.prepare_len);
            //     braek;
        }
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_led_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_LED_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_LED_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_LED_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_LED_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_LED;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(LED_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        // // config adv data
        // esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        // if (ret)
        // {
        //     ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x ", ret);
        // }
        // adv_config_done |= adv_config_flag;
        // // config scan response data
        // ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        // if (ret)
        // {
        //     ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        // }
        // adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_LED_APP_ID].service_id, GATTS_NUM_HANDLE_LED);
        break;

    case ESP_GATTS_READ_EVT:
        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, connection_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));

        if (param->read.handle == gl_profile_tab[PROFILE_LED_APP_ID].char1_handle)
        {
            led_power_status_t led_status;
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = gatts_led_power_status_val.attr_len;
            // syhcronize the value of the attribute with the device info
            led_status = (led_power_status_t)Get_LED_Status();
            ESP_LOGI(DEBUG_TAG, "led_status = %d", led_status);
            memcpy(gatts_led_power_status_val.attr_value, &led_status, gatts_led_power_status_val.attr_len);
            memcpy(rsp.attr_value.value, gatts_led_power_status_val.attr_value, gatts_led_power_status_val.attr_len);
            ESP_LOGI(DEBUG_TAG, "rsp.attr_value.len=%d, rsp.attr_value.value=%d\r\n", rsp.attr_value.len, rsp.attr_value.value[0]);
        }
        else if (param->read.handle == gl_profile_tab[PROFILE_LED_APP_ID].char2_handle)
        {
            led_pin_number_t led_pin_number;
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = gatts_led_pin_number_val.attr_len;
            led_pin_number = (led_pin_number_t)Get_LED_PIN_Number();
            ESP_LOGI(DEBUG_TAG, "led_pin_number = %d", led_pin_number);
            memcpy(gatts_led_pin_number_val.attr_value, &led_pin_number, gatts_led_pin_number_val.attr_len);
            memcpy(rsp.attr_value.value, gatts_led_pin_number_val.attr_value, gatts_led_pin_number_val.attr_len);
            ESP_LOGI(DEBUG_TAG, "rsp.attr_value.len=%d, rsp.attr_value.value=%d\r\n", rsp.attr_value.len, rsp.attr_value.value[0]);
        }
        // the following read operation will be filled in later
        // rsp.attr_value.handle = param->read.handle;
        // rsp.attr_value.len = 4;
        // rsp.attr_value.value[0] = 0xde;
        // rsp.attr_value.value[1] = 0xed;
        // rsp.attr_value.value[2] = 0xbe;
        // rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        // if (!param->write.is_prep)
        // {
        //     ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        //     esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //     if (gl_profile_tab[PROFILE_LED_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
        //     {
        //         uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
        //         if (descr_value == 0x0001)
        //         {
        //             if (led_power_status_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "notify enable");
        //                 uint8_t notify_data[15];
        //                 for (int i = 0; i < sizeof(notify_data); ++i)
        //                 {
        //                     notify_data[i] = i % 0xff;
        //                 }
        //                 // the size of notify_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_LED_APP_ID].char_handle,
        //                                             sizeof(notify_data), notify_data, false);
        //             }
        //         }
        //         else if (descr_value == 0x0002)
        //         {
        //             if (led_pin_number_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "indicate enable");
        //                 uint8_t indicate_data[15];
        //                 for (int i = 0; i < sizeof(indicate_data); ++i)
        //                 {
        //                     indicate_data[i] = i % 0xff;
        //                 }
        //                 // the size of indicate_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_LED_APP_ID].char_handle,
        //                                             sizeof(indicate_data), indicate_data, true);
        //             }
        //         }
        //         else if (descr_value == 0x0000)
        //         {
        //             ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
        //         }
        //         else
        //         {
        //             ESP_LOGE(GATTS_TAG, "unknown descr value");
        //             esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //         }
        //     }
        // }
        write_event_env(gatts_if, &led_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        exec_write_event_env(&led_prepare_write_env, param);
        break;

    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_LED_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_LED_APP_ID].char1_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_LED_APP_ID].char1_uuid.uuid.uuid16 = GATTS_CHAR_POWER_STATUS_UUID_LED;
        gl_profile_tab[PROFILE_LED_APP_ID].char2_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_LED_APP_ID].char2_uuid.uuid.uuid16 = GATTS_CHAR_PIN_NUMBER_UUID_LED;
        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_LED_APP_ID].service_handle);

        ESP_LOGI(GATTS_TAG, "add_power_status_char_ret");
        led_power_status_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_power_status_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_LED_APP_ID].service_handle, &gl_profile_tab[PROFILE_LED_APP_ID].char1_uuid,
                                                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                                     led_power_status_property,
                                                                     &gatts_led_power_status_val, NULL);
        ESP_LOGI(GATTS_TAG, "add_power_status_char_ret----> END");
        if (add_power_status_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_power_status_char_ret);
        }

        ESP_LOGI(GATTS_TAG, "add_pin_number_char_ret");
        led_pin_number_property = ESP_GATT_CHAR_PROP_BIT_READ;
        esp_err_t add_pin_number_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_LED_APP_ID].service_handle, &gl_profile_tab[PROFILE_LED_APP_ID].char2_uuid,
                                                                   ESP_GATT_PERM_READ,
                                                                   led_pin_number_property,
                                                                   &gatts_led_pin_number_val, NULL);
        ESP_LOGI(GATTS_TAG, "add_pin_number_char_ret----> END");
        if (add_pin_number_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_pin_number_char_ret);
        }

        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {

        ESP_LOGI(GATTS_TAG, "esp gatts led add char event\r");
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        if (param->add_char.char_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_LED_APP_ID].char1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_LED_APP_ID].char1_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_LED_APP_ID].descr1_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_LED_APP_ID].descr1_uuid.uuid.uuid16 = GATTS_DESCR_POWER_STATUS_UUID_LED;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL)
            {
                ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_LED_APP_ID].service_handle, &gl_profile_tab[PROFILE_LED_APP_ID].descr1_uuid,
                                                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
            if (add_descr_ret)
            {
                ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        else if (param->add_char.char_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_LED_APP_ID].char2_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_LED_APP_ID].char2_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_LED_APP_ID].descr2_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_LED_APP_ID].descr2_uuid.uuid.uuid16 = GATTS_DESCR_PIN_NUMBER_UUID_LED;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL)
            {
                ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_LED_APP_ID].service_handle, &gl_profile_tab[PROFILE_LED_APP_ID].descr2_uuid,
                                                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
            if (add_descr_ret)
            {
                ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        else
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", param->add_char.status);
        }

        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        if (param->add_char_descr.descr_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_LED_APP_ID].descr1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_LED_APP_ID].descr1_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
            break;
        }
        else if (param->add_char_descr.descr_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_LED_APP_ID].descr2_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_LED_APP_ID]
                .descr2_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
            break;
        }
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_LED_APP_ID].conn_id = param->connect.conn_id;
        // start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_profile_smoke_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_SMOKE_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_SMOKE_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_SMOKE_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_SMOKE_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_SMOKE;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SMOKE_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        // // config adv data
        // esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        // if (ret)
        // {
        //     ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x ", ret);
        // }
        // adv_config_done |= adv_config_flag;
        // // config scan response data
        // ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        // if (ret)
        // {
        //     ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        // }
        // adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_SMOKE_APP_ID].service_id, GATTS_NUM_HANDLE_SMOKE);
        break;

    case ESP_GATTS_READ_EVT:
        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, connection_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));

        if (param->read.handle == gl_profile_tab[PROFILE_SMOKE_APP_ID].char1_handle)
        {
            smoke_smoke_t smoke_smoke_value;
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = gatts_smoke_smoke_val.attr_len;
            smoke_smoke_value = (smoke_smoke_t)GetSmoke();
            memcpy(gatts_smoke_smoke_val.attr_value, &smoke_smoke_value, gatts_smoke_smoke_val.attr_len);
            memcpy(rsp.attr_value.value, gatts_smoke_smoke_val.attr_value, gatts_smoke_smoke_val.attr_len);
        }
        else if (param->read.handle == gl_profile_tab[PROFILE_SMOKE_APP_ID].char2_handle)
        {
            smoke_alarm_t smoke_alarm_value;
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = gatts_smoke_alarm_val.attr_len;
            smoke_alarm_value = (smoke_alarm_t)GetAlarm();
            memcpy(gatts_smoke_alarm_val.attr_value, &smoke_alarm_value, gatts_smoke_alarm_val.attr_len);
            memcpy(rsp.attr_value.value, gatts_smoke_alarm_val.attr_value, gatts_smoke_alarm_val.attr_len);
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, read error handle %d\n\r", param->read.handle);
        }
        // the following read operation will be filled in later

        // rsp.attr_value.handle = param->read.handle;
        // rsp.attr_value.len = 4;
        // rsp.attr_value.value[0] = 0xde;
        // rsp.attr_value.value[1] = 0xed;
        // rsp.attr_value.value[2] = 0xbe;
        // rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        // if (!param->write.is_prep)
        // {
        //     ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        //     esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //     if (gl_profile_tab[PROFILE_SMO].descr_handle == param->write.handle && param->write.len == 2)
        //     {
        //         uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
        //         if (descr_value == 0x0001)
        //         {
        //             if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "notify enable");
        //                 uint8_t notify_data[15];
        //                 for (int i = 0; i < sizeof(notify_data); ++i)
        //                 {
        //                     notify_data[i] = i % 0xff;
        //                 }
        //                 // the size of notify_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
        //                                             sizeof(notify_data), notify_data, false);
        //             }
        //         }
        //         else if (descr_value == 0x0002)
        //         {
        //             if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "indicate enable");
        //                 uint8_t indicate_data[15];
        //                 for (int i = 0; i < sizeof(indicate_data); ++i)
        //                 {
        //                     indicate_data[i] = i % 0xff;
        //                 }
        //                 // the size of indicate_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
        //                                             sizeof(indicate_data), indicate_data, true);
        //             }
        //         }
        //         else if (descr_value == 0x0000)
        //         {
        //             ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
        //         }
        //         else
        //         {
        //             ESP_LOGE(GATTS_TAG, "unknown descr value");
        //             esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //         }
        //     }
        // }
        write_event_env(gatts_if, &smoke_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        exec_write_event_env(&smoke_prepare_write_env, param);
        break;

    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_SMOKE_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_SMOKE_APP_ID].char1_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_SMOKE_APP_ID].char1_uuid.uuid.uuid16 = GATTS_CHAR_SOMKE_UUID_SMOKE;
        gl_profile_tab[PROFILE_SMOKE_APP_ID].char2_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_SMOKE_APP_ID].char2_uuid.uuid.uuid16 = GATTS_CHAR_ALARM_UUID_SMOKE;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_SMOKE_APP_ID].service_handle);

        ESP_LOGI(GATTS_TAG, "add_smoke_char_ret");
        smoke_smoke_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_smoke_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_SMOKE_APP_ID].service_handle, &gl_profile_tab[PROFILE_SMOKE_APP_ID].char1_uuid,
                                                              ESP_GATT_PERM_READ,
                                                              smoke_smoke_property,
                                                              &gatts_smoke_smoke_val, NULL);
        ESP_LOGI(GATTS_TAG, "add_smoke_char_ret---> END");
        if (add_smoke_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_smoke_char_ret);
        }

        ESP_LOGI(GATTS_TAG, "add_alarm_char_ret");
        smoke_alarm_property = ESP_GATT_CHAR_PROP_BIT_READ;
        esp_err_t add_alarm_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_SMOKE_APP_ID].service_handle, &gl_profile_tab[PROFILE_SMOKE_APP_ID].char2_uuid,
                                                              ESP_GATT_PERM_READ,
                                                              smoke_alarm_property,
                                                              &gatts_smoke_alarm_val, NULL);
        ESP_LOGI(GATTS_TAG, "add_alarm_char_ret---> END");
        if (add_alarm_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_alarm_char_ret);
        }

        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        ESP_LOGI(GATTS_TAG, "esp gatts smoke add char event\r");
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        if (param->add_char.char_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_SMOKE_APP_ID].char1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_SMOKE_APP_ID].char1_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_SMOKE_APP_ID].descr1_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_SMOKE_APP_ID].descr1_uuid.uuid.uuid16 = GATTS_DESCR_SOMKE_UUID_SMOKE;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL)
            {
                ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_SMOKE_APP_ID].service_handle, &gl_profile_tab[PROFILE_SMOKE_APP_ID].descr1_uuid,
                                                                   ESP_GATT_PERM_READ, NULL, NULL);
            if (add_descr_ret)
            {
                ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        else if (param->add_char.char_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_SMOKE_APP_ID].char2_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_SMOKE_APP_ID].char2_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_SMOKE_APP_ID].descr2_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_SMOKE_APP_ID].descr2_uuid.uuid.uuid16 = GATTS_DESCR_ALARM_UUID_SMOKE;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL)
            {
                ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_SMOKE_APP_ID].service_handle, &gl_profile_tab[PROFILE_SMOKE_APP_ID].descr2_uuid,
                                                                   ESP_GATT_PERM_READ, NULL, NULL);
            if (add_descr_ret)
            {
                ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        else
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", param->add_char.status);
        }

        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        if (param->add_char_descr.descr_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_SMOKE_APP_ID].descr1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_SMOKE_APP_ID].descr1_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
            break;
        }
        else if (param->add_char_descr.descr_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_SMOKE_APP_ID].descr2_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_SMOKE_APP_ID]
                .descr2_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
            break;
        }
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_SMOKE_APP_ID].conn_id = param->connect.conn_id;
        // start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_profile_dth11_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_DTH11_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_DTH11_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_DTH11_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_DTH11_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_DTH11;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DTH11_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        // config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret)
        {
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x ", ret);
        }
        adv_config_done |= adv_config_flag;
        // config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret)
        {
            ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_DTH11_APP_ID].service_id, GATTS_NUM_HANDLE_DTH11);
        break;

    case ESP_GATTS_READ_EVT:
        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, connection_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        if (param->read.handle == gl_profile_tab[PROFILE_DTH11_APP_ID].char1_handle)
        {
            dth11_temperature_t dth11_temperature_value;
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = gatts_dth11_temperature_val.attr_len;
            dth11_temperature_value = (dth11_temperature_t)DHT11_ReadTemp();
            memcpy(gatts_dth11_temperature_val.attr_value, &dth11_temperature_value, gatts_dth11_temperature_val.attr_len);
            memcpy(rsp.attr_value.value, gatts_dth11_temperature_val.attr_value, gatts_dth11_temperature_val.attr_len);
        }
        else if (param->read.handle == gl_profile_tab[PROFILE_DTH11_APP_ID].char2_handle)
        {
            dth11_humidity_t dth11_humidity_value;
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = gatts_dth11_humidity_val.attr_len;
            dth11_humidity_value = (dth11_humidity_t)DHT11_ReadHumi();
            memcpy(gatts_dth11_humidity_val.attr_value, &dth11_humidity_value, gatts_dth11_humidity_val.attr_len);
            memcpy(rsp.attr_value.value, gatts_dth11_humidity_val.attr_value, gatts_dth11_humidity_val.attr_len);
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, read error handle %d\n\r", param->read.handle);
        }
        // the following read operation will be filled in later

        // rsp.attr_value.handle = param->read.handle;
        // rsp.attr_value.len = 4;
        // rsp.attr_value.value[0] = 0xde;
        // rsp.attr_value.value[1] = 0xed;
        // rsp.attr_value.value[2] = 0xbe;
        // rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        // if (!param->write.is_prep)
        // {
        //     ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        //     esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //     if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
        //     {
        //         uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
        //         if (descr_value == 0x0001)
        //         {
        //             if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "notify enable");
        //                 uint8_t notify_data[15];
        //                 for (int i = 0; i < sizeof(notify_data); ++i)
        //                 {
        //                     notify_data[i] = i % 0xff;
        //                 }
        //                 // the size of notify_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
        //                                             sizeof(notify_data), notify_data, false);
        //             }
        //         }
        //         else if (descr_value == 0x0002)
        //         {
        //             if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "indicate enable");
        //                 uint8_t indicate_data[15];
        //                 for (int i = 0; i < sizeof(indicate_data); ++i)
        //                 {
        //                     indicate_data[i] = i % 0xff;
        //                 }
        //                 // the size of indicate_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
        //                                             sizeof(indicate_data), indicate_data, true);
        //             }
        //         }
        //         else if (descr_value == 0x0000)
        //         {
        //             ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
        //         }
        //         else
        //         {
        //             ESP_LOGE(GATTS_TAG, "unknown descr value");
        //             esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //         }
        //     }
        // }
        write_event_env(gatts_if, &dth11_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        exec_write_event_env(&dth11_prepare_write_env, param);
        break;

    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_DTH11_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_DTH11_APP_ID].char1_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_DTH11_APP_ID].char1_uuid.uuid.uuid16 = GATTS_CHAR_TEMPERATURE_UUID_DTH11;
        gl_profile_tab[PROFILE_DTH11_APP_ID].char2_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_DTH11_APP_ID].char2_uuid.uuid.uuid16 = GATTS_CHAR_HUMIDITY_UUID_DTH11;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_DTH11_APP_ID].service_handle);

        ESP_LOGI(GATTS_TAG, "add_temperature_char_ret");
        dth11_temperature_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_temperature_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_DTH11_APP_ID].service_handle, &gl_profile_tab[PROFILE_DTH11_APP_ID].char1_uuid,
                                                                    ESP_GATT_PERM_READ,
                                                                    dth11_temperature_property,
                                                                    &gatts_dth11_temperature_val, NULL);
        ESP_LOGI(GATTS_TAG, "add_temperature_char_ret--->END");
        if (add_temperature_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_temperature_char_ret);
        }

        ESP_LOGI(GATTS_TAG, "add_humidity_char_ret");
        dth11_humidity_property = ESP_GATT_CHAR_PROP_BIT_READ;
        esp_err_t add_humidity_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_DTH11_APP_ID].service_handle, &gl_profile_tab[PROFILE_DTH11_APP_ID].char2_uuid,
                                                                 ESP_GATT_PERM_READ,
                                                                 dth11_humidity_property,
                                                                 &gatts_dth11_humidity_val, NULL);
        ESP_LOGI(GATTS_TAG, "add_humidity_char_ret--->END");
        if (add_humidity_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_humidity_char_ret);
        }

        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        ESP_LOGI(GATTS_TAG, "esp gatts dth11 add char event\r");
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        if (param->add_char.char_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_DTH11_APP_ID].char1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_DTH11_APP_ID].char1_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_DTH11_APP_ID].descr1_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_DTH11_APP_ID].descr1_uuid.uuid.uuid16 = GATTS_DESCR_TEMPERATURE_UUID_DTH11;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL)
            {
                ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_DTH11_APP_ID].service_handle, &gl_profile_tab[PROFILE_DTH11_APP_ID].descr1_uuid,
                                                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
            if (add_descr_ret)
            {
                ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        else if (param->add_char.char_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_LED_APP_ID].char2_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_DTH11_APP_ID].char2_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_DTH11_APP_ID].descr2_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_DTH11_APP_ID].descr2_uuid.uuid.uuid16 = GATTS_DESCR_HUMIDITY_UUID_DTH11;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL)
            {
                ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_DTH11_APP_ID].service_handle, &gl_profile_tab[PROFILE_DTH11_APP_ID].descr2_uuid,
                                                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
            if (add_descr_ret)
            {
                ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        else
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", param->add_char.status);
        }

        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        if (param->add_char_descr.descr_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_DTH11_APP_ID].descr1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_DTH11_APP_ID].descr1_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
            break;
        }
        else if (param->add_char_descr.descr_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_DTH11_APP_ID].descr2_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_DTH11_APP_ID]
                .descr2_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
            break;
        }
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_DTH11_APP_ID].conn_id = param->connect.conn_id;
        // start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_profile_bh1750_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_BH1750_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_BH1750_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_BH1750_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_BH1750_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_BH1750;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(BH1750_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        // // config adv data
        // esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        // if (ret)
        // {
        //     ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x ", ret);
        // }
        // adv_config_done |= adv_config_flag;
        // // config scan response data
        // ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        // if (ret)
        // {
        //     ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        // }
        // adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_BH1750_APP_ID].service_id, GATTS_NUM_HANDLE_BH1750);
        break;

    case ESP_GATTS_READ_EVT:
        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, connection_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));

        if (param->read.handle == gl_profile_tab[PROFILE_BH1750_APP_ID].char1_handle)
        {
            bh1750_lux_t bh1750_lux;
            rsp.attr_value.handle = param->read.handle;
            rsp.attr_value.len = gatts_bh1750_lux_val.attr_len;
            // syhcronize the value of the attribute with the device info
            bh1750_lux = (bh1750_lux_t)get_bh1750_lux();
            ESP_LOGI(DEBUG_TAG, "bh1750_lux = %f", bh1750_lux);
            memcpy(gatts_bh1750_lux_val.attr_value, &bh1750_lux, gatts_bh1750_lux_val.attr_len);
            memcpy(rsp.attr_value.value, gatts_bh1750_lux_val.attr_value, gatts_bh1750_lux_val.attr_len);
            ESP_LOGI(DEBUG_TAG, "rsp.attr_value.len=%d, rsp.attr_value.value=%d\r\n", rsp.attr_value.len, rsp.attr_value.value[0]);
        }
        // the following read operation will be filled in later
        // rsp.attr_value.handle = param->read.handle;
        // rsp.attr_value.len = 4;
        // rsp.attr_value.value[0] = 0xde;
        // rsp.attr_value.value[1] = 0xed;
        // rsp.attr_value.value[2] = 0xbe;
        // rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        // if (!param->write.is_prep)
        // {
        //     ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        //     esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //     if (gl_profile_tab[PROFILE_LED_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
        //     {
        //         uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
        //         if (descr_value == 0x0001)
        //         {
        //             if (led_power_status_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "notify enable");
        //                 uint8_t notify_data[15];
        //                 for (int i = 0; i < sizeof(notify_data); ++i)
        //                 {
        //                     notify_data[i] = i % 0xff;
        //                 }
        //                 // the size of notify_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_LED_APP_ID].char_handle,
        //                                             sizeof(notify_data), notify_data, false);
        //             }
        //         }
        //         else if (descr_value == 0x0002)
        //         {
        //             if (led_pin_number_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
        //             {
        //                 ESP_LOGI(GATTS_TAG, "indicate enable");
        //                 uint8_t indicate_data[15];
        //                 for (int i = 0; i < sizeof(indicate_data); ++i)
        //                 {
        //                     indicate_data[i] = i % 0xff;
        //                 }
        //                 // the size of indicate_data[] need less than MTU size
        //                 esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_LED_APP_ID].char_handle,
        //                                             sizeof(indicate_data), indicate_data, true);
        //             }
        //         }
        //         else if (descr_value == 0x0000)
        //         {
        //             ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
        //         }
        //         else
        //         {
        //             ESP_LOGE(GATTS_TAG, "unknown descr value");
        //             esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //         }
        //     }
        // }
        write_event_env(gatts_if, &bh1750_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        exec_write_event_env(&bh1750_prepare_write_env, param);
        break;

    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_BH1750_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_BH1750_APP_ID].char1_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_BH1750_APP_ID].char1_uuid.uuid.uuid16 = GATTS_CHAR_LUX_UUID_BH1750;
        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_BH1750_APP_ID].service_handle);

        ESP_LOGI(GATTS_TAG, "add_power_status_char_ret");
        bh1750_lux_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_power_status_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_BH1750_APP_ID].service_handle, &gl_profile_tab[PROFILE_BH1750_APP_ID].char1_uuid,
                                                                     ESP_GATT_PERM_READ,
                                                                     bh1750_lux_property,
                                                                     &gatts_bh1750_lux_val, NULL);
        ESP_LOGI(GATTS_TAG, "add_power_status_char_ret----> END");
        if (add_power_status_char_ret)
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", add_power_status_char_ret);
        }

        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {

        ESP_LOGI(GATTS_TAG, "esp gatts bh1750 add char event\r");
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        if (param->add_char.char_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_BH1750_APP_ID].char1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_BH1750_APP_ID].char1_handle = param->add_char.attr_handle;
            gl_profile_tab[PROFILE_BH1750_APP_ID].descr1_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab[PROFILE_BH1750_APP_ID].descr1_uuid.uuid.uuid16 = GATTS_DESCR_LUX_UUID_BH1750;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle, &length, &prf_char);
            if (get_attr_ret == ESP_FAIL)
            {
                ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
            }

            ESP_LOGI(GATTS_TAG, "the gatts demo char length = %x\n", length);
            for (int i = 0; i < length; i++)
            {
                ESP_LOGI(GATTS_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_BH1750_APP_ID].service_handle, &gl_profile_tab[PROFILE_BH1750_APP_ID].descr1_uuid,
                                                                   ESP_GATT_PERM_READ, NULL, NULL);
            if (add_descr_ret)
            {
                ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
            }
            break;
        }
        else
        {
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x", param->add_char.status);
        }

        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        if (param->add_char_descr.descr_uuid.uuid.uuid16 == gl_profile_tab[PROFILE_BH1750_APP_ID].descr1_uuid.uuid.uuid16)
        {
            gl_profile_tab[PROFILE_BH1750_APP_ID].descr1_handle = param->add_char_descr.attr_handle;
            ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                     param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
            break;
        }
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_BH1750_APP_ID].conn_id = param->connect.conn_id;
        // start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == gl_profile_tab[idx].gatts_if)
            {
                if (gl_profile_tab[idx].gatts_cb)
                {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void gatts_server_task_handler(void *pvParameters)
{
    ESP_LOGI(GATTS_TAG, "gatts_server_task_handler");

    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize the led, smoke and dth11 profile characteristics
    led_power_status_value_value = malloc(sizeof(led_power_status_t));
    led_pin_number_value_value = malloc(sizeof(led_pin_number_t));
    dth11_humidity_value_value = malloc(sizeof(dth11_humidity_t));
    dth11_temperature_value_value = malloc(sizeof(dth11_temperature_t));
    smoke_smoke_value_value = malloc(sizeof(smoke_smoke_t));
    smoke_alarm_value_value = malloc(sizeof(smoke_alarm_t));
    bh1750_lux_value_value = malloc(sizeof(bh1750_lux_t));
    gatts_led_power_status_val.attr_value = led_power_status_value_value;
    gatts_led_pin_number_val.attr_value = led_pin_number_value_value;
    gatts_dth11_humidity_val.attr_value = dth11_humidity_value_value;
    gatts_dth11_temperature_val.attr_value = dth11_temperature_value_value;
    gatts_smoke_smoke_val.attr_value = smoke_smoke_value_value;
    gatts_smoke_alarm_val.attr_value = smoke_alarm_value_value;
    gatts_bh1750_lux_val.attr_value = bh1750_lux_value_value;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_LED_APP_ID);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_SMOKE_APP_ID);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_DTH11_APP_ID);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
    {
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_BT);
    ESP_LOG_BUFFER_HEX(GATTS_TAG, mac, 6);

    vTaskDelete(NULL);
}