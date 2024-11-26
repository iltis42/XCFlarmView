/*
 * Buzzer.h
 *
 *  Created on: Nov 19, 2023
 *      Author: esp32s2
 */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <Arduino.h>

#ifndef MAIN_BUZZER_H_
#define MAIN_BUZZER_H_

#define BUZZ_C  2093
#define BUZZ_CH 2217
#define BUZZ_D  2349
#define BUZZ_DH 2489
#define BUZZ_E  2637
#define BUZZ_F  2794
#define BUZZ_FH 2960

#define BUZZ_G  3136
#define BUZZ_GH 3332
#define BUZZ_A  3520
#define BUZZ_AH 3729
#define BUZZ_H  3951


class Buzzer {
public:
	Buzzer();
	virtual ~Buzzer();
	static void init( uint frequency=2700 );
	static void volume( uint vol=0 );
	static void frequency( uint f);
	static void play2( uint f1=BUZZ_DH, uint d1=200, uint v1=100, uint f2=BUZZ_E, uint d2=200, uint v2=0, uint repetition=1 );
	static void play( uint freq=BUZZ_DH, uint duration=200, uint volume=100, uint freq2=BUZZ_DH, uint duration2=0, uint volume2=100 );
private:
	static TaskHandle_t pid;
	static void taskStart();
	static void buzz_task(void *pvParameters);
	static int freq, dur, vol;
	static xQueueHandle queue;
};

#endif /* MAIN_BUZZER_H_ */
