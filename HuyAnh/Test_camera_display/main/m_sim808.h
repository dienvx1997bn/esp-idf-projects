#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

#define SIM808_BUF_SIZE (250)

typedef struct
{
	//struct tm  m_time;
	uint32_t m_timestamp;
	float lat;
	float lon;
}gnss_t;

void sim_init();