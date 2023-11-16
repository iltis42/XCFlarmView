/*
 * TargetManager.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "TargetManager.h"
#include "inttypes.h"
#include "Flarm.h"

std::map< unsigned int, Target*> TargetManager::targets;

TargetManager::TargetManager() {
	// TODO Auto-generated constructor stub

}

void TargetManager::receiveTarget( nmea_pflaa_s &pflaa ){

    // ESP_LOGI(FNAME,"ID %06X (dec) %d ", pflaa.ID, pflaa.ID );
    if( targets.find(pflaa.ID) == targets.end() )
    	targets[ pflaa.ID ] = new Target ( pflaa );
    else
    	targets[ pflaa.ID ]->update( pflaa );
    targets[ pflaa.ID ]->dumpInfo();
}

TargetManager::~TargetManager() {
	// TODO Auto-generated destructor stub
}

void TargetManager::tick(){
	for (auto it=targets.begin(); it!=targets.end(); ){
		it->second->ageTarget();
		if( it->second->getAge() > 35 ){
			ESP_LOGI(FNAME,"ID %06X, ERASE from ageout", it->second->getID() );
			delete it->second;
			targets.erase( it++ );
		}else{
			it->second->dumpInfo();
			++it;
		}
//		ESP_LOGI(FNAME,"ID %06X, AGE: %d ", it->first, it->second->getAge() );

	}
}
