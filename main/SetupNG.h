/*
 * SetupNG.h
 *
 *  Created on: August 23, 2020
 *      Author: iltis
 */

#pragma once

#include <esp_partition.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <nvs.h>

#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <freertos/queue.h>
#include <esp_system.h>
#include <esp_log.h>

#include <string>
#include <stdio.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>
#include "logdef.h"
#include "SetupCommon.h"
#include "ESP32NVS.h"
#include "SetupMenuValCommon.h"


/*
 *
 * NEW Simplified and distributed non volatile config data API with template classes:
 * SetupNG<int>  airspeed_mode( "AIRSPEED_MODE", MODE_IAS );
 *
 *  int as = airspeed_mode.get();
 *  airspeed_mode.set( MODE_TAS );
 *
 */


typedef enum rs232linemode { RS232_NORMAL, RS232_INVERTED } rs232lm_t;
typedef enum e_display_variant { DISPLAY_WHITE_ON_BLACK, DISPLAY_BLACK_ON_WHITE } display_variant_t;
typedef enum e_wireless_type { WL_DISABLE, WL_BLUETOOTH, WL_WLAN_MASTER, WL_WLAN_CLIENT, WL_WLAN_STANDALONE, WL_BLUETOOTH_LE } e_wireless_t;
typedef enum e_sync { SYNC_NONE, SYNC_FROM_MASTER, SYNC_FROM_CLIENT, SYNC_BIDIR } e_sync_t;       // determines if data is synched from/to client. BIDIR means sync at commit from both sides
typedef enum e_reset { RESET_NO, RESET_YES } e_reset_t;   // determines if data is reset to defaults on factory reset
typedef enum e_volatility { VOLATILE, PERSISTENT, SEMI_VOLATILE } e_volatility_t;  // stored in RAM, FLASH, or into FLASH after a while
typedef enum e_display_orientation { DISPLAY_NORMAL, DISPLAY_TOPDOWN } e_display_orientation_t;
typedef enum e_display_mode { DISPLAY_MULTI, DISPLAY_SIMPLE } e_display_mode_t;
typedef enum e_unit_type{ UNIT_NONE, UNIT_TEMPERATURE, UNIT_ALT, UNIT_SPEED, UNIT_VARIO, UNIT_QNH } e_unit_type_t;
typedef enum e_alt_unit { ALT_UNIT_METER, ALT_UNIT_FT, ALT_UNIT_FL } e_alt_unit_t;
typedef enum e_dst_unit { DST_UNIT_KM, DST_UNIT_FT, DST_UNIT_MILES } e_dst_unit_t;
typedef enum e_speed_unit { SPEED_UNIT_KMH, SPEED_UNIT_MPH, SPEED_UNIT_KNOTS } e_speed_unit_t;
typedef enum e_vario_unit { VARIO_UNIT_MS, VARIO_UNIT_FPM, VARIO_UNIT_KNOTS } e_vario_unit_t;
typedef enum e_data_monitor { MON_OFF, MON_S1 }  e_data_monitor_t;
typedef enum e_non_move { NON_MOVE_HIDE, NON_MOVE_DISPLAY } e_non_move_t;
typedef enum e_buzz_notify { BUZZ_OFF, BUZZ_1KM, BUZZ_2KM } e_buzz_notify_t;

void change_bal();

typedef struct setup_flags{
	bool _reset    :1;
	bool _volatile :1;
	uint8_t _sync  :2;
	uint8_t _unit  :3;
	bool _dirty    :1;
} t_setup_flags;


class test{
	public:
		test(){};
};

template<typename T> class SetupNG: public SetupCommon
{
	public:
	char typeName(void){
		if( typeid( T ) == typeid( float ) )
			return 'F';
		else if( typeid( T ) == typeid( int ) )
			return 'I';
		return 'U';
	}
	SetupNG( const char * akey,
			T adefault,  				   // unique identification TAG
			bool reset=true,               // reset data on factory reset
			e_sync_t sync=SYNC_NONE,
			e_volatility vol=PERSISTENT, // sync with client device is applicable
			void (* action)()=0,
			e_unit_type_t unit = UNIT_NONE
	)
	{
		ESP_LOGI(FNAME,"SetupNG(%s)", akey );
		if( strlen( akey ) > 15 )
			ESP_LOGE(FNAME,"SetupNG(%s) key > 15 char !", akey );
		instances->push_back( this );  // add into vector
		_key = akey;
		_default = adefault;
		flags._reset = reset;
		flags._sync = sync;
		flags._volatile = vol;
		flags._unit = unit;
		flags._dirty = false;
		_action = action;
	}

	virtual bool dirty() {
		return flags._dirty;
	}

	virtual void setValueStr( const char * val ){
		if( flags._volatile != VOLATILE ){
			if( typeid( T ) == typeid( float ) ){
				float t;
				sscanf( val,"%f", &t );
				memcpy((char *)&_value, &t, sizeof(t) );
			}
			else if( typeid( T ) == typeid( int ) ){
				int t;
				sscanf( val,"%d", &t );
				memcpy((char *)&_value, &t, sizeof(t) );
			}
			flags._dirty = true;
		}

	}

	inline T* getPtr() {
		return &_value;
	}
	inline T& getRef() {
		return _value;
	}
	inline T get() const {
		return _value;
	}
	const char * key() {
		return _key;
	}

	virtual T getGui() const { return get(); } // tb. overloaded for blackboard
	virtual const char* unit() const { return ""; } // tb. overloaded for blackboard

	virtual bool value_str(char *str){
		if( flags._volatile != VOLATILE ){
			if( typeid( T ) == typeid( float ) ){
				float t;
				memcpy(&t, &_value, sizeof(t) );
				sprintf( str,"%f", t );
				return true;
			}
			else if( typeid( T ) == typeid( int ) ){
				int t;
				memcpy(&t, &_value, sizeof(t) );
				sprintf( str,"%d", t );
				return true;
			}
		}
		return false;
	}

	bool set( T aval, bool dosync=true, bool doAct=true ) {
		if( _value == aval ){
			ESP_LOGI(FNAME,"Value already in config: %s", _key );
			return( true );
		}
		_value = aval;
		if ( dosync ) {
			sync();
		}
		if( doAct ){
			if( _action != 0 ) {
				(*_action)();
			}
		}
		if( flags._volatile == VOLATILE ){
			return true;
		}
		flags._dirty = true;
		ESP_LOGI(FNAME,"set() %s", _key );
		return true;
	}

	e_unit_type_t unitType() {
		return (e_unit_type_t)flags._unit;
	}

	void ack( T aval ){
		if( aval != _value ){
			ESP_LOGI(FNAME,"sync to value client has acked");
			_value = aval;
		}
	}


	bool sync(){
		return true;
	}

	bool commit() {
		ESP_LOGI(FNAME,"NVS commit(): %s ", _key );
		if( flags._volatile != PERSISTENT ){
				return true;
		}
		write();
		bool ret = NVS.commit();
		if( !ret )
			return false;
		flags._dirty = false;
		return true;
	}

	bool write() { // do the set blob that actually seems to write to the flash either
		// ESP_LOGI(FNAME,"NVS write(): ");
		char val[30];
		value_str(val);
		ESP_LOGI(FNAME,"write() NVS set blob(key:%s, val: %s, len:%d )", _key, val, sizeof( _value ) );
		bool ret = NVS.setBlob( _key, (void *)(&_value), sizeof( _value ) );
		if( !ret )
			return false;
		return true;
	}

	bool exists() {
		if( flags._volatile != PERSISTENT ) {
			return true;
		}
		size_t size;
		bool ret = NVS.getBlob(_key, NULL, &size);
		return ret;
	}

	virtual bool init() {
		if( flags._volatile != PERSISTENT ){
			// ESP_LOGI(FNAME,"NVS volatile set default");
			set( _default );
			return true;
		}
		size_t required_size;
		bool ret = NVS.getBlob(_key, NULL, &required_size);
		if ( !ret ){
			ESP_LOGE(FNAME, "%s: NVS nvs_get_blob error", _key );
			set( _default );  // try to init
			commit();
		}
		else {
			// ESP_LOGI(FNAME,"NVS %s size: %d", _key, required_size );
			if( required_size > sizeof( T ) ) {
				ESP_LOGE(FNAME,"NVS error: size too big: %d > %d", required_size , sizeof( T ) );
				erase();
				set( _default );  // try to init
				return false;
			}
			else {
				// ESP_LOGI(FNAME,"NVS size okay");
				ret = NVS.getBlob(_key, &_value, &required_size);

				if ( !ret ){
					ESP_LOGE(FNAME, "NVS nvs_get_blob returned error");
					erase();
					set( _default );  // try to init
					commit();
				}
				else {
					// char val[30];
					// value_str(val);
					// ESP_LOGI(FNAME,"NVS key %s exists len: %d, value: %s", _key, required_size, val );
				}
			}
		}
		return true;
	}

	virtual bool erase() {
		if( flags._volatile != PERSISTENT ){
			return true;
		}
		bool ret = NVS.erase(_key);
		if( !ret ){
			return false;
		}
		else {
			ESP_LOGI(FNAME,"NVS erased key  %s", _key );
			return set( _default );
		}
	}

	virtual bool mustReset() {
		return flags._reset;
	}

	virtual bool isDefault() {
		if( _default == _value )
			return true;
		else
			return false;
	}

	inline T getDefault() const { return _default; }
	inline uint8_t getSync() { return flags._sync; }

private:
	T _value;
	T _default;
	const char * _key;
	t_setup_flags flags;
	void (* _action)();
};

extern SetupNG<int>  		serial1_speed;
extern SetupNG<int>  		serial1_rxloop;
extern SetupNG<int>  		serial1_tx;
extern SetupNG<int>		    serial1_pins_twisted;
extern SetupNG<int>  		serial1_tx_inverted;
extern SetupNG<int>  		serial1_rx_inverted;
extern SetupNG<int>  		serial1_tx_enable;

extern SetupNG<int>  		software_update;
extern SetupNG<int>		    log_level;
extern SetupNG<int>         factory_reset;
extern SetupNG<int>         log_scale;

extern uint8_t g_col_background;
extern uint8_t g_col_highlight;

extern SetupNG<int>         ias_unit;
extern SetupNG<int>         alt_unit;
extern SetupNG<int>         dst_unit;
extern SetupNG<int>         vario_unit;

extern SetupNG<int>  		display_test;
extern SetupNG<float>  		audio_volume;
extern SetupNG<int> 		data_monitor;
extern SetupNG<int> 		traffic_demo;
extern SetupNG<int>  		display_orientation;
extern SetupNG<int>  		display_mode;
extern SetupNG<int>  		display_non_moving_target;
extern SetupNG<int>  		notify_near;
extern SetupNG<int>  		log_scale;


