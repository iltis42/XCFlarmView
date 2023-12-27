/*
 * Switch.cpp
 *
 *  Created on: Nov 11, 2023
 *      Author: iltis
 *
 *      Static class for a single push buttom
 *
 *      Example:
 *      sw = Switch();
 *      sw.begin( GPIO_NUM_1 );
 *      if( sw.state() )
 *         ESP_LOGI(FNAME,"Open");
 *
 */

#include "esp32-hal.h"
#include "driver/gpio.h"
#include "Switch.h"
#include <logdef.h>
#include "Setup.h"
#include "average.h"
#include "vector.h"
#include "Units.h"


bool Switch::_closed = false;
int Switch::_holddown = 0;
int Switch::_tick = 0;
int Switch::_closed_timer = 0;
int Switch::_long_timer = 0;
std::list<SwitchObserver *> Switch::observers;
TaskHandle_t Switch::pid = 0;
int Switch::_click_timer=0;
int Switch::_clicks=0;

gpio_num_t Switch::_sw = GPIO_NUM_0;

#define TASK_PERIOD 10

Switch::Switch() {
}

Switch::~Switch() {
}

void Switch::attach(SwitchObserver *obs) {
        // ESP_LOGI(FNAME,"Attach obs: %p", obs );
        observers.push_back(obs);
}

void Switch::detach(SwitchObserver *obs) {
        // ESP_LOGI(FNAME,"Detach obs: %p", obs );
	/*
        auto it = std::find(observers.begin(), observers.end(), obs);
        if ( it != observers.end() ) {
                observers.erase(it);
        }
        */
}


void Switch::switchTask(void *pvParameters){
	while(1){
		tick();
		delay(TASK_PERIOD);
	}
}

void Switch::startTask(){
	ESP_LOGI(FNAME,"taskStart");
	xTaskCreatePinnedToCore(&switchTask, "Switch", 4096, NULL, 12, &pid, 0);
}

void Switch::begin( gpio_num_t sw ){
	ESP_LOGI(FNAME,"Switch::begin GPIO: %d", sw);
	_sw = sw;
	gpio_set_direction(_sw, GPIO_MODE_INPUT);
	gpio_set_pull_mode(_sw, GPIO_PULLUP_ONLY);

}

bool Switch::isClosed() {

	int level = gpio_get_level(_sw );
	if( level )
		return false;
	else
		return true;
}

bool Switch::isOpen() {
	return( !isClosed() );
}

void Switch::tick() {
	_tick++;
	if( _click_timer ){
		_click_timer--;
		if( _clicks == 2 ){
			sendDoubleClick();
			_clicks = 0;
			_click_timer = 0;
		}
	}else{
		_clicks = 0;
	}
	// ESP_LOGI(FNAME,"tick %d", _tick);
	if( _closed ) {   // state is switch closed
		_closed_timer++;
		_long_timer++;
		// ESP_LOGI(FNAME,"closed: %d clicks: %d clicks_timer: %d", _closed_timer, _clicks, _click_timer );
		if( _long_timer > 50 ){
			sendLongPress();
			_long_timer = 0;
		}
		if( !isClosed() ){
			if( _holddown ){   // debouncing
				_holddown--;
			}else{
				_closed = false;
				_holddown=2;
				if( _closed_timer < 50 ){
					sendPress();  // we sent at quick release the press
					_closed_timer = 0;
				}
			}
		}
	}
	else {  // state is switch open
		if( isClosed() ) {
			if( _holddown ){   // debouncing
				_holddown--;
			}else{
				_closed = true;
				_closed_timer = 0;
				_long_timer = 0;
				_holddown = 2;
				_clicks++;
				_click_timer = 25;
			}
		}
	}
}


void Switch::sendRelease(){
        ESP_LOGI(FNAME,"send release");
        for (auto &observer : observers)
                observer->release();
        // ESP_LOGI(FNAME,"End switch release action");
}

void Switch::sendPress(){
        ESP_LOGI(FNAME,"send press");
        for (auto &observer : observers)
                observer->press();
        // ESP_LOGI(FNAME,"End pressed action");

}

void Switch::sendLongPress(){
        ESP_LOGI(FNAME,"send longPress");
        for (auto &observer : observers)
                observer->longPress();
        // ESP_LOGI(FNAME,"End long pressed action");
}

void Switch::sendDoubleClick(){
        ESP_LOGI(FNAME,"send doubleClick");
        for (auto &observer : observers)
                observer->doubleClick();
        // ESP_LOGI(FNAME,"End long pressed action");
}


