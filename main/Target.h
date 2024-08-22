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


#define SCALE 30  // 1km

class Target {
public:
	Target();
	Target( nmea_pflaa_s a_pflaa );
	virtual ~Target();
	void ageTarget();
	void update( nmea_pflaa_s a_pflaa );
	inline int getAge() { return age; };
	inline int getID() { return pflaa.ID; };
	inline float getDist() { return is_nearest ? dist*0.9 : dist; }; // hysteresis 10%
	inline float getProximity() { return prox; };
	void dumpInfo();
	void drawInfo(bool erase=false);
	void redrawInfo();
	void draw();
	void checkClose();
	inline bool haveAlarm(){ return pflaa.alarmLevel != 0; };
	inline bool sameAlt( uint tolerance=150 ) { return( abs( pflaa.relVertical )< tolerance ); };
	inline void nearest( bool n ) { is_nearest=n; };
	inline bool isNearest() { return is_nearest; };

private:
	void checkAlarm();
	void drawFlarmTarget( int x, int y, float bearing, int sideLength, bool erase=false, bool closest=false );
	void drawDist( uint8_t r, uint8_t g, uint8_t b );
	void drawVar( uint8_t r, uint8_t g, uint8_t b );
	void drawAlt( uint8_t r, uint8_t g, uint8_t b );
	void drawID( uint8_t r, uint8_t g, uint8_t b );
	nmea_pflaa_s pflaa;
	int age;
	int _buzzedHoldDown;
	float rel_target_heading;
	float rel_target_dir;
	float dist, prox, old_track;
	int x,y,old_x, old_y, old_size, old_closest;
	char * reg;  // registration from flarmnet DB
	char * comp; // competition ID
	void recalc();
	bool is_nearest;

	static char cur_dist[32];
	static char cur_alt[32];
	static char cur_id[32];
	static char cur_var[32];

	static int old_dist;
	static unsigned int old_alt;
	static unsigned int old_id;
	static int old_var;
};

#endif /* MAIN_TARGET_H_ */
