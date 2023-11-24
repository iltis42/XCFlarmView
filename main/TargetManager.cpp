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

std::map< unsigned int, Target*> TargetManager::targets;
extern AdaptUGC *egl;

TargetManager::TargetManager() {
	// TODO Auto-generated constructor stub

}

void TargetManager::receiveTarget( nmea_pflaa_s &pflaa ){

    // ESP_LOGI(FNAME,"ID %06X (dec) %d ", pflaa.ID, pflaa.ID );
    if( targets.find(pflaa.ID) == targets.end() ){
    	targets[ pflaa.ID ] = new Target ( pflaa );
    }
    else
    	targets[ pflaa.ID ]->update( pflaa );
    targets[ pflaa.ID ]->dumpInfo();

}

TargetManager::~TargetManager() {
	// TODO Auto-generated destructor stub
}

void TargetManager::drawAirplane( int x, int y ){
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setColor( 255, 255, 255 );
	egl->drawTetragon( x-15,y-1, x-15,y+1, x+15,y+1, x+15,y-1 );  // wings
	egl->drawTetragon( x-1,y+10, x-1,y-6, x+1,y-6, x+1,y+10 ); // fuselage
	egl->drawTetragon( x-4,y+10, x-4,y+9, x+4,y+9, x+4,y+10 ); // elevator
	egl->setColor( 0, 255, 0 ); // green
	egl->drawCircle( x,y, 25 );
}

void TargetManager::tick(){
	float min_dist = 10000;
	unsigned int min_id = 0;
	drawAirplane( 160,86 );
	for (auto it=targets.begin(); it!=targets.end(); ){
		it->second->ageTarget();
		if( it->second->getAge() > 35 ){
			ESP_LOGI(FNAME,"ID %06X, ERASE from ageout", it->second->getID() );
			it->second->draw(true);
			delete it->second;
			targets.erase( it++ );
		}else{
			if( (it->second->getProximity() < min_dist)  ){
				min_dist = it->second->getDist();
				min_id = it->first;
			}
			++it;
		}
		//		ESP_LOGI(FNAME,"ID %06X, AGE: %d ", it->first, it->second->getAge() );
	}
	for (auto it=targets.begin(); it!=targets.end(); it++ ){
		if( it->second->getAge() < 30 ){
			if( it->first == min_id || it->second->haveAlarm() ){
				it->second->draw(true);
				// it->second->dumpInfo();
				it->second->drawInfo();
			}
			else{
				// it->second->dumpInfo();
				it->second->draw();
			}
			it->second->checkClose();
		}else{
			if( it->first == min_id ){
				it->second->drawInfo(true);
				it->second->draw(true);
			}
		}
	}
}
