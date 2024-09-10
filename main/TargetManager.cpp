/*
 * TargetManager.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "TargetManager.h"
#include "inttypes.h"
#include "Flarm.h"
#include "Buzzer.h"
#include "Colors.h"
#include "vector.h"
#include "Switch.h"
#include "SetupMenu.h"

std::map< unsigned int, Target> TargetManager::targets;
std::map< unsigned int, Target>::iterator TargetManager::id_iter = targets.begin();
extern AdaptUGC *egl;
float TargetManager::oldN   = -1.0;
int TargetManager::old_TX   = -1;
int TargetManager::old_GPS  = -1;
int TargetManager::id_timer =  0;
int TargetManager::_tick =  0;
int TargetManager::holddown =  0;
TaskHandle_t TargetManager::pid = 0;
unsigned int TargetManager::min_id = 0;
bool TargetManager::redrawNeeded = true;
int  TargetManager::old_error = 0;
int  TargetManager::old_severity = 0;
int TargetManager::old_sw_len = 0;
int TargetManager::old_hw_len = 0;
int TargetManager::old_obst_len = 0;
int TargetManager::old_prog = 0;

#define TASK_PERIOD 100

void TargetManager::begin(){
	xTaskCreatePinnedToCore(&taskTargetMgr, "taskTargetMgr", 4096, NULL, 13, &pid, 0);
}

void TargetManager::taskTargetMgr(void *pvParameters){
	while(1){
		if( !SetupMenu::isActive() ){
			tick();
		}
		delay(TASK_PERIOD);
	}
}

TargetManager::TargetManager() {
	// TODO Auto-generated constructor stub

}

void TargetManager::receiveTarget( nmea_pflaa_s &pflaa ){
	// ESP_LOGI(FNAME,"ID %06X (dec) %d ", pflaa.ID, pflaa.ID );
	if( (pflaa.groundSpeed < 10) && (display_non_moving_target.get() == NON_MOVE_HIDE) ){
			return;
	}
	if( targets.find(pflaa.ID) == targets.end() ){
		targets[ pflaa.ID ] = Target ( pflaa );
	}
	else
		targets[ pflaa.ID ].update( pflaa );
	targets[ pflaa.ID ].dumpInfo();
}

TargetManager::~TargetManager() {
	// TODO Auto-generated destructor stub
}

void TargetManager::drawN( int x, int y, bool erase, float north ){
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setFontPosCenter();
	egl->setPrintPos( x-SCALE*sin(D2R(north))-5, y-SCALE*cos(D2R(north))+6 );
	egl->setFont(ucg_font_ncenR14_hr);
	if(erase)
		egl->setColor(COLOR_BLACK);
	else
		egl->setColor(COLOR_GREEN);
	egl->print("N");
	oldN = north;
}

void TargetManager::drawAirplane( int x, int y, float north ){
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setColor( COLOR_WHITE );
	egl->drawTetragon( x-15,y-1, x-15,y+1, x+15,y+1, x+15,y-1 );  // wings
	egl->drawTetragon( x-1,y+10, x-1,y-6, x+1,y-6, x+1,y+10 ); // fuselage
	egl->drawTetragon( x-4,y+10, x-4,y+9, x+4,y+9, x+4,y+10 ); // elevator
	egl->setColor(COLOR_GREEN);
	egl->drawCircle( x,y, 25 );
	if( north != oldN ){
		if( oldN != -1.0 )
			drawN( x,y, true, oldN );
		drawN( x,y, false, north );
	}
}

void TargetManager::printAlarm( const char*alarm, int x, int y, int inactive ){
	if( inactive == 0 ){
		egl->setColor(COLOR_RED); // G=0 R=255 B=0  RED Color
	}else{
		egl->setColor(COLOR_BLACK);
	}
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( x, y );
	egl->printf( alarm );
}

/*
<Severity> Decimal integer value. Range: from 0 to 3.
0 = no error, i.e. normal operation. Disregard other parameters.
1 = information only, i.e. normal operation
2 = functionality may be reduced
3 = fatal problem, device will not work
*/

void TargetManager::printAlarmLevel( const char*alarm, int x, int y, int level ){
	if( level == 0 ){
		egl->setColor(COLOR_BLACK); // G=0 R=255 B=0  RED Color
	}else if( level == 1 ){
		egl->setColor(COLOR_GREEN);
	}else if( level == 2 ){
		egl->setColor(COLOR_YELLOW);
	}else if( level == 3 ){
		egl->setColor(COLOR_RED);
	}
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( x, y );
	egl->printf( alarm );
}

void TargetManager::nextTarget(int timer){
	// ESP_LOGI(FNAME,"nextTarget size:%d", targets.size() );
	if( targets.size() ){
		if( ++id_iter == targets.end() )
			id_iter = targets.begin();
		if( (timer == 0) && (id_iter != targets.end()) ){ // move away on first call from closest (displayed per default)
			if( id_iter->first == min_id ){
				if( ++id_iter == targets.end() )
					id_iter = targets.begin();
			}
		}
		if( id_iter != targets.end() )
			ESP_LOGI( FNAME, "next target: %06X", id_iter->first );
	}
}



void TargetManager::tick(){
	float min_dist = 10000;
	_tick++;
	if( holddown )
		holddown--;
	int tx=Flarm::getTXBit();  // 0 or 1
	if( !holddown && Switch::isClosed() ){
		// ESP_LOGI(FNAME,"SW closed");
		nextTarget( id_timer );
		id_timer = 10 * (1000/TASK_PERIOD);
		holddown=5;
	}else{
		if( id_timer )
			id_timer --;
	}
	if( !(_tick%1200) ){
		egl->clearScreen();
		old_TX = -1;
		old_GPS = -1;
		redrawNeeded = true;
	}
	if( !(_tick%5) ){
		if( old_TX != tx){
			ESP_LOGI(FNAME,"TX changed, old: %d, new: %d", old_TX, tx );
			printAlarm( "NO TX", 10, 100, tx );
			old_TX = tx;
		}
		int gps=Flarm::getGPSBit();
		if( old_GPS != gps ){  // 0,1 or 2
			ESP_LOGI(FNAME,"GPS changed, old: %d, new: %d", old_GPS, gps );
			printAlarm( "NO GPS", 10, 120, gps );
			old_GPS = gps;
		}
		int severity =  Flarm::getErrorSeverity();
		int error_code =  Flarm::getErrorCode();
		if( old_error != error_code || old_severity != severity ){
				printAlarmLevel( Flarm::getErrorString(error_code), 10, 140, severity );
				old_error = error_code;
				old_severity = severity;
		}
		int len=strlen( Flarm::getSwVersion() );
		if( len != old_sw_len )
		{
			egl->setColor(COLOR_WHITE);
			egl->setFont(ucg_font_ncenR14_hr);
			egl->setPrintPos( 10, 20 );
			egl->printf( "Flarm SW: %s", Flarm::getSwVersion() );
			old_sw_len = len;
		}
		len=strlen( Flarm::getHwVersion() );
		if( len != old_hw_len )
		{
			egl->setColor(COLOR_WHITE);
			egl->setFont(ucg_font_ncenR14_hr);
			egl->setPrintPos( 10, 40 );
			egl->printf( "Flarm HW: %s", Flarm::getHwVersion() );
			old_hw_len = len;
		}
		len=strlen( Flarm::getObstVersion() );
		if( len != old_obst_len )
		{
			egl->setColor(COLOR_WHITE);
			egl->setFont(ucg_font_ncenR14_hr);
			egl->setPrintPos( 10, 60 );
			egl->printf( "Flarm Obst: %s", Flarm::getObstVersion() );
			old_obst_len = len;
		}
		unsigned int prog = Flarm::getProgress();
		if( prog != old_prog ){
			egl->setColor(COLOR_WHITE);
			egl->setFont(ucg_font_ncenR14_hr);
			egl->setPrintPos( 10, 60 );
			egl->printf( "%s: %d %%", Flarm::getOperationString(), prog );
			old_prog = prog;
		}
		drawAirplane( DISPLAY_W/2,DISPLAY_H/2, Flarm::getGndCourse() );

		// Pass one: determine proximity
		for (auto it=targets.begin(); it!=targets.end(); ){
			it->second.ageTarget();
			it->second.nearest(false);
			if( it->second.getAge() > 35 ){
				ESP_LOGI(FNAME,"ID %06X, ERASE from ageout", it->first );
				it->second.draw();
				if( id_iter->first == it->first ){
					id_iter++;
				}
				targets.erase( it++ );
			}else{
				if( it->second.haveAlarm() )
					id_timer=0;
				if( !id_timer ){
					if( (it->second.getProximity() < min_dist)  ){
						min_dist = it->second.getDist();
						min_id = it->first;
						it->second.nearest(true);
					}else{
						it->second.nearest(false);
					}
				}else{
					if( id_iter != targets.end() && it->first == id_iter->first ){
						it->second.nearest(true);
					}else{
						it->second.nearest(false);
					}
				}
				++it;
			}
			//		ESP_LOGI(FNAME,"ID %06X, AGE: %d ", it->first, it->second.getAge() );
		}
		// Pass 2, draw targets
		for (auto it=targets.begin(); it!=targets.end(); it++ ){
			if( !id_timer )
			{
				if( it->first == min_id ){
					it->second.nearest(true);
				}else{
					it->second.nearest(false);
				}
			}
			if( it->second.getAge() < 30 ){
				if( it->second.isNearest() || it->second.haveAlarm() ){
					it->second.draw();       // closest == true
					if( redrawNeeded ){
						it->second.redrawInfo(); // forced redraw of all fields
						redrawNeeded = false;
					}
					else{
						it->second.drawInfo();
					}
				}
				else{
					it->second.draw();
				}
				// it->second.dumpInfo();
				it->second.checkClose();
			}else{
				if( it->second.isNearest() ){   // why only nearest here ?
					it->second.drawInfo(true); // erase == true
					it->second.draw();     // age/erase
				}
			}
		}
	}
}
