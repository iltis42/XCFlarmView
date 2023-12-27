/*
 * MenuEntry.h
 *
 *  Created on: Dec 26, 2023
 *      Author: iltis
 */

#pragma once

#include "Switch.h"
#include "SetupNG.h"
#include "AdaptUGC.h"

#include <vector>
#include <string>


class MenuEntry: public SwitchObserver {
public:
	MenuEntry() : SwitchObserver() {
		highlight = 0;
		_parent = 0;
		pressed = false;
		helptext = 0;
		hypos = 0;
		_title = 0;
		subtree_created = 0;
		menu_create_ptr = 0;
	};
	virtual ~MenuEntry();
	virtual void display( int mode=0 ) = 0;
	virtual void release() { display(); };
	virtual void longPress() {};
	virtual void press() {};
	virtual void doubleClick() {};
	virtual const char* value() = 0;
    MenuEntry* getFirst() const;
	MenuEntry* addEntry( MenuEntry * item );
	MenuEntry* addEntry( MenuEntry * item, const MenuEntry* after );
	void       delEntry( MenuEntry * item );
	MenuEntry* findMenu( std::string title, MenuEntry* start=root  );
	void togglePressed() { pressed = ! pressed; }
	void setHelp( const char *txt, int y=180 ) { helptext = (char*)txt; hypos = y; };
	void showhelp( int y );
	void clear();
	void uprintf( int x, int y, const char* format, ...);
	void uprint( int x, int y, const char* str );
	void semaphoreTake();
    void semaphoreGive();
    void restart();
    bool get_restart() { return _restart; };
    void addCreator( void (menu_create)(MenuEntry*ptr) ){ menu_create_ptr=menu_create; }
    static void setRoot( MenuEntry *root ) { selected = root; };
public:
	std::vector<MenuEntry*>  _childs;
	MenuEntry *_parent;
	const char * _title;
	int8_t    highlight;
	uint8_t   pressed;
	char      *helptext;
	int16_t    hypos;
	void (*menu_create_ptr)(MenuEntry*);
	uint8_t subtree_created;
	static AdaptUGC *ucg;
	static MenuEntry *root;
	static MenuEntry *selected;
	static bool _restart;
};
