/**
 * @file MQ2.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "mq2_task.h"

/**
 * CONNECTION:
 *
 * ----------------------------------------------------------
 *              MQ2 Sensor                ESP32
 * ----------------------------------------------------------
 *                  Vin                    3.3
 *                  GND                    GND
 *                Output Pin               36 (Channel 0)
 * -----------------------------------------------------------
 */

/* GPIO 36 For channel 0 --> gas sensor analog pin */
static const adc_channel_t channel = ADC1_CHANNEL_0;

/* ADC capture width is 10 (Range 0-1023) */
static const adc_bits_width_t width = ADC_WIDTH_BIT_10;

/* No input attenumation, ADC can measure up to approx. 800 mV. */
static const adc_atten_t atten = ADC_ATTEN_DB_0;

static esp_adc_cal_characteristics_t *adc_chars;

/*****************************Globals***********************************************/
float LPGCurve[3] = {2.3, 0.21, -0.47};   // two points are taken from the curve.
                                          // with these two points, a line is formed which is "approximately equivalent"
                                          // to the original curve.
                                          // data format:{ x, y, slope}; point1: (lg200, 0.21), point2: (lg10000, -0.59)
float COCurve[3] = {2.3, 0.72, -0.34};    // two points are taken from the curve.
                                          // with these two points, a line is formed which is "approximately equivalent"
                                          // to the original curve.
                                          // data format:{ x, y, slope}; point1: (lg200, 0.72), point2: (lg10000,  0.15)
float SmokeCurve[3] = {2.3, 0.53, -0.44}; // two points are taken from the curve.
                                          // with these two points, a line is formed which is "approximately equivalent"
                                          // to the original curve.
                                          // data format:{ x, y, slope}; point1: (lg200, 0.53), point2: (lg10000,  -0.22)
float Ro = 10;                            // Ro is initialized to 10 kilo ohms
float lpg = 0;
float co = 0;
float smoke = 0;
int lastReadTime = 0;

typedef struct
{
    float lpg;
    float co;
    float smoke;
    int alarm;
    char *alarm_type;

    SemaphoreHandle_t mutex;
} mq2_value_t;

static mq2_value_t mq2_value;

void begin()
{
    Ro = MQCalibration();
    printf("Ro: %.2lf Kohm\n", Ro);

    // initialize the mq2 value
    mq2_value.lpg = 0;
    mq2_value.co = 0;
    mq2_value.smoke = 0;
    mq2_value.alarm = 0;
    mq2_value.alarm_type = NULL;
    mq2_value.mutex = xSemaphoreCreateMutex();
    return;
}

/***************************** MQCalibration ****************************************
Input:   mq_pin - analog channel
Output:  Ro of the sensor
Remarks: This function assumes that the sensor is in clean air. It use
         MQResistanceCalculation to calculates the sensor resistance in clean air
         and then divides it with RO_CLEAN_AIR_FACTOR. RO_CLEAN_AIR_FACTOR is about
         10, which differs slightly between different sensors.
************************************************************************************/

float MQCalibration()
{

    float val = 0;
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
    for (int i = 0; i < CALIBARAION_SAMPLE_TIMES; i++)
    {
        /* take multiple samples */
        int caliread = adc1_get_raw(channel);
        val += MQResistanceCalculation(caliread);
        printf("Sample: %d : Calibration ADC :%d: MQResistanceCalculation: %.4f:\n", i, caliread, val);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    val = val / CALIBARAION_SAMPLE_TIMES; // calculate the average value

    val = val / RO_CLEAN_AIR_FACTOR; // divided by RO_CLEAN_AIR_FACTOR yields the Ro
                                     // according to the chart in the datasheet

    printf("MQ2 Calibration value: %f\n", val);

    return val;
}

/****************** MQResistanceCalculation ****************************************
Input:   raw_adc - raw value read from adc, which represents the voltage
Output:  the calculated sensor resistance
Remarks: The sensor and the load resistor forms a voltage divider. Given the voltage
         across the load resistor and its resistance, the resistance of the sensor
         could be derived.
************************************************************************************/
float MQResistanceCalculation(int raw_adc)
{
    return (((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

/****************** read infinite times ****************************************/
float *read(bool print)
{

    lpg = MQGetGasPercentage(MQRead() / Ro, GAS_LPG);
    co = MQGetGasPercentage(MQRead() / Ro, GAS_CO);
    smoke = MQGetGasPercentage(MQRead() / Ro, GAS_SMOKE);

    if (lpg > 1000 || co > 1000 || smoke > 1000)
    {
        printf("[ GAS DETECTTED ]\n");
        printf("LPG::%.2f::ppm CO::%.2f::ppm SMOKE::%.2f::ppm \n", lpg, co, smoke);
    }
    else
    {
        printf("XXX [ GAS NOT DETECTED ] XXX\n");
    }
    lastReadTime = xthal_get_ccount();

    return NULL;
}

/*****************************  MQRead *********************************************
Input:   mq_pin - analog channel
Output:  Rs of the sensor
Remarks: This function use MQResistanceCalculation to caculate the sensor resistenc (Rs).
         The Rs changes as the sensor is in the different consentration of the target
         gas. The sample times and the time interval between samples could be configured
         by changing the definition of the macros.
************************************************************************************/
float MQRead()
{
    int i;
    float rs = 0;
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);

    int val = adc1_get_raw(channel);
    uint32_t voltage = esp_adc_cal_raw_to_voltage(val, adc_chars);

    for (i = 0; i < READ_SAMPLE_TIMES; i++)
    {
        rs += MQResistanceCalculation(val);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    rs = rs / READ_SAMPLE_TIMES;
    return rs;
}

/*****************************  MQGetGasPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         gas_id      - target gas type
Output:  ppm of the target gas
Remarks: This function passes different curves to the MQGetPercentage function which
         calculates the ppm (parts per million) of the target gas.
************************************************************************************/

float MQGetGasPercentage(float rs_ro_ratio, int gas_id)
{
    if (gas_id == GAS_LPG)
    {
        return MQGetPercentage(rs_ro_ratio, LPGCurve);
    }
    else if (gas_id == GAS_CO)
    {
        return MQGetPercentage(rs_ro_ratio, COCurve);
    }
    else if (gas_id == GAS_SMOKE)
    {
        return MQGetPercentage(rs_ro_ratio, SmokeCurve);
    }
    return 0;
}

/*****************************  MQGetPercentage **********************************
Input:   rs_ro_ratio - Rs divided by Ro
         pcurve      - pointer to the curve of the target gas
Output:  ppm of the target gas
Remarks: By using the slope and a point of the line. The x(logarithmic value of ppm)
         of the line could be derived if y(rs_ro_ratio) is provided. As it is a
         logarithmic coordinate, power of 10 is used to convert the result to non-logarithmic
         value.
************************************************************************************/
int MQGetPercentage(float rs_ro_ratio, float *pcurve)
{
    return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

/*****************************  readLPG Value --every 10sec interval **********************************/
float readLPG()
{
    if (xthal_get_ccount() < (lastReadTime + 10000) && lpg != 0)
    {
        return lpg;
    }
    else
    {
        return lpg = MQGetGasPercentage(MQRead() / 10, GAS_LPG);
    }
}

/*****************************  readCO Value -- every 10sec interval **********************************/
float readCO()
{
    if (xthal_get_ccount() < (lastReadTime + 10000) && co != 0)
    {
        return co;
    }
    else
    {
        return co = MQGetGasPercentage(MQRead() / 10, GAS_CO);
    }
}
/*****************************  readSmoke Value -- every 10sec interval **********************************/

float readSmoke()
{
    if (xthal_get_ccount() < (lastReadTime + 10000) && smoke != 0)
    {
        return smoke;
    }
    else
    {
        return smoke = MQGetGasPercentage(MQRead() / 10, GAS_SMOKE);
    }
}

float GetLPG()
{
    float lgp;
    xSemaphoreTake(mq2_value.mutex, 1);
    lgp = mq2_value.lpg;
    xSemaphoreGive(mq2_value.mutex);
    return lpg;
}

float GetCO()
{
    float co;
    xSemaphoreTake(mq2_value.mutex, 1);
    co = mq2_value.co;
    xSemaphoreGive(mq2_value.mutex);
    return co;
}

float GetSmoke()
{
    float smoke;
    xSemaphoreTake(mq2_value.mutex, 1);
    smoke = mq2_value.smoke;
    xSemaphoreGive(mq2_value.mutex);
    return smoke;
}

// the alarm function is not implemented yet
int GetAlarm()
{
    float alarm;
    xSemaphoreTake(mq2_value.mutex, 1);
    alarm = mq2_value.alarm;
    xSemaphoreGive(mq2_value.mutex);
    return alarm;
}

void mq2_task_handler(void *arg)
{
    ESP_LOGI(MQ2_TAG, "MQ2 Task Started");
    begin();
    while (1)
    {
        float *values;
        values = read(true);

        // update mq2 values
        xSemaphoreTake(mq2_value.mutex, 1);
        mq2_value.lpg = lpg;
        mq2_value.co = co;
        mq2_value.smoke = smoke;
        if (mq2_value.lpg > 100)
        {
            mq2_value.alarm = 1;
            mq2_value.alarm_type = "lpg";
        }
        else if (mq2_value.co > 100)
        {
            mq2_value.alarm = 1;
            mq2_value.alarm_type = "co";
        }
        else if (mq2_value.smoke > 100)
        {
            mq2_value.alarm = 1;
            mq2_value.alarm_type = "smoke";
        }
        else
        {
            mq2_value.alarm = 0;
        }
        // Should notify the alarm signal to user, if the alram is enabled. But
        // the function is not implemented yet.
        xSemaphoreGive(mq2_value.mutex);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// void app_main()
// {
//     ESP_LOGI(MQ2_TAG, "Starting LED Task\n\r");
//     xTaskCreate(mq2_task_handler, "mq2_task_entry", 4096, NULL, 5, &mq2_handler);
// }