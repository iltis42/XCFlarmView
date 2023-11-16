/*
 * Target.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "Target.h"


Target::Target() {
	// TODO Auto-generated constructor stub
}

Target::Target( nmea_pflaa_s a_pflaa ) {
	ESP_LOGI(FNAME,"Target (ID %06X) Creation()", pflaa.ID );
    pflaa = a_pflaa;
    age = 0;
}

void Target::update( nmea_pflaa_s a_pflaa ){
	// ESP_LOGI(FNAME,"Target (ID %06X) update()", pflaa.ID );
	pflaa = a_pflaa;
	age = 0;
}

void Target::ageTarget(){
	if( age < 1000 )
		age++;
}

void Target::dumpInfo(){
	ESP_LOGI(FNAME,"Target (ID %06X) age:%d alt:%d m, dis:%.2lf km, var:%.1f m/s, trck:%d", pflaa.ID, age, pflaa.relVertical, sqrt( pflaa.relNorth*pflaa.relNorth + pflaa.relEast*pflaa.relEast )/1000.0, pflaa.climbRate, pflaa.track );
}

Target::~Target() {
	// TODO Auto-generated destructor stub
}

