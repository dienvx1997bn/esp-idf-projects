/* Ethernet iperf example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_fat.h"
#include "cmd_system.h"
#include "cmd_ethernet.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_ibeacon_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "cJSON.h"


#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
// #include "protocol_examples_common.h"
#include "esp_tls.h"

#include "esp_http_client.h"
#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 512

static const char* TAG = "IBEACON_DEMO";
extern esp_ble_ibeacon_vendor_t vendor_config;

extern volatile bool m_eth_ready;
QueueHandle_t queue1;

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "example.com"
#define WEB_PORT "80"
#define WEB_PATH "/"

#define DEVICE_SESSION 2

///Declare static functions
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void http_native_request(char * post_data);

#if (IBEACON_MODE == IBEACON_RECEIVER)
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

#elif (IBEACON_MODE == IBEACON_SENDER)
static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = 0x640,
    .adv_int_max        = 0x640,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
#endif


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
#if (IBEACON_MODE == IBEACON_SENDER)
        esp_ble_gap_start_advertising(&ble_adv_params);
#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
#if (IBEACON_MODE == IBEACON_RECEIVER)
        //the unit of the duration is second, 0 means scan permanently
        uint32_t duration = 0;
        esp_ble_gap_start_scanning(duration);
#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //adv start complete event to indicate adv start successfully or failed
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Adv start failed: %s", esp_err_to_name(err));
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            /* Search for BLE iBeacon Packet */
            if (esp_ble_is_ibeacon_packet(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len)){
                esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t*)(scan_result->scan_rst.ble_adv);
                ESP_LOGI(TAG, "----------iBeacon Found----------");
                esp_log_buffer_hex("IBEACON_DEMO: Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN );
                esp_log_buffer_hex("IBEACON_DEMO: Proximity UUID:", ibeacon_data->ibeacon_vendor.proximity_uuid, ESP_UUID_LEN_128);

                uint16_t major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
                uint16_t minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
                ESP_LOGI(TAG, "Major: 0x%04x (%d)", major, major);
                ESP_LOGI(TAG, "Minor: 0x%04x (%d)", minor, minor);
                ESP_LOGI(TAG, "Measured power (RSSI at a 1m distance):%d dbm", ibeacon_data->ibeacon_vendor.measured_power);
                ESP_LOGI(TAG, "RSSI of packet:%d dbm", scan_result->scan_rst.rssi);

                //create queue to save beacon data
                cJSON *root;
                root=cJSON_CreateObject();
                // cJSON *fmt;
                // cJSON_AddItemToObject(root, "id", fmt=cJSON_CreateIntArray((const int *)scan_result->scan_rst.bda, ESP_BD_ADDR_LEN));
                char buf[ESP_BD_ADDR_LEN*3];
                sprintf(buf, "%02X %02X %02X %02X %02X %02X", scan_result->scan_rst.bda[0], scan_result->scan_rst.bda[1], scan_result->scan_rst.bda[2], 
                                                        scan_result->scan_rst.bda[3], scan_result->scan_rst.bda[4], scan_result->scan_rst.bda[5]); 
                cJSON_AddNumberToObject(root, "RSSI", scan_result->scan_rst.rssi);
                cJSON_AddNumberToObject(root, "idSession", DEVICE_SESSION);
                cJSON_AddStringToObject(root, "id", buf);
                char *post_data = cJSON_Print(root);
                // char *post_data = "{ \"id\" : 5632, \"RSSI\" : 0.999, \"idSession\" : 767 }";
                xQueueSendFromISR(queue1, (void*)post_data , (TickType_t)0 );

            }
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "Scan stop failed: %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(TAG, "Stop scan successfully");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "Adv stop failed: %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(TAG, "Stop adv successfully");
        }
        break;

    default:
        break;
    }
}



void ble_ibeacon_appRegister(void)
{
    esp_err_t status;

    ESP_LOGI(TAG, "register callback");

    //register the scan callback function to the gap module
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(status));
        return;
    }

}

void ble_ibeacon_init(void)
{
    esp_bluedroid_init();
    esp_bluedroid_enable();
    ble_ibeacon_appRegister();
}

static void m_beacon_init(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    ble_ibeacon_init();

    /* set scan parameters */
    esp_ble_gap_set_scan_params(&ble_scan_params);

}


/*
 *  http_native_request() demonstrates use of low level APIs to connect to a server,
 *  make a http request and read response. Event handler is not used in this case.
 *  Note: This approach should only be used in case use of low level APIs is required.
 *  The easiest way is to use esp_http_perform()
 */
static void http_native_request(char * post_data)
{
    char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};   // Buffer to store response of http request
    esp_http_client_config_t config = {
        .url = "https://location-checking.herokuapp.com",
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err;

    // // GET Request
    // int content_length = 0;
    // esp_http_client_set_method(client, HTTP_METHOD_GET);
    // esp_err_t err = esp_http_client_open(client, 0);
    // if (err != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    // } else {
    //     content_length = esp_http_client_fetch_headers(client);
    //     if (content_length < 0) {
    //         ESP_LOGE(TAG, "HTTP client fetch headers failed");
    //     } else {
    //         int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
    //         if (data_read >= 0) {
    //             ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
    //             esp_http_client_get_status_code(client),
    //             esp_http_client_get_content_length(client));
    //             ESP_LOGI(TAG, "%s", output_buffer);
    //         } else {
    //             ESP_LOGE(TAG, "Failed to read response");
    //         }
    //     }
    // }
    // esp_http_client_close(client);

    if(!m_eth_ready) {
        ESP_LOGI(TAG, "Network did not ready");
        return;
    }

    printf("post_data length %d :\n%s \n",strlen(post_data), post_data);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    err = esp_http_client_open(client, strlen(post_data));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } else {
        int wlen = esp_http_client_write(client, post_data, strlen(post_data));
        if (wlen < 0) {
            ESP_LOGE(TAG, "Write failed");
        }
        int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
        if (data_read >= 0) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, strlen(output_buffer));
            ESP_LOGI(TAG, "%s", output_buffer);
        } else {
            ESP_LOGE(TAG, "Failed to read response");
        }
    }
    esp_http_client_cleanup(client);
    
}


void get_send_beacon(void *arg)
{
    char rxbuff[100];
    queue1= xQueueCreate(5, sizeof(rxbuff));

    while(1) { 
        //if(xQueuePeek(queue1, &(rxbuff) , (TickType_t)5 ))
        if(xQueueReceive(queue1, &(rxbuff) , (TickType_t)5 )) { 
            // printf("got a data from queue!  === \n%s \n",rxbuff);
            http_native_request(rxbuff);
        }
    }
}


void app_main(void)
{
    /* Register commands */
    register_system();
    register_ethernet();

    m_beacon_init();

    printf("DEVICE_SESSION %d\n", DEVICE_SESSION);

    xTaskCreate(&get_send_beacon, "get_send_beacon", 4096, NULL, 10, NULL);

}
