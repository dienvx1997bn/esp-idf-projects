#include "esp_camera.h"

//KIT
// #define CAM_PIN_PWDN    -1 //power down is not used
// #define CAM_PIN_RESET   -1 //software reset will be performed
// #define CAM_PIN_XCLK    4
// #define CAM_PIN_SIOD    18
// #define CAM_PIN_SIOC    23

// #define CAM_PIN_D7      36
// #define CAM_PIN_D6      37
// #define CAM_PIN_D5      38
// #define CAM_PIN_D4      39
// #define CAM_PIN_D3      35
// #define CAM_PIN_D2      26
// #define CAM_PIN_D1      13
// #define CAM_PIN_D0      34
// #define CAM_PIN_VSYNC    5
// #define CAM_PIN_HREF    27
// #define CAM_PIN_PCLK    25

//Ai thinker
#define CAM_PIN_PWDN    32 //power down is not used
#define CAM_PIN_RESET   -1 //software reset will be performed
#define CAM_PIN_XCLK    0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27

#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0      5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22


esp_err_t m_camera_init();
esp_err_t m_camera_capture();