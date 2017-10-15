#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <esp_log.h>
#include "ADXL345.h"
#include "driver/i2c.h"

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

// this is the task in which the accelration measurements are performed
void vAccelerometerTask(void *pvParameters)
{
	TickType_t tElapsedTicks;
	TickType_t tLastTimeCall;
	TickType_t tSamplingTime = pdMS_TO_TICKS(1);
	bool bEnableMeas = true;
	int16_t x, y, z;
	//initialize the i2c comm
	initI2C();
	//initialize the accelerometer
	initADXL345();
	printf("Accelerometer ID: %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_DEVID));
	printf("PWR reg val: %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_POWER_CTL));
	printf("BW_RATE reg: %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_BW_RATE));
	tLastTimeCall = xTaskGetTickCount(); //get the tick count since the execution of the programm
	while (1)
	{
		if (bEnableMeas)
			{
				getAcceleration(&x,&y,&z);
				tElapsedTicks = xTaskGetTickCount();
				printf("Time: %d;X: %5.2f; Y: %5.2f; Z: %5.2f;\r\n", tElapsedTicks * portTICK_RATE_MS, (double)(x)/256, (double)(y)/256, (double)(z)/256);
			}
		vTaskDelayUntil(&tLastTimeCall, tSamplingTime);
	}

}

void app_main(void)
{
    nvs_flash_init();
    ESP_LOGD("Main","Execution period is %d\r\n",portTICK_RATE_MS);
    //start the execution
    xTaskCreate(&vAccelerometerTask,"AccelerometerTask", 10000, NULL, 5,NULL);
//sda
    //unsigned int x = 0;
    //while (true) {
    	//printf("Running app:%u\r\n", x++);
    	//printf("X: %5.2f\r\n", (double)getAccelerationX()/256);
    	//int16_t x, y, z;
    	//getAcceleration(&x,&y,&z);

    	//printf("PWR reg val: %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_POWER_CTL));
    	//i2cWriteByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_POWER_CTL, 0x8);
    	//i2cWiteBit(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_POWER_CTL, ADXL345_PCTL_MEASURE_BIT, 1 );
    	//printf("PWR reg val after write %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_POWER_CTL));
        //vTaskDelay(pdMS_TO_TICKS(10));
    //}
}

