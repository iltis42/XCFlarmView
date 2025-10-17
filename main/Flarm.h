#ifndef FLARM_H
#define FLARM_H
#include <cstdlib> // abs
#include <string> // std::string
#include <locale> // std::locale, std::toupper
#include <AdaptUGC.h>
#include "RingBufCPP.h"  // SString, tbd: extra header
#include "Units.h"
#include "freertos/FreeRTOS.h"
#include <map>

typedef enum e_audio_alarm_type { AUDIO_ALARM_OFF, AUDIO_ALARM_NEAR, AUDIO_ALARM_FLARM_1, AUDIO_ALARM_FLARM_2, AUDIO_ALARM_FLARM_3  } e_audio_alarm_type_t;

typedef struct {
	int alarmLevel;
	int relNorth;
	int relEast;
	int relVertical;
	int idType;
	unsigned int ID;
	int track;
	float turnRate;
	float groundSpeed;
	float climbRate;
	char acftType[3];
} nmea_pflaa_s;

typedef struct flarm_flags{
	bool error;
	bool swVersion;
	bool hwVersion;
	bool odbVersion;
	bool progress;
	bool tx;
	bool rx;
	bool gps;
	bool connected;
}t_flags;

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
	static void parsePFLAV( const char* pflav );
	static void parsePFLAX( const char *pflax, int port );
	static void parsePFLAQ( const char *pflaq );
	static void parseGPRMC( const char *gprmc );
	static void parseGPGGA( const char *gpgga );
	static void parsePGRMZ( const char *pgrmz );
	static void drawAirplane( int x, int y, bool fromBehind=false, bool smallSize=false );
	static inline int alarmLevel(){ return AlarmLevel; };
	// static void drawDownloadInfo();
	static void progress();
	static bool connected() { return _connected; };     // returns true if Flarm is connected
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
	static inline float getGndCourse() { return gndCourse; }
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
	static void startSim() { flarm_sim = true; };
	static inline bool getSim() { return flarm_sim; };
	static inline int getTXBit() { return TX; };
	static inline int getRXNum() { return RX; };
	static inline int getGPSBit() { return GPS; };
	static inline int getErrorSeverity() { return pflae_severity; };
	static inline int getErrorCode() { return pflae_error; };
	static const char * getErrorString( int index );
	static inline const char * getSwVersion() { return SwVersion; };
	static inline const char * getHwVersion()  { return HwVersion; };
	static inline const char * getObstVersion()  { return ObstVersion; };
	static inline unsigned int getProgress()  { return Progress; };
	static const char* getOperationString( const char* key );
	static const char* getOperationKey() { return Operation;};
	static inline bool getSwVersionFlag() { return flags.swVersion; };
	static inline void resetSwVersionFlag() {  flags.swVersion = false; };
	static inline bool getHwVersionFlag() {  return flags.hwVersion; };
	static inline void resetHwVersionFlag() {  flags.hwVersion = false; };
	static inline bool getODBVersionFlag() {  return flags.odbVersion;  };
	static inline void resetODBVersionFlag() {  flags.odbVersion = false; };
	static inline bool getProgressFlag() {  return flags.progress; };
	static inline void resetProgressFlag() {  flags.progress = false; };
	static inline bool getErrorFlag() {  return flags.error; };
	static inline void resetErrorFlag() {  flags.error = false; };
	static inline bool getTxFlag() {  return flags.tx; };
	static inline void resetTxFlag() {  flags.tx = false; };
	static inline bool getRxFlag() {  return flags.rx; };
	static inline void resetRxFlag() {  flags.rx = false; };
	static inline bool getGPSFlag() {  return flags.gps; };
	static inline void resetGPSFlag() {  flags.gps = false; };
	static inline bool getConnectedFlag() {  return flags.connected; };
	static inline void resetConnectedFlag() {  flags.connected = false; };

private:
	static int calcNMEACheckSum(const char *nmea);
	static int getNMEACheckSum(const char *nmea);

	static void flarmSim();
	static void pflau_timeout();

	static t_flags flags;
	static AdaptUGC* ucg;
	static int RX,TX,GPS,Power;
	static int last_RX,last_TX,last_GPS;
	static int AlarmLevel;
	static int RelativeBearing,RelativeVertical,RelativeDistance;
	static float gndSpeedKnots;
	static float gndCourse;
	static bool  myGPS_OK;
	static bool _connected;
	static int AlarmType;
	static char ID[20];
	static int oldDist;
	static int oldVertical;
	static int oldBear;
	static int alarmOld;
	static int _tick;
	static int connected_timeout;
	static int alarm_timeout;
	static int ext_alt_timer;
	static int _numSat;
	static int sim_tick;
	static e_audio_alarm_type_t alarm;
	static TaskHandle_t pid;
	static bool flarm_sim;
	static int _pflau_timeout;
	static int  pflae_severity;
	static int  pflae_error;
	static char HwVersion[16];
	static char SwVersion[16];
	static char ObstVersion[32];
	static char Operation[16];
	static char Info[16];
	static unsigned int Progress;
};

#endif
