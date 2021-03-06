#include "string.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include <lwip/sockets.h>
#include <esp_log.h>

#include <errno.h>
#include "sdkconfig.h"

#define PORT_NUMBER 8001
static char tag[] = "socket_server";

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void socket_server(void *ignore) {
	struct sockaddr_in clientAddress;
	struct sockaddr_in serverAddress;

	// Create a socket that we will listen upon.
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		ESP_LOGE(tag, "socket: %d %s", sock, strerror(errno));
		goto END;
	}

	// Bind our server socket to a port.
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(PORT_NUMBER);
	int rc  = bind(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if (rc < 0) {
		ESP_LOGE(tag, "bind: %d %s", rc, strerror(errno));
		goto END;
	}

	// Flag the socket as listening for new connections.
	rc = listen(sock, 5);
	if (rc < 0) {
		ESP_LOGE(tag, "listen: %d %s", rc, strerror(errno));
		goto END;
	}

	while (1) {
		// Listen for a new client connection.
		socklen_t clientAddressLength = sizeof(clientAddress);
		int clientSock = accept(sock, (struct sockaddr *)&clientAddress, &clientAddressLength);
		if (clientSock < 0) {
			ESP_LOGE(tag, "accept: %d %s", clientSock, strerror(errno));
			goto END;
		}
		printf("New client\n");
		// We now have a new client ...
		int total =	10*1024;
		int sizeUsed = 0;
		char *data = malloc(total);
		ssize_t sizeRead;
		// Loop reading data.
		while(1) {
			sizeRead = recv(clientSock, data + sizeUsed, total-sizeUsed, 0);
			if (sizeRead < 0) {

				ESP_LOGE(tag, "recv: %d %s", sizeRead, strerror(errno));
				goto END;
			}
			if (sizeRead > 0)
			{
				printf("Data read (size: %d) was: %.*s\n", sizeUsed + sizeRead, sizeUsed+ sizeRead, data);
				vTaskDelay(pdMS_TO_TICKS(5));
				int transm = send(clientSock, data, sizeRead ,0);
				vTaskDelay(pdMS_TO_TICKS(1));
				close(clientSock);
				printf("Written data: %d\n", transm);
				break;
			}
			if (sizeRead == 0) {
				break;
			}
			sizeUsed += sizeRead;
		}

		// Finished reading data.
		ESP_LOGD(tag, "dData read (size: %d) was: %.*s", sizeUsed, sizeUsed, data);
		printf("Data read (size: %d) was: %.*s\n", sizeUsed, sizeUsed, data);

		free(data);
		close(clientSock);
	}
	END:
	vTaskDelete(NULL);
}

void app_main(void)
{
    nvs_flash_init();
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
    xTaskCreate(&socket_server, "tcp_conn", 4096, NULL, 5, NULL);
    while (true) {
    }
}

