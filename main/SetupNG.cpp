/*
 * SetupNG.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: iltis
 */

#include "SetupNG.h"
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
#include <iostream>
#include <map>
#include <math.h>
#include <esp32/rom/miniz.h>
#include "esp_task_wdt.h"
#include <esp_http_server.h>


void send_config( httpd_req *req ){
	SetupCommon::giveConfigChanges( req );
};

int restore_config(int len, char *data){
	return( SetupCommon::restoreConfigChanges( len, data ) );
};

SetupNG<int>  			serial1_speed( "SERIAL1_SPEED", 3 );
SetupNG<int>  			serial1_pins_twisted( "SERIAL1_PINS", 0 );
SetupNG<int>  			serial1_rxloop( "SERIAL1_RXLOOP", 0 );
SetupNG<int>  			serial1_tx_inverted( "SERIAL1_TX_INV", RS232_INVERTED );
SetupNG<int>  			serial1_rx_inverted( "SERIAL1_RX_INV", RS232_INVERTED );
SetupNG<int>  			serial1_tx_enable( "SER1_TX_ENA", 1 );

SetupNG<int>  			software_update( "SOFTWARE_UPDATE", 0 );
SetupNG<int>		        log_level( "LOG_LEVEL", 3 );
SetupNG<int>                    factory_reset( "FACTORY_RES" , 0 );

