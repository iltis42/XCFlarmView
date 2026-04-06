#include "DataMonitor.h"
#include "flarmview.h"
#include <logdef.h>
#include "Flarm.h"
#include "SetupMenu.h"


#define SCROLL_BOTTOM  DISPLAY_H

#if( DISPLAY_W == 240 )
#define BINLEN 95
#define SCROLL_LINES 20
#define INFO_START 40
#else
#define BINLEN 130
#define SCROLL_LINES 15
#define INFO_START  30
#endif

extern xSemaphoreHandle _display;
extern bool enable_restart;

DataMonitor::DataMonitor(){
	mon_started = false;
	ucg = 0;
	scrollpos = SCROLL_BOTTOM;
	paused = true;
	setup = 0;
	channel = MON_OFF;
	first=true;
	rx_total = 0;
	tx_total = 0;
}

int DataMonitor::maxChar( const char *str, int pos, int len, bool binary ){
	int N=0;
	int i=0;
	char s[4] = { 0 };
	while( N <= DISPLAY_W && (i + pos) < len ){
		if( binary ){
			sprintf( s, "%02x ", str[i+pos] );
		}
		else{
			s[0] = str[i+pos];
		}
		N += ucg->getStrWidth( s );
		if( N<DISPLAY_W-20 && (i+pos)<len ){
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
		case MON_S1:  what = "S1"; break;
		default:      what = "OFF"; break;
	}
	const char * b;
	if( binary )
		b = "B-";
	else
		b = "";
	ucg->setPrintPos( 20, INFO_START );
	ucg->printf( "%s%s: RX:%d TX:%d bytes   ", b, what, rx_total, tx_total );
}


void DataMonitor::monitorString( int ch, e_dir_t dir, const char *str, int len ){
	if( !mon_started || paused || (ch != channel) ){
		// ESP_LOGI(FNAME,"not active, return started:%d paused:%d", mon_started, paused );
		return;
	}else{
		DisplayLock lock(_display);
		bool binary = false;
		printString( ch, dir, str, binary, len );
	}
}



void DataMonitor::printString( int ch, e_dir_t dir, const char *str, bool binary, int len ){
	// ESP_LOGI(FNAME,"DM ch:%d dir:%d len:%d data:%s", ch, dir, len, str );
	const int scroll_lines = SCROLL_LINES;
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
		ucg->drawBox( 0, INFO_START,DISPLAY_W,DISPLAY_H );
	}
	ucg->setColor( COLOR_WHITE );
    header( ch, binary );

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
			ucg->drawBox( 0, scrollpos-3, DISPLAY_W,scroll_lines+3 );
			ucg->setColor( COLOR_WHITE );
			ucg->setPrintPos( 0, scrollpos+scroll_lines );
			ucg->setFont(ucg_font_fub11_tr, true );
			char txt[512];
			int hpos = 0;
			if( binary ){   // format data as readable text
				hpos += sprintf( txt, "%c ", dirsym );
				for( int i=0; i<hunklen && hpos<BINLEN ; i++ ){
					hpos += sprintf( txt+hpos, "%02x ", hunk[i] );
				}
				txt[hpos] = 0; // zero terminate string
				ucg->print( txt );
				// ESP_LOGI(FNAME,"DM binary ch:%d dir:%d string:%s", ch, dir, txt );
			}
			else{
				hpos += sprintf( txt, "%c ", dirsym );
				hpos += snprintf(txt + hpos, sizeof(txt) - hpos, "%s", hunk);
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
		scrollpos = INFO_START;
#if( DISPLAY_W == 240 )   // small display can't scroll vertically (as built in)
	ucg->scrollLines( scrollpos );  // set frame origin
#endif
}

void DataMonitor::up( int count ){
	ESP_LOGI(FNAME,"up" );
	if( paused )
		paused = false;
	else
		paused = true;
}

void DataMonitor::press(){
	ESP_LOGI(FNAME,"press" );
}

void DataMonitor::longLongPress(){
	ESP_LOGI(FNAME,"longPress" );
	stop();
}

void DataMonitor::start(SetupMenuSelect * p){
	ESP_LOGI(FNAME,"start");
	if( !setup )
		attach( this );
	setup = p;
	tx_total = 0;
	rx_total = 0;
	channel = p->getSelect();
	SetupMenu::catchFocus( true );
	ucg->setColor( COLOR_BLACK );
	ucg->drawBox( 0,0,DISPLAY_W,DISPLAY_H );
	ucg->setColor( COLOR_WHITE );
	ucg->setFont(ucg_font_fub11_tr, true );
	header( channel );
#if( DISPLAY_W == 240 )
	ucg->scrollSetMargins( INFO_START, 0 );
#endif
	mon_started = true;
	paused = false;
	ESP_LOGI(FNAME,"started");
}

void DataMonitor::stop(){
	ESP_LOGI(FNAME,"stop");
	delay(100);
	esp_restart();
}

