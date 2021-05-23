#ifndef CONFIG_H_
#define CONFIG_H_

#define CUSTOM_BOARD    0

#if CUSTOM_BOARD
#define ESP_NOW_MODE_SENDER 1
#else
#define ESP_NOW_MODE_SENDER 0
#endif

#endif