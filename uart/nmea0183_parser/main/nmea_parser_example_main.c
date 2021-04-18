/* NMEA Parser example, that decode data stream from GPS receiver

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nmea_parser.h"
#include "driver/gpio.h"

static const char *TAG = "gps_demo";

#define CONFIG_LED_PIN 33
#define QUEUE_SIZE  10

#define TIME_ZONE (+7)   //Beijing Time
#define YEAR_BASE (2000) //date in GPS starts from 2000

QueueHandle_t queue_gps_status;

void led_status_task(void *arg)
{
    bool led_status = false;
    while(1) {
        int gps_updated;
        if(xQueueReceive(queue_gps_status, &gps_updated, (TickType_t)5 )) {
            led_status = !led_status;
            gpio_set_level(CONFIG_LED_PIN, led_status);
            // printf("GPS_UPDATE!!!\n");
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

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
        xQueueSend(queue_gps_status, &event_id , (TickType_t)0 );
        break;
    case GPS_UNKNOWN:
        /* print unknown statements */
        ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
        break;
    default:
        break;
    }
}

void app_main(void)
{
    gpio_pad_select_gpio(CONFIG_LED_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(CONFIG_LED_PIN, GPIO_MODE_OUTPUT);

    queue_gps_status= xQueueCreate(QUEUE_SIZE, sizeof(int32_t));

    if( queue_gps_status == 0 ) {
        printf("failed to create queue1= %p \n", queue_gps_status); // Failed to create the queue.
        int32_t event_id = GPS_UNKNOWN;
        xQueueSend(queue_gps_status, &event_id , (TickType_t)0 );
    }


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
    xTaskCreate( led_status_task, "button_task", 4096, NULL , 10, NULL);
}