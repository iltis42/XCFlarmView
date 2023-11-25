/*
 * OTA.h
 *
 *  Created on: Feb 24, 2019
 *      Author: iltis
 */
#ifndef MAIN_OTA_H_
#define MAIN_OTA_H_

#include "Switch.h"
#include "AdaptUGC.h"


class OTA
{
public:
	OTA();
	~OTA() {};
	void doSoftwareUpdate();
private:
	void writeText( int line, const char* text );
    int  tick;
};

#endif /* MAIN_SWITCH_H_ */
