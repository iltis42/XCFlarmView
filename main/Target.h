/*
 * Target.h
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Flarm.h"
#include "Buzzer.h"

#ifndef MAIN_TARGET_H_
#define MAIN_TARGET_H_

class Target {
public:
	Target();
	Target( nmea_pflaa_s a_pflaa );
	virtual ~Target();
	void ageTarget();
	void update( nmea_pflaa_s a_pflaa );
	inline int getAge() { return age; };
	inline int getID() { return pflaa.ID; };
	inline float getDist() { return dist; };
	inline float getProximity() { return prox; };
	void dumpInfo();
	void drawInfo(bool erase=false);
	void draw( bool closest=false );
	void checkClose();
	inline bool haveAlarm(){ return pflaa.alarmLevel != 0; };
	inline bool sameAlt( uint tolerance=150 ) { return( abs( pflaa.relVertical )< tolerance ); };

private:
	void checkAlarm();
	void drawFlarmTarget( int x, int y, float bearing, int sideLength, bool erase=false, bool closest=false );
	nmea_pflaa_s pflaa;
	int age;
	int _buzzedHoldDown;
	float rel_target_heading;
	float rel_target_dir;
	float dist, prox, old_track;
	int x,y,old_x, old_y, old_size, old_closest;
	void recalc();
};

#endif /* MAIN_TARGET_H_ */
