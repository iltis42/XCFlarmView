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
SetupNG<int>  			serial1_tx_enable( "SER1_TX_ENA", 0 );

SetupNG<int>  			software_update( "SOFTWARE_UPDATE", 0 );
SetupNG<int>	        log_level( "LOG_LEVEL", 3 );
SetupNG<int>            factory_reset( "FACTORY_RES" , 0 );
SetupNG<int>	        log_scale( "LOG_SCALE", 0 );

SetupNG<int>            alt_unit( "ALT_UNIT", ALT_UNIT_METER );
SetupNG<int>            ias_unit( "IAS_UNIT", SPEED_UNIT_KMH );
SetupNG<int>            vario_unit( "VARIO_UNIT", VARIO_UNIT_MS );
SetupNG<int>            dst_unit( "DST_UNIT", DST_UNIT_KM );
SetupNG<float>		    audio_volume( "AUDVOL", 100.0 );
SetupNG<int>  			display_test( "DISPLAY_TEST", 0, RST_NONE, SYNC_NONE, VOLATILE );
SetupNG<int> 			data_monitor("DATAMON", MON_OFF, RST_NONE, SYNC_NONE, VOLATILE  );
SetupNG<int> 			traffic_demo("TRADEM", 0 );
SetupNG<int>  			display_orientation("DISPLAY_ORIENT" , DISPLAY_NORMAL );
SetupNG<int>  			display_mode("DISPLAY_MODE" , DISPLAY_MULTI );
SetupNG<int>  			display_non_moving_target("NON_MOVE" , NON_MOVE_HIDE );
SetupNG<int>  			notify_near( "NOTFNEAR", BUZZ_2KM );
SetupNG<int>  			rs232_polarity( "RS232POL", RS232_INVERTED );

