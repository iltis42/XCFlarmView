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

AdaptUGC *egl = 0;
OTA *ota = 0;


class SwitchObs: public SwitchObserver
{
public:
	SwitchObs(const char* name) : SwitchObserver() {
		ESP_LOGI(FNAME,"attach me %s", name );
		Switch::attach(this);
	};
	~SwitchObs() {};
	void doubleClick() {};
	void press() {};
	void longPress() {  ESP_LOGI(FNAME,"LONGPRESS");

					 };
};

static SetupMenu *menu=0;

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
    //  serial1_speed.set( 1 );  // test for autoBaud

    egl = new AdaptUGC();
    egl->begin();
    // egl->setRedBlueTwist( true );
    egl->clearScreen();
    Buzzer::init(2700);
    Buzzer::play2( BUZZ_C, 500,audio_volume.get(), BUZZ_C, 1000, 0, 1 );

    Version V;
    std::string ver( "SW Ver.: " );
    ver += V.version();

    egl->setFont(ucg_font_fub20_hn);
    egl->setColor(COLOR_WHITE);
    egl->setPrintPos( 50, 35 );
    egl->print("XVFlarmView 2.0");

    egl->setPrintPos( 10, 80 );
    egl->printf("%s",ver.c_str() );

    egl->setPrintPos( 10, 115 );
    egl->printf("Flarmnet: %s", FLARMNET_VERSION );

    egl->setFont(ucg_font_ncenR14_hr);
    egl->setPrintPos( 10, 150 );
    egl->printf("Press Button for SW-Update");
    Switch::begin(GPIO_NUM_0);
    for(int i=0; i<20; i++){  // 40
    	if( Switch::isClosed() ){
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

    // SwitchObs obs("main");
    menu = new SetupMenu();
    menu->begin();
    Switch::startTask();

    if( traffic_demo.get() ){
    	ESP_LOGI(FNAME,"Traffic Demo");
    	traffic_demo.set(0);
    	traffic_demo.commit();
    	Flarm::startSim();
    }

    egl->clearScreen();
    Flarm::begin();
    Serial::begin();
    TargetManager::begin();
    Buzzer::play2( BUZZ_DH, 150,audio_volume.get(), BUZZ_DH, 1000, 0, 1 );

    if( Serial::selfTest() )
    	printf("Serial Loop Test OK");
    else
    	printf("Self Loop Test Failed");

    while(1){
    	delay(1000);
    }

}
