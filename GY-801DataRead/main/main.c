#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include <lwip/sockets.h>
#include <esp_log.h>
#include "ADXL345.h"
#include "driver/i2c.h"

#define BUFFERLENGTH 1000
#define PORT_NUMBER 8001

struct accelData {
   TickType_t  time[BUFFERLENGTH];
   int16_t  xData[BUFFERLENGTH];
   int16_t  yData[BUFFERLENGTH];
   int16_t  zData[BUFFERLENGTH];
} accelData;

xQueueHandle timeQueue;
xQueueHandle xAccelQueue;
xQueueHandle xDataQueue;


esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void vSetupTCP()
{
	tcpip_adapter_init();
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	wifi_config_t sta_config = {
		.sta = {
			.ssid = "TP-LINK_POCKET_3020_79D6BA",
			.password = "22383358",
			.bssid_set = false
		}
	};
	ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
	ESP_ERROR_CHECK( esp_wifi_start() );
	ESP_ERROR_CHECK( esp_wifi_connect() );
}

void vPrintData(TickType_t *timeBuffer, int16_t *xBuffer)
{
	printf("New dataset received\r\n");
	for (int i = 0; i < BUFFERLENGTH; i++) {
		printf("Time: %d, xAccel: %d\r\n", *(timeBuffer + i), *(xBuffer + i));
	}
}

void vPrintData1(struct accelData *data)
{
	printf("New dataset received\r\n");
	char dataString[] = "";
	for (int i = 0; i < BUFFERLENGTH; i++) {
		//printf("Time: %d, xAccel: %d\r\n", *(timeBuffer + i), *(xBuffer + i));
		sprintf(dataString, "%d, %d, %d, %d\r\n", (*data).time[i], (*data).xData[i], (*data).yData[i], (*data).zData[i]);
	}
	printf(dataString);
}

//this is the task that takes care of the communication aspect
void vCommunicationTask(void *pvParameters)
{
	struct accelData sData;
	BaseType_t xStatus;
	uint16_t xBuffer[BUFFERLENGTH];
	uint16_t yBuffer[BUFFERLENGTH];
	uint16_t zBuffer[BUFFERLENGTH];
	TickType_t timeBuffer[BUFFERLENGTH];
	//task execution time
	TickType_t tSamplingTime = pdMS_TO_TICKS(500);
	TickType_t tLastTimeCall;
	tLastTimeCall = xTaskGetTickCount(); //get the tick count since the execution of the programm
		while (1)
		{
			//ESP_LOGD("Communication", "Comm ok");
			xStatus = xQueueReceive(xDataQueue, &sData, pdMS_TO_TICKS(10));
			switch (xStatus) {
			case pdPASS:
				ESP_LOGD("Communication", "Received array from time measurement");
				vPrintData1(&sData);
				break;
			default:
				//ESP_LOGD("Communication", "Data not available");
				break;
			}
			vTaskDelayUntil(&tLastTimeCall, tSamplingTime);
		}
}



// this is the task in which the accelration measurements are performed
void vAccelerometerTask(void *pvParameters)
{
	//BaseType_t xStatus;
	BaseType_t result;
	//
	//uint16_t xBuffer[BUFFERLENGTH];
	//uint16_t yBuffer[BUFFERLENGTH];
	//uint16_t zBuffer[BUFFERLENGTH];
	//TickType_t timeBuffer[BUFFERLENGTH];
	struct accelData sData;


	TickType_t tElapsedTicks;
	TickType_t tLastTimeCall;
	TickType_t tSamplingTime = pdMS_TO_TICKS(10);
	bool bEnableMeas = true;
	int16_t x, y, z;
	uint16_t index;
	//initialize the i2c comm
	initI2C();
	//initialize the accelerometer
	initADXL345();
	printf("Accelerometer ID: %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_DEVID));
	printf("PWR reg val: %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_POWER_CTL));
	printf("BW_RATE reg: %u\r\n", i2cReadByte(ADXL345_DEFAULT_ADDRESS, ADXL345_RA_BW_RATE));
	tLastTimeCall = xTaskGetTickCount(); //get the tick count since the execution of the programm
	index = 0;
	while (1)
	{
		if (bEnableMeas && index < BUFFERLENGTH)
			{
				getAcceleration(&x,&y,&z);
				tElapsedTicks = xTaskGetTickCount();
				*(sData.xData + index) = x;
				*(sData.yData + index) = y;
				*(sData.zData + index) = z;
				*(sData.time + index) = tElapsedTicks;
				//printf("Time: %d;X: %5.2f; Y: %5.2f; Z: %5.2f;\r\n", tElapsedTicks * portTICK_RATE_MS, (double)(x)/256, (double)(y)/256, (double)(z)/256);
				index += 1;
			}
		else if (index >= BUFFERLENGTH)
		{
			//store the buffer in the queue
			result = xQueueSend(xDataQueue, &sData, pdMS_TO_TICKS(100));
			char *resultTest;
			if (result == pdPASS)
				resultTest = "pdPASS";
			else
				resultTest = "errQUEUE_FULL";
			ESP_LOGD("Accelerometer"," Queue written with the result: %s\n", resultTest);
			//reset the couter
			index = 0;

		}
		vTaskDelayUntil(&tLastTimeCall, tSamplingTime);
	}

}

void app_main(void)
{
	TickType_t temp[BUFFERLENGTH];
    nvs_flash_init();
    ESP_LOGD("Main","Execution period is %d\r\n",portTICK_RATE_MS);
    ESP_LOGI("Generic", "TimeQueue computed size; %d", sizeof(TickType_t)*BUFFERLENGTH);
	ESP_LOGI("Generic", "TimeQueue real size; %d", sizeof(accelData));
	timeQueue = xQueueCreate(3,sizeof(TickType_t)*BUFFERLENGTH);
	xDataQueue = xQueueCreate(3,sizeof(accelData));
	xAccelQueue = xQueueCreate(3,sizeof(int16_t)*BUFFERLENGTH);
    //start the execution
    xTaskCreate(&vAccelerometerTask,"AccelerometerTask", 20000, NULL, 5,NULL);
    xTaskCreate(&vCommunicationTask,"CommunicationTask", 20000, NULL, 2,NULL);


}

