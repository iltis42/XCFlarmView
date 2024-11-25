/*
 * Buzzer.cpp
 *
 *  Created on: Nov 19, 2023
 *      Author: esp32s2
 */

#include "Buzzer.h"
/* LEDC (LED Controller) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include <algorithm>
#include "logdef.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (35) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (0) // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY          (2700) // Frequency in Hertz. Set frequency at 2.7 kHz

TaskHandle_t Buzzer::pid = 0;
int Buzzer::freq = 0;
int Buzzer::dur = 0;
int Buzzer::vol = 0;

xQueueHandle Buzzer::queue = 0;

typedef struct s_tone{
	uint frequency: 12;  // max 4096 Hz
	uint duration: 12;   // max 4096 mS
	uint volume:8;       // max 100
	uint frequency2: 12;  // max 4096 Hz
	uint duration2: 12;   // max 4096 mS
	uint volume2:8;       // max 100
}tone_t;

/* Warning:
 * For ESP32, ESP32S2, ESP32S3, ESP32C3, ESP32C2, ESP32C6, ESP32H2, ESP32P4 targets,
 * when LEDC_DUTY_RES selects the maximum duty resolution (i.e. value equal to SOC_LEDC_TIMER_BIT_WIDTH),
 * 100% duty cycle is not reachable (duty cannot be set to (2 ** SOC_LEDC_TIMER_BIT_WIDTH)).
 */

Buzzer::Buzzer() {
	// TODO Auto-generated constructor stub

}

Buzzer::~Buzzer() {
	// TODO Auto-generated destructor stub
}

void Buzzer::init(uint freq)
{
    // Prepare and then apply the LEDC PWM timer configuration
	ESP_LOGI(FNAME,"Buzzer::init() F=%d", freq );
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = freq,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
/*
    int gpio_num;                   //!< the LEDC output gpio_num, if you want to use gpio16, gpio_num = 16
    ledc_mode_t speed_mode;         //!< LEDC speed speed_mode, high-speed mode or low-speed mode
    ledc_channel_t channel;         //!< LEDC channel (0 - 7)
    ledc_intr_type_t intr_type;     //!< configure interrupt, Fade interrupt enable  or Fade interrupt disable
    ledc_timer_t timer_sel;         //!< Select the timer source of channel (0 - 3)
    uint32_t duty;                  //!< LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)]
    int hpoint;                     //!< LEDC channel hpoint value, the max value is 0xfffff
    struct {
        uint output_invert: 1;//!< Enable (1) or disable (0) gpio output invert
    } flags;                        //!< LEDC flags
} ledc_channel_config_t;
*/

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
    	.gpio_num       = LEDC_OUTPUT_IO,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
		.intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0, {
		.output_invert          = 0
    	}
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    volume(0);
    queue = xQueueCreate(200, sizeof(tone_t));
    taskStart();
}

void Buzzer::frequency( uint f ){
	ESP_ERROR_CHECK(ledc_set_freq( LEDC_MODE, LEDC_TIMER, f));
}

void Buzzer::play2( uint f1, uint d1, uint v1, uint f2, uint d2, uint v2, uint repetition )
{
	xQueueReset(queue); // empty queue
	// ESP_LOGI(FNAME,"Buzzer play2() f:%d d:%d v:%d r:%d", f1, d1, v1, repetition );
	for(int i=0; i<repetition; i++ ){
		play(f1,d1,v1,f2,d2,v2);
	}
}

void Buzzer::buzz_task(void *pvParameters)
{
	ESP_LOGI(FNAME,"Buzzer task startup");
	while(1){
		tone_t t;
		if( xQueueReceive(queue, &t, portMAX_DELAY) == pdTRUE  ){
			frequency(t.frequency);
			volume(t.volume);
    		delay(t.duration);
    		volume(0);
    		if( t.duration2 )
    		{
    			frequency(t.frequency2);
    			volume(t.volume2);
    			delay(t.duration2);
    			volume(0);
    		}
		}
		// delay(20);
	}
}

void Buzzer::play( uint f, uint d, uint v, uint f2, uint d2, uint v2 ) {
	tone_t t = { f, d, v, f2, d2, v2 };
	xQueueSend(queue, &t, 0);
};

void Buzzer::volume( uint vol ){
	uint volume =  (vol*4096)/(100);
	// ESP_LOGI(FNAME,"Buzzer::vol=%d", vol);
	ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, volume ));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

void Buzzer::taskStart(){
	ESP_LOGI(FNAME,"Buzzer::taskStart()" );
	xTaskCreatePinnedToCore(&buzz_task, "Buzzer", 4096, NULL, 20, &pid, 0);
}

