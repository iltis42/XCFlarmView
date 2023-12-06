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
float TargetManager::oldN = 0.0;

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
	if(erase)
		egl->setColor(COLOR_BLACK);
	else
		egl->setColor(0,0,255); // G=0 R=Max B=0
	egl->setFontPosCenter();
	egl->setPrintPos( x-25*sin(D2R(north))-5, y+25*cos(D2R(north))+6 );
	egl->setFont(ucg_font_ncenR14_hr);
	egl->printf("N");
	oldN = north;
}

void TargetManager::drawAirplane( int x, int y, float north ){
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setColor( 255, 255, 255 );
	egl->drawTetragon( x-15,y-1, x-15,y+1, x+15,y+1, x+15,y-1 );  // wings
	egl->drawTetragon( x-1,y+10, x-1,y-6, x+1,y-6, x+1,y+10 ); // fuselage
	egl->drawTetragon( x-4,y+10, x-4,y+9, x+4,y+9, x+4,y+10 ); // elevator
	egl->setColor( 0, 255, 0 ); // green
	egl->drawCircle( x,y, 25 );
	if( north != oldN ){
		if( oldN != 0.0 )
			drawN( x,y, true, oldN );
		drawN( x,y, false, north );
	}
}

void TargetManager::tick(){
	float min_dist = 10000;
	unsigned int min_id = 0;
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
