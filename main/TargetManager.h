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
	static void begin();
private:
	static std::map< unsigned int, Target> targets;
	static std::map< unsigned int, Target>::iterator id_iter;
	static float oldN;
	static void drawN( int x, int y, bool erase, float north, float azoom );
	static void printAlarm( const char*alarm, int x, int y, int inactive );
	static void nextTarget(int timer);
	static void taskTargetMgr(void *pvParameters);
	static int old_TX;
	static int old_GPS;
	static int id_timer;
	static int _tick;
	static int holddown;
	static TaskHandle_t pid;
	static unsigned int min_id;
	static float old_radius;
};

#endif /* MAIN_TARGETMANAGER_H_ */
