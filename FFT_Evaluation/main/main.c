#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/timers.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "fix_fft.h"
#include "lookup.h"
#include <esp_log.h>
#include <string.h>

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

xQueueHandle globalQueue1;
uint32_t executionCount;
MachineState_t mainState;
short *real;
short *imag;

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

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[timer_idx].update = 1;
    //actual timer counter
    uint64_t timer_counter_value =
        ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
        | TIMERG0.hw_timer[timer_idx].cnt_low;

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
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
        xQueueSendFromISR(globalQueue1, &computedValue, NULL);
        executionCount +=1;
    } else {
        ;
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
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
    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.alarm_en = TIMER_ALARM_EN;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = auto_reload;
    timer_init(TIMER_GROUP_0, timer_idx, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(TIMER_GROUP_0, timer_idx);
    timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
        (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

    timer_start(TIMER_GROUP_0, timer_idx);
}

void vContinuousTask(void *pvParameters)
{
	int sampleToCollect = N;
	int sampleCount = 0;
	TickType_t exitIdleTime = 0;
	//allocate the input buffer
	short *inputBuffer = malloc(sampleToCollect*sizeof(short));
	real = malloc(sampleToCollect*sizeof(short));
	imag = malloc(sampleToCollect*sizeof(short));
	short i;
	mainState = IDLE;
	while(1)
	{
		switch (mainState) {
			case IDLE:
				sampleCount = 0;
				timer_start(TIMER_GROUP_0, 0);
				mainState = COLLECTSAMPLE;
				exitIdleTime = xTaskGetTickCount();
				break;
			case COLLECTSAMPLE:
			{
				BaseType_t	xStatus = xQueueReceive(globalQueue1, &inputBuffer[sampleCount], pdMS_TO_TICKS(1));
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
					mainState = IDLE;
				}
				break;
			}
			case ANALYSIS:
//				for (int i = 0; i < sampleToCollect ; i++) {
//					printf("%d, %d\n",i, (short)inputBuffer[i]);
//				}
				{
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
				mainState = STOPPING;
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

void app_main(void)
{
    nvs_flash_init();
    executionCount = 0;
    printf("Startingo\n\r");
    //xTaskCreate(&vContinuousTask, "ContTask", 2048,NULL,1,NULL);
    my_timer_init(0,0,TIMER_INTERVAL0_SEC);
    //create the queue handler. the pointer should be passed to the task that are gonna to access it
    globalQueue1 = xQueueCreate(1000, sizeof(short)); //this is for int16
    if (globalQueue1 != NULL)
    {
    	printf("Different from null");
    	xTaskCreate(&vContinuousTask, "TaskCont", 20000,NULL,5,NULL);
    }
}




