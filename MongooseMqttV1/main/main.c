#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>
#include "freertos/queue.h"
#include "freertos/task.h"

#include "mongoose.h"
#include "sdkconfig.h"
#include "test1_html.h"

// Defines for WiFi
#define SSID     "TP-LINK_POCKET_3020_79D6BA"
#define PASSWORD "22383358"
#define BROKERADDRESS "192.168.20.40:1883"

static char tag []="mongooseTests";
xQueueHandle globalQueue1;
static const char * s_user_name = NULL;
static const char * s_password = NULL;
static const char *s_topic = "/stuff";
static struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};

typedef struct
{
	bool enableCom;
	struct mg_connection* connection;
} Data_t;

/**
 * Convert a Mongoose event type to a string.
 */
char *mongoose_eventToString(int ev) {
	static char temp[100];
	switch (ev) {
	case MG_EV_CONNECT:
		return "MG_EV_CONNECT";
	case MG_EV_ACCEPT:
		return "MG_EV_ACCEPT";
	case MG_EV_CLOSE:
		return "MG_EV_CLOSE";
	case MG_EV_SEND:
		return "MG_EV_SEND";
	case MG_EV_RECV:
		return "MG_EV_RECV";
	case MG_EV_HTTP_REQUEST:
		return "MG_EV_HTTP_REQUEST";
	case MG_EV_HTTP_REPLY:
		return "MG_EV_HTTP_REPLY";
	case MG_EV_MQTT_CONNACK:
		return "MG_EV_MQTT_CONNACK";
	case MG_EV_MQTT_CONNACK_ACCEPTED:
		return "MG_EV_MQTT_CONNACK";
	case MG_EV_MQTT_CONNECT:
		return "MG_EV_MQTT_CONNECT";
	case MG_EV_MQTT_DISCONNECT:
		return "MG_EV_MQTT_DISCONNECT";
	case MG_EV_MQTT_PINGREQ:
		return "MG_EV_MQTT_PINGREQ";
	case MG_EV_MQTT_PINGRESP:
		return "MG_EV_MQTT_PINGRESP";
	case MG_EV_MQTT_PUBACK:
		return "MG_EV_MQTT_PUBACK";
	case MG_EV_MQTT_PUBCOMP:
		return "MG_EV_MQTT_PUBCOMP";
	case MG_EV_MQTT_PUBLISH:
		return "MG_EV_MQTT_PUBLISH";
	case MG_EV_MQTT_PUBREC:
		return "MG_EV_MQTT_PUBREC";
	case MG_EV_MQTT_PUBREL:
		return "MG_EV_MQTT_PUBREL";
	case MG_EV_MQTT_SUBACK:
		return "MG_EV_MQTT_SUBACK";
	case MG_EV_MQTT_SUBSCRIBE:
		return "MG_EV_MQTT_SUBSCRIBE";
	case MG_EV_MQTT_UNSUBACK:
		return "MG_EV_MQTT_UNSUBACK";
	case MG_EV_MQTT_UNSUBSCRIBE:
		return "MG_EV_MQTT_UNSUBSCRIBE";
	case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
		return "MG_EV_WEBSOCKET_HANDSHAKE_REQUEST";
	case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
		return "MG_EV_WEBSOCKET_HANDSHAKE_DONE";
	case MG_EV_WEBSOCKET_FRAME:
		return "MG_EV_WEBSOCKET_FRAME";
	}
	sprintf(temp, "Unknown event: %d", ev);
	return temp;
} //eventToString

void messageInterpreter(struct mg_mqtt_message *msg, struct mg_connection *nc)
{
	ESP_LOGD("interpreter", "Interpreter");
	char* messageData = malloc(msg->payload.len+1);
	memcpy(messageData, msg->payload.p, (int)msg->payload.len);
	messageData[msg->payload.len] = 0;
	ESP_LOGD("interpreter", "Analizing the following message: %s", messageData);
	if(strcmp(messageData, "AccelOn") == 0)
	{
		ESP_LOGD("interpreter", "Enabling accelerometer data release");
		Data_t actualData;
		actualData.enableCom = true;
		actualData.connection = nc;
		//send the queue with the enable
		BaseType_t result = xQueueSend(globalQueue1, &actualData, pdMS_TO_TICKS(100));
	}
	if(strcmp(messageData, "AccelOff") == 0)
		{
			ESP_LOGD("interpreter", "Disabling accelerometer data release");
			Data_t actualData;
			actualData.enableCom = false;
			actualData.connection = nc;
			//send the queue with the enable
			BaseType_t result = xQueueSend(globalQueue1, &actualData, pdMS_TO_TICKS(100));
		}
}

// Convert a Mongoose string type to a string.
char *mgStrToStr(struct mg_str mgStr) {
	char *retStr = (char *) malloc(mgStr.len + 1);
	memcpy(retStr, mgStr.p, mgStr.len);
	retStr[mgStr.len] = 0;
	return retStr;
} // mgStrToStr

// Mongoose event handler.
void mongoose_event_handler(struct mg_connection *nc, int ev, void *evData) {
	struct mg_mqtt_message *msg = (struct mg_mqtt_message *) evData;
	(void) nc;

//	if(ev != MG_EV_MQTT_CONNACK)
//	{
//		printf( "Handler call %s\n", mongoose_eventToString(ev));
//	}
	switch (ev) {
	case MG_EV_CONNECT: {
	      struct mg_send_mqtt_handshake_opts opts;
	      memset(&opts, 0, sizeof(opts));
	      opts.user_name = s_user_name;
	      opts.password = s_password;

	      mg_set_protocol_mqtt(nc);
	      mg_send_mqtt_handshake_opt(nc, "dummy", opts);
	      break;
	    }
	case MG_EV_MQTT_CONNACK:
	      if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
	        printf("Got mqtt connection error: %d\n", msg->connack_ret_code);
	        exit(1);
	      }
	      s_topic_expr.topic = s_topic;
	      printf("Subscribing to '%s'\n", s_topic);
	      mg_mqtt_subscribe(nc, &s_topic_expr, 1, 42);
	      break;
	case MG_EV_MQTT_PUBACK:
			printf("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
			break;
	case MG_EV_MQTT_SUBACK:
		 	 printf("Subscription acknowledged, forwarding to '/test'\n");
		 	 break;
	case MG_EV_MQTT_PUBLISH: {
	#if 0
	        char hex[1024] = {0};
	        mg_hexdump(nc->recv_mbuf.buf, msg->payload.len, hex, sizeof(hex));
	        printf("Got incoming message %.*s:\n%s", (int)msg->topic.len, msg->topic.p, hex);
	#else
	        printf("Got incoming message %.*s: %.*s\n", (int) msg->topic.len,
	             msg->topic.p, (int) msg->payload.len, msg->payload.p);

	#endif
	        messageInterpreter(msg, nc);
	        ESP_LOGD("handler","Forwarding to /test\n");
	        mg_mqtt_publish(nc, "/test", 65, MG_MQTT_QOS(0), msg->payload.p,msg->payload.len);
	        break;
	    }
	case MG_EV_WEBSOCKET_FRAME:
	{
		break;
	}

	case MG_EV_HTTP_REQUEST: {
			break;
		}
	case MG_EV_CLOSE:{
		ESP_LOGD("handler","Connection closed\n");
		break;
	}
	default:

		break;
	} // End of switch
} // End of mongoose_event_handler




void accelTask(void *data){

	ESP_LOGD(tag, "Starting accel task");
	bool enableSend = 0;
	char lineBuffer[40];
	struct mg_connection *nc;
	char *message = malloc(40000);
	int i;
	int msgLen = 0;
	for (i = 0; i < 200; i++) {
		int stringLength = sprintf(lineBuffer, "X:%dY:%dZ:%d,",i,i+1000,i+2000);
		memcpy(message + msgLen, lineBuffer, stringLength);
		msgLen += stringLength;
		//printf("LineBuffer data %s, length %d\n", lineBuffer,position);
	}
	TickType_t lastTimeCall = xTaskGetTickCount();
	//printf("Result array: %s\n", message);
	while(1)
	{

		Data_t actualData;
		BaseType_t xStatus = xQueueReceive(globalQueue1, &actualData, pdMS_TO_TICKS(100));

		switch (xStatus) {
			case pdPASS:
				if (actualData.enableCom)
				{
					enableSend = true;
					nc = actualData.connection;
					ESP_LOGD("Acelerometer", "enableSend true");
				}
				else
				{
					enableSend = false;
					ESP_LOGD("Acelerometer", "enableSend false");
				}
				break;
			default:
				//ESP_LOGD("Acelerometer", "Unable to receive from the queue");
				break;
		}

		//enable the accelerometer data send
		if(enableSend)
		{
			//mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, message, msgLen);
			char number[100];
			int size = sprintf(number, "Sending Time: %d", xTaskGetTickCount());
			mg_mqtt_publish(nc, "/accelerometer", 65, MG_MQTT_QOS(0), number,size);
			mg_mqtt_publish(nc, "/accelerometer", 65, MG_MQTT_QOS(0), message,msgLen);
			ESP_LOGD("Acelerometer", "Sending data %.*s", size, number);
		}
		vTaskDelay(pdMS_TO_TICKS(500));

	}
}

// FreeRTOS task to start Mongoose.
void mongooseTask(void *data) {
	ESP_LOGD(tag, "Mongoose task starting");
	struct mg_mgr mgr;
	ESP_LOGD(tag, "Mongoose: Starting setup");
	mg_mgr_init(&mgr, NULL);
	ESP_LOGD(tag, "Mongoose: Succesfully inited");
	if (mg_connect(&mgr, BROKERADDRESS, mongoose_event_handler) == NULL) {
		ESP_LOGD(tag,"mg_connect failed\n");
	    exit(EXIT_FAILURE);
	  }
	else
		ESP_LOGD(tag,"Correctly connected to the broker");
	while (1) {
		mg_mgr_poll(&mgr, 1000);
	}
} // mongooseTask


// ESP32 WiFi handler.
esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
	if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
		ESP_LOGD(tag, "Got an IP: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
		xTaskCreatePinnedToCore(&mongooseTask, "mongooseTask", 20000, NULL, 5, NULL,0);
		xTaskCreatePinnedToCore(&accelTask, "accelerometerTask", 50000, NULL, 3, NULL,0);
	}
	if (event->event_id == SYSTEM_EVENT_STA_START) {
		ESP_LOGD(tag, "Received a start request");
		ESP_ERROR_CHECK(esp_wifi_connect());
	}
	return ESP_OK;
} // wifi_event_handler


int app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  globalQueue1 = xQueueCreate(100, sizeof(Data_t)); //this is for int16
  ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  wifi_config_t sta_config = {
      .sta = {
          .ssid = SSID,
          .password = PASSWORD,
          .bssid_set = 0
      }
  };
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

	return 0;
} // app_main
