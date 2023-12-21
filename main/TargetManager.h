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
	static void drawAirplane( int x, int y, float north=0.0 );
private:
	static std::map< unsigned int, Target> targets;
	static float oldN;
	static void drawN( int x, int y, bool erase, float north );
	static void printAlarm( const char*alarm, int x, int y, int inactive );
	static int old_TX;
	static int old_GPS;
};

#endif /* MAIN_TARGETMANAGER_H_ */
