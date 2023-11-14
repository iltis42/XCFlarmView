#ifndef FLARM_H
#define FLARM_H
#include <cstdlib> // abs
#include <string> // std::string
#include <locale> // std::locale, std::toupper
#include <AdaptUGC.h>
#include "RingBufCPP.h"  // SString, tbd: extra header
#include "Units.h"
#include "freertos/FreeRTOS.h"

typedef enum e_audio_alarm_type { AUDIO_ALARM_OFF, AUDIO_ALARM_NEAR, AUDIO_ALARM_FLARM_1, AUDIO_ALARM_FLARM_2, AUDIO_ALARM_FLARM_3  } e_audio_alarm_type_t;

typedef struct {
	int alarmLevel;
	int relNorth;
	int relEast;
	int relVertical;
	int idType;
	char ID[6];
	int track;
	int groundSpeed;
	float climbRate;
	char acftType[1];
	int noTrack;
} nmea_pflaa_s;

/* Value indexes */
#define NMEA_PFLAA_ALARMLEVEL		 0
#define NMEA_PFLAA_RELATIVE_NORTH	 1
#define NMEA_PFLAA_RELATIVE_EAST	 2
#define NMEA_PFLAA_RELATIVE_VERTICAL 3
#define NMEA_PFLAA_IDTYPE			 4
#define NMEA_PFLAA_ID				 5
#define NMEA_PFLAA_TRACK			 6
//#define NMEA_PFLAA_TURN_RATE		 7
#define NMEA_PFLAA_GROUND_SPEED		 7
#define NMEA_PFLAA_CLIMB_RATE		 8
#define NMEA_PFLAA_ACFTTYPE		     9
#define NMEA_PFLAA_NO_TRACK		    10



class Flarm {
public:
	static void setDisplay( AdaptUGC *theUcg ) { ucg = theUcg; };
	static void parseNMEA( const char *str, int len );
	static void parsePFLAE( const char *pflae );
	static void parsePFLAU( const char *pflau, bool sim=false );
	static void parsePFLAA( const char *pflaa );
	static void parsePFLAX( const char *pflax, int port );
	static void parseGPRMC( const char *gprmc );
	static void parseGPGGA( const char *gpgga );
	static void parsePGRMZ( const char *pgrmz );
	static void drawAirplane( int x, int y, bool fromBehind=false, bool smallSize=false );
	static inline int alarmLevel(){ return AlarmLevel; };
	static void drawDownloadInfo();
	static void drawFlarmWarning();
	static void initFlarmWarning();
	static void progress();
	static bool connected(); // returns true if Flarm is connected
	static inline bool getGPS( float &gndSpeedKmh, float &gndTrack ) {
		if( myGPS_OK ) {
			gndSpeedKmh = Units::knots2kmh(gndSpeedKnots);
			gndTrack = gndCourse;
			return true;
		}
		else{
			return false;
		}
	}
	static inline bool getGPSknots( float &gndSpeed ) {
			if( myGPS_OK ) {
				gndSpeed = gndSpeedKnots;
				return true;
			}
			else{
				return false;
			}
	}
	static inline bool gpsStatus() { return myGPS_OK; }
	static float getGndSpeedKnots() { return gndSpeedKnots; }
	static float getGndCourse() { return gndCourse; }
	static int bincom;
	static int bincom_port;
	static void tick();
	static bool validExtAlt() { if( ext_alt_timer )
		return true;
	else
		return false;
	}
	static void begin();
	static void taskFlarm(void *pvParameters);

private:
	static int calcNMEACheckSum(const char *nmea);
	static int getNMEACheckSum(const char *nmea);
	static void drawClearTriangle( int x, int y, int rb, int dist, int size, int factor );
	static void drawClearVerticalTriangle( int x, int y, int rb, int dist, int size, int factor );
	static void drawTriangle( int x, int y, int rb, int dist, int size=15, int factor=2, bool erase=false );
	static void flarmSim();

	static AdaptUGC* ucg;
	static int RX,TX,GPS,Power;
	static int AlarmLevel;
	static int RelativeBearing,RelativeVertical,RelativeDistance;
	static float gndSpeedKnots;
	static float gndCourse;
	static bool  myGPS_OK;
	static int AlarmType;
	static char ID[20];
	static int oldDist;
	static int oldVertical;
	static int oldBear;
	static int alarmOld;
	static int _tick;
	static int timeout;
	static int ext_alt_timer;
	static int _numSat;
	static int sim_tick;
	static nmea_pflaa_s PFLAA;
	static e_audio_alarm_type_t alarm;
	static TaskHandle_t pid;
};

#endif
