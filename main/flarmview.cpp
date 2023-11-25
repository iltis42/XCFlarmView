/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "Arduino.h"
#include "SetupNG.h"
#include <HardwareSerial.h>
#include <AdaptUGC.h>
#include <cmath>
#include <Serial.h>
#include "Flarm.h"
#include "driver/ledc.h"
#include "Buzzer.h"
#include "OTA.h"
#include "Version.h"


AdaptUGC *egl = 0;
OTA *ota = 0;

void drawAirplane( int x, int y, bool fromBehind=false, bool smallSize=false ){
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setColor( 255, 255, 255 );
	if( fromBehind ){
		egl->drawTetragon( x-30,y-2, x-30,y+2, x+30,y+2, x+30,y-2 );
		egl->drawTetragon( x-2,y-2, x-2,y-10, x+2,y-10, x+2,y-2 );
		egl->drawTetragon( x-8,y-12, x-8,y-16, x+8,y-16, x+8,y-12 );
		egl->drawDisc( x,y, 4, UCG_DRAW_ALL );
	}else{
		if( smallSize ){
			egl->drawTetragon( x-15,y-1, x-15,y+1, x+15,y+1, x+15,y-1 );  // wings
			egl->drawTetragon( x-1,y+10, x-1,y-3, x+1,y-3, x+1,y+10 ); // fuselage
			egl->drawTetragon( x-4,y+10, x-4,y+9, x+4,y+9, x+4,y+10 ); // elevator

		}else{
			egl->drawTetragon( x-30,y-2, x-30,y+2, x+30,y+2, x+30,y-2 );  // wings
			egl->drawTetragon( x-2,y+25, x-2,y-10, x+2,y-10, x+2,y+25 ); // fuselage
			egl->drawTetragon( x-8,y+25, x-8,y+21, x+8,y+21, x+8,y+25 ); // elevator
		}
	}
}

void drawFlarmTarget( int x, int y, float bearing, int sideLength ){
	 float radians = (bearing-90.0) * M_PI / 180;
	  // Calculate the triangle's vertices
	  int x0 = x + sideLength * cos(radians);
	  int y0 = y + sideLength * sin(radians);
	  int x1 = x + sideLength/2 * cos(radians + 2 * M_PI / 3);
	  int y1 = y + sideLength/2 * sin(radians + 2 * M_PI / 3);
	  int x2 = x + sideLength/2 * cos(radians - 2 * M_PI / 3);
	  int y2 = y + sideLength/2 * sin(radians - 2 * M_PI / 3);
	  egl->drawTriangle( x0,y0,x1,y1,x2,y2 );
}

extern "C" void app_main(void)
{
    initArduino();
    bool setupPresent;
    SetupCommon::initSetup( setupPresent );
    printf("Setup present: %d speed: %d\n", setupPresent, serial1_speed.get() );

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "/EMB_FLASH" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    // begin(unsigned long baud, uint32_t config=SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1)

    delay(100);
    //  serial1_speed.set( 1 );  // test for autoBaud

    egl = new AdaptUGC();
    egl->begin();
    egl->clearScreen();
    egl->setRedBlueTwist( true );
    Buzzer::init(2700);
    Buzzer::play2( BUZZ_C, 500,70, BUZZ_C, 1000, 0, 1 );

    Version V;
    for(int i=0; i<30; i++){
    	if( Switch::isClosed() ){
    		ota = new OTA();
    		ota->doSoftwareUpdate();
    		while(1){
    			delay(100);
    		}
    	}
    	delay( 100 );
    }

    Flarm::begin();
    Serial::begin();

    Buzzer::play2( BUZZ_DH, 150,100, BUZZ_DH, 1000, 0, 1 );

    if( Serial::selfTest() )
    	printf("Self TEST OK");
    else
    	printf("Self TEST Failed");
/*
    while( 1 ){
    	delay(100);
    }

    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
    */
}
