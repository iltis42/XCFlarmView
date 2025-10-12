/*
 * TargetManager.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2
 */

#include "TargetManager.h"
#include "inttypes.h"
#include "Flarm.h"
#include "Buzzer.h"
#include "Colors.h"
#include "vector.h"
#include "Switch.h"
#include "SetupMenu.h"
#include "flarmview.h"

std::map< unsigned int, Target> TargetManager::targets;
std::map< unsigned int, Target>::iterator TargetManager::id_iter = targets.begin();
extern AdaptUGC *egl;
float TargetManager::oldN   = -1.0;
int TargetManager::id_timer =  0;
int TargetManager::_tick =  0;
int TargetManager::holddown =  0;
TaskHandle_t TargetManager::pid = 0;
unsigned int TargetManager::min_id = 0;
unsigned int TargetManager::maxcl_id = 0;
bool TargetManager::redrawNeeded = true;
bool TargetManager::erase_info = false;
unsigned int TargetManager::team_id = 0;
int TargetManager::info_timer = 0;
float TargetManager::old_radius=0.0;
xSemaphoreHandle _display=NULL;
xSemaphoreHandle _pflaa_receive=NULL;
Target* TargetManager::theInfoTarget=NULL;

#define INFO_TIME (5*(1000/TASKPERIOD)/DISPLAYTICK)  // all ~10 sec

void TargetManager::begin(){
	_display=xSemaphoreCreateMutex();
	_pflaa_receive=xSemaphoreCreateMutex();
	xTaskCreatePinnedToCore(&taskTargetMgr, "taskTargetMgr", 4096, NULL, 10, &pid, 0);
	attach( this );
}

void TargetManager::taskTargetMgr(void *pvParameters){
	while(1){
		if( !SetupMenu::isActive() ){
			tick();
		}
		delay(TASKPERIOD);
	}
}

TargetManager::TargetManager() {
	// TODO Auto-generated constructor stub
}

void TargetManager::receiveTarget( nmea_pflaa_s &pflaa ){
	// ESP_LOGI(FNAME,"ID %06X (dec) %d ", pflaa.ID, pflaa.ID );
	if( (pflaa.groundSpeed < 10) && (display_non_moving_target.get() == NON_MOVE_HIDE) ){
			return;
	}
	xSemaphoreTake(_pflaa_receive,portMAX_DELAY );
	if( targets.find(pflaa.ID) == targets.end() ){
		targets[ pflaa.ID ] = Target ( pflaa );
	}
	else
		targets[ pflaa.ID ].update( pflaa );
	targets[ pflaa.ID ].dumpInfo();
	xSemaphoreGive(_pflaa_receive);
}

TargetManager::~TargetManager() {
	// TODO Auto-generated destructor stub
}

void TargetManager::drawN( int x, int y, bool erase, float north, float dist ){
  if( SetupMenu::isActive() )
		return;
	// ESP_LOGI(FNAME,"drawAirplane x:%d y:%d small:%d", x, y, smallSize );
	egl->setFontPosCenter();
	egl->setPrintPos( x-dist*sin(D2R(north))-5, y-dist*cos(D2R(north))+6 );
	egl->setFont(ucg_font_ncenR14_hr);
	if(erase)
		egl->setColor(COLOR_BLACK);
	else
		egl->setColor(COLOR_GREEN);
	egl->print("N");
	oldN = north;
}

void TargetManager::drawAirplane(int x, int y, float north) {
    if (SetupMenu::isActive())
        return;

    xSemaphoreTake(_display, portMAX_DELAY);

    // --- Draw airplane body ---
    egl->setColor(COLOR_WHITE);
    // Wings
    egl->drawTetragon(x - 15, y - 1, x - 15, y + 1, x + 15, y + 1, x + 15, y - 1);
    // Fuselage
    egl->drawTetragon(x - 1, y + 10, x - 1, y - 6, x + 1, y - 6, x + 1, y + 10);
    // Elevator
    egl->drawTetragon(x - 4, y + 10, x - 4, y + 9, x + 4, y + 9, x + 4, y + 10);

    // --- Compute new radius ---
    float new_radius = 25;
    if (inch2dot4) {
        float logs = log_scale.get() ? log(3) : 1;  // log(2 + 1) = log(3)
        new_radius = zoom * logs * SCALE;
    }

    // --- Clear previous drawings if needed ---
    if (oldN != -1.0 && (oldN != north || old_radius != new_radius)) {
        drawN(x, y, true, oldN, old_radius);
    }

    if (old_radius != 0.0 && old_radius != new_radius) {
        egl->setColor(COLOR_BLACK);
        egl->drawCircle(x, y, old_radius);
    }

    // --- Draw new orientation and circle ---
    egl->setColor(COLOR_GREEN);
    drawN(x, y, false, north, new_radius);
    egl->drawCircle(x, y, new_radius);

    old_radius = new_radius;

    xSemaphoreGive(_display);
}


void TargetManager::printAlarm( const char*alarm, int x, int y, bool print, ucg_color_t color ){
	// ESP_LOGI(FNAME,"printAlarm: %s, x:%d y:%d", alarm, x,y );
	xSemaphoreTake(_display,portMAX_DELAY );
	if( print ){
		egl->setColor( color ); // G=0 R=255 B=0  RED Color
	}else{
		egl->setColor(COLOR_BLACK);
	}
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( x, y );
	egl->printf( alarm );
	xSemaphoreGive(_display);
}

/*
<Severity> Decimal integer value. Range: from 0 to 3.
0 = no error, i.e. normal operation. Disregard other parameters.
1 = information only, i.e. normal operation
2 = functionality may be reduced
3 = fatal problem, device will not work
*/

void TargetManager::printAlarmLevel( const char*alarm, int x, int y, int level ){
	xSemaphoreTake(_display,portMAX_DELAY );
	if( level == 0 ){
		egl->setColor(COLOR_BLACK); // G=0 R=255 B=0  RED Color
	}else if( level == 1 ){
		egl->setColor(COLOR_GREEN);
	}else if( level == 2 ){
		egl->setColor(COLOR_YELLOW);
	}else if( level == 3 ){
		egl->setColor(COLOR_RED);
	}
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( x, y );
	egl->printf( alarm );
	xSemaphoreGive(_display);
}

void TargetManager::nextTarget(int timer){
	// ESP_LOGI(FNAME,"nextTarget size:%d", targets.size() );
	if( targets.size() ){
		if( ++id_iter == targets.end() )
			id_iter = targets.begin();
		if( (timer == 0) && (id_iter != targets.end()) ){ // move away on first call from closest (displayed per default)
			if( id_iter->first == min_id ){
				if( ++id_iter == targets.end() )
					id_iter = targets.begin();
			}
		}
		if( id_iter != targets.end() ){
			ESP_LOGI( FNAME, "next target: %06X",id_iter->first );
		}
	}
}

void TargetManager::printVersions( int x, int y, const char *prefix, const char *ver, int erase ){
	if( erase )
		egl->setColor(COLOR_BLACK);
	else{
		egl->setColor(COLOR_WHITE);
		info_timer = INFO_TIME;
	}
	xSemaphoreTake(_display, portMAX_DELAY);
	egl->setFont(ucg_font_ncenR14_hr);
	egl->setPrintPos( x, y );
	egl->printf( "%s %s", prefix, ver );
	xSemaphoreGive(_display);
}

void TargetManager::clearScreen(){
	xSemaphoreTake(_display, portMAX_DELAY);
	egl->clearScreen();
	xSemaphoreGive(_display);
	// redrawNeeded = true;
}

void TargetManager::rewindInfoTimer(){
	if( info_timer == 0 )
		clearScreen();
	ESP_LOGI(FNAME,"rewindInfoTimer() %d", info_timer );
	info_timer = INFO_TIME;
}

//void release() {};
//	void longPress() {};
void TargetManager::press() {
	ESP_LOGI(FNAME,"press()");
	if( display_mode.get() == DISPLAY_MULTI ){
		id_timer = 10 * (1000/TASKPERIOD);  // 10 seconds
		holddown=5;
		nextTarget( id_timer );
	}
};

void TargetManager::longLongPress() {
	if( id_iter != targets.end() ){
		team_id = id_iter->first;
		ESP_LOGI(FNAME,"long long press: target ID locked: %X", team_id );
	}else{
		if( theInfoTarget ){
			ESP_LOGI(FNAME,"long long press: nothing selected so fas, use closest: %X", theInfoTarget->getID() );
			team_id = theInfoTarget->getID();
		}else{
			ESP_LOGI(FNAME,"No target");
		}

	}
};

void TargetManager::printRX(){
	// --- RX Flag ---
	if (Flarm::getRxFlag() ) {
		xSemaphoreTake(_display,portMAX_DELAY );
		int rx = Flarm::getRXNum();
		if( rx > 0 ){
			egl->setColor(COLOR_GREEN); // G=0 R=255 B=0  RED Color
		}else{
			egl->setColor(COLOR_BLACK);
		}
		egl->setFont(ucg_font_ncenR14_hr);
		egl->setPrintPos( 5, DISPLAY_H - 60 );
		egl->printf( "RX %d  ", rx );
		xSemaphoreGive(_display);
		Flarm::resetRxFlag();
	}
}

void TargetManager::handleFlarmFlags() {
    // --- TX Flag ---

    if (Flarm::getTxFlag() || !(_tick % 200)) {
        int tx = Flarm::getTXBit();  // 0 or 1
        ESP_LOGI(FNAME, "TX alarm: %d", tx);
        printAlarm("NO TX", 10, 100, tx == 0);
        Flarm::resetTxFlag();
    }



    // --- GPS Flag ---
    if (Flarm::getGPSFlag() || !(_tick % 200)) {
        int gps = Flarm::getGPSBit();
        ESP_LOGI(FNAME, "GPS status: %d", gps);
        printAlarm("NO GPS", 10, 120, gps == 0);
        Flarm::resetGPSFlag();
    }

    // --- Connection Flag ---
    if (Flarm::getConnectedFlag()) {
        bool conn = Flarm::connected();
        ESP_LOGI(FNAME, "Flarm connected alarm: %d", !conn);
        clearScreen();
        printAlarm("NO FLARM", 10, 140, !conn);
        Flarm::resetConnectedFlag();
    }

    // --- Error Flag ---
    if (Flarm::getErrorFlag()) {
        int severity = Flarm::getErrorSeverity();
        int error_code = Flarm::getErrorCode();
        ESP_LOGI(FNAME, "PFLAE error code:%d severity:%d error:%s",
                 error_code, severity, Flarm::getErrorString(error_code));

        rewindInfoTimer();
        if (inch2dot4)
            printAlarmLevel(Flarm::getErrorString(error_code), 10, 140, severity);
        else
            printAlarmLevel(Flarm::getErrorString(error_code), 10, 80, severity);

        Flarm::resetErrorFlag();
    }

    // --- Software Version ---
    if (Flarm::getSwVersionFlag()) {
        rewindInfoTimer();
        printVersions(10, 20, "Flarm SW: ", Flarm::getSwVersion(), erase_info);
        Flarm::resetSwVersionFlag();
    }

    // --- Hardware Version ---
    if (Flarm::getHwVersionFlag()) {
        rewindInfoTimer();
        printVersions(10, 40, "Flarm HW: ", Flarm::getHwVersion(), erase_info);
        Flarm::resetHwVersionFlag();
    }

    // --- ODB Version ---
    if (Flarm::getODBVersionFlag()) {
        rewindInfoTimer();
        printVersions(10, 60, "Flarm ODB: ", Flarm::getObstVersion(), erase_info);
        Flarm::resetODBVersionFlag();
    }

    // --- Operation Progress ---
    if (Flarm::getProgressFlag()) {
        rewindInfoTimer();
        xSemaphoreTake(_display, portMAX_DELAY);
        egl->setColor(COLOR_WHITE);
        egl->setFont(ucg_font_ncenR14_hr);
        egl->setPrintPos(10, 60);
        egl->printf("%s: %d %%", Flarm::getOperationString(Flarm::getOperationKey()),
                    Flarm::getProgress());
        xSemaphoreGive(_display);
        Flarm::resetProgressFlag();
    }

    // --- Clear info if timer expired ---
    if (info_timer == 1) {
        ESP_LOGI(FNAME, "NOW CLEAR info");
        clearScreen();
        redrawNeeded = true;
    }

}

void TargetManager::tick() {
    _tick++;
    float min_dist   = 10000.0f;
    float max_climb  = -1000.0f;
    maxcl_id = 0;
    min_id = 0;

    // --- Update timers ---
    if (holddown > 0) holddown--;
    if (id_timer  > 0) id_timer--;
    if (info_timer > 0) info_timer--;

    // --- Periodic logging / redraw trigger ---
    if (!(_tick % 20)) { // ~1 s
        ESP_LOGI(FNAME, "Num targets: %d", (int)targets.size());
    }
    if (!(_tick % 30)) { // ~1.5 s
        redrawNeeded = true;
    }

    // --- Main tick block (every 5 ticks ~250 ms) ---
    if (_tick % 5 != 0) return;
    if (SetupMenu::isActive()) return;

    handleFlarmFlags();

    const bool flarm_ok = (!info_timer && Flarm::connected());
    if (flarm_ok) {
        drawAirplane(DISPLAY_W / 2, DISPLAY_H / 2, Flarm::getGndCourse());
    }

    // --- Pass 1: Determine nearest and max climb ---
    for (auto &kv : targets) {
        Target &tgt = kv.second;
        tgt.ageTarget();
        tgt.nearest(false);
        tgt.best(false);

        if (tgt.getAge() < AGEOUT) {
            if (tgt.haveAlarm()) id_timer = 0;

            if (tgt.getClimb() > max_climb) {
                max_climb = tgt.getClimb();
                maxcl_id = kv.first;
            }

            if (!id_timer) {
                if (tgt.getProximity() < min_dist) {
                    min_dist = tgt.getDist();
                    min_id = kv.first;
                    id_iter = targets.end(); // deselect again
                }
            } else if (id_iter != targets.end() && kv.first == id_iter->first) {
                tgt.nearest(true);
            }
        }
    }

    // --- Pass 2: Draw all visible targets ---
    if (flarm_ok) {
        std::vector<std::pair<uint32_t, Target*>> visible;

        // Collect visible targets
        for (auto it = targets.begin(); it != targets.end();) {
            Target &tgt = it->second;
            tgt.best(it->first == maxcl_id);
            if (!id_timer) tgt.nearest(it->first == min_id);

            bool displayTarget = (tgt.getAge() < AGEOUT) &&
                ((display_mode.get() == DISPLAY_MULTI) ||
                 ((display_mode.get() == DISPLAY_SIMPLE) && tgt.isNearest()));

            if (displayTarget) {
                visible.emplace_back(it->first, &tgt);
                ++it;
            } else {
                // --- Remove invisible / aged-out target ---
                if (id_iter != targets.end() && it->first == id_iter->first) id_iter++;
                tgt.drawInfo(true); // only erase info here
                tgt.draw(true, it->first == team_id);
                it = targets.erase(it);
            }
        }

        // --- Select exactly one info/priority target ---
        Target* infoTarget = nullptr;
        uint32_t infoId = 0;

        if (!visible.empty()) {
            // 1) alarmed first
            for (auto &p : visible) if (p.second->haveAlarm()) { infoTarget = p.second; infoId = p.first; break; }
            // 2) nearest
            if (!infoTarget) for (auto &p : visible) if (p.second->isNearest()) { infoTarget = p.second; infoId = p.first; break; }

        }

        // --- Draw all normal targets first (without info) ---
        for (auto &p : visible) {
            if (p.first == infoId) continue; // skip priority target
            Target &tgt = *p.second;
            tgt.draw(false, p.first == team_id);
            if (!(_tick % 2)) tgt.checkClose();
            // Do NOT call drawInfo(false) here — prevents flashing
        }

        // --- Draw the priority target last (on top) ---
        if (infoTarget) {
        	theInfoTarget = infoTarget;
            if (redrawNeeded) { infoTarget->redrawInfo(); redrawNeeded = false; }
            infoTarget->drawInfo();  // show info
            infoTarget->draw(false, infoId == team_id);
            min_id = infoId;
            if (!(_tick % 2)) infoTarget->checkClose();
        }else{
        	theInfoTarget = NULL;
        }
    }
    printRX();
}


