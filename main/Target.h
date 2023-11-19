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
	void dumpInfo();
	void drawInfo(bool erase=false);
	void draw( bool closest=false );
	void drawFlarmTarget( int x, int y, float bearing, int sideLength, bool erase=false, bool closest=false );
	void checkClose();

private:

	nmea_pflaa_s pflaa;
	int age;
	int _buzzedHoldDown;
	float dist;
	int x,y,old_x, old_y, old_size, old_track;
	void recalc();
};

#endif /* MAIN_TARGET_H_ */
