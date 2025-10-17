/*
 * TargetManager.h
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Flarm.h"
#include <map>
#include "Target.h"
#include "Switch.h"
#include <mutex>

#ifndef MAIN_TARGETMANAGER_H_
#define MAIN_TARGETMANAGER_H_


class TargetManager: public SwitchObserver {
public:
	TargetManager();
	~TargetManager();
	static void receiveTarget( const nmea_pflaa_s &target );
	static void tick();
	static void drawAirplane( int x, int y, float north=0.0 );
	void begin();
	inline static void redrawInfo() { redrawNeeded = true; };
    void release() {};
	void longPress() {};
	void press();
	void up(int n) {};
	void down(int n) {};
	void longLongPress();
	static void handleFlarmFlags();
	static void handleFlarmAlarms();
	static void handleFlarmVersions();
	static void handleProgress();
	void updateTargets(float &min_dist, float &max_climb);
	void drawTargets(float min_dist, float max_climb);

private:
	static TargetManager* instance;
	static std::map< unsigned int, Target> targets;
	static std::mutex targets_mutex;
	static std::map< unsigned int, Target>::iterator id_iter;
	static float oldN;
	static void drawN( int x, int y, bool erase, float north, float azoom );
	static void printAlarm( const char*alarm, int x, int y, bool print, ucg_color_t color={ COLOR_RED } );
	static void printAlarmLevel( const char*alarm, int x, int y, int level );
	static void printRX();
	static void nextTarget(int timer);
	static void taskTargetMgr(void *pvParameters);
	static void printVersions( int x, int y, const char *prefix, const char *ver, int len );
	static void clearScreen();
	static void rewindInfoTimer();
	static int id_timer;
	static int _tick;
	static int holddown;
	static TaskHandle_t pid;
	static unsigned int min_id;
	static unsigned int maxcl_id;
	static bool redrawNeeded;
	static bool erase_info;
	static int info_timer;
	static float old_radius;
	static unsigned int team_id;
	static Target* theInfoTarget;
};

#endif /* MAIN_TARGETMANAGER_H_ */
