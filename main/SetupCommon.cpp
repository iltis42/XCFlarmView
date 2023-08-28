/*
 * Setup.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: iltis
 */

#include <string>
#include <stdio.h>
#include "esp_system.h"
#include <esp_log.h>
#include "sdkconfig.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ESP32NVS.h"
#include "esp32/rom/uart.h"
#include <math.h>
#include <esp32/rom/miniz.h>
#include "esp_task_wdt.h"
#include <logdef.h>
#include "SetupCommon.h"
#include <iostream>
#include <string>
#include <sstream>
#include "SetupNG.h"

// QueueHandle_t SetupCommon::commitSema = nullptr;
// esp_timer_handle_t SetupCommon::_timer = nullptr;

char SetupCommon::_ID[16] = { 0 };
char SetupCommon::default_id[6] = { 0 };
std::vector<SetupCommon *> *SetupCommon::instances = 0;



SetupCommon::SetupCommon() {
	if( !instances )  // instantiate first
		instances = new std::vector<SetupCommon *>;
}

SetupCommon::~SetupCommon() {
	if( !instances->size() )
		delete instances;
}


SetupCommon * SetupCommon::getMember( const char * key ){
	for(int i = 0; i < instances->size(); i++ ) {
		if( std::string( key ) == std::string( (*instances)[i]->key() )){
			// ESP_LOGI(FNAME,"found key %s", (*instances)[i]->key() );
			return (*instances)[i];
		}
	}
	return 0;
}

// at time of connection establishment
bool SetupCommon::syncEntry( int entry ){
    if( entry < instances->size() ) {
        return (*instances)[entry]->sync();
    }
    return false;
}


void SetupCommon::giveConfigChanges( httpd_req *req, bool log_only ){
	ESP_LOGI(FNAME,"giveConfigChanges");
	char cfg[50];
	for(int i = 0; i < instances->size(); i++ ) {
		if( (*instances)[i]->isDefault() == false ){
			char val[20];
			if( (*instances)[i]->value_str( val ) ){
				sprintf( cfg, "%s,%s\n", (*instances)[i]->key(), val );
				ESP_LOGI(FNAME,"%s,%s", (*instances)[i]->key(), val );
				if( !log_only )
					httpd_resp_send_chunk( req, cfg, strlen(cfg) );
			}
		}
	}
	if( !log_only )
		httpd_resp_send_chunk( req, cfg, 0 );
}


int SetupCommon::restoreConfigChanges( int len, char *data ){
	ESP_LOGI(FNAME,"restoreConfigChanges len: %d \n %s", len, data );
	std::istringstream fs;
	fs.str( data );
	std::string line;
	int i=0;
	int valid=0;
	while( std::getline(fs, line, '\n') ) {
		if( line.find( "xcvario-config" ) != std::string::npos ){
			valid++;
			ESP_LOGI(FNAME,"found xcvario-config, valid=%d", valid );
		}
		else if( line.find( "text/comma-separated-values" ) != std::string::npos ){
			valid++;
			ESP_LOGI(FNAME,"found text/comma-separated-values, valid=%d", valid );
		}
		else if( line.find( "text/csv" ) != std::string::npos ){
			valid++;
			ESP_LOGI(FNAME,"found text/csv, valid=%d", valid );
		}
		else if( (line.length() > 1) && (valid >= 2) && line.find( "," ) != std::string::npos ){
			printf( "%d, len:%d, %s\n", i, line.length(), line.c_str() );
			std::string key = line.substr(0, line.find(','));
			std::string value = line.substr(line.find(',')+1, line.length());
			printf( "%d %s ", i, key.c_str()  );
			SetupCommon * item = getMember( key.c_str() );
			printf( ", typename: %c \n", item->typeName()  );
			item->setValueStr( value.c_str() );
			item->commit();  // lets do that lazy later
			i++;
		}
	}
	ESP_LOGI(FNAME,"return %d", i);
	return i;
}

void SetupCommon::commitDirty(){
	for(int i = 0; i < instances->size(); i++ ) {
		if( (*instances)[i]->dirty() )
			(*instances)[i]->commit();
	}
}

bool SetupCommon::factoryReset(){
	ESP_LOGI(FNAME,"\n\n******  FACTORY RESET ******");
	bool retsum = true;
	for(int i = 0; i < instances->size(); i++ ) {
		ESP_LOGI(FNAME,"i=%d %s erase", i, (*instances)[i]->key() );
		if( (*instances)[i]->mustReset() ){
			bool ret = (*instances)[i]->erase();
			if( ret != true ) {
				ESP_LOGE(FNAME,"Error erasing %s", (*instances)[i]->key() );
				retsum = false;
			}
			ret = (*instances)[i]->init();
			if( ret != true ) {
				for(int i = 0; i < instances->size(); i++ ) {
					if( (*instances)[i]->dirty() )
						(*instances)[i]->commit();
				}
				return true;
				ESP_LOGE(FNAME,"Error init with default %s", (*instances)[i]->key() );
				retsum = false;
			}
			else
				ESP_LOGI(FNAME,"%s successfully initialized with default", (*instances)[i]->key() );
		}
	}
	if( retsum )
		ESP_LOGI(FNAME,"Factory reset SUCCESS");
	else
		ESP_LOGI(FNAME,"Factory reset FAILED!");
	return retsum;
}

bool SetupCommon::initSetup( bool& present ) {
	bool retsum=true;
	ESP_LOGI(FNAME,"SetupCommon::initSetup()");
	NVS.begin();

	for(int i = 0; i < instances->size(); i++ ) {
			bool ret = (*instances)[i]->init();
			if( ret != true ){
				ESP_LOGE(FNAME,"Error init with default NVS: %s", (*instances)[i]->key() );
				retsum=false;
			}
	}

	if( factory_reset.get() ) {
		ESP_LOGI(FNAME,"\n\n******  FACTORY RESET ******");
		for(int i = 0; i < instances->size(); i++ ) {
			ESP_LOGI(FNAME,"i=%d %s erase", i, (*instances)[i]->key() );
			if( (*instances)[i]->mustReset() ){
				bool ret = (*instances)[i]->erase();
				if( ret != true ) {
					ESP_LOGE(FNAME,"Error erasing %s", (*instances)[i]->key() );
					retsum = false;
				}
				ret = (*instances)[i]->init();
				if( ret != true ) {
					ESP_LOGE(FNAME,"Error init with default %s", (*instances)[i]->key() );
					retsum = false;
				}
				else
					ESP_LOGI(FNAME,"%s successfully initialized with default", (*instances)[i]->key() );
			}
		}
	}
	giveConfigChanges( 0, true );
	return retsum;
};


int SetupCommon::numEntries() {
	return instances->size();
}

