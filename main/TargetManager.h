/*
 * TargetManager.h
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Flarm.h"

#ifndef MAIN_TARGETMANAGER_H_
#define MAIN_TARGETMANAGER_H_

class TargetManager {
public:
	TargetManager();
	virtual ~TargetManager();
	static void receiveTarget( nmea_pflaa_s &target );
};

#endif /* MAIN_TARGETMANAGER_H_ */
