/**
 * Serial.cpp
 *
 * 02.01.2022 Axel Pauli: handling of 2 uart channels in one method accomplished.
 * 01.01.2022 Axel Pauli: updates after first delivery.
 * 24.12.2021 Axel Pauli: added some RX/TX handling stuff.
 */

#include <esp_log.h>
#include <cstring>
#include "sdkconfig.h"
#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include <freertos/semphr.h>
#include <algorithm>
#include <HardwareSerial.h>
#include "RingBufCPP.h"
#include <logdef.h>
#include "Switch.h"
#include "Serial.h"
#include "Flarm.h"
#include "driver/uart.h"
#include "DataMonitor.h"
#include "SetupMenu.h"

/* Note that the standard NMEA 0183 baud rate is only 4.8 kBaud.
Nevertheless, a lot of NMEA-compatible devices can properly work with
higher transmission speeds, especially at 9.6 and 19.2 kBaud.
As any sentence can consist of 82 characters maximum with 10 bit each (including start and stop bit),
any sentence might take up to 171 ms (at 4.8k Baud), 85 ms (at 9.6 kBaud) or 43 ms (at 19.2 kBaud).
This limits the overall channel capacity to 5 sentences per second (at 4.8k Baud), 11 msg/s (at 9.6 kBaud) or 23 msg/s (at 19.2 kBaud).
If  too  many  sentences  are  produced  with  regard  to  the  available  transmission  speed,
some sentences might be lost or truncated.
 */


bool Serial::_selfTest = false;
EventGroupHandle_t Serial::rxTxNotifier = 0;
extern xSemaphoreHandle _display;

// Event group bits
#define RX0_CHAR 1
#define RX0_NL 2

#define TX1_REQ 64
#define RX1_CHAR 4
#define RX1_NL 8

const uint8_t NMEA_START1 = '$';
const uint8_t NMEA_START2 = '!';
// NMEA stream
const uint8_t NMEA_MIN = 0x20;
const uint8_t NMEA_MAX = 0x7e;

const uint8_t NMEA_CR = '\r';  // 13 0d
const uint8_t NMEA_LF = '\n';  // 10 0a

enum state_t Serial::state = GET_NMEA_SYNC;

RingBufCPP<SString, QUEUE_SIZE> s1_tx_q;
RingBufCPP<SString, QUEUE_SIZE> s1_rx_q;

static xSemaphoreHandle qMutex=NULL;
#define SERIAL_BUFLEN 1024

char Serial::framebuffer[512];
int  Serial::pos = 0;
int  Serial::len = 0;
TaskHandle_t Serial::pid = 0;
const uart_port_t uart_num = UART_NUM_1;

bool Serial::bincom_mode = false;  // we start with bincom timer inactive
int Serial::trials=0;
int Serial::baudrate = 0;

#define HUNTBAUDRATE_HOLDDOWN 24000   // 120 sec

int Serial::pullBlock( RingBufCPP<SString, QUEUE_SIZE>& q, char *block, int size ){
        xSemaphoreTake(qMutex,portMAX_DELAY );
        int total_len = 0;
        while( !q.isEmpty() ){
                int len = q.pull( block+total_len );
                total_len += len;
                if( (total_len + SSTRLEN) > size )
                        break;
        }
        block[total_len]=0;
        xSemaphoreGive(qMutex);
        return total_len;
};

void Serial::process( const char *packet, int len ) {
	// process every frame byte through state machine
	// ESP_LOGI(FNAME,"Port %d: RX len: %d bytes", port, len );
	// ESP_LOG_BUFFER_HEXDUMP(FNAME,packet, len, ESP_LOG_INFO);
	for (int i = 0; i < len; i++) {
		parse_NMEA(packet[i]  );
	}
};

void Serial::parse_NMEA( char c ){
	// ESP_LOGI(FNAME, "Port S%1d: char=%c pos=%d  state=%d", port, c, pos, state );
	switch(state) {
	case GET_NMEA_SYNC:
		switch(c) {
		case NMEA_START1:
		case NMEA_START2:
			pos = 0;
			framebuffer[pos] = c;
			pos++;
			state = GET_NMEA_STREAM;
			// ESP_LOGI(FNAME, "Port S%1d: NMEA Start at %d", port, pos);
			break;
		}
		break;
		case GET_NMEA_STREAM:
			if ((c < NMEA_MIN || c > NMEA_MAX) && (c != NMEA_CR && c != NMEA_LF)) {
				// ESP_LOGE(FNAME, "Port S%1d: Invalid NMEA character %x, restart, pos: %d, state: %d", port, (int)c, pos, state );
				// ESP_LOG_BUFFER_HEXDUMP(FNAME, framebuffer, pos+1, ESP_LOG_INFO);
				state = GET_NMEA_SYNC;
				break;
			}
			if (pos >= sizeof(framebuffer) - 1) {
				ESP_LOGE(FNAME, "Port S1 NMEA buffer not large enough, restart" );
				pos = 0;
				state = GET_NMEA_SYNC;
			}
			if ( c == NMEA_CR || c == NMEA_LF ) { // normal case, accordign to NMEA 183 protocol, first CR, then LF as the last char  (<CR><LF> ends the message.)
				// but we accept also a single terminator as not relevant for the data carried        0d  0a
				// make things clean!                                                                                                   \r  \n
				framebuffer[pos] = NMEA_CR;       // append a CR
				pos++;
				framebuffer[pos] = NMEA_LF;       // append a LF
				pos++;
				framebuffer[pos] = 0;  // framebuffer is zero terminated
				// pos++;
				if( !Flarm::getSim() )
					Flarm::parseNMEA( framebuffer, pos );
				state = GET_NMEA_SYNC;
				pos = 0;
			}else{
				framebuffer[pos] = c;
				pos++;
			}
			break;
	}
};



// Serial Handler ttyS1, S1, port 8881
void Serial::serialHandler(void *pvParameters)
{
	char buf[SERIAL_BUFLEN];  // 6 messages @ 80 byte
	// Make a pause, that has avoided core dumps during enable the RX interrupt.
	delay( 1000 );  // delay a bit serial task startup unit startup of system is through
	ESP_LOGI(FNAME,"S1 serial handler startup");
    unsigned int start_holddown = HUNTBAUDRATE_HOLDDOWN;  // 14000 * 5 mS = 120 sec
	while( true ) {
		// Stack supervision
		if( uxTaskGetStackHighWaterMark( pid ) < 256 )
			ESP_LOGW(FNAME,"Warning serial task stack low: %d bytes", uxTaskGetStackHighWaterMark( pid ) );
		if( _selfTest ) {
			delay( 100 );
			continue;
		}
		// TX part, check if there is data for Serial Interface to send
		if( uart_wait_tx_done(uart_num, 100) ) {
			int len = pullBlock( s1_tx_q, buf, 512 );
			if( len ){
				// ESP_LOGI(FNAME,"S1: TX len: %d bytes",  len );
				// ESP_LOG_BUFFER_HEXDUMP(FNAME,buf,len, ESP_LOG_INFO);
				int wr = uart_write_bytes(uart_num, buf, len );
				ESP_LOGD(FNAME,"S1: TX written: %d", wr);
				DM.monitorString( MON_S1, DIR_TX, buf, len );
			}
		}
		// --- RX handling (optimized) ---
		uint8_t buf[SERIAL_BUFLEN];
		uint16_t rxBytes = 0;

		while (true) {
		    size_t available = 0;
		    // Check how many bytes are waiting in the HW buffer
		    esp_err_t err = uart_get_buffered_data_len(uart_num, &available);
		    if (err != ESP_OK || available == 0) break;

		    // Limit read length to remaining buffer space
		    size_t toRead = std::min(available, (size_t)(SERIAL_BUFLEN - rxBytes - 1));
		    if (toRead == 0) break; // no room left

		    // Read from UART hardware buffer
		    int bytes = uart_read_bytes(
		        uart_num,
		        buf + rxBytes,
		        toRead,
		        20 / portTICK_PERIOD_MS   // short timeout
		    );
		    if (bytes <= 0) break;

		    // Log the received data BEFORE increasing rxBytes
		    DM.monitorString(MON_S1, DIR_RX, (char*)(buf + rxBytes), bytes);

		    // Advance pointer
		    rxBytes += bytes;

		    // Safety: stop if buffer almost full
		    if (rxBytes >= SERIAL_BUFLEN - 4) {
		        ESP_LOGW(FNAME, "UART RX buffer nearly full (%d bytes)", rxBytes);
		        break;
		    }
		    vTaskDelay(pdMS_TO_TICKS(5));
		}

		// Null-terminate for safety (for text/NMEA parsing)
		buf[rxBytes] = '\0';

		// Process only if something was received
		if (rxBytes > 0) {
		    process((char*)buf, rxBytes);
		}
		if( Flarm::connected() ){ // normal operation
			if( serial1_speed.get() != baudrate )    // save when new baudrate has have been detected
				saveBaudrate();
			start_holddown = HUNTBAUDRATE_HOLDDOWN;  // rewind autobaud hunting holddown timer
		}
		// ESP_LOGI(FNAME,"FC: %d, HD: %d", Flarm::connected(), start_holddown );
		if( !Flarm::connected() && !start_holddown ){
			huntBaudrate();
		}
		if( start_holddown > 0 )
			start_holddown--;
		vTaskDelay(pdMS_TO_TICKS(5));
	} // end while( true )
}


bool Serial::selfTest(){
	ESP_LOGI(FNAME,"Serial S1 selftest");
	delay(100);  // wait for serial hardware init
	_selfTest = true;
	std::string test( PROGMEM "The quick brown fox jumps over the lazy dog" );
	int tx = 0;
	uart_flush(uart_num);
	if( uart_wait_tx_done(uart_num, 5000) == ESP_OK ) {
		tx = uart_write_bytes(uart_num, (const char*)test.c_str(), test.length() );
		ESP_LOGI(FNAME,"Serial TX written: %d", tx );
	}
	else {
		ESP_LOGI(FNAME,"Serial not avail for sending, abort");
		return false;
	}
	char recv[256];
	memset(recv,0,256);
	delay( 10 );
	int numread = 0;
	for( int i=1; i<10; i++ ){
		int avail = 0;
		uart_get_buffered_data_len(uart_num, (size_t*)&avail);
		ESP_LOGI(FNAME,"Serial RX bytes avail: %d", avail );
		if( avail >= tx ){
			if( avail > 256 )
				avail = 256;
			numread = uart_read_bytes(uart_num, recv, avail, 256);
			ESP_LOGI(FNAME,"Serial RX bytes read: %d %s", numread, recv );
			break;
		}
		delay( 10 );
	}
	_selfTest = false;
	std::string r( recv );
	if( r.find( test ) != std::string::npos )  {
		ESP_LOGI(FNAME,"Serial Test PASSED");
		return true;
	}
	else {
		ESP_LOGI(FNAME,"Serial Test FAILED !");
		return false;
	}
	return false;
}

void Serial::saveBaudrate(){
	ESP_LOGI(FNAME,"Save auto detected serial baudrate: %d", baud[baudrate] );
	serial1_speed.set( baudrate );
	serial1_speed.commit(); // save to NVS
	int b = serial1_speed.get();
	ESP_LOGI(FNAME,"serial1_speed.get(): %d", b );
	if( b != baudrate ){
		ESP_LOGW(FNAME,"WARNING auto detected serial baudrate saved: %d != detected: %d", baud[b], baud[baudrate] );
	}
}


void Serial::huntBaudrate(){
	trials++;
	if( trials>400 ) { // An active Flarm sends every second at least
		trials = 0;
		baudrate++;
		if( baudrate > 6 ){
			baudrate=1;  // 4800
		}
		uart_set_baudrate(uart_num, baud[baudrate]);
		if( !SetupMenu::isActive() ){
			DisplayLock lock(_display);
			egl->setColor(COLOR_WHITE);
			egl->setPrintPos( 10, 40 );
			egl->printf("Autobaud: %d    ", baud[baudrate] );
		}
		ESP_LOGI(FNAME,"Serial Interface ttyS1 next baudrate: %d", baud[baudrate] );
	}
}

void Serial::begin(){
	ESP_LOGI(FNAME,"Serial::begin()" );
	// Initialize static configuration
	qMutex = xSemaphoreCreateMutex();
	baudrate = serial1_speed.get();
	ESP_LOGI(FNAME,"serial1_speed.get(): %d", baudrate );
	int br = baud[baudrate];

	uart_config_t uart_config = {
	    .baud_rate = br,
	    .data_bits = UART_DATA_8_BITS,
	    .parity = UART_PARITY_DISABLE,
	    .stop_bits = UART_STOP_BITS_1,
	    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	    .rx_flow_ctrl_thresh = 122,
		.source_clk = UART_SCLK_REF_TICK
	};

	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
	ESP_LOGI(FNAME,"Serial param config, baudrate (baud)=%d", br );
	uart_set_baudrate(uart_num, br);

	int umask = UART_SIGNAL_INV_DISABLE;
	if( rs232_polarity.get() == RS232_INVERTED ){
		umask |= UART_SIGNAL_TXD_INV;
		umask |= UART_SIGNAL_RXD_INV;
	}
	if( umask ){
		ESP_ERROR_CHECK( uart_set_line_inverse( uart_num, umask ) );
		ESP_LOGI(FNAME,"Serial param line inverse" );
	}
	if( baudrate != 0 ) {
		gpio_pullup_en( GPIO_NUM_16 );
		gpio_pullup_en( GPIO_NUM_17 );
		// Pin 38 is standard IGC RX pin, Pin 37 TX pin
		if( serial1_pins_twisted.get() ){
			if( serial1_tx_enable.get() ){
				ESP_LOGI(FNAME,"Serial pins twisted, TX enabled" );
				ESP_ERROR_CHECK(uart_set_pin(uart_num, GPIO_NUM_38, GPIO_NUM_37, GPIO_NUM_33, GPIO_NUM_34));
			}else{
				ESP_LOGI(FNAME,"Serial pins twisted, TX disabled" );
				ESP_ERROR_CHECK(uart_set_pin(uart_num, GPIO_NUM_36, GPIO_NUM_37, GPIO_NUM_33, GPIO_NUM_34));
				gpio_set_direction(GPIO_NUM_38, GPIO_MODE_INPUT);     // high impedance
				gpio_pullup_dis( GPIO_NUM_38 );
			}
		}
		else{
			if( serial1_tx_enable.get() ){
				ESP_LOGI(FNAME,"Serial pins normal, TX enabled" );
				// Set UART pins(TX, RX, RTS, CTS ) RX, RTS and CTS not wired, dummy
				ESP_ERROR_CHECK(uart_set_pin(uart_num, GPIO_NUM_37, GPIO_NUM_38, GPIO_NUM_33, GPIO_NUM_34));
			}else{

				ESP_LOGI(FNAME,"Serial pins normal, TX disable" );
				ESP_ERROR_CHECK(uart_set_pin(uart_num, GPIO_NUM_36, GPIO_NUM_38, GPIO_NUM_33, GPIO_NUM_34));
				gpio_set_direction(GPIO_NUM_37, GPIO_MODE_INPUT);     // high impedance
				gpio_pullup_dis( GPIO_NUM_37 );
			}
		}
	}
	ESP_LOGI(FNAME,"Serial Interface ttyS1 enabled with serial speed: %d baud: %d tx_inv: %d rx_inv: %d",  serial1_speed.get(), baud[serial1_speed.get()], serial1_tx_inverted.get(), serial1_rx_inverted.get() );


	const int uart_buffer_size = 512;
	QueueHandle_t uart_queue;
	// Install UART driver using an event queue here
	// esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t *uart_queue, int intr_alloc_flags)
	ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
    taskStart();

}

void Serial::taskStart(){
	ESP_LOGI(FNAME,"Serial::taskStart()" );
	xTaskCreatePinnedToCore(&serialHandler, "serialHandler1", 6192, NULL, 21, &pid, 0);
}
