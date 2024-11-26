/*
 * SetupMenu.h
 *
 *  Created on: Feb 4, 2018
 *      Author: iltis
 */

#ifndef _SetupMenu_H_
#define _SetupMenu_H_
#include "MenuEntry.h"
#include <string>
#include <driver/gpio.h>
#include "SetupMenuValFloat.h"

class IpsDisplay;
class ESPRotary;
class PressureSensor;
class AnalogInput;

class SetupMenu:  public MenuEntry {
public:
	SetupMenu();
	SetupMenu( const char* title );
	virtual ~SetupMenu();
	void begin();
	void setup();
	void display( int mode=0 );
	const char *value() { return 0; };
	void up(int count);  // step up to parent
	void down(int count);
	void press();
	void longPress();
	void escape();
	void showMenu();
	static inline bool isActive() { return _menu_active; };

	static void catchFocus( bool activate );
	static bool focus;
	static bool _menu_active;
	static int hpos;

	static void setup_create_root( MenuEntry *top );
	static void options_menu_create_units( MenuEntry *top );
	static void options_menu_create_buzz( MenuEntry *top );
	static void options_menu_create_settings( MenuEntry *top );
};

#endif
