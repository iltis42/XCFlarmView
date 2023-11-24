/*
 * TargetManager.h
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Flarm.h"
#include <map>
#include "Target.h"

#ifndef MAIN_TARGETMANAGER_H_
#define MAIN_TARGETMANAGER_H_

class TargetManager {
public:
	TargetManager();
	virtual ~TargetManager();
	static void receiveTarget( nmea_pflaa_s &target );
	static void tick();
	static void drawAirplane( int x, int y );
private:
	static std::map< unsigned int, Target *> targets;

};

#endif /* MAIN_TARGETMANAGER_H_ */
