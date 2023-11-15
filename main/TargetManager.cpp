/*
 * TargetManager.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "TargetManager.h"
#include "inttypes.h"
#include "Flarm.h"

TargetManager::TargetManager() {
	// TODO Auto-generated constructor stub


}

void TargetManager::receiveTarget( nmea_pflaa_s &target ){

    ESP_LOGI(FNAME,"ID %06X (dec) %d ", target.ID, target.ID );
}

TargetManager::~TargetManager() {
	// TODO Auto-generated destructor stub
}

