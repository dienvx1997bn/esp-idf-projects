#include "m_espnow.h"

static const char *TAG = "espnow";


/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    // example_espnow_event_t evt;
    // example_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    // if (mac_addr == NULL) {
    //     ESP_LOGE(TAG, "Send cb arg error");
    //     return;
    // }

    // evt.id = EXAMPLE_ESPNOW_SEND_CB;
    // memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    // send_cb->status = status;
    // if (xQueueSend(s_example_espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send send queue fail");
    // }
}

static void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    // example_espnow_event_t evt;
    // example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

    // if (mac_addr == NULL || data == NULL || len <= 0) {
    //     ESP_LOGE(TAG, "Receive cb arg error");
    //     return;
    // }

    // evt.id = EXAMPLE_ESPNOW_RECV_CB;
    // memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    // recv_cb->data = malloc(len);
    // if (recv_cb->data == NULL) {
    //     ESP_LOGE(TAG, "Malloc receive data fail");
    //     return;
    // }
    // memcpy(recv_cb->data, data, len);
    // recv_cb->data_len = len;
    // if (xQueueSend(s_example_espnow_queue, &evt, portMAX_DELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send receive queue fail");
    //     free(recv_cb->data);
    // }
    extern char recv_msg[50];
    memset(recv_msg, 0, sizeof(recv_msg));
    memcpy((uint8_t *)recv_msg, data, len);

    ESP_LOGI(TAG, "get data: %s  len %d", recv_msg, len);
}



esp_err_t example_espnow_init(void)
{
    // example_espnow_send_param_t *send_param;

    // s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    // if (s_example_espnow_queue == NULL) {
    //     ESP_LOGE(TAG, "Create mutex fail");
    //     return ESP_FAIL;
    // }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    

    // /* Initialize sending parameters. */
    // send_param = malloc(sizeof(example_espnow_send_param_t));
    // memset(send_param, 0, sizeof(example_espnow_send_param_t));
    // if (send_param == NULL) {
    //     ESP_LOGE(TAG, "Malloc send parameter fail");
    //     vSemaphoreDelete(s_example_espnow_queue);
    //     esp_now_deinit();
    //     return ESP_FAIL;
    // }
    // send_param->unicast = false;
    // send_param->broadcast = true;
    // send_param->state = 0;
    // send_param->magic = esp_random();
    // send_param->count = CONFIG_ESPNOW_SEND_COUNT;
    // send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
    // send_param->len = CONFIG_ESPNOW_SEND_LEN;
    // send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);
    // if (send_param->buffer == NULL) {
    //     ESP_LOGE(TAG, "Malloc send buffer fail");
    //     free(send_param);
    //     vSemaphoreDelete(s_example_espnow_queue);
    //     esp_now_deinit();
    //     return ESP_FAIL;
    // }
    // memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    // example_espnow_data_prepare(send_param);

    return ESP_OK;
}
