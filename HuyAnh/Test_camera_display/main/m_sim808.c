#include "m_sim808.h"
#include "esp_log.h"

static const char *TAG = "uart_events";

gnss_t m_gnss;

char date_time[50];
char sim_808_recv_data[SIM808_BUF_SIZE];
static uint8_t cur_pos = 0;

static char data[SIM808_BUF_SIZE] = {0};
static char temp_position[SIM808_BUF_SIZE] = {0};
static uint8_t state = 0;

static char get_next_char() {
    char ch =  sim_808_recv_data[cur_pos];
    cur_pos ++;
    return ch;
}

static esp_err_t parse_GPS_data(uint8_t len){
    char ch;
    uint8_t temp_position_pos = 0;

    memcpy((uint8_t *)date_time, (uint8_t *)&sim_808_recv_data[27], 40);
    cur_pos = 45;

    while (get_next_char() != ',');

    //get lat
    do {
        ch = get_next_char();
        if(ch == ',') {
            m_gnss.lat = strtof(temp_position, NULL);
            temp_position_pos = 0;
            memset(temp_position, 0, SIM808_BUF_SIZE);
            break;
        }
            
        temp_position[temp_position_pos] = ch;
        temp_position_pos ++;

    } while (1);

    //get lon
    do {
        ch = get_next_char();
        if(ch == ',') {
            m_gnss.lon = strtof(temp_position, NULL);
            temp_position_pos = 0;
            memset(temp_position, 0, SIM808_BUF_SIZE);
            break;
        }
            
        temp_position[temp_position_pos] = ch;
        temp_position_pos ++;

    } while (1);

    
    return ESP_OK;
    
}

void get_gps() {

    //send AT command
    // sprintf(data, "AT\r\n");
    // uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
    // vTaskDelay(1000/portTICK_PERIOD_MS);
    // sprintf(data, "AT\r\n");
    // uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));

    // //turn on GPS
    // vTaskDelay(1000/portTICK_PERIOD_MS);
    // sprintf(data, "AT+CGNSPWR=1\r\n");
    // uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));

    //save string command to read GPS data
    sprintf(data, "AT+CGNSINF\r\n");

    while (1)
    {
        sprintf(data, "AT\r\n");
        uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
        uart_read_bytes(UART_NUM_0, (uint8_t *)sim_808_recv_data, SIM808_BUF_SIZE, 20 / portTICK_RATE_MS);
        vTaskDelay(100/portTICK_PERIOD_MS);
        sprintf(data, "AT\r\n");
        uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
        uart_read_bytes(UART_NUM_0, (uint8_t *)sim_808_recv_data, SIM808_BUF_SIZE, 20 / portTICK_RATE_MS);
        vTaskDelay(100/portTICK_PERIOD_MS);
        sprintf(data, "AT\r\n");
        uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
        uart_read_bytes(UART_NUM_0, (uint8_t *)sim_808_recv_data, SIM808_BUF_SIZE, 20 / portTICK_RATE_MS);
        vTaskDelay(100/portTICK_PERIOD_MS);
        sprintf(data, "AT\r\n");
        uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
        uart_read_bytes(UART_NUM_0, (uint8_t *)sim_808_recv_data, SIM808_BUF_SIZE, 20 / portTICK_RATE_MS);
        vTaskDelay(100/portTICK_PERIOD_MS);
        sprintf(data, "AT+CGNSPWR=1\r\n");
        uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
        uart_read_bytes(UART_NUM_0, (uint8_t *)sim_808_recv_data, SIM808_BUF_SIZE, 20 / portTICK_RATE_MS);

        // uart_flush(UART_NUM_0);
        // Write data to the UART
        sprintf(data, "AT+CGNSINF\r\n");
        uart_write_bytes(UART_NUM_0, (const char *) data, strlen(data));
        // ESP_LOGI(TAG,"send: %s", data);
        
        // Read data from the UART
        int len = uart_read_bytes(UART_NUM_0, (uint8_t *)sim_808_recv_data, SIM808_BUF_SIZE, 20 / portTICK_RATE_MS);
        // ESP_LOGI(TAG,"recv: %s", sim_808_recv_data);

        if(len)
		    parse_GPS_data(len);

        vTaskDelay(4000/portTICK_PERIOD_MS);
    }
    
}

void sim_init() {
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_0, SIM808_BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
    
    xTaskCreate(get_gps, "get_gps_task", 2048, NULL, 10, NULL);
}