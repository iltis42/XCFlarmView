/*
 * OTA.cpp
 *
 *  Created on: Mar 17, 2020
 *      Author: iltis
 *
 *
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include <esp_netif.h>

#include "esp_ota_ops.h"
#include "freertos/event_groups.h"
#include "MyWiFi.h"
#include "Setup.h"
#include "OTA.h"
#include <logdef.h>
#include "Webserver.h"
#include "qrcodegen.h"
#include "AdaptUGC.h"
#include "Colors.h"
#include "flarmview.h"

extern AdaptUGC *egl;

OTA::OTA(){
	tick = 0;
}

const char* ssid = CONFIG_AP_SSID;
const char* wifi_password = "esp32-ota";

void OTA::writeText( int line, const char *text ){
	//
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( 10, 30*line );
	egl->setColor(COLOR_WHITE);
	egl->printf("%s",text);
}

// OTA
void OTA::doSoftwareUpdate( ){
	ESP_LOGI(FNAME,"Now start Wifi OTA");
	init_wifi_softap(nullptr);
	delay(100);
	int line=1;
	char text[80];
	{
		DisplayLock lock(_display);
		writeText(line++, "Software Update" );
		writeText(line++, "WIFI" );
		sprintf(text,    "  SSID: %s", ssid);
		writeText(line++, text );
		sprintf(text,"  Password : %s", wifi_password );
		writeText(line++,text);
		writeText(line++, "URL: http://192.168.4.1");
	}
    Webserver.start();

    line = 1;
	for( tick=0; tick<900; tick++ ) {
		if( Webserver.getOtaProgress() > 0 ){
			DisplayLock lock(_display);
			std::string pro( "Progress:                     ");
			pro += std::to_string( Webserver.getOtaProgress() ) + " %";
			writeText(line,pro.c_str());
		}
		vTaskDelay(1000/portTICK_PERIOD_MS);
		if( Webserver.getOtaStatus() == otaStatus::DONE ){
			DisplayLock lock(_display);
			ESP_LOGI(FNAME,"Flash status, Now restart");
			writeText(line,"Download SUCCESS !");
			vTaskDelay(3000/portTICK_PERIOD_MS);
			break;
		}
		if( swMode.isClosed() ) {
			DisplayLock lock(_display);
			ESP_LOGI(FNAME,"pressed");
			writeText(line,"Abort, Now Restart");
			vTaskDelay(3000/portTICK_PERIOD_MS);
			break;
		}
	}
    Webserver.stop();
	ESP_LOGI(FNAME,"Now restart");
	esp_restart();
}
