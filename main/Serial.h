/**
 * Serial.h
 *
 * 02.01.2022 Axel Pauli: handling of 2 uart channels in one method accomplished.
 * 01.01.2022 Axel Pauli: updates after first delivery.
 * 24.12.2021 Axel Pauli: added some RX/TX handling stuff.
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <cstring>
#include "SString.h"
#include "driver/gpio.h"
#include <esp_log.h>
#include "RingBufCPP.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "HardwareSerial.h"

#define SERIAL_STRLEN SSTRLEN

// Event mask definitions
#define RX0_CHAR 1
#define RX0_NL 2
#define RX1_CHAR 4
#define RX1_NL 8
#define RX2_CHAR 16
#define RX2_NL 32
#define TX1_REQ 64
#define TX2_REQ 128

#define QUEUE_SIZE 8

const int baud[] = { 0, 4800, 9600, 19200, 38400, 57600, 115200 };

// state machine definition
enum state_t {
	GET_NMEA_SYNC,
	GET_NMEA_STREAM
};


class Serial {
public:
	Serial(){
	}

	static void begin();
	static void taskStart();
	static void serialHandler(void *pvParameters);
	static bool selfTest();
	static int pullBlock( RingBufCPP<SString, QUEUE_SIZE>& q, char *block, int size );
	static void process( const char *packet, int len );
	static void parse_NMEA( char c );


private:
	static enum state_t state;
	static bool _selfTest;
	static EventGroupHandle_t rxTxNotifier;
	// Stop routing of TX/RX data. That is used in case of Flarm binary download.
	static bool bincom_mode;
	static char framebuffer[128];
	static int  pos;
	static int  len;
	static TaskHandle_t pid;
};

#endif
