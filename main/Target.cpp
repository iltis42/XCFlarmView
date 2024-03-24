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
#include "flarmnetdata.h"
#include "Colors.h"
#include <cmath>
#include <algorithm>
#include <flarmview.h>


extern AdaptUGC *egl;


Target::Target() {
	ESP_LOGI(FNAME,"Target DAFAULT constructor");
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
	reg=0;
	comp=0;
	is_nearest=false;
	for( int i=0; i<(sizeof(flarmnet)/sizeof(flarmnet[0])); i++){
		if( pflaa.ID == flarmnet[i].flarmID ){
			reg=(char*)flarmnet[i].reg;
			comp=(char*)flarmnet[i].comp;
		}
	}
}


void Target::drawInfo(bool erase){
	char s[32];
	int w=0;
	// ESP_LOGI(FNAME,"ID %06X, drawInfo, erase=%d", pflaa.ID, erase );
	if( pflaa.ID == 0 )
		return;

	if( erase )
		egl->setColor(COLOR_BLACK);
	else
		egl->setColor(COLOR_WHITE);

	egl->setFont( ucg_font_fub20_hf ); // big letters are a bit spacey
	// Flarm ID right down
	if( reg ){
		if( comp )
			sprintf(s,"   %s %s", reg, comp );
		else
			sprintf(s,"      %s", reg );
	}else{
		sprintf(s,"      %06X", pflaa.ID );
	}
	w=egl->getStrWidth(s);
	egl->setPrintPos( (DISPLAY_W-10)-w, DISPLAY_H-7 );
	egl->printf("%s",s);

	egl->setFont( ucg_font_fub25_hf );
	// Distance right upper corner, constant dist to right end
	sprintf(s,"  %.2f   ", Units::Distance( dist ) );
	w=egl->getStrWidth(s);
	egl->setPrintPos( (DISPLAY_W-2)-w, 30 );
	egl->printf("%s", s );

	// relative vertical
	egl->setPrintPos( 5, DISPLAY_H-7 );
	int alt = (int)(Units::Altitude( (float)pflaa.relVertical)+0.5);
	if( pflaa.relVertical > 0 )
		egl->printf("+%d    ", alt );
	else
		egl->printf("%d    ", alt );


	egl->setPrintPos( 5, 30 );
	// climb rate
	float climb = Units::Vario( (float)pflaa.climbRate );
	if( climb > 0 )
		sprintf(s,"+%.1f  ", climb );
	else
		sprintf(s," %.1f  ", climb );
	egl->printf("%s", s);

	// Units
	if( !erase )
		egl->setColor( COLOR_BLUE );
	egl->setFont( ucg_font_fub14_hf );

	if( inch2dot4 ){
		egl->setPrintPos( DISPLAY_W-40, DISPLAY_H-37 );
		egl->printf("ID");
	}

	if( inch2dot4 ){
		sprintf(s,"  Dis %s ", Units::DistanceUnit() );
		w=egl->getStrWidth(s);
		egl->setPrintPos( (DISPLAY_W-10)-w, 50 );
		egl->printf("%s",s);
	}
	else{
		egl->setPrintPos( DISPLAY_W-65, 50 );
		egl->printf("%s ", Units::DistanceUnit() );
	}

	egl->setPrintPos( 5, 50 );
	if( inch2dot4 )
		egl->printf("Var %s ", Units::VarioUnit() );
	else
		egl->printf("%s ", Units::VarioUnit() );


	if( inch2dot4 ){
		egl->setPrintPos( 5, DISPLAY_H-37 );
		egl->printf("Alt %s ", Units::AltitudeUnit() );
	}
	else{
		egl->setPrintPos( 25, DISPLAY_H-37 );
		egl->printf("%s ", Units::AltitudeUnit() );
	}
}

void Target::checkClose(){
	// ESP_LOGI(FNAME,"ID %06X, close Target Buzzer dist=%.2f Holddown= %d", pflaa.ID, dist, _buzzedHoldDown );
	if( dist < 2.0 && (_buzzedHoldDown == 0) ){
		ESP_LOGI(FNAME,"BUZZ dist=%.2f", dist );
		Buzzer::play2( BUZZ_DH, 200,audio_volume.get() , BUZZ_E, 200, audio_volume.get() );
		_buzzedHoldDown = 300;
	}
}


// Transform to heading from ground track
void Target::recalc(){
	age = 0;  // reset age
	rel_target_heading = Vector::angleDiffDeg( (float)pflaa.track, Flarm::getGndCourse() );
	rel_target_dir = Vector::angleDiffDeg( R2D(atan2( pflaa.relEast, pflaa.relNorth )), Flarm::getGndCourse() );
	dist = sqrt( pflaa.relNorth*pflaa.relNorth + pflaa.relEast*pflaa.relEast )/1000.0; // distance in km float
	float relV=float(pflaa.relVertical/1000.0);
	prox=sqrt( relV*relV + dist*dist );
	float logs = log( 2+prox );
	float pix = fmax( logs*SCALE, 30.0 );
	// ESP_LOGI(FNAME,"prox: %f, log:%f, pix:%f", prox, logs, pix );
	x=(DISPLAY_W/2)+pix*sin(D2R(rel_target_dir));
	y=(DISPLAY_H/2)-pix*cos(D2R(rel_target_dir));
	// ESP_LOGI(FNAME,"recalc ID: %06X, own heading:%d targ-head:%d rel-target-head:%d (N:%.2f, E:%.2f) x:%d y:%d", pflaa.ID, int(Flarm::getGndCourse()), int(rel_target_heading), int(rel_target_dir) , pflaa.relNorth, pflaa.relEast, x, y ) ;
}

void Target::drawFlarmTarget( int ax, int ay, float bearing, int sideLength, bool erase, bool closest ){
	// ESP_LOGI(FNAME,"drawFlarmTarget (ID: %06X): x:%d, y:%d, bear:%.1f, len:%d, ers:%d", pflaa.ID, ax,ay,bearing, sideLength, erase );
	float radians = D2R(bearing-90.0);
	float axt=ax-sideLength/4*sin(D2R(bearing));  // offset the Triangle to center of gravity (and circle)
	float ayt=ay+sideLength/4*cos(D2R(bearing));
	// Calculate the triangle's vertices
	int x0 = axt + sideLength * cos(radians);   // arrow head
	int y0 = ayt + sideLength * sin(radians);
	int x1 = axt + sideLength/2 * cos(radians + 2 * M_PI / 3);  // base left
	int y1 = ayt + sideLength/2 * sin(radians + 2 * M_PI / 3);
	int x2 = axt + sideLength/2 * cos(radians - 2 * M_PI / 3);  // base right
	int y2 = ayt + sideLength/2 * sin(radians - 2 * M_PI / 3);
	egl->drawTriangle( x0,y0,x1,y1,x2,y2 );
	if( closest ){
		egl->drawCircle( ax,ay, int( sideLength*0.75 ) );
	}
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
		Buzzer::play2( BUZZ_DH, 150,audio_volume.get(), BUZZ_DH, 150, 0, 2 );
	}else if( pflaa.alarmLevel == 2 ){
		Buzzer::play2( BUZZ_E, 100,audio_volume.get(), BUZZ_E, 100, 0, 3 );
	}else if( pflaa.alarmLevel == 3 ){
		Buzzer::play2( BUZZ_F, 70,audio_volume.get(), BUZZ_F, 70, 0, 5 );
	}
}

int blink = 0;

void Target::draw(){
	checkAlarm();
	int size = std::min( 30.0, std::min( 80.0, 10.0+10.0/dist )  );
	if( old_x != -1000 && x != -1000 ){
		// ESP_LOGI(FNAME,"drawFlarmTarget() erase old x:%d old_x:%d", x, old_x );
		egl->setColor( COLOR_BLACK );   // BLACK
		drawFlarmTarget( old_x, old_y, old_track, old_size, true, old_closest );
		old_x = -1000;
	}
	if( age < 30 && ( (display_mode.get() == DISPLAY_MULTI) || ((display_mode.get() == DISPLAY_SIMPLE) && isNearest() )) ){
		int brightness=int(255.0 - 255.0 * std::min(1.0, (age/30.0)) ); // fade out with growing age
		if( (dist < 1.0) && sameAlt() ){
			if( haveAlarm() ){
				if( !(blink%2) )
					egl->setColor( brightness, brightness, brightness ); // white
				else
					egl->setColor( 0, 0, brightness );  // red
				blink++;
			}else{
				egl->setColor( brightness, brightness, brightness ); // white
			}
		}
		else{
			egl->setColor( 0, brightness, 0 ); // green
		}
		if( x > 0 && x < DISPLAY_W && y > 0 && y < DISPLAY_H ){
			// ESP_LOGI(FNAME,"drawFlarmTarget() ID:%06X, heading:%d, target-heading:%d, rel-targ-head:%d rel-targ-dir:%d dist:%.2f", pflaa.ID, int(Flarm::getGndCourse()), int(pflaa.track),  int(rel_target_heading), (int)rel_target_dir, dist );
			drawFlarmTarget( x, y, rel_target_heading, size, false, is_nearest );
		}
	}
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
	// ESP_LOGI(FNAME,"Target (ID %06X) age:%d alt:%d m, dis:%.2lf km, var:%.1f m/s, trck:%d", pflaa.ID, age, pflaa.relVertical, dist, pflaa.climbRate, pflaa.track );
}

Target::~Target() {
	// TODO Auto-generated destructor stub
}

