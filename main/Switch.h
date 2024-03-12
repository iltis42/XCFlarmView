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

typedef enum e_button_state { B_IDLE, B_PRESSED } t_button_state;
typedef enum e_button       { B_MODE, B_UP, B_DOWN } t_button;

class Switch {
public:
	Switch( );
	virtual ~Switch();
	void begin( gpio_num_t sw, t_button mode=B_MODE );
	bool isClosed();
	bool isOpen();
	void tick();   // call al least every 100 mS
	static void attach( SwitchObserver *obs);
	static void detach( SwitchObserver *obs);
	void sendPress();
	void sendLongPress();
	void sendDoubleClick();
	static void startTask();

private:
	static void switchTask(void *pvParameters);
	static std::list<SwitchObserver *> observers;
	gpio_num_t _sw;
	bool _closed;
	int _holddown;
	int _tick;
	int _closed_timer;
	int _long_timer;
	static TaskHandle_t pid;
	int _click_timer;
	int _clicks;
	long int p_time;
	long int r_time;
	t_button_state _state;
	t_button _mode;
};

#endif /* MAIN_SWITCH_H_ */


class SwitchObserver{
public:
	SwitchObserver(){};
	virtual void press() = 0;
	virtual void up(int count) = 0;
	virtual void down(int count) = 0;
	virtual void longPress() = 0;
	virtual void doubleClick() = 0;
	virtual ~SwitchObserver() {};
	void attach( SwitchObserver *instance)  { Switch::attach( instance ); };
	void detach( SwitchObserver *instance)  { Switch::detach( instance ); };
	bool isClosed();
private:
};
