/*
 * Target.h
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Flarm.h"
#include "Buzzer.h"
#include "Colors.h"

#ifndef MAIN_TARGET_H_
#define MAIN_TARGET_H_


#define SCALE 30  // 1km @ zoom = 1
#define TASKPERIOD 50  // ms
#define DISPLAYTICK  5  // all 5 ticks = 250 mS
                               //  5   * 50  = 250 mS -> 1000 / 250 = 4
#define AGEOUT (30*((1000/((DISPLAYTICK*TASKPERIOD)))))  // 15 seconds

class Target {
public:
	Target();
	Target( nmea_pflaa_s a_pflaa );
	virtual ~Target();
	void ageTarget();
	void update( nmea_pflaa_s a_pflaa );
	inline int getAge() { return age; };
	inline int getID() { return pflaa.ID; };
	inline float getClimb(){ return pflaa.climbRate; };
	inline float getDist() { return is_nearest ? dist*0.9 : dist; }; // hysteresis 10%
	inline float getProximity() { return prox; };
	void dumpInfo();
	void drawInfo(bool erase=false);
	void redrawInfo();
	void draw(bool erase);
	void checkClose();
	inline bool haveAlarm(){ return alarm; };
	inline bool sameAlt( uint tolerance=150 ) { return( abs( pflaa.relVertical )< tolerance ); };
	inline void nearest( bool n ) { is_nearest=n; };
	inline void best( bool n ) { is_best=n; };
	inline bool isNearest() { return is_nearest; };
	inline bool isBestClimber() { return is_best; };

private:
	void drawClimb( int x, int y, int size, int climb );
	void checkAlarm();
	void drawFlarmTarget( int x, int y, int bearing, int sideLength, bool erase=false, bool closest=false, ucg_color_t color={ COLOR_GREEN } );
	void drawDist( uint8_t r, uint8_t g, uint8_t b );
	void drawVar( uint8_t r, uint8_t g, uint8_t b );
	void drawAlt( uint8_t r, uint8_t g, uint8_t b );
	void drawID( uint8_t r, uint8_t g, uint8_t b );
	inline void setAlarm(){
		alarm = true;
		alarm_timer = 8;
	};
	nmea_pflaa_s pflaa;
	int age;
	float dist_buzz;
	int _buzzedHoldDown;
	int rel_target_heading;
	float rel_target_dir;
	int old_track;
	float dist, prox;
	int x,y,old_ax, old_ay, old_x0, old_y0, old_x1, old_y1, old_x2, old_y2, old_closest, old_sidelen;
	char * reg;  // registration from flarmnet DB
	char * comp; // competition ID
	void recalc();
	bool is_nearest;
	bool is_best;
	bool alarm;
	int alarm_timer;
	int old_climb;
	int old_x;
	int old_y;
	int old_size;

	static char cur_dist[32];
	static char cur_alt[32];
	static char cur_id[32];
	static char cur_var[32];

	static int old_dist;
	static unsigned int old_alt;
	static unsigned int old_id;
	static int old_var;
	static int blink;
};

#endif /* MAIN_TARGET_H_ */
