#include "m_sim808.h"

static const char *TAG = "uart_events";
char sim_808_recv_data[SIM808_BUF_SIZE];


static char data[SIM808_BUF_SIZE] = {0};
static uint8_t state = 0;


void get_gps() {
    
    while (1)
    {
        
        // Write data to the UART
        if(data[0] != 0) {
            uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
            // ESP_LOGI(TAG,"send: %s", data);
            
        }

        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, (uint8_t *)sim_808_recv_data, SIM808_BUF_SIZE, 20 / portTICK_RATE_MS);
        // ESP_LOGI(TAG,"recv: %s", sim_808_recv_data);
		
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    
}

void sim_init() {
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_0, SIM808_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    vTaskDelay(1000/portTICK_PERIOD_MS);
    sprintf(data, "AT\r\n");
    uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));

    vTaskDelay(1000/portTICK_PERIOD_MS);
    sprintf(data, "AT+CGNSPWR=1\r\n");
    uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));

    sprintf(data, "AT+CGNSINF\r\n");
    
    xTaskCreate(get_gps, "get_gps_task", 1024, NULL, 10, NULL);
}