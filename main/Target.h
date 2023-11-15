/*
 * Target.h
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#ifndef MAIN_TARGET_H_
#define MAIN_TARGET_H_

class Target {
public:
	Target();
	virtual ~Target();

private:
	int x;
	int y;
};

#endif /* MAIN_TARGET_H_ */
