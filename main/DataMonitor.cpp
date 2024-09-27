#include "DataMonitor.h"
#include "flarmview.h"
#include "logdef.h"
#include "Flarm.h"
#include "Colors.h"

#define SCROLL_TOP      30
#define SCROLL_BOTTOM  172

xSemaphoreHandle DataMonitor::mutex = 0;

DataMonitor::DataMonitor(){
	mon_started = false;
	ucg = 0;
	scrollpos = SCROLL_BOTTOM;
	paused = true;
	setup = 0;
	channel = MON_OFF;
	mutex = xSemaphoreCreateMutex();
	first=true;
	rx_total = 0;
	tx_total = 0;
}

int DataMonitor::maxChar( const char *str, int pos, int len, bool binary ){
	int N=0;
	int i=0;
	char s[4] = { 0 };
	while( N <= 320 ){
		if( binary ){
			sprintf( s, "%02x ", str[i+pos] );
		}
		else{
			s[0] = str[i+pos];
		}
		N += ucg->getStrWidth( s );
		if( N<290 && (i+pos)<len ){
			i++;
		}else{
			break;
		}
	}
	return i;
}

void DataMonitor::header( int ch, bool binary ){
	const char * what;
	switch( ch ) {
		case 1:  what = "S1"; break;
		default: what = "OFF"; break;
	}
	const char * b;
	if( binary )
		b = "B-";
	else
		b = "";
	ucg->setPrintPos( 20, SCROLL_TOP );
	ucg->printf( "%s%s: RX:%d TX:%d bytes    ", b, what, rx_total, tx_total );
}

void DataMonitor::monitorString( int ch, e_dir_t dir, const char *str, int len ){
	if( xSemaphoreTake(mutex,portMAX_DELAY ) ){
		if( !mon_started || paused || (ch != channel) ){
			// ESP_LOGI(FNAME,"not active, return started:%d paused:%d  (%d-%d)", mon_started, paused, ch, channel );
			xSemaphoreGive(mutex);
			return;
		}
		bool binary = false;
		printString( ch, dir, str, binary, len );
		xSemaphoreGive(mutex);
	}
}

void DataMonitor::printString( int ch, e_dir_t dir, const char *str, bool binary, int len ){
	// ESP_LOGI(FNAME,"DM ch:%d dir:%d len:%d data:%s", ch, dir, len, str );
	const int scroll_lines = 15;
	char dirsym = 0;
	if( dir == DIR_RX ){
		dirsym = '>';
		rx_total += len;
	}
	else{
		dirsym = '<';
		tx_total += len;
	}
	if( first ){
		first = false;
		ucg->setColor( COLOR_BLACK );
		ucg->drawBox( 0,SCROLL_TOP,320,172 );
	}
	ucg->setColor( COLOR_WHITE );
    header( ch, binary );
	//if( !binary )
	// 	len = len-1;  // ignore the \n in ASCII mode
	int hunklen = 0;
	int pos=0;
	do {
		// ESP_LOGI(FNAME,"DM 1 len: %d pos: %d", len, pos );
		hunklen = maxChar( str, pos, len, binary );
		if( hunklen ){
			char hunk[128] = { 0 };
			memcpy( (void*)hunk, (void*)(str+pos), hunklen );
			// ESP_LOGI(FNAME,"DM 2 hunklen: %d pos: %d  h:%s", hunklen, pos, hunk );
			ucg->setColor( COLOR_BLACK );
			ucg->drawBox( 0, scrollpos-3, 320,scroll_lines+3 );
			ucg->setColor( COLOR_WHITE );
			ucg->setPrintPos( 0, scrollpos+scroll_lines );
			ucg->setFont(ucg_font_fub11_tr, true );
			char txt[256];
			int hpos = 0;
			if( binary ){   // format data as readable text
				hpos += sprintf( txt, "%c ", dirsym );
				for( int i=0; i<hunklen && hpos<130 ; i++ ){
					hpos += sprintf( txt+hpos, "%02x ", hunk[i] );
				}
				txt[hpos] = 0; // zero terminate string
				ucg->print( txt );
				// ESP_LOGI(FNAME,"DM binary ch:%d dir:%d string:%s", ch, dir, txt );
			}
			else{
				hpos += sprintf( txt, "%c ", dirsym );
				hpos += sprintf( txt+hpos, "%s", hunk );
				txt[hpos] = 0;
				ucg->print( txt );
				// ESP_LOGI(FNAME,"DM ascii ch:%d dir:%d data:%s", ch, dir, txt );
			}
			pos+=hunklen;
			// ESP_LOGI(FNAME,"DM 3 pos: %d", pos );
			scroll(scroll_lines);
		}
	}while( hunklen );
}

void DataMonitor::scroll(int scroll){
	scrollpos+=scroll;
	if( scrollpos >= SCROLL_BOTTOM )
		scrollpos = SCROLL_TOP;
	// ucg->scrollLines( scrollpos );  // set frame origin
}

void DataMonitor::press(){
	ESP_LOGI(FNAME,"press paused: %d", paused );
	if( !Switch::isClosed() ){ // only process press here
	if( paused )
		paused = false;
	else
		paused = true;
	}
	delay( 100 );
}

void DataMonitor::longPress(){
	ESP_LOGI(FNAME,"longPress" );
	if( !mon_started ){
		ESP_LOGI(FNAME,"longPress, but not started, return" );
		return;
	}
	mon_started = false;
	delay( 500 );
}

void DataMonitor::start(SetupMenuSelect * p){
	ESP_LOGI(FNAME,"start");
	if( !setup )
		attach( this );
	setup = p;
	tx_total = 0;
	rx_total = 0;
	channel = p->getSelect();
	ucg->setColor( COLOR_BLACK );
	ucg->drawBox( 0,0,320,172 );
	ucg->setColor( COLOR_WHITE );
	ucg->setFont(ucg_font_fub11_tr, true );
	header( channel );
	if( display_orientation.get() == DISPLAY_TOPDOWN )
		ucg->scrollSetMargins( 0, SCROLL_TOP );
	else
		ucg->scrollSetMargins( SCROLL_TOP, 0 );
	mon_started = true;
	paused = false;
	ESP_LOGI(FNAME,"started");
	int timer=0;
	while( mon_started ){
		delay( 10 );
		// ESP_LOGI(FNAME,"started %d", timer);
		if( Switch::isClosed() ){ // only process press here
			timer++;
			if( paused )
				paused = false;
			else
				paused = true;
			delay(200);
		}else{
			timer=0;
		}
		if(timer>5){
			stop();
			break;
		}
	}
	// stop();
	ESP_LOGI(FNAME,"finished");
}

void DataMonitor::stop(){
	ESP_LOGI(FNAME,"stop");
	channel = MON_OFF;
	mon_started = false;
	paused = false;
	delay(100);
	// ucg->scrollLines( 0 );
	setup->setSelect( MON_OFF );
}

