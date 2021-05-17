#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include <driver/i2c.h>
#include <math.h>

#include "st7789.h"
#include "fontx.h"
#include "bmpfile.h"
#include "decode_image.h"

#include "m_camera.h"
#include "ultrasonic.h"


#define PIN_SDA 15
#define PIN_CLK 14
#define I2C_ADDRESS 0x68 // I2C address of MPU6050
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_PWR_MGMT_1   0x6B

#define	INTERVAL		400
#define WAIT	vTaskDelay(INTERVAL)

static const char *TAG = "main";
volatile uint32_t distance;

short accel_x;
short accel_y;
short accel_z;

static void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(__FUNCTION__,"d_name=%s d_ino=%d d_type=%x", pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

void task_mpu6050(void *ignore) {
	// ESP_LOGD(TAG, ">> mpu6050");
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = PIN_SDA;
	conf.scl_io_num = PIN_CLK;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 400000;
	ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

	i2c_cmd_handle_t cmd;
	vTaskDelay(200/portTICK_PERIOD_MS);

	cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
	i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, 1);
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
	i2c_master_write_byte(cmd, MPU6050_PWR_MGMT_1, 1);
	i2c_master_write_byte(cmd, 0, 1);
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);


	uint8_t data[14];

	// short accel_x;
	// short accel_y;
	// short accel_z;

	while(1) {
		// Tell the MPU6050 to position the internal register pointer to register
		// MPU6050_ACCEL_XOUT_H.
		cmd = i2c_cmd_link_create();
		ESP_ERROR_CHECK(i2c_master_start(cmd));
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_WRITE, 1));
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, MPU6050_ACCEL_XOUT_H, 1));
		ESP_ERROR_CHECK(i2c_master_stop(cmd));
		ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
		i2c_cmd_link_delete(cmd);

		cmd = i2c_cmd_link_create();
		ESP_ERROR_CHECK(i2c_master_start(cmd));
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (I2C_ADDRESS << 1) | I2C_MASTER_READ, 1));

		ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data,   0));
		ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+1, 0));
		ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+2, 0));
		ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+3, 0));
		ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+4, 0));
		ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data+5, 1));

		//i2c_master_read(cmd, data, sizeof(data), 1);
		ESP_ERROR_CHECK(i2c_master_stop(cmd));
		ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
		i2c_cmd_link_delete(cmd);

		accel_x = (data[0] << 8) | data[1];
		accel_y = (data[2] << 8) | data[3];
		accel_z = (data[4] << 8) | data[5];
		// ESP_LOGI(TAG, "accel_x: %d, accel_y: %d, accel_z: %d", accel_x, accel_y, accel_z);
		ESP_LOGI(TAG, "accel_x: %.4f, accel_y: %.4f, accel_z: %.4f", accel_x/16384.0, accel_y/16384.0, accel_z/16384.0);
		vTaskDelay(500/portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
} // task_mpu6050

TickType_t JPEGTest(TFT_t * dev, char * file, int width, int height) {
	TickType_t startTick, endTick, diffTick;
	startTick = xTaskGetTickCount();

	lcdSetFontDirection(dev, 0);
	lcdFillScreen(dev, BLACK);


	pixel_s **pixels;
	uint16_t imageWidth;
	uint16_t imageHeight;
	esp_err_t err = decode_image(&pixels, file, width, height, &imageWidth, &imageHeight);
	ESP_LOGI(__FUNCTION__, "decode_image err=%d imageWidth=%d imageHeight=%d", err, imageWidth, imageHeight);
	if (err == ESP_OK) {

		uint16_t _width = width;
		uint16_t _cols = 0;
		if (width > imageWidth) {
			_width = imageWidth;
			_cols = (width - imageWidth) / 2;
		}
		ESP_LOGD(__FUNCTION__, "_width=%d _cols=%d", _width, _cols);

		uint16_t _height = height;
		uint16_t _rows = 0;
		if (height > imageHeight) {
			_height = imageHeight;
			_rows = (height - imageHeight) / 2;
		}
		ESP_LOGD(__FUNCTION__, "_height=%d _rows=%d", _height, _rows);
		uint16_t *colors = (uint16_t*)malloc(sizeof(uint16_t) * _width);

		for(int y = 0; y < _height; y++){
			for(int x = 0;x < _width; x++){
				pixel_s pixel = pixels[y][x];
				colors[x] = rgb565_conv(pixel.red, pixel.green, pixel.blue);
			}
			lcdDrawMultiPixels(dev, _cols, y+_rows, _width, colors);
			vTaskDelay(1);
		}

		free(colors);
		release_image(&pixels, width, height);
		ESP_LOGD(__FUNCTION__, "Finish");
	}

	endTick = xTaskGetTickCount();
	diffTick = endTick - startTick;
	ESP_LOGI(__FUNCTION__, "elapsed time[ms]:%d",diffTick*portTICK_RATE_MS);
	return diffTick;
}

void ST7789(void *pvParameters)
{
	TFT_t dev;
	
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&dev, CONFIG_WIDTH, CONFIG_HEIGHT, CONFIG_OFFSETX, CONFIG_OFFSETY);

#if CONFIG_INVERSION
	ESP_LOGI(TAG, "Enable Display Inversion");
	lcdInversionOn(&dev);
#endif

	char file[32];

	FontxFile fx24G[2];
	InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic

	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx24G, 0, buffer, &fontWidth, &fontHeight);

	bool display_img = false;
	while(1) {
		uint8_t  dst[20] = {0};
		if(display_img == false) {
			// strcpy(file, "/spiffs/esp32.jpeg");
			strcpy(file, "/spiffs/camera.jpg");
			JPEGTest(&dev, file, CONFIG_WIDTH, CONFIG_HEIGHT);
			vTaskDelay(pdMS_TO_TICKS(4000));
			display_img = true;
			// vTaskDelete(NULL);
		} else {
			//Test draw string
			lcdFillScreen(&dev, BLACK);
			lcdSetFontDirection(&dev, 0);

			sprintf((char *)dst, "Distance: %d cm\n", distance);
			lcdDrawString(&dev, fx24G, 0, 20, dst, RED);
			// ESP_LOGI(TAG, "accel_x: %.4f, accel_y: %.4f, accel_z: %.4f", accel_x/16384.0, accel_y/16384.0, accel_z/16384.0);
			sprintf((char *)dst, "accel_x: %.4f\n", accel_x/16384.0);
			lcdDrawString(&dev, fx24G, 0, 50, dst, RED);
			sprintf((char *)dst, "accel_y: %.4f \n", accel_y/16384.0);
			lcdDrawString(&dev, fx24G, 0, 80, dst, RED);
			sprintf((char *)dst, "accel_z: %.4f\n", accel_z/16384.0);
			lcdDrawString(&dev, fx24G, 0, 110, dst, RED);
		}
		vTaskDelay(pdMS_TO_TICKS(1200));
	} // end while

}


void ultrasonic_test(void *pvParameters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = ULTRASONIC_TRIGGER_GPIO,
        .echo_pin = ULTRASONIC_ECHO_GPIO
    };

	// uint32_t distance;
	ultrasonic_init(&sensor);

    while (true)
    {
		esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK)
        {
            printf("Error %d: ", res);
            switch (res)
            {
                case ESP_ERR_ULTRASONIC_PING:
                    printf("Cannot ping (device is in invalid state)\n");
                    break;
                case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                    printf("Ping timeout (no device found)\n");
                    break;
                case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                    printf("Echo timeout (i.e. distance too big)\n");
                    break;
                default:
                    printf("%s\n", esp_err_to_name(res));
            }
			// esp_restart();
			ultrasonic_init(&sensor);
        }
        else {
			static uint16_t time = 0;
			printf("Time %d -Distance: %d cm\n", time, distance);
			time++;
		}

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void app_main(void)
{
	esp_err_t ret;

	gpio_pad_select_gpio(33);
	gpio_set_direction(33, GPIO_MODE_OUTPUT);
	gpio_set_level(33, 0);
	vTaskDelay(200/portTICK_PERIOD_MS);
	gpio_set_level(33, 1);
	vTaskDelay(200/portTICK_PERIOD_MS);
	gpio_set_level(33, 0);
	vTaskDelay(200/portTICK_PERIOD_MS);
	gpio_set_level(33, 1);
	vTaskDelay(200/portTICK_PERIOD_MS);

	ESP_LOGI(TAG, "Initializing SPIFFS");

	esp_vfs_spiffs_conf_t conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 10,
		.format_if_mount_failed =true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total,&used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}
	SPIFFS_Directory("/spiffs/");

	m_camera_init();
	ret = m_camera_capture();
	if (ret != ESP_OK) {
        ESP_LOGE(TAG, "take photo fail");
    }

	
	xTaskCreate(ST7789, "ST7789", configMINIMAL_STACK_SIZE * 10, NULL, 6, NULL);
	xTaskCreate(ultrasonic_test, "ultrasonic_test", configMINIMAL_STACK_SIZE * 5, NULL, 6, NULL);
	xTaskCreate(task_mpu6050, "task_mpu6050", configMINIMAL_STACK_SIZE * 8, NULL, 2, NULL);

	while (1)
	{
		/* code */
		vTaskDelay(INTERVAL);
	}	//never reach
	
}
