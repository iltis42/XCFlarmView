/*
 * Switch.h
 *
 *  Created on: Feb 24, 2019
 *      Author: iltis
 */

#include <driver/gpio.h>
#include <list>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#pragma once

#ifndef MAIN_SWITCH_H_
#define MAIN_SWITCH_H_

class SwitchObserver;

typedef enum e_button_state { B_IDLE, B_PRESSED, B_ONCE_RELEASED, B_TWICE_CLOSED } t_button_state;

class Switch {
public:
	Switch( );
	virtual ~Switch();
	static void begin( gpio_num_t sw );
	static bool isClosed();
	static bool isOpen();
	static void tick();   // call al least every 100 mS
    static void attach( SwitchObserver *obs);
	static void detach( SwitchObserver *obs);
    static void sendRelease();
	static void sendPress();
	static void sendLongPress();
	static void sendDoubleClick();
	static void startTask();

private:
	static void switchTask(void *pvParameters);
	static std::list<SwitchObserver *> observers;
	static gpio_num_t _sw;
	static bool _closed;
	static int _holddown;
	static int _tick;
	static int _closed_timer;
	static int _long_timer;
	static TaskHandle_t pid;
	static int _click_timer;
	static int _clicks;
	static long int p_time;
	static long int r_time;
	static t_button_state _state;
};

#endif /* MAIN_SWITCH_H_ */


class SwitchObserver{
public:
		SwitchObserver(){};
        virtual void press() = 0;
        virtual void release() = 0;
        virtual void longPress() = 0;
        virtual void doubleClick() = 0;
        virtual ~SwitchObserver() {};
        void attach( SwitchObserver *instance) { Switch::attach( instance ); }
        void detach( SwitchObserver *instance) { Switch::detach( instance ); }
        bool isClosed() {  return( Switch::isClosed() ); }
private:
};
