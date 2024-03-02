/*
 * SetupMenu.cpp
 *
 *  Created on: Feb 4, 2018
 *      Author: iltis
 */

#include "SetupMenu.h"
#include "Version.h"
#include "Units.h"
#include "Switch.h"
#include "SetupMenuSelect.h"
#include "SetupMenuValFloat.h"
#include "SetupMenuChar.h"
#include "MenuEntry.h"
#include "esp_wifi.h"

#include <inttypes.h>
#include <iterator>
#include <algorithm>
#include <logdef.h>
#include <cstring>
#include <string>
#include "SetupNG.h"
#include "Colors.h"
#include "flarmview.h"

SetupMenuSelect * audio_range_sm = 0;
SetupMenuSelect * mpu = 0;
static bool enable_restart = false;

// Menu for flap setup

float elev_step = 1;
bool SetupMenu::focus = false;
bool SetupMenu::_menu_active = false;

int do_display_test(SetupMenuSelect * p){
	if( display_test.get() ){
		egl->setColor( COLOR_WHITE );
		egl->drawBox( 0, 0, 320, 176 );
		while( swMode.isOpen() ){
			delay(100);
			ESP_LOGI(FNAME,"Wait for key press");
		}
		egl->setColor( COLOR_BLACK );
		egl->drawBox( 0, 0, 320,176 );
		while( swMode.isOpen() ){
			delay(100);
			ESP_LOGI(FNAME,"Wait for key press");
		}
		esp_restart();
	}
	return 0;
}

int data_mon( SetupMenuSelect * p ){
	ESP_LOGI(FNAME,"data_mon( %d ) ", data_monitor.get() );
	if( data_monitor.get() != MON_OFF ){
		// DM.start(p);
	}
	return 0;
}

int vol_adj( SetupMenuValFloat * p ){
        // Audio::setVolume( (*(p->_value)) );
        return 0;
}


SetupMenu::SetupMenu() : MenuEntry() {
	highlight = -1;
	_parent = 0;
	helptext = 0;
}

SetupMenu::SetupMenu( const char *title ) : MenuEntry() {
	ESP_LOGI(FNAME,"SetupMenu::SetupMenu( %s ) ", title );
	attach(this);
	_title = title;
	highlight = -1;
}

SetupMenu::~SetupMenu()
{
	// ESP_LOGI(FNAME,"del SetupMenu( %s ) ", _title );
	detach(this);
}

void SetupMenu::begin(){
	ESP_LOGI(FNAME,"SetupMenu() begin");
	setup();
}

void SetupMenu::setup( )
{
	ESP_LOGI(FNAME,"SetupMenu setup()");
	SetupMenu * root = new SetupMenu( PROGMEM"Setup" );
	root->addCreator( setup_create_root );
	root->create_subtree();
	selected = root;
	root->pressed = true;
	root->_parent = 0;
	ESP_LOGI(FNAME,"%p pressed %d", root, root->pressed );
}

void SetupMenu::catchFocus( bool activate ){
	focus = activate;
}

void SetupMenu::display( int mode ){
	ESP_LOGI(FNAME,"SetupMenu display (%s) s:%p t:%p", _title, selected, this );
	if( (selected != this) ){
		ESP_LOGI(FNAME,"Not me: return");
		return;
	}
	if( !dirty ){
		ESP_LOGI(FNAME,"Not dirty: return");
		return;
	}
	ESP_LOGI(FNAME,"SetupMenu display(%s) %d", _title, focus );
	clear();
	int y=25;
	// ESP_LOGI(FNAME,"Title: %s y=%d child size:%d", selected->_title,y, _childs.size()  );
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos(1,y);
	egl->setFontPosBottom();
	egl->printf("<< %s",selected->_title);
	egl->drawFrame( 1,(selected->highlight+1)*25+3,318,25 );
	for (int i=0; i<_childs.size(); i++ ){
		MenuEntry * child = _childs[i];
		egl->setPrintPos(1,(i+1)*25+25);
		egl->setColor( COLOR_HEADER_LIGHT );
		egl->printf("%s",child->_title);
		// ESP_LOGI(FNAME,"Child Title: %s", child->_title );
		if( child->value() ){
			int fl=egl->getStrWidth( child->_title );
			egl->setPrintPos(1+fl,(i+1)*25+25);
			egl->printf(": ");
			egl->setPrintPos(1+fl+egl->getStrWidth( ":" ),(i+1)*25+25);
			egl->setColor( COLOR_WHITE );
			egl->printf(" %s",child->value());
		}
		egl->setColor( COLOR_WHITE );
		// ESP_LOGI(FNAME,"Child: %s y=%d",child->_title ,y );
	}
	y+=170;
	showhelp( y );
}

void SetupMenu::up(int count){
	if( (selected != this) || !_menu_active )
		return;
	ESP_LOGI(FNAME,"down %d %d", highlight, _childs.size() );
	if( focus )
		return;
	egl->setColor(COLOR_BLACK);
	egl->drawFrame( 1,(highlight+1)*25+3,318,25 );
	egl->setColor(COLOR_WHITE);
	if( highlight  > -1 ){
		highlight --;
	}
	else
		highlight = (int)(_childs.size() -1 );
	egl->drawFrame( 1,(highlight+1)*25+3,318,25 );
	pressed = true;
}

void SetupMenu::down(int count){
	if( (selected != this) || !_menu_active )
		return;
	ESP_LOGI(FNAME,"SetupMenu::up %d %d", highlight, _childs.size() );
	if( focus )
		return;
	egl->setColor(COLOR_BLACK);
	egl->drawFrame( 1,(highlight+1)*25+3,318,25 );
	egl->setColor(COLOR_WHITE);
	if( highlight < (int)(_childs.size()-1) ){
		highlight ++;
	}
	else
		highlight = -1;
	egl->drawFrame( 1,(highlight+1)*25+3,318,25 );
	pressed = true;
}

void SetupMenu::showMenu(){
	ESP_LOGI(FNAME,"showMenu(%s) s:%p parent %p", _title, this,  _parent );
	// default is not pressed, so just display, but we toogle pressed state at the end
	// so next time we either step up to parent,
	if( pressed )
	{
		if( highlight == -1 ) {
			if( _parent != 0 ){
				ESP_LOGI(FNAME,"SetupMenu to parent");
				selected = _parent;
				selected->highlight = -1;
				selected->pressed = true;
				// delete_subtree();
			}
		}
		else {
			if( (highlight >=0) && (highlight < (int)(_childs.size()) ) ){
				ESP_LOGI(FNAME,"SetupMenu to child %d size: %d", highlight, _childs.size() );
				selected = _childs[highlight];
				selected->create_subtree();
				selected->pressed = false;
			}
		}
		selected->dirty = true;
		selected->display();
	}
	if( (_parent == 0) && (highlight == -1) ) // entering setup menu root
	{
		ESP_LOGI(FNAME,"Check End Setup Menu");
		if( enable_restart ){
			ESP_LOGI(FNAME,"Restart enabled");
			if( selected->get_restart() )
				selected->restart();
			esp_restart();
		}else{
			ESP_LOGI(FNAME,"Now enable Restart");
			enable_restart = true;
		}
	}
	ESP_LOGI(FNAME,"end showMenu()");
}

void SetupMenu::press(){
	ESP_LOGI(FNAME,"press() %s s:%p t:%p pressed:%d", _title, selected, this, pressed );
	if( selected != this ){
		ESP_LOGI(FNAME,"Not me: %s return()", _title  );
		return;
	}
	_menu_active = true;
	showMenu();
	if( pressed )
		pressed = false;
	else
		pressed = true;
	ESP_LOGI(FNAME,"End Longpress()");
}

void SetupMenu::longPress(){
	ESP_LOGI(FNAME,"Longpress() %s s:%p t:%p pressed:%d", _title, selected, this, pressed );
	if( selected != this ){
		ESP_LOGI(FNAME,"Not me: %s return()", _title  );
		return;
	}
	ESP_LOGI(FNAME,"End Longpress()");
}

void SetupMenu::escape(){
	ESP_LOGI(FNAME,"escape now Setup Menu");
}


void SetupMenu::options_menu_create_units( MenuEntry *top ){
        SetupMenuSelect * alu = new SetupMenuSelect( PROGMEM"Altitude", RST_NONE, 0, true, &alt_unit );
        alu->addEntry( PROGMEM"Meter (m)");
        alu->addEntry( PROGMEM"Feet (ft)");
        alu->addEntry( PROGMEM"FL (FL)");
        top->addEntry( alu );
        SetupMenuSelect * vau = new SetupMenuSelect( PROGMEM"Vario", RST_NONE , 0, true, &vario_unit );
        vau->addEntry( PROGMEM"Meters/sec (m/s)");
        vau->addEntry( PROGMEM"Feet/min x 100 (fpm)");
        vau->addEntry( PROGMEM"Knots (kt)");
        top->addEntry( vau );
        SetupMenuSelect * dst = new SetupMenuSelect( PROGMEM"Distance", RST_NONE , 0, true, &dst_unit );
        dst->addEntry( PROGMEM"KiloMeter (km)");
        dst->addEntry( PROGMEM"KiloFeet (kft)");
        dst->addEntry( PROGMEM"NauticalMiles (nm)");
        top->addEntry( dst );
}



void SetupMenu::setup_create_root(MenuEntry *top ){
	ESP_LOGI(FNAME,"setup_create_root()");
	SetupMenuValFloat * vol = new SetupMenuValFloat( PROGMEM"Buzzer Volume", "%", 0.0, 100, 10, vol_adj, false, &audio_volume );
	vol->setHelp(PROGMEM"Buzzer volume maximum level", 110 );
	top->addEntry( vol );

	SetupMenu * un = new SetupMenu( PROGMEM"Units" );
	top->addEntry( un );
	un->setHelp( PROGMEM"Setup imperial units for alt(itude), dis(tance), var(iometer)", 125);
	un->addCreator(options_menu_create_units);

	SetupMenuSelect * demo = new SetupMenuSelect( PROGMEM"Traffic Demo", RST_IMMEDIATE, 0, true, &traffic_demo );
	demo->addEntry( PROGMEM"Cancel");
	demo->addEntry( PROGMEM"Start");
	top->addEntry( demo );

	// Orientation   _display_orientation
	if( inch2dot4 ) {
		SetupMenuSelect * diso = new SetupMenuSelect( "Orientation", RST_ON_EXIT, 0, true, &display_orientation );
		top->addEntry( diso );
		diso->setHelp( "Display Orientation. NORMAL means Up/Down on left side, TOPDOWN means Up/Down on the right (reboots)");
		diso->addEntry( "NORMAL");
		diso->addEntry( "TOPDOWN");
	}


}



