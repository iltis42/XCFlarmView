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
	// egl->clearScreen();
	int line=1;
	char text[80];
	writeText(line++, "Software Update" );
	writeText(line++, "WIFI" );
	sprintf(text,    "  SSID: %s", ssid);
	writeText(line++, text );
	sprintf(text,"  Password : %s", wifi_password );
	writeText(line++,text);
	writeText(line++, "URL: http://192.168.4.1");

	// sprintf(text, "                  PW: %s", wifi_password );
	// writeText(line++,text );

	// enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW; // Error correction level
	// ESP_LOGI(FNAME, "Generate QRCODE");

	// Make and print the QR Code symbol
	// const size_t textBufferSize = 128;
	// char *textBuffer = (char *)malloc(textBufferSize);
	// uint8_t *tempBuffer = (uint8_t *)malloc(qrcodegen_BUFFER_LEN_FOR_VERSION(4));
	// uint8_t *qrcodeBuffer = (uint8_t *)malloc(qrcodegen_BUFFER_LEN_FOR_VERSION(4));

	// snprintf(textBuffer, textBufferSize, "WIFI:S:%s;T:WPA;P:%s;;", ssid, wifi_password);
	// bool qrSuccess = qrcodegen_encodeText(textBuffer, tempBuffer, qrcodeBuffer, errCorLvl, 4, 4, qrcodegen_Mask_AUTO, true);
/*
	size_t qrCodeMaxWidth = 114;
	size_t yOffset = 70;
	size_t xOffset = 0;


	size_t strWidth = egl->getStrWidth(text);
	egl->setPrintPos(10, 68);
    egl->print(wifiText);

	if( qrSuccess ) {
		// Calculate module size for best fit
		int size = qrcodegen_getSize(qrcodeBuffer);
		int dotSize = qrCodeMaxWidth / size;

		// Center QRCode
		xOffset += (120 - (size * dotSize)) / 2;

		for( int y = 0; y < size; y++ ) {
			for( int x = 0; x < size; x++ ) {
				if( qrcodegen_getModule(qrcodeBuffer, x, y) ) {
				  egl->drawBox(xOffset + (x * dotSize), yOffset + (y * dotSize), dotSize, dotSize);
				}
			}
		}
	} else {
		egl->drawFrame(xOffset + ((120 - qrCodeMaxWidth) / 2), yOffset, qrCodeMaxWidth, qrCodeMaxWidth);
	}

    // Generate URL QR Code using the AP IP-address
    // TODO: Show after Wifi has been connected?
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);    // Return ESPs IP for everybody else

	snprintf(textBuffer, textBufferSize, "http://" IPSTR, IP2STR(&ip_info.ip));
	qrSuccess = qrcodegen_encodeText(textBuffer, tempBuffer, qrcodeBuffer, errCorLvl, 4, 4, qrcodegen_Mask_AUTO, true);

	const char *urlText = "URL:";
	strWidth = egl->getStrWidth(urlText);
	egl->setPrintPos(120 + (120 - strWidth) / 2, 68);
	egl->print(urlText);

	xOffset = 120;
	if( qrSuccess ) {
		// Calculate module size for best fit
		int size = qrcodegen_getSize(qrcodeBuffer);
		int dotSize = qrCodeMaxWidth / size;

		// Center QRCode
		xOffset += (120 - (size * dotSize)) / 2;

		for( int y = 0; y < size; y++ ) {
			for( int x = 0; x < size; x++ ) {
				if( qrcodegen_getModule(qrcodeBuffer, x, y) ) {
				  egl->drawBox(xOffset + (x * dotSize), yOffset + (y * dotSize), dotSize, dotSize);
				}
			}
		}
	} else {
		// In case of error draw empty rectangle
		egl->drawFrame(xOffset + ((120 - qrCodeMaxWidth) / 2), yOffset, qrCodeMaxWidth, qrCodeMaxWidth);
	}

	free(textBuffer);
	free(qrcodeBuffer);
	free(tempBuffer);
	*/

    Webserver.start();

    line = 1;
	for( tick=0; tick<900; tick++ ) {
		if( Webserver.getOtaProgress() > 0 ){
			std::string pro( "Progress:                     ");
			pro += std::to_string( Webserver.getOtaProgress() ) + " %";
			writeText(line,pro.c_str());
		}
		vTaskDelay(1000/portTICK_PERIOD_MS);
		if( Webserver.getOtaStatus() == otaStatus::DONE ){
			ESP_LOGI(FNAME,"Flash status, Now restart");
			writeText(line,"Download SUCCESS !");
			vTaskDelay(3000/portTICK_PERIOD_MS);
			break;
		}
		if( swMode.isClosed() ) {
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
