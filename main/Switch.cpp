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

gpio_num_t Switch::_sw = GPIO_NUM_0;

Switch::Switch() {
}

Switch::~Switch() {
}

void Switch::begin( gpio_num_t sw ){
	_sw = sw;
	gpio_set_direction(_sw, GPIO_MODE_INPUT);
	gpio_set_pull_mode(_sw, GPIO_PULLDOWN_ONLY);
}

bool Switch::isClosed() {
	gpio_set_pull_mode(_sw, GPIO_PULLUP_ONLY);
	delay(10);
	int level = gpio_get_level(_sw );
	gpio_set_pull_mode(_sw, GPIO_PULLDOWN_ONLY);
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
	if( _holddown ){   // debouncing
		_holddown--;
	}
	else{
		if( _closed ) {
			if( !isClosed() )
				_closed = false;
		}
		else {
			if( isClosed() ) {
				_closed = true;
				_holddown = 5;
			}
		}
	}
}
