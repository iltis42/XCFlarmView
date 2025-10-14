/*
 * Target.cpp
 *
 *  Created on: Nov 15, 2023
 *      Author: esp32s2 (refactored)
 */

#include "Target.h"
#include <cmath>
#include <AdaptUGC.h>
#include "vector.h"
#include "flarmnetdata.h"
#include "flarmview.h"
#include "TargetManager.h"

extern AdaptUGC *egl;
#define TARGET_SIZE_MIN 10
#define TARGET_SIZE_MAX 25

// Static members initialization
char Target::cur_dist[32] = {0};
char Target::cur_alt[32] = {0};
char Target::cur_id[32] = {0};
char Target::cur_var[32] = {0};
int Target::old_dist = -10000;
unsigned int Target::old_alt = 100000;
unsigned int Target::old_id = 0;
int Target::old_var = -10000;
int Target::blink = 0;
extern xSemaphoreHandle _display;

// Helper clamp for ESP32 (C++11 compatibility)
template<typename T>
inline T clamp(T val, T minVal, T maxVal) {
    if (val < minVal) return minVal;
    if (val > maxVal) return maxVal;
    return val;
}

Target::Target() {}

Target::Target(nmea_pflaa_s a_pflaa) {
    pflaa = a_pflaa;
    old_x0 = old_y0 = old_x1 = old_y1 = old_x2 = old_y2 = -1000;
    old_track = 0; old_climb = -1000; old_x = old_y = 0;
    old_size = old_sidelen = old_cirsize = old_cirsizeteam = -1;
    tek_climb = 0.0; last_groundspeed = pflaa.groundSpeed;
    tick = 0; last_pflaa_time = -1; _buzzedHoldDown = 0;
    dist = prox = 10000.0; recalc();
    reg = comp = nullptr; age = 0; is_nearest = false; alarm = false; alarm_timer = 0;
    _isPriority = false;
    is_best = false;
    is_nearest = false;

    for (int i=0;i<sizeof(flarmnet_db)/sizeof(flarmnet_db[0]);i++){
        if (pflaa.ID == flarmnet_db[i].id){
            reg = (char*)flarmnet_db[i].reg;
            comp = (char*)flarmnet_db[i].cn;
        }
    }

    switch (notify_near.get()) {
        case BUZZ_OFF: dist_buzz=-1.0; break;
        case BUZZ_1KM: dist_buzz=1.0; break;
        case BUZZ_2KM: dist_buzz=2.0; break;
        default: dist_buzz=10.0; break;
    }
}

// --- Drawing small info ---
void Target::drawDist(uint8_t r, uint8_t g, uint8_t b){
    egl->setColor(r,g,b);
    egl->setFont(ucg_font_fub20_hf);
    int w = egl->getStrWidth(cur_dist);
    egl->setPrintPos((DISPLAY_W-5)-w,30);
    egl->printf("%s",cur_dist);
}

void Target::drawID(uint8_t r, uint8_t g, uint8_t b){
    egl->setColor(r,g,b);
    egl->setFont(ucg_font_fub20_hf);
    int w = egl->getStrWidth(cur_id);
    if(w>150){ egl->setFont(ucg_font_fub17_hf); w=egl->getStrWidth(cur_id); }
    if(w>150){ egl->setFont(ucg_font_fub14_hf); w=egl->getStrWidth(cur_id); }
    egl->setPrintPos((DISPLAY_W-5)-w,DISPLAY_H-7);
    egl->printf("%s",cur_id);
}

void Target::drawAlt(uint8_t r,uint8_t g,uint8_t b){
    egl->setColor(r,g,b);
    egl->setFont(ucg_font_fub20_hf);
    egl->setPrintPos(5,DISPLAY_H-7);
    egl->printf("%s",cur_alt);
}

void Target::drawVar(uint8_t r,uint8_t g,uint8_t b){
    egl->setColor(r,g,b);
    egl->setFont(ucg_font_fub20_hf);
    egl->setPrintPos(5,30);
    egl->printf("%s",cur_var);
}

void Target::redrawInfo(){
    old_dist=-10000; old_alt=100000; old_id=0; old_var=-10000;
}


void Target::drawInfo(bool erase) {
	char buf[32] = {0};
	if (pflaa.ID == 0) return;
	xSemaphoreTake(_display, portMAX_DELAY);

	// --- Distance ---
	if ((old_dist != (int)(dist * 100)) || erase) {
		if (strlen(cur_dist)) drawDist(COLOR_BLACK);
		if (!erase) {
			sprintf(cur_dist, "%.2f", Units::Distance(dist));
			drawDist(COLOR_WHITE);
			old_dist = (int)(dist * 100);
		} else {
			cur_dist[0] = '\0';
		}
	}

	// --- ID ---
	if ((old_id != pflaa.ID) || erase) {
		if (strlen(cur_id)) drawID(COLOR_BLACK);
		if (!erase) {
			if (reg) {
				if (comp) sprintf(cur_id, "%s %s", reg, comp);
				else      sprintf(cur_id, "%s", reg);
			} else {
				sprintf(cur_id, "%06X", pflaa.ID);
			}
			drawID(COLOR_WHITE);
			old_id = pflaa.ID;
		} else {
			cur_id[0] = '\0';
		}
	}

	// --- Altitude ---
	if ((old_alt != pflaa.relVertical) || erase) {
		if (strlen(cur_alt)) drawAlt(COLOR_BLACK);
		if (!erase) {
			int alt = (int)(Units::Altitude(pflaa.relVertical + 0.5));
			sprintf(cur_alt, "%s%d", (pflaa.relVertical > 0) ? "+" : "", alt);
			drawAlt(COLOR_WHITE);
			old_alt = pflaa.relVertical;
		} else {
			cur_alt[0] = '\0';
		}
	}

	// --- Vario ---
	if ((old_var != (int)(pflaa.climbRate * 10)) || erase) {
		if (strlen(cur_var)) drawVar(COLOR_BLACK);
		if (!erase) {
			float climb = Units::Vario((float)pflaa.climbRate);
			sprintf(cur_var, "%+.1f", climb);
			drawVar(COLOR_WHITE);
			old_var = (int)(pflaa.climbRate * 10);
		} else {
			cur_var[0] = '\0';
		}
	}

	// --- Units display ---
	if (erase) egl->setColor(COLOR_BLACK);
	else       egl->setColor(COLOR_BLUE);

	egl->setFont(ucg_font_fub14_hf);

	int w = egl->getStrWidth("ID");
	egl->setPrintPos(DISPLAY_W - 5 - w, DISPLAY_H - 37);
	egl->printf("ID");

	sprintf(buf, "Dis %s", Units::DistanceUnit());
	w = egl->getStrWidth(buf);
	egl->setPrintPos(DISPLAY_W - 5 - w, 50);
	egl->printf("%s", buf);

	egl->setPrintPos(5, 50);
	egl->printf("Var %s", Units::VarioUnit());

	egl->setPrintPos(5, DISPLAY_H - 37);
	egl->printf("Alt %s", Units::AltitudeUnit());
	xSemaphoreGive(_display);
}



// --- drawClimb ---
void Target::drawClimb(int x,int y,int size,int climb){
    if(climb>1){
        egl->setPrintPos(x-4,y-size);
        egl->setFont(ucg_font_ncenR14_hr);
        egl->printf("%d",climb);
    }
}

// --- drawFlarmTarget ---
void Target::drawFlarmTarget(int ax,int ay,int bearing,int sideLength,bool erase,bool closest,ucg_color_t color,bool follow){
    float radians=D2R(bearing-90.0f);
    float axt=ax-sideLength/4.0f*sin(D2R((float)bearing));
    float ayt=ay+sideLength/4.0f*cos(D2R((float)bearing));

    int x0=rint(axt + sideLength*cos(radians));
    int y0=rint(ayt + sideLength*sin(radians));
    int x1=rint(axt + sideLength/2.0f*cos(radians+2*M_PI/3));
    int y1=rint(ayt + sideLength/2.0f*sin(radians+2*M_PI/3));
    int x2=rint(axt + sideLength/2.0f*cos(radians-2*M_PI/3));
    int y2=rint(ayt + sideLength/2.0f*sin(radians-2*M_PI/3));
    int climb=int(tek_climb+0.5f);

    if(erase || old_closest!=closest || old_climb!=climb || old_sidelen!=sideLength ||
       old_x0!=x0 || old_y0!=y0 || old_x1!=x1 || old_y1!=y1 || old_x2!=x2 || old_y2!=y2) {
        egl->setColor(COLOR_BLACK);
        if(old_x0>0) egl->drawTriangle(old_x0,old_y0,old_x1,old_y1,old_x2,old_y2);
        if(old_closest && old_cirsize>0){ egl->drawCircle(old_ax,old_ay,old_cirsize); }
        if(old_cirsizeteam>0){ egl->drawCircle(old_ax,old_ay,old_cirsizeteam); egl->drawCircle(old_ax,old_ay,old_cirsizeteam+1); }
        if(old_climb!=-1000) drawClimb(old_x,old_y,old_size,old_climb);
        old_x0=old_x1=old_x2=-1; old_sidelen=old_climb=old_cirsize=old_cirsizeteam=-1;
    }

    if(x0<=0||x0>=DISPLAY_W||y0<=0||y0>=DISPLAY_H) return;

    if(!erase){
        egl->setColor(color.color[0],color.color[1],color.color[2]);
        egl->drawTriangle(x0,y0,x1,y1,x2,y2);
        if(is_best){ drawClimb(ax,ay,sideLength,climb); old_climb=climb; old_size=sideLength; old_x=ax; old_y=ay; }
        if(closest){ int len=rint(sideLength*0.75f); egl->drawCircle(ax,ay,len); old_cirsize=len; }
        if(follow){ int len=rint(sideLength*0.75f+2.0f); egl->setColor(COLOR_RED); egl->drawCircle(ax,ay,len); egl->drawCircle(ax,ay,len+1); old_cirsizeteam=len; }
        old_x0=x0; old_y0=y0; old_x1=x1; old_y1=y1; old_x2=x2; old_y2=y2; old_ax=ax; old_ay=ay; old_closest=closest;
    }
}

// --- draw ---
void Target::draw(bool erase, bool follow){
    checkAlarm();
    int size = clamp(10 + int(10.0/dist), TARGET_SIZE_MIN, TARGET_SIZE_MAX);
    uint8_t brightness = uint8_t(255 - 255.0 * std::min(1.0, age/(double)AGEOUT));
    ucg_color_t color;
    if(dist<1.0 && sameAlt()){
        if(haveAlarm()) color = (blink++%2)? ucg_color_t{COLOR_WHITE}: ucg_color_t{COLOR_RED};
        else color = {brightness,brightness,brightness};
    } else color = {0, brightness, 0};
    drawFlarmTarget(x,y,rel_target_heading,size,erase,is_nearest,color,follow);
}

// --- update ---
void Target::update(nmea_pflaa_s a_pflaa){
    pflaa=a_pflaa; recalc(); if(last_pflaa_time>0) tekCalc();
    last_groundspeed=pflaa.groundSpeed;
    last_pflaa_time=tick;
    age=0;
}

// --- ageTarget ---
void Target::ageTarget(){
    raw_tick++; if(!(raw_tick%4)) tick++;
    if(age<1000) age++;
    if(_buzzedHoldDown) _buzzedHoldDown--;
    recalc();
}

// --- recalc ---
void Target::recalc(){
    rel_target_heading=rint(Vector::angleDiffDeg((float)pflaa.track,Flarm::getGndCourse()));
    rel_target_dir=Vector::angleDiffDeg(R2D(atan2(pflaa.relEast,pflaa.relNorth)),Flarm::getGndCourse());
    dist=sqrt(pflaa.relNorth*pflaa.relNorth+pflaa.relEast*pflaa.relEast)/1000.0f;
    float relV = pflaa.relVertical/1000.0f;
    prox = sqrt(relV*relV + dist*dist);
    float pix = inch2dot4 ? std::max(20.0f, zoom*(log_scale.get()?log(2+dist):dist)*SCALE)
                          : std::max(30.0f, log(2+prox)*SCALE);
    x=DISPLAY_W/2 + pix*sin(D2R(rel_target_dir));
    y=DISPLAY_H/2 - pix*cos(D2R(rel_target_dir));
}

// --- tekCalc ---
void Target::tekCalc(){
    tek_climb = pflaa.climbRate;
    int dt = tick - last_pflaa_time;
    int v = pflaa.groundSpeed;
    int dv = v-last_groundspeed;
    if(dv<5 && last_groundspeed>0 && pflaa.groundSpeed>12 && dt>=1 && dt<10){
        if(dt!=0) tek_climb += ((v*dv)/(9.81f*dt)-tek_climb)*0.2f;
    }
}

// --- checkClose ---
void Target::checkClose(){
    if(dist_buzz<0) return;
    if(dist<dist_buzz && _buzzedHoldDown==0){
        Buzzer::play2(BUZZ_DH,200,audio_volume.get(),BUZZ_E,200,audio_volume.get());
        _buzzedHoldDown=12000;
    } else if(dist>(dist_buzz*2.0)) _buzzedHoldDown=0;
}

// --- checkAlarm ---
void Target::checkAlarm(){
    if(pflaa.alarmLevel==1) { Buzzer::play2(BUZZ_DH,150,audio_volume.get(),BUZZ_DH,150,0,6); setAlarm(); }
    else if(pflaa.alarmLevel==2) { Buzzer::play2(BUZZ_E,100,audio_volume.get(),BUZZ_E,100,0,10); setAlarm(); }
    else if(pflaa.alarmLevel==3) { Buzzer::play2(BUZZ_F,70,audio_volume.get(),BUZZ_F,70,0,15); setAlarm(); }
    if(alarm_timer==0) alarm=false;
    if(alarm_timer) alarm_timer--;
}

// --- dumpInfo ---
void Target::dumpInfo(){
    // ESP_LOGI(FNAME,"Target ID: %06X | Age: %d | Dist: %.2f km | Alt: %d m | Climb: %.1f m/s | Track: %d",
    //         pflaa.ID, age, dist, pflaa.relVertical, pflaa.climbRate, pflaa.track);
    // if(reg || comp) ESP_LOGI(FNAME,"  Reg: %s | Comp: %s", reg?reg:"-", comp?comp:"-");
}

// --- destructor ---
Target::~Target(){}
