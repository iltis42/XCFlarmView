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
#include "Colors.h"
#include "flarmnetdata.h"
#include "TargetManager.h"
#include "Switch.h"
#include "SetupMenu.h"
#include "DataMonitor.h"
#include "esp_task_wdt.h"

AdaptUGC *egl = 0;
OTA *ota = 0;
DataMonitor DM;
TargetManager TM;
#define WDT_TIMEOUT_S 10

SetupMenu *menu=0;
bool inch2dot4=false;
#if( DISPLAY_W == 240 )
Switch swUp;
Switch swDown;
#endif
Switch swMode;
float zoom=1.0;


void showText( int ypos, const char*text ){
	if( text != 0 ){
		int w=0;
		char *buf = (char *)malloc(512);
		memset(buf, 0, 512);
		memcpy( buf, text, strlen(text));
		char *p = strtok (buf, " ");
		char *words[100];
		while (p != NULL)
		{
			words[w++] = p;
			p = strtok (NULL, " ");
		}
		// ESP_LOGI(FNAME,"show text number of words: %d", w);
		int x=1;
		int y=ypos;
		egl->setFont(ucg_font_ncenR14_hr);
		for( int p=0; p<w; p++ )
		{
			int len = egl->getStrWidth( words[p] );
			// ESP_LOGI(FNAME,"showhelp pix len word #%d = %d, %s ", p, len, words[p]);
			if( x+len > DISPLAY_W ) {   // does still fit on line
				y+=25;
				x=1;
			}
			egl->setPrintPos(x, y);
			egl->print( words[p] );
			x+=len+5;
		}
		free( buf );
	}
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

    delay(100);
	esp_task_wdt_init(WDT_TIMEOUT_S, true);
    //  serial1_speed.set( 1 );  // test for autoBaud

    egl = new AdaptUGC();
    DM.begin( egl );
    egl->begin();
    egl->setColor(0, COLOR_WHITE );
    egl->setColor(1, COLOR_BLACK );
    egl->clearScreen();
    Buzzer::init(2700);
    Buzzer::play2( BUZZ_C, 500,audio_volume.get(), BUZZ_C, 1000, 0, 1 );
    DM.begin( egl );

    if( DISPLAY_W == 240 )
       	inch2dot4 = true;

    Version V;
    std::string ver = std::string("XCF") + (inch2dot4 ? "2.4" : "1.4") + " SW: ";
    ver += V.version();

    if( inch2dot4 )
    	egl->setFont(ucg_font_fub14_hn);
    else
    	egl->setFont(ucg_font_fub20_hn);

    egl->setColor(COLOR_WHITE);
    if( serial1_tx_enable.get() ){ // we don't need TX pin, so disable
    	serial1_tx_enable.set(0);
    }

    egl->setPrintPos( 10, 30 );
    egl->printf("%s",ver.c_str() );

	showText( 60,  "ID-Button Actions:" );
	showText( 80,  "Short  (<0.3s): Next ID" );
	showText( 100,  "Long   (>0.3s): Setup");
    showText( 120,  "Hold   (>2s)  : Mark Team");

    if( serial1_tx_enable.get() ){ // we don't need TX pin, so disable
      	serial1_tx_enable.set(0);
    }

    egl->setFont(ucg_font_ncenR14_hr);

    const char updatetxt[] = "Press Button for SW-Update";
    int x,y;
    if( inch2dot4 ) {x = 0; y = 270;}
    else    		{x = 10;y = 165;}
    egl->setPrintPos( x, y );
    egl->printf(updatetxt);

    if( inch2dot4 ){
		#if( DISPLAY_W == 240 )
    	swUp.begin(GPIO_NUM_0, B_UP );
    	swDown.begin(GPIO_NUM_3, B_DOWN );
		#endif
        swMode.begin(GPIO_NUM_34, B_MODE );
    }else{
    	swMode.begin(GPIO_NUM_0, B_MODE );
    }

    for(int i=0; i<40; i++){
#if( DISPLAY_W == 240 )
    	if( swMode.isClosed() || swUp.isClosed() || swDown.isClosed() ){
#else
    	if( swMode.isClosed() ){
#endif
    		egl->clearScreen();
    		ota = new OTA();
    		ota->doSoftwareUpdate();
    		while(1){
    			delay(100);
    		}
    	}
    	delay( 100 );
    }
    egl->clearScreen();
    egl->setColor(COLOR_WHITE);
   	egl->setPrintPos( 10, 35 );

    menu = new SetupMenu();
    menu->begin();
    Switch::startTask();

    egl->clearScreen();
    Flarm::begin();
    Serial::begin();
    TM.begin();
    Buzzer::play( BUZZ_DH, 250,audio_volume.get());

    ESP_LOGI(FNAME,"Team ID: %X", team_id.get() );

    if( traffic_demo.get() ){
    	ESP_LOGI(FNAME,"Traffic Demo");
    	traffic_demo.set(0);
    	traffic_demo.commit();
    	delay( 100 );
    	Flarm::startSim();
    }

    if( Serial::selfTest() )
    	printf("Serial Loop Test OK");
    else
    	printf("Self Loop Test Failed");

    ESP_LOGI(FNAME,"Team ID: %X", team_id.get() );
    esp_task_wdt_add(NULL);
    while(1){
    	ESP_LOGI(FNAME,"Free Heap: %d bytes", heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    	esp_task_wdt_reset();
    	delay(5000);
    }

}
