#include "Flarm.h"
#include "logdef.h"
#include "Colors.h"
#include "math.h"
#include "pflaa2.h"
#include "TargetManager.h"
#include <iostream>
#include <sstream>

#define TASK_PERIOD 1000  // ms

#define FLARM_TIMEOUT (10* (1000/TASK_PERIOD))
#define PFLAU_TIMEOUT (10* (1000/TASK_PERIOD))
#define ALARM_TIMEOUT (10* (1000/TASK_PERIOD))

int Flarm::RX = 0;
int Flarm::TX = 0;
int Flarm::GPS = 0;
int Flarm::last_RX = -1;
int Flarm::last_TX = 1;
int Flarm::last_GPS = -1;
int Flarm::Power = 0;
int Flarm::AlarmLevel = 0;
int Flarm::RelativeBearing = 0;
int Flarm::AlarmType = 0;
int Flarm::RelativeVertical = 0;
int Flarm::RelativeDistance = 0;
float Flarm::gndSpeedKnots = 0;
float Flarm::gndCourse = 0;
bool Flarm::myGPS_OK = false;
bool Flarm::_connected = true;
char Flarm::ID[20] = "";
int Flarm::bincom = 0;
int Flarm::_pflau_timeout = PFLAU_TIMEOUT;
TaskHandle_t Flarm::pid = 0;
AdaptUGC* Flarm::ucg;
e_audio_alarm_type_t Flarm::alarm = AUDIO_ALARM_OFF;
char Flarm::Operation[16] = "\0";
char Flarm::Info[16] = "\0";
char Flarm::HwVersion[16] = "\0";
char Flarm::SwVersion[16] = "\0";
char Flarm::ObstVersion[32] = "\0";
unsigned int Flarm::Progress = 0;

#define CENTERX 120
#define CENTERY 120

#define RTD(x) (x*RAD_TO_DEG)
#define DTR(x) (x*DEG_TO_RAD)

int Flarm::oldDist = 0;
int Flarm::oldVertical = 0;
int Flarm::oldBear = 0;
int Flarm::alarmOld=0;
int Flarm::_tick=0;
int Flarm::connected_timeout=ALARM_TIMEOUT;
int Flarm::alarm_timeout=0;
int Flarm::ext_alt_timer=0;
int Flarm::_numSat=0;
int Flarm::bincom_port=0;
int Flarm::pflae_severity=0;
int Flarm::pflae_error=0;

bool Flarm::flarm_sim = false;
t_flags Flarm::flags = { false, false, false, false, false, false, false, false, false };

extern xSemaphoreHandle spiMutex;

#define END_SIM NUM_PFLAA2_SIM
// #define END_SIM 200

int Flarm::sim_tick=0;

static std::map<int, const char*> FlarmErrors = {
		{ 0x00, "                                                 " },
		{ 0x11, "Firmware expired" },
		{ 0x12, "Firmware update error" },
		{ 0x21, "Power voltage low" },
		{ 0x22, "UI error" },
		{ 0x23, "Audio error" },
		{ 0x24, "ADC error" },
		{ 0x25, "SD card error" },
		{ 0x26, "USB error" },
		{ 0x27, "LED error" },
		{ 0x28, "EEPROM error" },
		{ 0x29, "General hardware error" },
		{ 0x2A, "XPDR Mode-C/S/ADS-B U/S" },
		{ 0x2B, "EEPROM error" },
		{ 0x2C, "GPIO error" },
		{ 0x31, "GPS communication" },
		{ 0x32, "GPS configuration" },
		{ 0x33, "GPS antenna" },
		{ 0x41, "RF communication" },
		{ 0x42, "FLARM ID dup., suppressed" },
		{ 0x43, "Invalid ICAO 24-bit/Rad ID" },
		{ 0x51, "Communication" },
		{ 0x61, "Flash memory" },
		{ 0x71, "Pressure sensor" },
		{ 0x81, "ODB file type error" },
		{ 0x82, "ODB expired" },
		{ 0x91, "Flight recorder" },
		{ 0x93, "ENL rec. not possible" },
		{ 0x94, "Range analyzer" },
		{ 0xA1, "Config error SD/USB" },
		{ 0xB1, "Obstacle DB invalid" },
		{ 0xB2, "IGC license invalid" },
		{ 0xB3, "AUD license invalid" },
		{ 0xB4, "ENL license invalid" },
		{ 0xB5, "RFB license invalid" },
		{ 0xB6, "TIS license invalid" },
		{ 0x100, "Generic error" },
		{ 0x101, "Flash FS error" },
		{ 0x110, "FW upd. error ext. display" },
		{ 0x120, "Outside design, region" },
		{ 0xF1, "Other" },
};

const char * Flarm::getErrorString( int index ) {
	ESP_LOGI(FNAME," index: %d find: %d", index, FlarmErrors.count( index ) );
	if( FlarmErrors.count( index ) )
		return FlarmErrors[ index ];
	else
		return "Unknown Error";
}

void Flarm::begin(){
	xTaskCreatePinnedToCore(&taskFlarm, "taskFlarm", 4096, NULL, 14, &pid, 0);
}

void Flarm::taskFlarm(void *pvParameters)
{
	while(1){
		progress();
		delay(TASK_PERIOD);
		_tick++;
	}
}

/*
PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,<IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<AcftType>
e.g.
$PFLAA,0,-1234,1234,220,2,DD8F12,180,,30,-1.4,1*

 */


void Flarm::parsePFLAA( const char *pflaa ){
	// ESP_LOGI(FNAME,"PFLAA %s", pflaa );
	/*
	http://delta-omega.com/download/EDIA/FLARM_DataportManual_v3.02E.pdf

		Periodicity:
				sent when available and port Baud rate is sufficient, can be sent several times per second with
				information on several (but maybe not all) targets around.

		PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,<IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<Type>

		<AlarmLevel>
					Alarm level as assessed by FLARM
					0 = no alarm (pure traffic, limited to 2km range and 500m altitude difference)
					1 = low-level alarm
					2 = important alarm
					3 = urgent alarm
		<RelativeNorth>
					Relative position in Meter true north from own position, signed integer
		<RelativeEast>
					Relative position in Meter true east from own position, signed integer
		<RelativeVertical>
					Relative vertical separation in Meter above own position, negative values indicate
					the other aircraft is lower, signed integer. Some distance-dependent random noise
					is applied to altitude data if the privacy for the target is active.
		<ID-Type>
					Defines the interpretation of the following field <ID>
					0 = stateless random-hopping pseudo-ID (chosen by FLARM)
					1 = official ICAO aircraft address
					2 = stable FLARM pseudo-ID (chosen by FLARM)
		<ID>
					6-digit hex value (e.g. "5A77B1") as configured in the target's PFLAC,ID sentence.
					The interpretation is delivered in <ID-Type>
		<Track>
					The target's true ground track in degrees. Integer between 0 and 359. The value 0
					indicates a true north track. This field is empty if the privacy for the target is active.
		<TurnRate>
					The target's turn rate. Positive values indicate a clockwise turn. Signed decimal
					value in °/s. Currently omitted. Field is empty if the privacy for the target is active.
		<GroundSpeed>
					The target's ground speed. Decimal value in m/s. The field is set to 0 to indicate
					the aircraft is not moving, i.e. on ground. This field is empty if the privacy for the
					target is active while the target is airborne.
		<ClimbRate>
					The target's climb rate. Positive values indicate a climbing aircraft. Signed decimal
					value in m/s. This field is empty if the privacy for the target is active.
		<Type>
					Up to two hex characters showing the object type
					0 = unknown
					1 = glider
					2 = tow plane
					3 = helicopter
					4 = parachute
					5 = drop plane
					6 = fixed hang-glider
					7 = soft para-glider
					8 = powered aircraft
					9 = jet aircraft
					A = UFO
					B = balloon
					C = blimp, zeppelin
					D = UAV
					F = static
	 */
	int cs;
	nmea_pflaa_s PFLAA;
	memset( &PFLAA, 0, sizeof(PFLAA) );
	int calc_cs=calcNMEACheckSum( pflaa );
	cs = getNMEACheckSum( pflaa );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"PFLAA CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", pflaa, calc_cs, cs );
		return;
	}
	// PFLAA,<AlarmLevel>,<RelativeNorth>,<RelativeEast>,<RelativeVertical>,<IDType>,<ID>,<Track>,<TurnRate>,<GroundSpeed>,<ClimbRate>,<Type>

	std::istringstream ss(pflaa);
	std::string token;
	std::getline(ss, token, ',');
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.alarmLevel = std::stoi(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.relNorth = std::stoi(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.relEast = std::stoi(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.relVertical = std::stoi(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.idType = std::stoi(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		sscanf(token.c_str(),"%06X", &PFLAA.ID );
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.track = std::stoi(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.turnRate = std::stof(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.groundSpeed = std::stof(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		PFLAA.climbRate = std::stof(token);
	std::getline(ss, token, ',');
	if( !token.empty() )
		sscanf(token.c_str(), "%2s", PFLAA.acftType);

	_tick=0;
	connected_timeout = FLARM_TIMEOUT;
	TargetManager::receiveTarget( PFLAA );
}



// Calculate the checksum and output it as an int
// is required as HEX in the NMEA data set
// between $ or! and * character
int Flarm::calcNMEACheckSum(const char *nmea) {
	int i, XOR, c;
	for (XOR = 0, i = 0; i < strlen(nmea); i++) {
		c = (unsigned char)nmea[i];
		if (c == '*') break;
		if ((c != '$') && (c != '!')) XOR ^= c;
	}
	return XOR;
}

int Flarm::getNMEACheckSum(const char *nmea) {
    const char *p = strchr(nmea, '*');
    if (!p) return -1;
    int cs = 0;
    if (sscanf(p, "*%02x", &cs) != 1) return -1;
    return cs;
}


void Flarm::flarmSim() {
    // ESP_LOGI(FNAME, "flarmSim sim-tick: %d", sim_tick);
    if (sim_tick >= 0 && sim_tick < END_SIM) {
        char str[80];
        snprintf(str, sizeof(str), "%s", pflaa2[sim_tick]);
        parseNMEA(str, strlen(str));
        sim_tick++;
    } else {
        flarm_sim = false;   // end sim mode
        sim_tick = -1;       // reset so it can restart cleanly
    }
}

void Flarm::pflau_timeout(){
	TX = 1;   // TX alarm on
	GPS = 0;  // GPS no GPS
	RX = 0;   // RX no target
	flags.tx = true;
	flags.gps = true;
	flags.rx  = true;
}

static bool old_connected = true;

void Flarm::progress(){  //  per second
	if( connected_timeout ){
		connected_timeout--;
	}
	if( connected_timeout == 0 )
		_connected = false;
	else
		_connected = true;

	if( old_connected != _connected ){
		old_connected = _connected;
		flags.connected = true;  // notify connected change
	}
	if( alarm_timeout ){
		alarm_timeout--;
	}
	// ESP_LOGI(FNAME,"progress, connected_timeout=%d", connected_timeout );
	// Two FLARM targets plus GPS info = 6 messages per second, we play double speed
	// "$GPRMC,134957.00,A,4900.97485,N,01032.86602,E,52.638,272.61,160622,,,A*58\n",
	// "$PGRMZ,6696,F,2*05\n",
	// "$GPGGA,134957.00,4900.97485,N,01032.86602,E,1,10,0.82,2147.3,M,47.9,M,,*63\n",
	// "$PFLAA,0,-741,1281,66,2,DDAC9C,305,,29,2.5,1*3C\n",
	// "$PFLAA,0,152,-447,-367,2,DF184C,76,,38,1.2,1*2D\n",
	// "$PFLAU,2,1,2,1,0,41,0,-367,516*4A\n",

	if( flarm_sim ){
		for( int i=0; i<12; i++ )
			flarmSim();
	}else{  // no PFLAU in flarm Simulation
		if( _pflau_timeout ){
			_pflau_timeout--;
			if( _pflau_timeout == 1 ){
				pflau_timeout();
			}
		}
	}
}


void Flarm::parseNMEA( const char *str, int len ){
	// ESP_LOGI(FNAME,"parseNMEA: %s, len: %d", str,  strlen(str) );
	if( !strncmp( str+1, "PFLAU,", 5 )) {
		parsePFLAU( str );
	}
	else if( !strncmp( str+1, "PFLAA,", 5 )) {
		parsePFLAA( str );
	}
	else if( !strncmp( str+3, "RMC,", 3 ) ) {
		parseGPRMC( str );
	}
	else if( !strncmp( str+3, "GGA,", 3 )) {
		parseGPGGA( str );
	}
	else if( !strncmp( str+3, "RMZ,", 3 )) {
		parsePGRMZ( str );
	}
	else if( !strncmp( str+1, "PFLAV,", 5 )) {
		parsePFLAV( str );
	}
	else if( !strncmp( str+1, "PFLAE,", 5 )) {  // On Task declaration or re-connect
		parsePFLAE( str );
	}
	else if( !strncmp( str+1, "PFLAQ,", 5 )) {
		parsePFLAQ( str );
	}
}


/*
eg1. $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
eg2. $GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68
     $GPRMC,201914.00,A,4857.58740,N,00856.94735,E,0.172,122.95,310321,,,A*6D

           225446.00    Time of fix 22:54:46 UTC
           A            Navigation receiver warning A = OK, V = warning
           4916.45,N    Latitude 49 deg. 16.45 min North
           12311.12,W   Longitude 123 deg. 11.12 min West
           000.5        Speed over ground, Knots
           054.7        Course Made Good, True
           191194       Date of fix  19 November 1994
           020.3,E      Magnetic variation 20.3 deg East
 *68          mandatory checksum


 */
void Flarm::parseGPRMC( const char *gprmc ) {
	char warn;
	int cs;
	int calc_cs=calcNMEACheckSum( gprmc );
	cs = getNMEACheckSum( gprmc );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", gprmc, calc_cs, cs );
		return;
	}
	sscanf( gprmc+3, "RMC,%*f,%c,%*f,%*c,%*f,%*c,%f,%f,%*d,%*f,%*c*%*02x", &warn, &gndSpeedKnots, &gndCourse);

	//ESP_LOGI(FNAME,"GPRMC myGPS_OK %d warn %c", myGPS_OK, warn );
	if( warn == 'A' ) {
		if( myGPS_OK == false ){
			myGPS_OK = true;
			ESP_LOGI(FNAME,"GPRMC, GPS status changed to good, rmc:%s gps:%d", gprmc, myGPS_OK );
		}
		// ESP_LOGI(FNAME,"Track: %3.2f, GPRMC: %s", gndCourse, gprmc );
	}
	else{
		if( myGPS_OK == true  ){
			myGPS_OK = false;
			ESP_LOGI(FNAME,"GPRMC, GPS status changed to bad, rmc:%s gps:%d", gprmc, myGPS_OK );
		}
	}
	connected_timeout =FLARM_TIMEOUT;
	// ESP_LOGI(FNAME,"parseGPRMC() GPS: %d, Speed: %3.1f knots, Track: %3.1f° ", myGPS_OK, gndSpeedKnots, gndCourse );
}

/*
  GPGGA

hhmmss.ss = UTC of position
llll.ll = latitude of position
a = N or S
yyyyy.yy = Longitude of position
a = E or W
x = GPS Quality indicator (0=no fix, 1=GPS fix, 2=Dif. GPS fix)
xx = number of satellites in use
x.x = horizontal dilution of precision
x.x = Antenna altitude above mean-sea-level
M = units of antenna altitude, meters
x.x = Geoidal separation
M = units of geoidal separation, meters
x.x = Age of Differential GPS data (seconds)
xxxx = Differential reference station ID

eg. $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
    $GPGGA,121318.00,4857.58750,N,00856.95715,E,1,05,3.87,247.7,M,48.0,M,,*52
 */


void Flarm::parseGPGGA( const char *gpgga ) {
	// ESP_LOGI(FNAME,"parseGPGGA");
	int numSat;
	int cs;
	// ESP_LOGI(FNAME,"parseG*GGA: %s", gpgga );
	int calc_cs=calcNMEACheckSum( gpgga );
	cs = getNMEACheckSum( gpgga );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", gpgga, calc_cs, cs );
		return;
	}
	int ret=sscanf( gpgga+3, "GGA,%*f,%*f,%*c,%*f,%*c,%*d,%d,%*f,%*f,M,%*f,M,%*f,%*d*%*02x", &numSat);
	// ESP_LOGI(FNAME,"parseG*GGA: %s numSat=%d ssf_ret=%d", gpgga, numSat, ret );
	if( ret >=1 ){
		if( numSat != _numSat ){
			_numSat = numSat;
		}
		connected_timeout =FLARM_TIMEOUT;
	}
}

// parsePFLAE $PFLAE,A,0,0*33   $PFLAE,A,2,2A,XPDR receiver*79
// PFLAE,<QueryType>,<Severity>,<ErrorCode>[,<Message>]

void Flarm::parsePFLAE( const char *pflae ) {
	ESP_LOGI(FNAME,"parsePFLAE %s", pflae );
	int cs;
	int calc_cs=calcNMEACheckSum( pflae );
	cs = getNMEACheckSum( pflae );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", pflae, calc_cs, cs );
		return;
	}
	connected_timeout =FLARM_TIMEOUT;
	char queryType;
	int ret=sscanf( pflae+3,"LAE,%c,%d,%x", &queryType, &pflae_severity, &pflae_error );
	if( queryType == 'A' && ret == 3 ){
			ESP_LOGI(FNAME,"PFLAE ret:%d, QT:%c SV:%d EC:%02X MSG:%s", ret, queryType, pflae_severity, pflae_error, getErrorString(pflae_error) );
			flags.error=true;
	}else
		ESP_LOGW(FNAME,"Ignored PFLAE Query or wrong format: <%s>", pflae );
}

/* PFLAU,<RX>,<TX>,<GPS>,<Power>,<AlarmLevel>,<RelativeBearing>,<AlarmType>,<RelativeVertical>,<RelativeDistance>,<ID>
		$PFLAU,3,1,2,1,2,-30,2,-32,755*FLARM is working properly and currently receives 3 other aircraft.
		The most dangerous of these aircraft is at 11 o’clock, position 32m below and 755m away. It is a level 2 alarm


<RX> Decimal integer value. Range: from 0 to 99.
Number of devices with unique IDs currently received
regardless of the horizontal or vertical separation

<TX>
Decimal integer value. Range: from 0 to 1.
Transmission status: 1 for OK and 0 for no transmission

<GPS>
Decimal integer value. Range: from 0 to 2.
GPS status:
0 = no GPS reception
1 = 3d-fix on ground, i.e. not airborne
2 = 3d-fix when airborne
If <GPS> goes to 0, FLARM will not work. Nevertheless,
wait for some seconds to issue any warnings

<AcftType>
0 = unknown
1 = glider / motor glider
2 = tow / tug plane
3 = helicopter / rotorcraft
4 = skydiver
5 = drop plane for skydivers
6 = hang glider (hard)
7 = paraglider (soft)
8 = aircraft with reciprocating engine(s)
9 = aircraft with jet/turboprop engine(s)
A = unknown
B = balloon
C = airship
D = unmanned aerial vehicle (UAV)
E = unknown
F = static object
 */

void Flarm::parsePFLAU( const char *pflau, bool sim_data ) {
	// ESP_LOGI(FNAME,"parsePFLAU");
	int cs;
	int id;
	int calc_cs=calcNMEACheckSum( pflau );
	cs = getNMEACheckSum( pflau );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", pflau, calc_cs, cs );
		return;
	}
	sscanf( pflau, "$PFLAU,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x*%02x",&RX,&TX,&GPS,&Power,&AlarmLevel,&RelativeBearing,&AlarmType,&RelativeVertical,&RelativeDistance,&id,&cs);
	// ESP_LOGI(FNAME,"parsePFLAU() RB: %d ALT:%d  DIST %d",RelativeBearing,RelativeVertical, RelativeDistance );
	sprintf( ID,"%06x", id );
	_tick=0;
	connected_timeout =FLARM_TIMEOUT;
	_pflau_timeout = PFLAU_TIMEOUT;
	if( TX != last_TX ){
		last_TX = TX;
		flags.tx = true;
	}
	if( RX != last_RX ){
		last_RX = RX;
		flags.rx = true;
	}
	if( GPS != last_GPS ){
		last_GPS = GPS;
		flags.gps = true;
	}

}

void Flarm::parsePFLAX( const char *msg, int port ) {
	// ESP_LOGI(FNAME,"parsePFLAX");
	// ESP_LOG_BUFFER_HEXDUMP(FNAME, msg.c_str(), msg.length(), ESP_LOG_INFO);
	int start=0;
	/* Solved now by DataLink frame recognition
	if( !strncmp( msg, "\n", 1 )  ){  // catch case when serial.cpp does not correctly dissect at '\n', needs further evaluation, maybe multiple '\n' sent ?
		start=1;
	}
	 */
	// Note, if the Flarm switch to binary mode was accepted, Flarm will answer
	// with $PFLAX,A*2E. In error case you will get as answer $PFLAX,A,<error>*
	// and the Flarm stays in text mode.
	const char* pflax = "$PFLAX,A*2E";
	const unsigned short lenPflax = strlen(pflax);

	if( strlen(msg + start) >= lenPflax && !strncmp(  msg + start, pflax, lenPflax )  ){
		bincom_port = port;
		int old = bincom;
		bincom = 5;
		ESP_LOGI(FNAME,"bincom: %d --> %d", old, bincom  );
		connected_timeout =FLARM_TIMEOUT;
	}
}

// $PGRMZ,880,F,2*3A  $PGRMZ,864,F,2*30
void Flarm::parsePGRMZ( const char *pgrmz ) {
	int cs;
	int alt1013_ft;
	int calc_cs=calcNMEACheckSum( pgrmz );
	cs = getNMEACheckSum( pgrmz );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", pgrmz, calc_cs, cs );
		return;
	}
	sscanf( pgrmz, "$PGRMZ,%d,F,2",&alt1013_ft );
	connected_timeout =FLARM_TIMEOUT;
	ext_alt_timer = 10;  // Fall back to internal Barometer after 10 seconds
}

// PFLAV,<QueryType>,<HwVersion>,<SwVersion>,<ObstVersion>
// e.g. $PFLAV,A,2.00,5.00,alps20110221_*
void Flarm::parsePFLAV( const char *pflav ) {
	ESP_LOGI(FNAME,"parse %s", pflav );
	int cs;
	int calc_cs=calcNMEACheckSum( pflav );
	cs = getNMEACheckSum( pflav );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", pflav, calc_cs, cs );
		return;
	}
	char query;
	sscanf( pflav, "$PFLAV,%c,%15[^,],%15[^,],%31[^,]", &query, HwVersion, SwVersion, ObstVersion );

	if( query == 'A' ){
		ESP_LOGI(FNAME,"PFLAV %c %s %s %s", query, HwVersion,SwVersion,ObstVersion );
		flags.swVersion=true;
		flags.hwVersion=true;
		flags.odbVersion=true;
	}
	if( ObstVersion[0] == '*' ){ // there is no obstacle database
		ObstVersion[0] = '\0';
		flags.odbVersion=false;
	}

	connected_timeout =FLARM_TIMEOUT;
}

static std::map<std::string, const char*> OperationTypes = {
    { "IGC", "IGC files download" },
    { "FW", "Firmware update" },
    { "OBST", "ODB update" },
    { "DUMP", "Diagnostic dump" },
    { "RESTORE", "Restore file system" },
    { "SCAN", "Internal consis. check" }
};

const char* Flarm::getOperationString(const char *key) {
    ESP_LOGI(FNAME, "getOperationString( key %s )", key);
    auto it = OperationTypes.find(key); // `std::string` unterstützt direkten Vergleich mit `const char*`.
    if (it != OperationTypes.end()) {
        ESP_LOGI(FNAME, "Return OP: key %s LongString: %s", key, it->second);
        return it->second;
    } else {
        ESP_LOGI(FNAME, "Key unknown, return internal operation");
        return "Internal OP";
    }
}

// PFLAQ,<Operation>,<Info>,<Progress>
void Flarm::parsePFLAQ( const char *pflaq ) {
	ESP_LOGI(FNAME,"PFLAQ %s", pflaq );
	int cs;
	int calc_cs=calcNMEACheckSum( pflaq );
	cs = getNMEACheckSum( pflaq );
	memset( Operation, 0, sizeof( Operation ) );
	if( cs != calc_cs ){
		ESP_LOGW(FNAME,"CHECKSUM ERROR: %s; calculcated CS: %d != delivered CS %d", pflaq, calc_cs, cs );
		return;
	}
	int commas=0;
	for( int i=0; i<strlen(pflaq); i++ ){
		if( pflaq[i] == ',' )
			commas++;
	}
	if( commas == 2 ){
		sscanf( pflaq, "$PFLAQ,%[^,],%d",Operation,&Progress );
		ESP_LOGI(FNAME,"2 PFLAQ %s %d", Operation, Progress );
		ESP_LOGI(FNAME,"2 PFLAQ %s %d %s", Operation, Progress, getOperationString(Operation) );
	}
	else if( commas == 3 ){
		sscanf( pflaq, "$PFLAQ,%[^,],%[^,],%d",Operation,Info,&Progress );
		ESP_LOGI(FNAME,"3 PFLAQ %s %s %d", Operation, Info, Progress );
	    // ESP_LOGI(FNAME,"3 PFLAQ %s %s %d %s", Operation,Info,Progress, getOperationString[Operation] );
	}
	flags.progress = true;
	connected_timeout = FLARM_TIMEOUT;
}

int rbOld = -500; // outside normal range

