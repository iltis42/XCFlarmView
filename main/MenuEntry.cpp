/*
 * MenuEntry.cpp
 *
 *  Created on: Dec 26, 2023
 *      Author: iltis
 */

// #include "SetupMenu.h"
// #include "IpsDisplay.h"
#include <inttypes.h>
#include <iterator>
#include <algorithm>
#include "Version.h"
#include <logdef.h>
#include "Units.h"
#include "Switch.h"
#include "MenuEntry.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Colors.h"
#include "AdaptUGC.h"

MenuEntry* MenuEntry::root = 0;
MenuEntry* MenuEntry::selected = 0;
bool MenuEntry::_restart = false;

extern xSemaphoreHandle _display;
xSemaphoreHandle spiMutex=NULL;

MenuEntry::~MenuEntry()
{
    // ESP_LOGI(FNAME,"del menu %s",_title );
    detach(this);
    for ( MenuEntry* c : _childs ) {
        delete c;
        c = nullptr;
    }
}

void MenuEntry::create_subtree(){
	if( !subtree_created && menu_create_ptr ){
		(menu_create_ptr)(this);
		subtree_created = true;
		// ESP_LOGI(FNAME,"create_subtree() %d", _childs.size() );
	}
}

void MenuEntry::delete_subtree(){
	// ESP_LOGI(FNAME,"delete_subtree() %d", _childs.size() );
	if( subtree_created && menu_create_ptr ){
		subtree_created = false;
		for (int i=0; i<_childs.size(); i++ ){
			delete _childs[i];
		}
		_childs.clear();
	}
}

void MenuEntry::uprintf( int x, int y, const char* format, ...) {
	if( egl == 0 ) {
		ESP_LOGE(FNAME,"Error egl not initialized !");
		return;
	}
	va_list argptr;
	va_start(argptr, format);
	egl->setPrintPos(x,y);
	egl->printf( format, argptr );
	va_end(argptr);
}

void MenuEntry::restart(){
	clear();
	ESP_LOGI(FNAME,"Restart now");
	egl->setPrintPos( 10, 50 );
	egl->print("...rebooting now" );
	delay(2000);
	esp_restart();
}

void MenuEntry::uprint( int x, int y, const char* str ) {
	if( egl == 0 ) {
		ESP_LOGE(FNAME,"Error egl not initialized !");
		return;
	}
	egl->setPrintPos(x,y);
	egl->print( str );
}

MenuEntry* MenuEntry::getFirst() const {
	// ESP_LOGI(FNAME,"MenuEntry::getFirst()");
	return _childs.front();
}

MenuEntry* MenuEntry::addEntry( MenuEntry * item ) {
	ESP_LOGI(FNAME,"MenuEntry addMenu() title %s", item->_title );
	/*
	if( root == 0 ){
		ESP_LOGI(FNAME,"Init root menu");
		root = item;
		item->_parent = 0;
		selected = item;
		return item;
	}
	else{
	*/
		// ESP_LOGI(FNAME,"add to childs");
		item->_parent = this;
		_childs.push_back( item );
		return item;
	// }
}

MenuEntry* MenuEntry::addEntry( MenuEntry * item, const MenuEntry* after ) {
	// ESP_LOGI(FNAME,"AddMenuEntry title %s after %s", item->_title, after->_title );
	if( root == 0   ){
		return addEntry(item);
	}
	else{
        std::vector<MenuEntry *>::iterator position = std::find(_childs.begin(), _childs.end(), after );
        if (position != _childs.end()) {
            item->_parent = this;
            _childs.insert( ++position, item );
            return item;
        }
        else { return addEntry(item); }
	}
}


void MenuEntry::delEntry( MenuEntry * item ) {
	ESP_LOGI(FNAME,"MenuEntry delMenu() title %s", item->_title );
	std::vector<MenuEntry *>::iterator position = std::find(_childs.begin(), _childs.end(), item );
	if (position != _childs.end()) { // == myVector.end() means the element was not found
		ESP_LOGI(FNAME,"found entry, now erase" );
		_childs.erase(position);
        delete *position;
	}
}

MenuEntry* MenuEntry::findMenu( std::string title, MenuEntry* start )
{
	ESP_LOGI(FNAME,"MenuEntry findMenu() %s %x", title.c_str(), (uint32_t)start );
	if( std::string(start->_title) == title ) {
		ESP_LOGI(FNAME,"Menu entry found for start %s", title.c_str() );
		return start;
	}
	for(MenuEntry* child : start->_childs) {
		if( std::string(start->_title) == title )
			return child;
		MenuEntry* m = child->findMenu( title, child );
		if( m != 0 ) {
			ESP_LOGI(FNAME,"Menu entry found for %s", title.c_str() );
			return m;
		}
	};
	ESP_LOGW(FNAME,"Menu entry not found for %s", title.c_str() );
	return 0;
}

void MenuEntry::showhelp( int y ){
	if( helptext != 0 ){
		int w=0;
		char *buf = (char *)malloc(512);
		memset(buf, 0, 512);
		memcpy( buf, helptext, strlen(helptext));
		char *p = strtok (buf, " ");
		char *words[100];
		while (p != NULL)
		{
			words[w++] = p;
			p = strtok (NULL, " ");
		}
		// ESP_LOGI(FNAME,"showhelp number of words: %d", w);
		int x=1;
		int y=hypos;
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

void MenuEntry::clear()
{
	// ESP_LOGI(FNAME,"MenuEntry::clear");
	DisplayLock lock(_display);
	egl->setColor(COLOR_BLACK);
	egl->drawBox( 0,0,DISPLAY_W,DISPLAY_H );
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( 1, 30 );
	egl->setColor(COLOR_WHITE);
}


