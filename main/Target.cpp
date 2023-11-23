/*
 * Target.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Target.h"
#include <algorithm>
#include <AdaptUGC.h>
#include "vector.h"

extern AdaptUGC *egl;


Target::Target() {
	ESP_LOGI(FNAME,"Target DAFAULT constructor");
}

void Target::drawInfo(bool erase){
	// ESP_LOGI(FNAME,"ID %06X, drawInfo, erase=%d", pflaa.ID, erase );
	if( pflaa.ID == 0 )
		return;
	egl->setFont( ucg_font_fub20_hf );
	if( erase )
		egl->setColor(0, 0, 0 );
	else
		egl->setColor( 255, 255, 255 );
	egl->setPrintPos( 200, 170 );
	egl->printf("%06X ", pflaa.ID );
	egl->setPrintPos( 200, 30 );
	egl->printf("%.2f km  ", dist );
	egl->setPrintPos( 5, 170 );
	if( pflaa.relVertical > 0 )
		egl->printf("+%d m  ", pflaa.relVertical );
	else
		egl->printf("%d m  ", pflaa.relVertical );
	egl->setPrintPos( 10, 30 );
	if( pflaa.climbRate > 0 )
		egl->printf("+%.1f m/s  ", pflaa.climbRate);
	else
		egl->printf("%.1f m/s  ", pflaa.climbRate);
}

void Target::checkClose(){
	// ESP_LOGI(FNAME,"ID %06X, close Target Buzzer dist=%.2f Holddown= %d", pflaa.ID, dist, _buzzedHoldDown );
	if( dist < 2.0 && (_buzzedHoldDown == 0) ){
		ESP_LOGI(FNAME,"BUZZ dist=%.2f", dist );
		Buzzer::play2( BUZZ_DH, 200,100, BUZZ_E, 200, 100 );
		_buzzedHoldDown = 300;
	}
}

#define SCALE 40
// Transform to heading from ground track
void Target::recalc(){
	age = 0;
	// float a=Vector::normalizeDeg180(Flarm::getGndCourse());
	rel_target_heading = Vector::angleDiffDeg( (float)pflaa.track, Flarm::getGndCourse() );
	float relE=float(pflaa.relEast)/1000.0;   // comes in meters
	float relN=float(pflaa.relNorth)/1000.0;
	float target_dir = Vector::normalizeDeg180( R2D(atan2( relE, relN )) );
	rel_target_dir = Vector::angleDiffDeg( target_dir, Flarm::getGndCourse() );
	dist = sqrt( relN*relN + relE*relE ); // distance in km float
	float relV=float(pflaa.relVertical/1000.0);
	prox=sqrt( relV*relV + dist*dist );
	float f=1.0;
	if( dist*SCALE < 40 ){
		float d = dist*SCALE < 1.0 ? 1.0 : dist;
		f=40/(d*SCALE);
	}
	x=160+(dist*f*SCALE)*sin(D2R(rel_target_dir));
	y=86-(dist*f*SCALE)*cos(D2R(rel_target_dir));
	// ESP_LOGI(FNAME,"recalc ID: %06X, own heading:%d targ-head:%d rel-target-head:%d (N:%.2f, E:%.2f) x:%d y:%d", pflaa.ID, int(Flarm::getGndCourse()), int(rel_target_heading), int(rel_target_dir) , relN, relE, x, y ) ;
}

void Target::drawFlarmTarget( int ax, int ay, float bearing, int sideLength, bool erase, bool closest ){
	// ESP_LOGI(FNAME,"drawFlarmTarget (ID: %06X): x:%d, y:%d, bear:%.1f, len:%d, ers:%d", pflaa.ID, ax,ay,bearing, sideLength, erase );
	float radians = (bearing-90.0) * M_PI / 180;
	// Calculate the triangle's vertices
	int x0 = ax + sideLength * cos(radians);
	int y0 = ay + sideLength * sin(radians);
	int x1 = ax + sideLength/2 * cos(radians + 2 * M_PI / 3);
	int y1 = ay + sideLength/2 * sin(radians + 2 * M_PI / 3);
	int x2 = ax + sideLength/2 * cos(radians - 2 * M_PI / 3);
	int y2 = ay + sideLength/2 * sin(radians - 2 * M_PI / 3);
	egl->drawTriangle( x0,y0,x1,y1,x2,y2 );
	if( closest )
		egl->drawCircle( ax,ay, sideLength );
	if( !erase ){
		// ESP_LOGI(FNAME,"drawFlarmTarget (ID: %06X): x:%d, y:%d, bear:%.1f, len:%d, ers:%d", pflaa.ID, ax,ay,bearing, sideLength, erase );
		old_x = ax;
		old_y = ay;
		old_size = sideLength;
		old_track = bearing;
		old_closest = closest;
	}
}

void Target::checkAlarm(){
	if( pflaa.alarmLevel == 1 ){
		Buzzer::play2( BUZZ_DH, 150,100, BUZZ_DH, 150, 0, 3 );
	}else if( pflaa.alarmLevel == 2 ){
		Buzzer::play2( BUZZ_E, 100,100, BUZZ_E, 100, 0, 5 );
	}else if( pflaa.alarmLevel == 3 ){
		Buzzer::play2( BUZZ_F, 70,100, BUZZ_F, 70, 0, 7 );
	}
}


void Target::draw( bool closest ){
	checkAlarm();
	int size = std::min( 30.0, std::min( 80.0, 10.0+10.0/dist )  );
	if( old_x != -1000 && x != -1000 ){
		// ESP_LOGI(FNAME,"drawFlarmTarget() erase old x:%d old_x:%d", x, old_x );
		egl->setColor( 0, 0, 0 );   // BLACK
		drawFlarmTarget( old_x, old_y, old_track, old_size, true, old_closest );
	}
	if( age < 30 ){
		int highlight = 0;
		if( closest )
			highlight = 0;
		if( (dist < 1.0) && sameAlt() ){
			egl->setColor( 255, highlight, highlight );
		}
		else{
			egl->setColor( highlight, 255, highlight );
		}
		if( x > 0 && x < 320 && y > 0 && y < 172 ){
			// ESP_LOGI(FNAME,"drawFlarmTarget() draw %06X: N/y:%d E/x:%d", pflaa.ID, x,y );
			ESP_LOGI(FNAME,"drawFlarmTarget() ID:%06X, heading:%d, target-heading:%d, rel-targ-head:%d rel-targ-dir:%d dist:%.2f", pflaa.ID, int(Flarm::getGndCourse()), int(pflaa.track),  int(rel_target_heading), (int)rel_target_dir, dist );
			drawFlarmTarget( x, y, rel_target_heading, size, false, closest );
		}
	}
}

Target::Target( nmea_pflaa_s a_pflaa ) {
	pflaa = a_pflaa;
	old_x=-1000;
	old_y=-1000;
	old_closest=false;
	old_track = 0;
	_buzzedHoldDown = 0;
	dist=10000.0;
	prox=10000.0;
	// ESP_LOGI(FNAME,"Target (ID %06X) Creation()", pflaa.ID );
	recalc();
}

void Target::update( nmea_pflaa_s a_pflaa ){
	pflaa = a_pflaa;
	// ESP_LOGI(FNAME,"Target (ID %06X) update()", pflaa.ID );
	recalc();
}

void Target::ageTarget(){
	if( age < 1000 )
		age++;
	if( _buzzedHoldDown )
		_buzzedHoldDown--;
}

void Target::dumpInfo(){
	ESP_LOGI(FNAME,"Target (ID %06X) age:%d alt:%d m, dis:%.2lf km, var:%.1f m/s, trck:%d", pflaa.ID, age, pflaa.relVertical, dist, pflaa.climbRate, pflaa.track );
}

Target::~Target() {
	// TODO Auto-generated destructor stub
}

