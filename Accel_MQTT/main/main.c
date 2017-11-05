#include <esp_event.h>
#include <esp_event_loop.h>
#include <stdio.h>
#include <math.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <string.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "freertos/timers.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"


#include "mongoose.h"
#include "fix_fft.h"
#include "lookup.h"
#include "sdkconfig.h"

// Defines for WiFi
#define SSID     "TP-LINK_POCKET_3020_79D6BA"
#define PASSWORD "22383358"
#define BROKERADDRESS "192.168.20.40:1883"

#define mainDelayLoopCount  10000
#define TIMER_DIVIDER         16  // Hardware timer clock divider, 80 to get 1MHz clock to timer */
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   (0.0001) // sample time in seconds
#define SINE_AMPLITUDE 		10
#define SINE_PERIOD			0.001 //period in seconds
#define log2N     10
#define N         1024     //number of sample points

typedef	enum
{
	IDLE,
	COLLECTSAMPLE,
	ANALYSIS,
	STOPPING
} MachineState_t;

static char tag []="mongooseTests";
xQueueHandle globalQueue1;
xQueueHandle globalQueueIncomingMessages;
uint32_t executionCount;
MachineState_t mainState;
short *real;
short *imag;

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
		BaseType_t result = xQueueSend(globalQueueIncomingMessages, &actualData, pdMS_TO_TICKS(100));
	}
	if(strcmp(messageData, "AccelOff") == 0)
		{
			ESP_LOGD("interpreter", "Disabling accelerometer data release");
			Data_t actualData;
			actualData.enableCom = false;
			actualData.connection = nc;
			//send the queue with the enable
			BaseType_t result = xQueueSend(globalQueueIncomingMessages, &actualData, pdMS_TO_TICKS(100));
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




void vContinuousTask(void *pvParameters)
{
	Data_t receivedCommand;
	bool enableSend = false;
	int sampleToCollect = N;
	int sampleCount = 0;
	char lineBuffer[6];
	char *message = malloc(10000);
	struct mg_connection *nc;
	TickType_t exitIdleTime = 0;
	//allocate the input buffer
	short *inputBuffer = malloc(sampleToCollect*sizeof(short));
	real = malloc(sampleToCollect*sizeof(short));
	imag = malloc(sampleToCollect*sizeof(short));
	short i;
	mainState = IDLE;
	while(1)
	{
		//check if we are autorized to proceed
		BaseType_t xStatus = xQueueReceive(globalQueueIncomingMessages, &receivedCommand, ( TickType_t ) 0);
		switch (xStatus) {
			case pdPASS:
				if (receivedCommand.enableCom)
				{
					enableSend = true;
					nc = receivedCommand.connection;
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
		switch (mainState) {
			case IDLE:
				if (enableSend) {
					sampleCount = 0;
					timer_start(TIMER_GROUP_0, 0);
					mainState = COLLECTSAMPLE;
					exitIdleTime = xTaskGetTickCount();
				}
				break;
			case COLLECTSAMPLE:{
				if (enableSend) {
					BaseType_t	xStatus = xQueueReceive(globalQueue1, &inputBuffer[sampleCount], ( TickType_t ) 0);
					switch (xStatus) {
					case pdPASS:
						if (sampleCount == sampleToCollect)
							mainState = ANALYSIS;
						else
							sampleCount += 1;
						break;
					default:
						break;
					}

					BaseType_t res = uxQueueMessagesWaitingFromISR(globalQueue1);
					if (res > 999) {
						// the queue is full
						printf("Queue full: %d\nResetting...\n",(int)res);
						mainState = STOPPING;
					}
				}
				else{
					//stop the timer
					timer_pause(TIMER_GROUP_0, 0);
					mainState = STOPPING;
				}

				break;
			}
			case ANALYSIS:{
				if (enableSend) {
					timer_pause(TIMER_GROUP_0, 0);
					//ESP_LOGD("machineState","Total sampling time %d", xTaskGetTickCount() - exitIdleTime);
					for( i=0; i<sampleToCollect; i++) imag[i] = 0;   // clear imag array
					memcpy(real, inputBuffer, sizeof(short)*sampleToCollect);
					TickType_t start = xTaskGetTickCount();
					fix_fft(real, imag, (short)log2(sampleToCollect), 0);
					//ESP_LOGD("machineState","FFT execution time %d", xTaskGetTickCount() - start);
					//ESP_LOGD("machineState", "waiting messages: %d", uxQueueMessagesWaiting(globalQueue1))
					for ( i = 0; i < N/2; i++)         //get the power magnitude in each bin
					{
					  real[i] =sqrt((long)real[i] * (long)real[i] + (long)imag[i] * (long)imag[i]);
					}
					printf("*\n");      //send fft results over serial to PC
					for (i=0; i<sampleToCollect/2; i++) {
						printf("%d\n",real[i]);
					}
					int msgLen = 0;
					for (i = 0; i < sampleToCollect/2; i++) {
						int stringLength = sprintf(lineBuffer, "%d, ",real[i]);
						memcpy(message + msgLen, lineBuffer, stringLength);
						msgLen += stringLength;
						//printf("LineBuffer data %s, length %d\n", lineBuffer,position);
					}
					message[msgLen] = '\n';
					msgLen+=1;

					char number[100];
					int size = sprintf(number, "Sending Time: %d", xTaskGetTickCount());
					mg_mqtt_publish(nc, "/FFT/Accel1", 65, MG_MQTT_QOS(0), number,size);
					mg_mqtt_publish(nc, "/FFT/Accel1", 65, MG_MQTT_QOS(0), message,msgLen);
					ESP_LOGD("Acelerometer", "Sending data %.*s", size, number);
					mainState = STOPPING;
				}
				else
				{
					timer_pause(TIMER_GROUP_0, 0);
					mainState = STOPPING;
				}
				break;
				}
			case STOPPING:
				//reset everything
				xQueueReset(globalQueue1);
				mainState = IDLE;
				break;
			default:
				break;
		}
	}
}

/*
 * Timer group0 ISR handler
 *
 * Note:
 * We don't call the timer API here because they are not declared with IRAM_ATTR.
 * If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
 * we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
 */
void IRAM_ATTR timer_group0_isr(void *para)
{
    int timer_idx = (int) para;

//     Retrieve the interrupt status and the counter value
//       from the timer that reported the interrupt
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;
    //actual timer counter
    uint64_t timer_counter_value =
        ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG0.hw_timer[timer_idx].cnt_low;

//     Clear the interrupt
//       and update the alarm time for the timer with without reload
    if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0) {
        TIMERG0.int_clr_timers.t0 = 1;
        timer_counter_value += (uint64_t) (TIMER_INTERVAL0_SEC * TIMER_SCALE);
        TIMERG0.hw_timer[timer_idx].alarm_high = (uint32_t) (timer_counter_value >> 32);
        TIMERG0.hw_timer[timer_idx].alarm_low = (uint32_t) timer_counter_value;

        //execute the reading from the device
        short computedValue = (short)((0.5*sin(2*3.14*executionCount*((double)TIMER_INTERVAL0_SEC/(double)SINE_PERIOD)) +
        		0.5*sin(2*3.14*executionCount*((double)TIMER_INTERVAL0_SEC/0.01)))*16384)+16384;
        //short computedValue = (short) executionCount;
        //add the value on the queue;
        xQueueSendFromISR(globalQueue1, &computedValue, ( TickType_t ) 0);
        executionCount +=1;
    } else {
        ;
    }

//     After the alarm has been triggered
//      we need enable it again, so it is triggered the next time
    TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;
}

/*
 * Initialize selected timer of the timer group 0
 *
 * timer_idx - the timer number to initialize
 * auto_reload - should the timer auto reload on alarm?
 * timer_interval_sec - the interval of alarm to set
 */
static void my_timer_init(int timer_idx,
    bool auto_reload, double timer_interval_sec)
{
    // Select and initialize basic parameters of the timer
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

     //Timer's counter will initially start from value below.
     //  Also, if auto_reload is set, this value will be automatically reload on alarm
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    // Configure the alarm value and the interrupt on alarm.
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
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
		mg_mgr_poll(&mgr, 100);
	}
} // mongooseTask


// ESP32 WiFi handler.
esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
	if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
		ESP_LOGD(tag, "Got an IP: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
		xTaskCreatePinnedToCore(&mongooseTask, "mongooseTask", 20000, NULL, 6, NULL,0);
		my_timer_init(0,1,TIMER_INTERVAL0_SEC);
		xTaskCreatePinnedToCore(&vContinuousTask, "accelerometerTask", 50000, NULL, 2, NULL,0);
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
	executionCount = 0;

	globalQueue1 = xQueueCreate(1000, sizeof(short)); //this is for int16
	globalQueueIncomingMessages = xQueueCreate(100, sizeof(Data_t));
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
	while(1)
	{

	}
	return 0;
} // app_main
