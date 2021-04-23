/* Copyright (c) 2017 pcbreflux. All Rights Reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
// #include "bt.h"
// #include "bta_api.h"
#include "nmea_parser.h"
#include "driver/gpio.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"

#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#include "ble_uart_server.h"

#include "nvs_flash.h"

#define GATTS_TAG "gps_demo"
#define CONFIG_LED_PIN 33

#define TIME_ZONE (+7)   //Beijing Time
#define YEAR_BASE (2000) //date in GPS starts from 2000

static const char *TAG = "gps_demo";

Inform inform;

// void led_status_task(void *arg)
// {
//     bool led_status = false;
//     while(1) {
//         int gps_updated;
//         if(xQueueReceive(queue_gps_status, &gps_updated, (TickType_t)5 )) {
//             led_status = !led_status;
//             gpio_set_level(CONFIG_LED_PIN, led_status);
//             vTaskDelay(pdMS_TO_TICKS(1000));
//             led_status = !led_status;
//             gpio_set_level(CONFIG_LED_PIN, led_status);
//             // printf("GPS_UPDATE!!!\n");
//         }
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

/**
 * @brief GPS Event Handler
 *
 * @param event_handler_arg handler specific arguments
 * @param event_base event base, here is fixed to ESP_NMEA_EVENT
 * @param event_id event id
 * @param event_data event specific arguments
 */
static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    gps_t *gps = NULL;
    switch (event_id) {
    case GPS_UPDATE:
        gps = (gps_t *)event_data;
        /* print information parsed from GPS statements */
        ESP_LOGI(TAG, "%d/%d/%d %d:%d:%d => \r\n"
                 "\t\t\t\t\t\tlatitude   = %.05f°N\r\n"
                 "\t\t\t\t\t\tlongitude = %.05f°E\r\n"
                 "\t\t\t\t\t\taltitude   = %.02fm\r\n"
                 "\t\t\t\t\t\tspeed      = %fm/s",
                 gps->date.year + YEAR_BASE, gps->date.month, gps->date.day,
                 gps->tim.hour + TIME_ZONE, gps->tim.minute, gps->tim.second,
                 gps->latitude, gps->longitude, gps->altitude, gps->speed);
                 
        // inform.date_time = (gps->date.year + YEAR_BASE-1970) * 31556926 + gps->date.month * 2629746 + gps->date.day * 86400 + 
        //                     (gps->tim.hour + TIME_ZONE)* 3600 + gps->tim.minute * 60 + gps->tim.second;
        struct tm tm;
        time_t seconds;
        tm.tm_year = gps->date.year + YEAR_BASE;
        tm.tm_mon = gps->date.month;
        tm.tm_mday = gps->date.day;
        tm.tm_hour = gps->tim.hour + TIME_ZONE;
        tm.tm_min = gps->tim.minute;
        tm.tm_sec = gps->tim.second;
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
        tm.tm_isdst = 0;
        seconds = mktime(&tm);

        inform.date_time = seconds;
        inform.latitude = gps->latitude;
        inform.longitude = gps->longitude;
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}

void app_main() {
    esp_err_t ret;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed\n", __func__);
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed\n", __func__);
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
        return;
    }

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(BLE_PROFILE_APP_ID);

    gpio_pad_select_gpio(CONFIG_LED_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(CONFIG_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_LED_PIN, 0);    

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    /* NMEA parser configuration */
    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    /* init NMEA parser library */
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    /* register event handler for NMEA parser library */
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

    // vTaskDelay(10000 / portTICK_PERIOD_MS);
    // /* unregister event handler */
    // nmea_parser_remove_handler(nmea_hdl, gps_event_handler);
    // /* deinit NMEA parser library */
    // nmea_parser_deinit(nmea_hdl);

    /* 
    BaseType_t xTaskCreate(    TaskFunction_t pvTaskCode,
                            const char * const pcName,
                            configSTACK_DEPTH_TYPE usStackDepth,
                            void *pvParameters,
                            UBaseType_t uxPriority,
                            TaskHandle_t *pxCreatedTask
                          );
    */

    // xTaskCreate( led_status_task, "button_task", 1024, NULL , 10, NULL);
    gpio_set_level(CONFIG_LED_PIN, 1);
    return;
}
