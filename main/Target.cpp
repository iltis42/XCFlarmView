/*
 * Target.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Target.h"
#include <algorithm>
#include <AdaptUGC.h>

extern AdaptUGC *egl;


Target::Target() {
	// TODO Auto-generated constructor stub
	x=-1;
	y=-1;
	old_x=-1000;
	old_y=-1000;
	dist = 100.0;
	memset( &pflaa, 0, sizeof( pflaa ) );
}

void Target::drawInfo(bool erase){
	if( pflaa.ID == 0 )
		return;
	egl->setFont( ucg_font_fub20_hf );
	if( erase )
		egl->setColor(0, 0, 0 );
	else
		egl->setColor( 255, 255, 255 );
	egl->setColor( 255, 255, 255 );
	egl->setPrintPos( 200, 170 );
	egl->printf("%06X ", pflaa.ID );
	egl->setPrintPos( 200, 30 );
	egl->printf("%.2f km  ", dist );
	egl->setPrintPos( 5, 170 );
	egl->printf("%d m  ", pflaa.relVertical );
	egl->setPrintPos( 10, 30 );
	egl->printf("%.1f m/s  ", pflaa.climbRate);
}


void Target::recalc(){
	age = 0;
	dist = sqrt( pflaa.relNorth*pflaa.relNorth + pflaa.relEast*pflaa.relEast )/1000.0; // distance in km float
	x=160+(pflaa.relEast/20);
	y=86-(pflaa.relNorth/20);
}

void Target::drawFlarmTarget( int x, int y, float bearing, int sideLength, bool erase, bool closest ){
	ESP_LOGI(FNAME,"draw x:%d, y:%d, bear:%.1f, len:%d, ers:%d", x,y,bearing, sideLength, erase );
	float radians = (bearing-90.0) * M_PI / 180;
	// Calculate the triangle's vertices
	int x0 = x + sideLength * cos(radians);
	int y0 = y + sideLength * sin(radians);
	int x1 = x + sideLength/2 * cos(radians + 2 * M_PI / 3);
	int y1 = y + sideLength/2 * sin(radians + 2 * M_PI / 3);
	int x2 = x + sideLength/2 * cos(radians - 2 * M_PI / 3);
	int y2 = y + sideLength/2 * sin(radians - 2 * M_PI / 3);
	egl->drawTriangle( x0,y0,x1,y1,x2,y2 );
	if( closest )
		egl->drawCircle( x,y, sideLength );
	if( !erase ){
		old_x = x;
		old_y = y;
		old_size = sideLength;
		old_track = bearing;
	}else{
		old_x = -1000;
	}
}


void Target::draw( bool closest ){
	int size = int( std::min( 50.0, 10.0+10.0/dist  ) );
	if( old_x != -1000 || age > 30 ){
		egl->setColor( 0, 0, 0 );   // BLACK
		drawFlarmTarget( old_x, old_y, old_track, old_size, true, closest );
	}
	if( age < 30 ){
		int highlight = 0;
		if( closest )
			highlight = 0;
		if( dist < 1.0 ){
			egl->setColor( 255, highlight, highlight );
		}
		else{
			egl->setColor( highlight, 255, highlight );
		}
		if( x > 0 && x < 320 && y > 0 && y < 172 )
			drawFlarmTarget( x, y, (float)pflaa.track, size, false, closest );
	}
}

Target::Target( nmea_pflaa_s a_pflaa ) {
	ESP_LOGI(FNAME,"Target (ID %06X) Creation()", pflaa.ID );
	pflaa = a_pflaa;
	recalc();
}

void Target::update( nmea_pflaa_s a_pflaa ){
	// ESP_LOGI(FNAME,"Target (ID %06X) update()", pflaa.ID );
	pflaa = a_pflaa;
	recalc();
}

void Target::ageTarget(){
	if( age < 1000 )
		age++;
}

void Target::dumpInfo(){
	ESP_LOGI(FNAME,"Target (ID %06X) age:%d alt:%d m, dis:%.2lf km, var:%.1f m/s, trck:%d", pflaa.ID, age, pflaa.relVertical, dist, pflaa.climbRate, pflaa.track );
}

Target::~Target() {
	// TODO Auto-generated destructor stub
}

