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


char Target::cur_dist[32] = { 0 };
char Target::cur_alt[32] = { 0 };
char Target::cur_id[32]= { 0 };
char Target::cur_var[32]= { 0 };

int Target::old_dist = -10000;
unsigned int Target::old_alt  = 100000;
unsigned int Target::old_id = 0;
int Target::old_var  = -10000.0;
int Target::blink  = 0;


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

void Target::drawDist( uint8_t r, uint8_t g, uint8_t b ){
	egl->setColor(r,g,b);
	egl->setFont( ucg_font_fub20_hf );  // big letters are a bit spacey
	int w=egl->getStrWidth(cur_dist);
	egl->setPrintPos( (DISPLAY_W-5)-w, 30 );
	egl->printf("%s", cur_dist );
}

void Target::drawID( uint8_t r, uint8_t g, uint8_t b ){
	egl->setColor(r,g,b);
	egl->setFont( ucg_font_fub20_hf );
	int w=egl->getStrWidth(cur_id);
	egl->setPrintPos( (DISPLAY_W-5)-w, DISPLAY_H-7 );
	egl->printf("%s",cur_id);
}

void Target::drawAlt( uint8_t r, uint8_t g, uint8_t b ){
	egl->setColor(r,g,b);
	egl->setFont( ucg_font_fub20_hf );
	egl->setPrintPos( 5, DISPLAY_H-7 );
	egl->printf("%s", cur_alt );
}

void Target::drawVar( uint8_t r, uint8_t g, uint8_t b ){
	egl->setColor(r,g,b);
	egl->setPrintPos( 5, 30 );
	egl->setFont( ucg_font_fub20_hf );
	egl->printf("%s", cur_var);
}

void Target::drawInfo(bool erase){
	char s[32]= { 0 };
	ESP_LOGI(FNAME,"ID %06X, drawInfo, erase=%d", pflaa.ID, erase );
	if( pflaa.ID == 0 )
		return;

	// distance info
	if( (old_dist != (int)(dist*100)) | erase ){
		if( strlen( cur_dist ) )
			drawDist( COLOR_BLACK );  // erase
		if( !erase ){
			sprintf(cur_dist,"%.2f", Units::Distance( dist ) );
			drawDist(COLOR_WHITE);
			old_dist = (int)(dist*100);
		}
	}

	// Flarm ID right down
	if( (old_id != pflaa.ID) | erase ){
		if( strlen( cur_id ) ){
			drawID( COLOR_BLACK );  // erase
			old_id = 0;
		}
		if( !erase ){
			if( reg ){
				if( comp )
					sprintf(cur_id,"%s %s", reg, comp );
				else
					sprintf(cur_id,"%s", reg );
			}else{
				sprintf(cur_id,"%06X", pflaa.ID );
			}
			drawID( COLOR_WHITE );
			old_id = pflaa.ID;
		}
	}

	// relative vertical
	if( (old_alt != pflaa.relVertical) | erase ){
		if( strlen( cur_alt ) )
			drawAlt( COLOR_BLACK );  // erase
		if( !erase ){
			int alt = (int)(Units::Altitude( (pflaa.relVertical)+0.5));
			if( pflaa.relVertical > 0 )
				sprintf(cur_alt,"+%d", alt );
			else
				sprintf(cur_alt,"%d", alt );
			drawAlt( COLOR_WHITE );
			old_alt = pflaa.relVertical;
		}
	}

	// climb rate
	if( (old_var != (int)(pflaa.climbRate*10)) | erase ){
		if( strlen( cur_var ) )
			drawVar( COLOR_BLACK );  // erase
		if( !erase ){
			float climb = Units::Vario( (float)pflaa.climbRate );
			if( climb > 0 )
				sprintf(cur_var,"+%.1f", climb );
			else
				sprintf(cur_var,"%.1f", climb );
			drawVar( COLOR_WHITE );
			old_var = (int)(pflaa.climbRate*10);
		}
	}

	// Units
	if( !erase )
		egl->setColor( COLOR_BLUE );
	else
		egl->setColor( COLOR_BLACK );

	egl->setFont( ucg_font_fub14_hf );

	if( inch2dot4 ){
		int w=egl->getStrWidth("ID");
		egl->setPrintPos( (DISPLAY_W-5)-w, DISPLAY_H-37 );
		egl->printf("ID");
	}

	if( inch2dot4 ){
		sprintf(s,"Dis %s", Units::DistanceUnit() );
		int w=egl->getStrWidth(s);
		egl->setPrintPos( (DISPLAY_W-5)-w, 50 );
		egl->printf("%s",s);
	}
	else{
		sprintf(s,"%s ", Units::DistanceUnit() );
		int w=egl->getStrWidth(s);
		egl->setPrintPos( DISPLAY_W-5-w, 50 );
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
		egl->setPrintPos( 5, DISPLAY_H-37 );
		egl->printf("%s ", Units::AltitudeUnit() );
	}

}

void Target::checkClose(){
	// ESP_LOGI(FNAME,"ID %06X, close Target Buzzer dist=%.2f Holddown= %d", pflaa.ID, dist, _buzzedHoldDown );
	if( dist < 2.0 && (_buzzedHoldDown == 0) ){
		ESP_LOGI(FNAME,"BUZZ dist=%.2f", dist );
		Buzzer::play2( BUZZ_DH, 200,audio_volume.get() , BUZZ_E, 200, audio_volume.get() );
		_buzzedHoldDown = 12000;
	}else if( dist > 3.0 ){
		_buzzedHoldDown = 0;
	}
}

// Transform to heading from ground track
void Target::recalc(){
	rel_target_heading = Vector::angleDiffDeg( (float)pflaa.track, Flarm::getGndCourse() );
	rel_target_dir = Vector::angleDiffDeg( R2D(atan2( pflaa.relEast, pflaa.relNorth )), Flarm::getGndCourse() );
	dist = sqrt( pflaa.relNorth*pflaa.relNorth + pflaa.relEast*pflaa.relEast )/1000.0; // distance in km float
	float relV=float(pflaa.relVertical/1000.0);
	prox=sqrt( relV*relV + dist*dist );  // proximity 3D
	float logs = dist;
	if( log_scale.get() )
		logs = log( 2+dist );
	float pix = fmax( zoom*logs*SCALE, 20.0 );
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

void Target::draw(){
	checkAlarm();
	int size = std::min( 30.0, std::min( 80.0, 10.0+10.0/dist )  );
	if( old_x != -1000  ){  // delete target if it was drawn before
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
	age=0;
}

void Target::ageTarget(){
	if( age < 1000 )
		age++;
	if( _buzzedHoldDown )
		_buzzedHoldDown--;
	recalc();
}

void Target::dumpInfo(){
	// ESP_LOGI(FNAME,"Target (ID %06X) age:%d alt:%d m, dis:%.2lf km, var:%.1f m/s, trck:%d", pflaa.ID, age, pflaa.relVertical, dist, pflaa.climbRate, pflaa.track );
}

Target::~Target() {
	// TODO Auto-generated destructor stub
}

