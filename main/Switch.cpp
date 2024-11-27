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
t_button_state Switch::_state=B_IDLE;
long int Switch::p_time = 0;
long int Switch::r_time = 0;

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
	xTaskCreatePinnedToCore(&switchTask, "Switch", 6096, NULL, 9, &pid, 0);
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
	// ESP_LOGI(FNAME,"tick %d", _tick );
	switch( _state ) {
	case B_IDLE:
		if( isClosed() ){   // button pressed
			_state = B_PRESSED;
			p_time = millis();
		}
		break;

	case B_PRESSED:
		if( isOpen() ){
			if( (millis() - p_time ) > 350 ){    // was this a long press?
				sendLongPress();
				_state = B_IDLE;
			}else if( (millis() - p_time ) > 50 ){   // if not, filter bounces and go to released state
				_state = B_ONCE_RELEASED;
				r_time = millis();
				// ESP_LOGI(FNAME,"->ONCE_RELEASED after %ld ms", millis() - p_time );
			}
			else{
				_state = B_IDLE;
			}
		}
		break;

	case B_ONCE_RELEASED:
		if( isClosed() ){             // closed again ?
			p_time = millis();
			_state = B_TWICE_CLOSED;
			// ESP_LOGI(FNAME,"->TWICE CLOSED after %ld ms", millis() - r_time );
		}
		else{ // remains open
			int rdelta = millis() - r_time;    // nope, after timeout its a normal press
			if(  rdelta > 150  ){ // timeout for DoubleClick, its a single click then
				// ESP_LOGI(FNAME,"PRESS and ->IDLE after released %d ms", rdelta );
				sendPress();
				_state = B_IDLE;
			}
		}
		break;

	case B_TWICE_CLOSED:
		if( isOpen() ){   // second press ended ?
			if( (millis() - p_time ) < 150 ){   // had a second short press ?
				sendDoubleClick();
				_state = B_IDLE;
			}else{
				_state = B_IDLE;   // nope, so throw this odd short/long press event
			}
		}
		break;
	}
}

void Switch::sendPress(){
	ESP_LOGI(FNAME,"send press p: %ld  r: %ld", millis()-p_time, millis()-r_time );
	for (auto &observer : observers)
		observer->press();
	// ESP_LOGI(FNAME,"End pressed action");

}

void Switch::sendLongPress(){
	ESP_LOGI(FNAME,"send longPress %ld", millis()-p_time );
	for (auto &observer : observers)
		observer->longPress();
	// ESP_LOGI(FNAME,"End long pressed action");
}

void Switch::sendDoubleClick(){
	ESP_LOGI(FNAME,"send doubleClick p:%ld r:%ld", millis()-p_time, millis()-r_time );
	for (auto &observer : observers)
		observer->doubleClick();
	// ESP_LOGI(FNAME,"End long pressed action");
}


