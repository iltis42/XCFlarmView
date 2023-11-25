/*
 * Switch.h
 *
 *  Created on: Feb 24, 2019
 *      Author: iltis
 */

#include <driver/gpio.h>

#ifndef MAIN_SWITCH_H_
#define MAIN_SWITCH_H_

class Switch {
public:
	Switch( );
	virtual ~Switch();
	static void begin( gpio_num_t sw );
	static bool isClosed();
	static bool isOpen();
	static void tick();   // call al least every 100 mS
private:
	static gpio_num_t _sw;
	static bool _closed;
	static int _holddown;
	static int _tick;
};

#endif /* MAIN_SWITCH_H_ */
