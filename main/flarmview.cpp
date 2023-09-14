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

HardwareSerial Flarm(1); // Uart 1 for serial interface to Flarm

AdaptUGC *egl = 0;



extern "C" void app_main(void)
{
    printf("Hello world!\n");
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
    Flarm.begin( 4800, SERIAL_8N1, 38, 37 );

    delay(1000);
    egl = new AdaptUGC();
    egl->begin();
    egl->clearScreen();
    egl->setRedBlueTwist( true );
    egl->setColor( 255, 255, 255 );   // 255 is white   B,G,R
    // egl->setColor( 0, 0, 0 );
    egl->drawFrame(50,50,10,10);
    egl->setColor( 255, 0, 0 );   // RED
    egl->drawFrame(50,250,10,10);
    egl->setColor( 255, 255, 255 );
    egl->drawLine( 0, 100, 170, 100 );
    egl->drawLine( 10, 102, 170, 102 );
    egl->drawLine( 20, 104, 170, 104 );

    egl->drawLine( 0, 110, 170, 110 );
    egl->drawLine( 10, 112, 170, 112 );
    egl->drawLine( 20, 114, 170, 114 );

    egl->drawLine( 0, 111, 170, 111 );
    egl->drawLine( 10, 113, 170, 113 );
    egl->drawLine( 20, 115, 170, 115 );
    egl->setColor( 0, 0, 255 );   // BLUE
    egl->drawCircle( 100,100, 40 );
    egl->setFont( ucg_font_fub20_hr );
    egl->setPrintPos( 5, 200 );
    egl->setColor( 255, 255, 255 );
    egl->print("Hello Iltis");
    delay(1000);

    while( 1 ){
    	delay(1000);
    	// egl->drawBox(10,10,5,5);
    }

    while( 1 ){
    	char buf[81];
    	if (Flarm.available()) {
    		size_t num = Flarm.readBytes( buf, 81 );
    		printf("num=%d, %s", num, buf );
    	}
    	delay(10);
     }

    vTaskDelay(1000000 / portTICK_PERIOD_MS);
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
