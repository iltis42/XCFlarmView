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

std::map< unsigned int, Target> TargetManager::targets;
extern AdaptUGC *egl;
float TargetManager::oldN = -1.0;
int TargetManager::old_TX = -1;
int TargetManager::old_GPS = -1;

TargetManager::TargetManager() {
	// TODO Auto-generated constructor stub

}

void TargetManager::receiveTarget( nmea_pflaa_s &pflaa ){

    // ESP_LOGI(FNAME,"ID %06X (dec) %d ", pflaa.ID, pflaa.ID );
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
	egl->setPrintPos( x-25*sin(D2R(north))-5, y+25*cos(D2R(north))+6 );
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


void TargetManager::tick(){
	float min_dist = 10000;
	unsigned int min_id = 0;
	int tx=Flarm::getTXBit();  // 0 or 1
	if( old_TX != tx){
		ESP_LOGI(FNAME,"TX changed, old: %d, new: %d", old_TX, tx );
		printAlarm( "TX", 10, 90, tx );
		old_TX = tx;
	}
	int gps=Flarm::getGPSBit();
	if( old_GPS != gps ){  // 0,1 or 2
			ESP_LOGI(FNAME,"GPS changed, old: %d, new: %d", old_GPS, gps );
			printAlarm( "GPS", 10, 110, gps );
			old_GPS = gps;
	}
	drawAirplane( 160,86, Flarm::getGndCourse() );
	for (auto it=targets.begin(); it!=targets.end(); ){
		it->second.ageTarget();
		if( it->second.getAge() > 35 ){
			ESP_LOGI(FNAME,"ID %06X, ERASE from ageout", it->second.getID() );
			it->second.draw(true);
			targets.erase( it++ );
		}else{
			if( (it->second.getProximity() < min_dist)  ){
				min_dist = it->second.getDist();
				min_id = it->first;
				it->second.nearest(true);
			}else{
				it->second.nearest(false);
			}
			++it;
		}
		//		ESP_LOGI(FNAME,"ID %06X, AGE: %d ", it->first, it->second.getAge() );
	}
	for (auto it=targets.begin(); it!=targets.end(); it++ ){
		if( it->second.getAge() < 30 ){
			if( it->first == min_id || it->second.haveAlarm() ){
				it->second.draw(true);
				it->second.drawInfo();
			}
			else{
				it->second.draw();
			}
			// it->second.dumpInfo();
			it->second.checkClose();
		}else{
			if( it->first == min_id ){
				it->second.drawInfo(true);
				it->second.draw(true);
			}
		}
	}
}
