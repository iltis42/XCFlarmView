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
#include "flarmview.h"

std::map< unsigned int, Target> TargetManager::targets;
std::map< unsigned int, Target>::iterator TargetManager::id_iter = targets.begin();
extern AdaptUGC *egl;
float TargetManager::oldN   = -1.0;
int TargetManager::id_timer =  0;
int TargetManager::_tick =  0;
int TargetManager::holddown =  0;
TaskHandle_t TargetManager::pid = 0;
unsigned int TargetManager::min_id = 0;
bool TargetManager::redrawNeeded = true;
bool TargetManager::erase_info = false;
int TargetManager::info_timer = 0;
float TargetManager::old_radius=0.0;
xSemaphoreHandle display=NULL;

#define INFO_TIME (5*(1000/TASKPERIOD)/DISPLAYTICK)  // all ~10 sec

void TargetManager::begin(){
	display=xSemaphoreCreateMutex();
	xTaskCreatePinnedToCore(&taskTargetMgr, "taskTargetMgr", 4096, NULL, 10, &pid, 0);
}

void TargetManager::taskTargetMgr(void *pvParameters){
	while(1){
		if( !SetupMenu::isActive() ){
			tick();
		}
		delay(TASKPERIOD);
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

void TargetManager::drawN( int x, int y, bool erase, float north, float dist ){
  if( SetupMenu::isActive() )
		return;
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setFontPosCenter();
	egl->setPrintPos( x-dist*sin(D2R(north))-5, y-dist*cos(D2R(north))+6 );
	egl->setFont(ucg_font_ncenR14_hr);
	if(erase)
		egl->setColor(COLOR_BLACK);
	else
		egl->setColor(COLOR_GREEN);
	egl->print("N");
	oldN = north;
}

void TargetManager::drawAirplane( int x, int y, float north ){
	if( SetupMenu::isActive() )
		return;
	xSemaphoreTake(display,portMAX_DELAY );
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setColor( COLOR_WHITE );
	egl->drawTetragon( x-15,y-1, x-15,y+1, x+15,y+1, x+15,y-1 );  // wings
	egl->drawTetragon( x-1,y+10, x-1,y-6, x+1,y-6, x+1,y+10 ); // fuselage
	egl->drawTetragon( x-4,y+10, x-4,y+9, x+4,y+9, x+4,y+10 ); // elevator
	float new_radius = 25;
	if( inch2dot4 ){
		float logs = 1;
		if( log_scale.get() )
			logs = log( 2+1 );
		new_radius = zoom*logs*SCALE;
	}

	if( oldN != -1.0 && ((oldN != north) || (old_radius != new_radius)) )
		drawN( x,y, true, oldN, old_radius );
	if( (old_radius != 0.0) && (old_radius != new_radius) ){
		egl->setColor(COLOR_BLACK);
		egl->drawCircle( x,y, old_radius );
	}
	egl->setColor(COLOR_GREEN);
	drawN( x,y, false, north, new_radius );
	egl->drawCircle( x,y, new_radius );
	old_radius = new_radius;
	xSemaphoreGive(display);
}

void TargetManager::printAlarm( const char*alarm, int x, int y, bool print, ucg_color_t color ){
	// ESP_LOGI(FNAME,"printAlarm: %s, x:%d y:%d", alarm, x,y );
	xSemaphoreTake(display,portMAX_DELAY );
	if( print ){
		egl->setColor( color ); // G=0 R=255 B=0  RED Color
	}else{
		egl->setColor(COLOR_BLACK);
	}
	egl->setPrintPos( x, y );

	egl->setFont(ucg_font_ncenR14_hr);
	egl->printf( alarm );
	xSemaphoreGive(display);
}

/*
<Severity> Decimal integer value. Range: from 0 to 3.
0 = no error, i.e. normal operation. Disregard other parameters.
1 = information only, i.e. normal operation
2 = functionality may be reduced
3 = fatal problem, device will not work
*/

void TargetManager::printAlarmLevel( const char*alarm, int x, int y, int level ){
	xSemaphoreTake(display,portMAX_DELAY );
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
	xSemaphoreGive(display);
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
			ESP_LOGI( FNAME, "next target: %06X",id_iter->first );
	}
}

void TargetManager::printVersions( int x, int y, const char *prefix, const char *ver, int erase ){
	if( erase )
		egl->setColor(COLOR_BLACK);
	else{
		egl->setColor(COLOR_WHITE);
		info_timer = INFO_TIME;
	}
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( x, y );
	egl->printf( "%s %s", prefix, ver );
}

void TargetManager::clearScreen(){
	egl->clearScreen();
	// redrawNeeded = true;
}

void TargetManager::rewindInfoTimer(){
	if( info_timer == 0 )
		egl->clearScreen();
	ESP_LOGI(FNAME,"rewindInfoTimer() %d", info_timer );
	info_timer = INFO_TIME;
}

void TargetManager::tick(){
	float min_dist = 10000;
	_tick++;
	if( holddown )
		holddown--;

	if( display_mode.get() == DISPLAY_MULTI ){
		if( !holddown && swMode::isClosed() ){
			// ESP_LOGI(FNAME,"SW closed");
			nextTarget( id_timer );
			id_timer = 10 * (1000/TASKPERIOD);  // 10 seconds
			holddown=5;
		}else{
			if( id_timer )
				id_timer --;
		}
	}
	if( !(_tick%20) ){  // all 50 mS
		ESP_LOGI(FNAME,"Num targets: %d", targets.size() );
	}

	if( !(_tick%5) ){ // all 5 ticks
		if( SetupMenu::isActive() )
			return;
		if( info_timer > 0 )
			info_timer--;

		if( Flarm::getTxFlag() || !(_tick%200) ){
			int tx=Flarm::getTXBit();  // 0 or 1
			ESP_LOGI(FNAME,"TX alarm: %d", tx );
			printAlarm( "NO TX ", 10, 100, tx==0 );
			Flarm::resetTxFlag();
		}
		if( Flarm::getRxFlag()){
			int rx=Flarm::getRXNum();
			// ESP_LOGI(FNAME,"New RX: %d", rx );
			char txt[16];
			sprintf( txt, " RX %d ", rx );
			printAlarm( txt, DISPLAY_W-egl->getStrWidth(txt)-5, DISPLAY_H-60, rx != 0, {COLOR_GREEN} );
			Flarm::resetRxFlag();
		}
		if( Flarm::getGPSFlag() || !(_tick%200) ){  // all 10 seconds
			int gps=Flarm::getGPSBit();  // 0,1 or 2
			ESP_LOGI(FNAME,"GPS status: %d", gps );
			printAlarm( "NO GPS ", 10, 120, gps==0 );
			Flarm::resetGPSFlag();
		}
		if( Flarm::getConnectedFlag() ){
			bool conn = Flarm::connected();
			ESP_LOGI(FNAME,"Flarm connected alarm: %d", !conn );
			if( conn==false )
				egl->clearScreen();
			printAlarm( "NO FLARM ", 10, 140, conn==false );
			Flarm::resetConnectedFlag();
		}
		if( Flarm::getErrorFlag() ){
			int severity =  Flarm::getErrorSeverity();
		    int error_code = Flarm::getErrorCode();
			ESP_LOGI(FNAME,"PFLAE error code new:%d  severity new:%d error-txt:%s", error_code, severity, Flarm::getErrorString(error_code)  );
			rewindInfoTimer();
			if( inch2dot4 )
				printAlarmLevel( Flarm::getErrorString(error_code), 10, 140, severity );
			else
				printAlarmLevel( Flarm::getErrorString(error_code), 10, 80, severity );
			Flarm::resetErrorFlag();
		}
		if( Flarm::getSwVersionFlag() ){  // flag ensures there is a valid string
			rewindInfoTimer();
			printVersions( 10, 20, "Flarm SW: ", Flarm::getSwVersion(), erase_info );
			Flarm::resetSwVersionFlag();
		}
		if( Flarm::getHwVersionFlag() ){
			rewindInfoTimer();
			printVersions( 10, 40, "Flarm HW: ", Flarm::getHwVersion(), erase_info );
			Flarm::resetHwVersionFlag();
		}
		if( Flarm::getODBVersionFlag()){
			rewindInfoTimer();
			printVersions( 10, 60, "Flarm ODB: ", Flarm::getObstVersion(), erase_info );
			Flarm::resetODBVersionFlag();
		}
		unsigned int prog = Flarm::getProgress();
		if( Flarm::getProgressFlag() ){
			rewindInfoTimer();
			xSemaphoreTake(display,portMAX_DELAY );
			egl->setColor(COLOR_WHITE);
			egl->setFont(ucg_font_ncenR14_hr);
			egl->setPrintPos( 10, 60 );
			egl->printf( "%s: %d %%  ", Flarm::getOperationString(Flarm::getOperationKey()), prog );
			xSemaphoreGive(display);
			Flarm::resetProgressFlag();
		}
	//	ESP_LOGI(FNAME,"info_timer=%d", info_timer);
		if( info_timer == 1 ){
			ESP_LOGI(FNAME,"NOW CLEAR info");
			egl->clearScreen();
			redrawNeeded = true; // redraw of traffic and info
		}

		// Pass one: determine proximity
		if( !info_timer && Flarm::connected() )
			drawAirplane( DISPLAY_W/2,DISPLAY_H/2, Flarm::getGndCourse() );

		for (auto it=targets.begin(); it!=targets.end(); it++ ){
			it->second.ageTarget();
			if( SetupMenu::isActive() )
				return;
			it->second.nearest(false);
			if( it->second.getAge() < AGEOUT ){
				if( it->second.haveAlarm() )
					id_timer=0;
				if( !id_timer ){
					if( (it->second.getProximity() < min_dist)  ){
						min_dist = it->second.getDist();
						min_id = it->first;
					}
				}else{
					if( id_iter != targets.end() && it->first == id_iter->first ){
						it->second.nearest(true);
					}
				}
			}
		}
		// Pass 2, draw targets
		if( !info_timer && Flarm::connected() ){
			for (auto it=targets.begin(); it!=targets.end(); ){
				if( SetupMenu::isActive() )
					return;
				if( !id_timer )
				{
					if( it->first == min_id ){
						it->second.nearest(true);
					}else{
						it->second.nearest(false);
					}
				}
				if( it->second.getAge() < AGEOUT && ((display_mode.get() == DISPLAY_MULTI) || ((display_mode.get() == DISPLAY_SIMPLE) && it->second.isNearest() ))){
					if( it->second.isNearest() || it->second.haveAlarm() ){
						// closest == true
						if( redrawNeeded ){
							it->second.redrawInfo(); // forced redraw of all fields
							redrawNeeded = false;
						}
						it->second.drawInfo();
					}
					it->second.draw(false);
					if( !(_tick%2) )
						it->second.checkClose();
					it++;
				}
				else{
					if( id_iter->first == it->first ){  // move on id_iter in case
						id_iter++;
					}
					if( it->second.isNearest() ){   // only nearest has info to erase
						it->second.drawInfo(true);
					}
					it->second.draw(true);     // age/erase
					targets.erase( it++ );
				}
			}
		}

	}
	}
