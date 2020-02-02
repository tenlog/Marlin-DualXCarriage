/* -*- c++ -*- */

/*
    Reprap firmware based on Sprinter and grbl.
 Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 Copyright (C) 2016-2019 zyf@tenlog3d.com


 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 This firmware is a mashup between Sprinter and grbl.
  (https://github.com/kliment/Sprinter)
  (https://github.com/simen/grbl/tree)

 It has preliminary support for Matthew Roberts advance algorithm
    http://reprap.org/pipermail/reprap-dev/2011-May/003323.html
 */

#include "Marlin.h"

#include "ultralcd.h"
#include "planner.h"
#include "stepper.h"
#include "temperature.h"
#include "motion_control.h"
#include "cardreader.h"
#include "watchdog.h"
#include "ConfigurationStore.h"
#include "language.h"
#include "pins_arduino.h"

#if NUM_SERVOS > 0
//#include "Servo.h"
#endif

#if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
#include <SPI.h>
#endif

#define VERSION_STRING  "1.0.0"

// look here for descriptions of gcodes: http://linuxcnc.org/handbook/gcode/g-code.html
// http://objects.reprap.org/wiki/Mendel_User_Manual:_RepRapGCodes

//Implemented Codes
//-------------------
// G0  -> G1
// G1  - Coordinated Movement X Y Z E
// G2  - CW ARC
// G3  - CCW ARC
// G4  - Dwell S<seconds> or P<milliseconds>
// G10 - retract filament according to settings of M207
// G11 - retract recover filament according to settings of M208
// G28 - Home all Axis
// G90 - Use Absolute Coordinates
// G91 - Use Relative Coordinates
// G92 - Set current position to cordinates given

// M Codes
// M0   - Unconditional stop - Wait for user to press a button on the LCD (Only if ULTRA_LCD is enabled)
// M1   - Same as M0
// M17  - Enable/Power all stepper motors
// M18  - Disable all stepper motors; same as M84
// M20  - List SD card
// M21  - Init SD card
// M22  - Release SD card
// M23  - Select SD file (M23 filename.g)
// M24  - Start/resume SD print
// M25  - Pause SD print
// M26  - Set SD position in bytes (M26 S12345)
// M27  - Report SD print status
// M28  - Start SD write (M28 filename.g)
// M29  - Stop SD write
// M30  - Delete file from SD (M30 filename.g)
// M31  - Output time since last M109 or SD card start to serial
// M32  - Select file and start SD print (Can be used when printing from SD card)
// M42  - Change pin status via gcode Use M42 Px Sy to set pin x to value y, when omitting Px the onboard led will be used.
// M80  - Turn on Power Supply
// M81  - Turn off Power Supply
// M82  - Set E codes absolute (default)
// M83  - Set E codes relative while in Absolute Coordinates (G90) mode
// M84  - Disable steppers until next move,
//        or use S<seconds> to specify an inactivity timeout, after which the steppers will be disabled.  S0 to disable the timeout.
// M85  - Set inactivity shutdown timer with parameter S<seconds>. To disable set zero (default)
// M92  - Set axis_steps_per_unit - same syntax as G92
// M104 - Set extruder target temp
// M105 - Read current temp
// M106 - Fan on
// M107 - Fan off
// M109 - Sxxx Wait for extruder current temp to rG1each target temp. Waits only when heating
//        Rxxx Wait for extruder current temp to reach target temp. Waits when heating and cooling
// M114 - Output current position to serial port
// M115 - Capabilities string
// M117 - display message
// M119 - Output Endstop status to serial port
// M126 - Solenoid Air Valve Open (BariCUDA support by jmil)
// M127 - Solenoid Air Valve Closed (BariCUDA vent to atmospheric pressure by jmil)
// M128 - EtoP Open (BariCUDA EtoP = electricity to air pressure transducer by jmil)
// M129 - EtoP Closed (BariCUDA EtoP = electricity to air pressure transducer by jmil)
// M140 - Set bed target temp
// M190 - Sxxx Wait for bed current temp to reach target temp. Waits only when heating
//        Rxxx Wait for bed current temp to reach target temp. Waits when heating and cooling
// M200 - Set filament diameter
// M201 - Set max acceleration in units/s^2 for print moves (M201 X1000 Y1000)
// M202 - Set max acceleration in units/s^2 for travel moves (M202 X1000 Y1000) Unused in Marlin!!
// M203 - Set maximum feedrate that your machine can sustain (M203 X200 Y200 Z300 E10000) in mm/sec
// M204 - Set default acceleration: S normal moves T filament only moves (M204 S3000 T7000) im mm/sec^2  also sets minimum segment time in ms (B20000) to prevent buffer underruns and M20 minimum feedrate
// M205 -  advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk, E=maximum E jerk
// M206 - set additional homeing offset
// M207 - set retract length S[positive mm] F[feedrate mm/sec] Z[additional zlift/hop]
// M208 - set recover=unretract length S[positive mm surplus to the M207 S*] F[feedrate mm/sec]
// M209 - S<1=true/0=false> enable automatic retract detect if the slicer did not support G10/11: every normal extrude-only move will be classified as retract depending on the direction.
// M218 - set hotend offset (in mm): T<extruder_number> X<offset_on_X> Y<offset_on_Y>
// M220 S<factor in percent>- set speed factor override percentage
// M221 S<factor in percent>- set extrude factor override percentage
// M240 - Trigger a camera to take a photograph
// M250 - Set LCD contrast C<contrast value> (value 0..63)
// M280 - set servo position absolute. P: servo index, S: angle or microseconds
// M300 - Play beepsound S<frequency Hz> P<duration ms>
// M301 - Set PID parameters P I and D
// M302 - Allow cold extrudes, or set the minimum extrude S<temperature>.
// M303 - PID relay autotune S<temperature> sets the target temperature. (default target temperature = 150C)
// M304 - Set bed PID parameters P I and D
// M400 - Finish all moves
// M500 - stores paramters in EEPROM
// M501 - reads parameters from EEPROM (if you need reset them after you changed them temporarily).
// M502 - reverts to the default "factory settings".  You still need to store them in EEPROM afterwards if you want to.
// M503 - print the current settings (from memory not from eeprom)
// M540 - Use S[0|1] to enable or disable the stop SD card print on endstop hit (requires ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED)
// M600 - Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]
// M605 - Set dual x-carriage movement mode: S<mode> [ X<duplication x-offset> R<duplication temp offset> ]
// M907 - Set digital trimpot motor current using axis codes.
// M908 - Control digital trimpot directly.
// M350 - Set microstepping mode.
// M351 - Toggle MS1 MS2 pins directly.
// M928 - Start SD logging (M928 filename.g) - ended by M29
// M999 - Restart after being stopped by error
// M1001 - Set & Get LanguageID
// 

//Stepper Movement Variables

//===========================================================================
//=============================imported variables============================
//===========================================================================


//===========================================================================
//=============================public variables=============================
//===========================================================================
#ifdef SDSUPPORT
CardReader card;
#endif
float homing_feedrate[] = HOMING_FEEDRATE;
bool axis_relative_modes[] = AXIS_RELATIVE_MODES;
int feedmultiply=100; //100->1 200->2
int saved_feedmultiply;
int extrudemultiply=100; //100->1 200->2
float current_position[NUM_AXIS] = {0.0, 0.0, 0.0, 0.0};
float add_homeing[3]={0,0,0};
float min_pos[3] = { X_MIN_POS, Y_MIN_POS, Z_MIN_POS };
float max_pos[3] = { X_MAX_POS, Y_MAX_POS, Z_MAX_POS };

// Extruder offset
#if EXTRUDERS > 1
#ifndef DUAL_X_CARRIAGE
  #define NUM_EXTRUDER_OFFSETS 3 // only in XY plane
#else
  #define NUM_EXTRUDER_OFFSETS 3 // 3 supports offsets in XYZ plane //By ZYF
#endif
float extruder_offset[NUM_EXTRUDER_OFFSETS][EXTRUDERS] = {
#if defined(EXTRUDER_OFFSET_X) && defined(EXTRUDER_OFFSET_Y)
  #ifdef CONFIG_E2_OFFSET
    {0,0}, {0,zyf_Y2_OFFSET}
  #else
    EXTRUDER_OFFSET_X, EXTRUDER_OFFSET_Y
  #endif
#endif
};
#endif
uint8_t active_extruder = 0;
int fanSpeed=0;
#ifdef SERVO_ENDSTOPS
  int servo_endstops[] = SERVO_ENDSTOPS;
  int servo_endstop_angles[] = SERVO_ENDSTOP_ANGLES;
#endif
#ifdef BARICUDA
int ValvePressure=0;
int EtoPPressure=0;
#endif

#ifdef FWRETRACT
  bool autoretract_enabled=true;
  bool retracted=false;
  float retract_length=3, retract_feedrate=17*60, retract_zlift=0.8;
  float retract_recover_length=0, retract_recover_feedrate=8*60;
#endif

//===========================================================================
//=============================private variables=============================
//===========================================================================
const char axis_codes[NUM_AXIS] = {'X', 'Y', 'Z', 'E'};
static float destination[NUM_AXIS] = {  0.0, 0.0, 0.0, 0.0};
#ifdef DELTA
static float delta[3] = {0.0, 0.0, 0.0};
#endif
static float offset[3] = {0.0, 0.0, 0.0};
static bool home_all_axis = true;
static float feedrate = 1500.0, next_feedrate, saved_feedrate;
static long gcode_N, gcode_LastN, Stopped_gcode_LastN = 0;

static bool relative_mode = false;  //Determines Absolute or Relative Coordinates

static char cmdbuffer[BUFSIZE][MAX_CMD_SIZE];
static bool fromsd[BUFSIZE];
static int bufindr = 0;
static int bufindw = 0;
static int buflen = 0;
//static int i = 0;
static char serial_char;
static int serial_count = 0;
static boolean comment_mode = false;
static char *strchr_pointer; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc

const int sensitive_pins[] = SENSITIVE_PINS; // Sensitive pin list for M42

//static float tt = 0;
//static float bt = 0;

//Inactivity shutdown variables
static unsigned long previous_millis_cmd = 0;
static unsigned long max_inactive_time = 0;
static unsigned long stepper_inactive_time = DEFAULT_STEPPER_DEACTIVE_TIME*1000l;

unsigned long starttime=0;
unsigned long stoptime=0;

static uint8_t tmp_extruder;

bool Stopped=false;

#ifdef DUAL_X_CARRIAGE
static bool active_extruder_parked = false; // used in mode 1 & 2
static float raised_parked_position[NUM_AXIS]; // used in mode 1 
static unsigned long delayed_move_time = 0; // used in mode 1 
static float duplicate_extruder_x_offset = DEFAULT_DUPLICATION_X_OFFSET; // used in mode 2
static float duplicate_extruder_temp_offset = 0; // used in mode 2
int extruder_carriage_mode = 1; // 1=autopark mode used in mode 2
#endif

#if NUM_SERVOS > 0
  Servo servos[NUM_SERVOS];
#endif

bool CooldownNoWait = true;
bool target_direction;

void sdcard_pause(int OValue=0);
void sdcard_resume();
void sdcard_stop();
void raise_Z_E(int Z, int E);

void load_filament(int LoadUnLoad, int TValue);

//===========================================================================
//=============================ROUTINES=============================
//===========================================================================

void get_arc_coordinates();
bool setTargetedHotend(int code);

void serial_echopair_P(const char *s_P, float v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, double v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, unsigned long v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }

extern "C"{
  extern unsigned int __bss_end;
  extern unsigned int __heap_start;
  extern void *__brkval;

  int freeMemory() {
    int free_memory;

    if((int)__brkval == 0)
      free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
      free_memory = ((int)&free_memory) - ((int)__brkval);

    return free_memory;
  }
}

//adds an command to the main command buffer
//thats really done in a non-safe way.
//needs overworking someday
void enquecommand(const char *cmd)
{
  if(buflen < BUFSIZE)
  {
    //this is dangerous if a mixing of serial and this happsens
    strcpy(&(cmdbuffer[bufindw][0]),cmd);
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM("enqueing \"");
    SERIAL_ECHO(cmdbuffer[bufindw]);
    SERIAL_ECHOLNPGM("\"");
    bufindw= (bufindw + 1)%BUFSIZE;
    buflen += 1;
  }
}

void enquecommand_P(const char *cmd)
{
  if(buflen < BUFSIZE)
  {
    //this is dangerous if a mixing of serial and this happsens
    strcpy_P(&(cmdbuffer[bufindw][0]),cmd);
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM("enqueing \"");
    SERIAL_ECHO(cmdbuffer[bufindw]);
    SERIAL_ECHOLNPGM("\"");
    bufindw= (bufindw + 1)%BUFSIZE;
    buflen += 1;
  }
}

void setup_killpin()
{
  #if defined(POWER_LOSS_DETECT_PIN) && POWER_LOSS_DETECT_PIN > -1
    pinMode(POWER_LOSS_DETECT_PIN,INPUT);
  #endif
}

void setup_photpin()
{
  #if defined(PHOTOGRAPH_PIN) && PHOTOGRAPH_PIN > -1
    SET_OUTPUT(PHOTOGRAPH_PIN);
    WRITE(PHOTOGRAPH_PIN, LOW);
  #endif
}

void setup_powerhold()
{
  #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
    SET_OUTPUT(SUICIDE_PIN);
    WRITE(SUICIDE_PIN, HIGH);
  #endif
  #if defined(PS_ON_PIN) && PS_ON_PIN > -1
    SET_OUTPUT(PS_ON_PIN);
    WRITE(PS_ON_PIN, PS_ON_AWAKE);
  #endif
}

void suicide()
{
  #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
    SET_OUTPUT(SUICIDE_PIN);
    WRITE(SUICIDE_PIN, LOW);
  #endif
}

void servo_init()
{
  #if (NUM_SERVOS >= 1) && defined(SERVO0_PIN) && (SERVO0_PIN > -1)
    servos[0].attach(SERVO0_PIN);
  #endif
  #if (NUM_SERVOS >= 2) && defined(SERVO1_PIN) && (SERVO1_PIN > -1)
    servos[1].attach(SERVO1_PIN);
  #endif
  #if (NUM_SERVOS >= 3) && defined(SERVO2_PIN) && (SERVO2_PIN > -1)
    servos[2].attach(SERVO2_PIN);
  #endif
  #if (NUM_SERVOS >= 4) && defined(SERVO3_PIN) && (SERVO3_PIN > -1)
    servos[3].attach(SERVO3_PIN);
  #endif
  #if (NUM_SERVOS >= 5)
    #error "TODO: enter initalisation code for more servos"
  #endif

  // Set position of Servo Endstops that are defined
  #ifdef SERVO_ENDSTOPS
  for(int8_t i = 0; i < 3; i++)
  {
    if(servo_endstops[i] > -1) {
      servos[servo_endstops[i]].write(servo_endstop_angles[i * 2 + 1]);
    }
  }
  #endif
}


///////////////////split by zyf
String getSplitValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {
    0, -1
  };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

float string2Float(String Value){
    char floatbuf[32]; // make this at least big enough for the whole string
    Value.toCharArray(floatbuf, sizeof(floatbuf));
    float fRet = atof(floatbuf);
    return fRet;
}


#ifdef TENLOG_CONTROLLER
void TenlogScreen_begin(int boud);
void TenlogScreen_println(const char s[]);
void TenlogScreen_print(const char s[]);
void TenlogScreen_printend();
void TenlogScreen_printEmptyend();
int i_print_page_id = 0;

int tenlog_status_update_delay;
int tenlogScreenUpdate;


void tenlog_screen_update()
{
    String strAll = "main.sStatus.txt=\"";
    long lN = current_position[X_AXIS]*10.0; //1
    String sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = current_position[Y_AXIS]*10.0;     //2
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = current_position[Z_AXIS]*10.0;     //3
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = current_position[E_AXIS]*10.0;     //4
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = int(degTargetHotend(0) + 0.5);     //5
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = int(degHotend(0) + 0.5);           //6
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = int(degTargetHotend(1) + 0.5);     //7
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = int(degHotend(1) + 0.5);           //8
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = int(degTargetBed() + 0.5);         //9
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = int(degBed() + 0.5);               //10
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = fanSpeed;                          //11
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN = feedmultiply;                      //12
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    if (card.sdprinting == 1)                     //13
    {
        strAll = strAll +  "1|";
        lN = card.percentDone();
        sSend = String(lN);					//14
        strAll = strAll +  sSend + "|";
    }
    else if(card.sdprinting == 0)
    {
        strAll = strAll + "0|0|";
    }
    else if(card.sdprinting == 2)
    {
        strAll = strAll + "2|0|";
    }

    lN=active_extruder;                     //15
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    lN=dual_x_carriage_mode;                //16
    sSend = String(lN);
    strAll = strAll +  sSend + "|";

    //lN=dual_x_carriage_mode;                //17 time
    if(IS_SD_PRINTING){
        uint16_t time = millis()/60000 - starttime/60000;
        sSend = String(itostr2(time/60)) + ":" + String(itostr2(time%60));
        strAll = strAll +  sSend + "|";
    }else{
        strAll = strAll + "00:00|";    
    }

    if(card.isFileOpen()){              //18 is file open
        strAll = strAll + "1|";
    }else{
        strAll = strAll + "0|";
    }

    if(isHeatingHotend(0)){              //19 is heating nozzle 0
        strAll = strAll + "1|";
    }else{
        strAll = strAll + "0|";
    }

    if(isHeatingHotend(1)){              //20 is heating nozzle 1
        strAll = strAll + "1|";
    }else{
        strAll = strAll + "0|";
    }

    if(isHeatingBed()){              //21 is heating Bed
        strAll = strAll + "1|";
    }else{
        strAll = strAll + "0|";
    }

    strAll = strAll + "\"";
	const char* strAll0 = strAll.c_str();
    TenlogScreen_println(strAll0);
    delay(50);
    TenlogScreen_println("click btReflush,0");
}

void tenlog_status_screen()
{
	if (tenlog_status_update_delay)
        tenlog_status_update_delay--;
    else
        tenlogScreenUpdate = 1;
    if (tenlogScreenUpdate)
    {
		tenlogScreenUpdate = 0;
        tenlog_screen_update();
        tenlog_status_update_delay = 5000;   /* redraw the main screen every second. This is easier then trying keep track of all things that change on the screen */
    }
}

//Get Data From Commport
String getSerial2Data() {
  String strSerialdata = "";
  while (MSerial2_available() > 0) {
      strSerialdata += char(MSerial2_read());
      delay(2);
  }
  return strSerialdata;

}

void Init_TenlogScreen()
{
  _delay_ms(20);
  TenlogScreen_print("main.vLanguageID.val=");
  TenlogScreen_print(String(languageID).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  long iSend=zyf_X2_MAX_POS * 100.0;
  TenlogScreen_print("setting.xX2.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);
  
  iSend=zyf_Y2_OFFSET * 100.0;
  TenlogScreen_print("setting.xY2.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);
  
  iSend=zyf_Z2_OFFSET * 100.0;
  TenlogScreen_print("setting.xZ2.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  iSend=axis_steps_per_unit[0] * 100.0;
  TenlogScreen_print("setting.xXs.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  iSend=axis_steps_per_unit[1] * 100.0;
  TenlogScreen_print("setting.xYs.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  iSend=axis_steps_per_unit[2] * 100.0;
  TenlogScreen_print("setting.xZs.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  iSend=axis_steps_per_unit[3] * 100.0;
  TenlogScreen_print("setting.xEs.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  #ifdef FAN2_CONTROL
  iSend=zyf_FAN2_VALUE;
  TenlogScreen_print("setting.nF2s.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  #else
  TenlogScreen_println("setting.nF2s.val=0");
  #endif
  _delay_ms(20);

  #ifdef FAN2_CONTROL
  iSend=zyf_FAN2_START_TEMP;
  TenlogScreen_print("setting.nF2t.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  #else
  TenlogScreen_println("setting.nF2t.val=0");
  #endif
  _delay_ms(20);

  iSend=zyf_X2_MAX_POS * 10.0;
  TenlogScreen_print("main.vXMax.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  iSend=Y_MAX_POS * 10.0;
  TenlogScreen_print("main.vYMax.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);

  iSend=Z_MAX_POS * 10.0;
  TenlogScreen_print("main.vZMax.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);
  
  #ifdef POWER_LOSS_TRIGGER_BY_PIN
  TenlogScreen_println("main.vPFR.val=1");
  TenlogScreen_printend();
  _delay_ms(20);

  iSend=zyf_AUTO_OFF;
  TenlogScreen_print("setting.cAutoOff.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);
  #endif //POWER_LOSS_TRIGGER_BY_PIN

  iSend=zyf_SLEEP_TIME;
  TenlogScreen_print("setting.nSleep.val=");
  TenlogScreen_print(String(iSend).c_str());
  TenlogScreen_printend();
  _delay_ms(20);
  TenlogScreen_println("sleep=0");
  _delay_ms(20);
}

unsigned long lScreenStart = 0;
void CSDI_TLS()
{
    lScreenStart = millis();
    TenlogScreen_printEmptyend();
    TenlogScreen_println("connect");
    bool bCheckDone = false;
    while (!bCheckDone)
    {
        String strSerial2 = getSerial2Data();
        if(strSerial2 != "")
        {
            strSerial2.replace("\r","");
            strSerial2.replace("\n","");
            String strDI =  getSplitValue(strSerial2,',',5);
            bCheckDone = true;
            TenlogScreen_print("loading.sDI.txt=\"");
			TenlogScreen_print(strDI.c_str());
            TenlogScreen_print("\"");
            TenlogScreen_printend();
            _delay_ms(20);
            TenlogScreen_println("click btA,0");                        
        }else{
            _delay_ms(10);
        }
		if(millis() - lScreenStart > 1000)
		{
			TenlogScreen_printEmptyend();
			TenlogScreen_println("connect");
			lScreenStart = millis();
		}
    }
}
#endif //TENLOG_CONTROLLER



void setup()
{
  setup_killpin();
  setup_powerhold();

    #ifdef ENGRAVE
        pinMode(ENGRAVE_PIN, OUTPUT);
        digitalWrite(ENGRAVE_PIN, ENGRAVE_OFF);
    #endif

  _delay_ms(20);
  #ifdef TENLOG_CONTROLLER
      TenlogScreen_begin(9600);
      _delay_ms(100);
      TenlogScreen_println("sleep=0");
      _delay_ms(20);
      #ifdef POWER_LOSS_TRIGGER_BY_PIN
      TenlogScreen_println("main.vPFR.val=1");
      _delay_ms(20);
      #endif
      TenlogScreen_println("page 0");
      _delay_ms(20);
  #endif
  MYSERIAL.begin(BAUDRATE);
  SERIAL_PROTOCOLLNPGM("start");
  SERIAL_ECHO_START;

  // Check startup - does nothing if bootloader sets MCUSR to 0
  byte mcu = MCUSR;
  if(mcu & 1) SERIAL_ECHOLNPGM(MSG_POWERUP);
  if(mcu & 2) SERIAL_ECHOLNPGM(MSG_EXTERNAL_RESET);
  if(mcu & 4) SERIAL_ECHOLNPGM(MSG_BROWNOUT_RESET);
  if(mcu & 8) SERIAL_ECHOLNPGM(MSG_WATCHDOG_RESET);
  if(mcu & 32) SERIAL_ECHOLNPGM(MSG_SOFTWARE_RESET);
  MCUSR=0;

  SERIAL_ECHOPGM(MSG_MARLIN);
  SERIAL_ECHOLNPGM(VERSION_STRING);
  #ifdef STRING_VERSION_CONFIG_H
    #ifdef STRING_CONFIG_H_AUTHOR
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM(MSG_CONFIGURATION_VER);
      SERIAL_ECHOPGM(STRING_VERSION_CONFIG_H);
      SERIAL_ECHOPGM(MSG_AUTHOR);
      SERIAL_ECHOLNPGM(STRING_CONFIG_H_AUTHOR);
      SERIAL_ECHOPGM("Compiled: ");
      SERIAL_ECHOLNPGM(__DATE__);
      #ifdef TENLOG_CONTROLLER   
		  String strDate = __DATE__;
		  TenlogScreen_print("about.tVer.txt=\"Firmware: ");
		  TenlogScreen_print(strDate.c_str());
          TenlogScreen_println("\"");
	  #endif
    #endif
  #endif
  SERIAL_ECHO_START;
  SERIAL_ECHOPGM(MSG_FREE_MEMORY);
  SERIAL_ECHO(freeMemory());
  //#ifdef TENLOG_CONTROLLER
  //String sFM = String(freeMemory());
  //TenlogScreen_print("tStatus.txt=\"Free Memory: ");
  //TenlogScreen_print(sFM.c_str());
  //TenlogScreen_println("\"");
  //#endif
  SERIAL_ECHOPGM(MSG_PLANNER_BUFFER_BYTES);
  SERIAL_ECHOLN((int)sizeof(block_t)*BLOCK_BUFFER_SIZE);
  for(int8_t i = 0; i < BUFSIZE; i++)
  {
    fromsd[i] = false;
  }

  // loads data from EEPROM if available else uses defaults (and resets step acceleration rate)
  Config_RetrieveSettings();

  #ifdef TENLOG_CONTROLLER 
  ZYF_DEBUG_PRINT_LN("Init Screen...");	
  #endif

  #ifdef TENLOG_CONTROLLER
    CSDI_TLS();
    Init_TenlogScreen();
  #endif


  tp_init();    // Initialize temperature loop
  plan_init();  // Initialize planner;
  watchdog_init();

  st_init();    // Initialize stepper, this enables interrupts!
  
  setup_photpin();
  servo_init();
  #ifdef TENLOG_CONTROLLER
  TenlogScreen_println("tStatus.txt=\"Init sd reader...\"");
  #endif 
  sd_init();

  _delay_ms(1000);	// wait 1sec to display the splash screen
  
  #if defined(CONTROLLERFAN_PIN) && CONTROLLERFAN_PIN > -1
    SET_OUTPUT(CONTROLLERFAN_PIN); //Set pin used for driver cooling fan
  #endif

  #ifdef TENLOG_CONTROLLER
    TenlogScreen_println("sleep=0");
    TenlogScreen_println("page main");
    #ifdef POWER_LOSS_RECOVERY
    String sFileName = card.isPowerLoss();

    if(sFileName != ""){        
        TenlogScreen_println("msgbox.vaFromPageID.val=1");
        TenlogScreen_println("msgbox.vaToPageID.val=6");
        TenlogScreen_println("msgbox.vtOKValue.txt=\"M1003\"");
        TenlogScreen_println("msgbox.vtCancelValue.txt=\"M1004\"");
        String strMessage="";
        if(languageID==0)
            strMessage="Power loss detected, Resume print " + sFileName + "?";
        else
            strMessage="检测到断电，恢复打印" + sFileName + "？";
		strMessage = "msgbox.tMessage.txt=\"" + strMessage + "\"";
        const char* str0 = strMessage.c_str();
		TenlogScreen_println(str0);
        TenlogScreen_println("page msgbox");            
    }
    #endif
  #endif

#ifdef PRINT_FROM_Z_HEIGHT
	PrintFromZLevelFound = true;
	print_from_z_target = 0.0;
#endif
    WRITE(PS_ON_PIN, PS_ON_AWAKE);
}

void loop()
{
  #ifdef TENLOG_CONTROLLER
  if(buflen < (BUFSIZE-1))
      get_command_1();
  #endif
    
  if(buflen < (BUFSIZE-1))
      get_command();
    
  #ifdef SDSUPPORT
  card.checkautostart(false);
  #endif
  if(buflen)
  {
    #ifdef SDSUPPORT
      if(card.saving)
      {
        if(strstr_P(cmdbuffer[bufindr], PSTR("M29")) == NULL)
        {
          card.write_command(cmdbuffer[bufindr]);
          if(card.logging)
          {
            process_commands();
          }
          else
          {
            SERIAL_PROTOCOLLNPGM(MSG_OK);
          }
        }
        else
        {
          card.closefile();
          SERIAL_PROTOCOLLNPGM(MSG_FILE_SAVED);
        }
      }
      else
      {
		#ifdef POWER_LOSS_RECOVERY
			#ifdef POWER_LOSS_TRIGGER_BY_PIN
				Check_Power_Loss();
			#endif
			int iBPos = degTargetBed() + 0.5;
			#if defined(POWER_LOSS_SAVE_TO_EEPROM)
				EEPROM_PRE_Write_PLR(card.sdpos, iBPos, dual_x_carriage_mode, duplicate_extruder_x_offset, feedrate);
			#elif defined(POWER_LOSS_SAVE_TO_SDCARD)
				card.PRE_Write_PLR(card.sdpos, iBPos, dual_x_carriage_mode, duplicate_extruder_x_offset, feedrate);
			#endif
		#endif //POWER_LOSS_RECOVERY
		process_commands();
      }
    #else
      process_commands();
    #endif //SDSUPPORT
    buflen = (buflen-1);
    bufindr = (bufindr + 1)%BUFSIZE;
  }
  //check heater every n milliseconds
  manage_heater();
  if(zyf_HEATER_FAIL){
      card.closefile();
      card.sdprinting = 0;
  }  

  manage_inactivity();
  checkHitEndstops();
  lcd_update();
}

#ifdef TENLOG_CONTROLLER

void get_command_1()
{
  while( MSerial2_available() > 0 && buflen < BUFSIZE) {
    
  serial_char = MSerial2_read();
  int iSC = (int)serial_char;

	if(serial_char == '\n' ||
       iSC == -1 ||
       serial_char == '\r' ||
       (serial_char == ':' && comment_mode == false) ||
       serial_count >= (MAX_CMD_SIZE - 1) )
    {
      if(!serial_count) { //if empty line
        comment_mode = false; //for new command
        return;
      }
      cmdbuffer[bufindw][serial_count] = 0; //terminate string
      if(!comment_mode){
        comment_mode = false; //for new command
        fromsd[bufindw] = false;
        if(strchr(cmdbuffer[bufindw], 'N') != NULL)
        {
          strchr_pointer = strchr(cmdbuffer[bufindw], 'N');
          gcode_N = (strtol(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL, 10));
          if(gcode_N != gcode_LastN+1 && (strstr_P(cmdbuffer[bufindw], PSTR("M110")) == NULL) ) {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_LINE_NO);
            SERIAL_ERRORLN(gcode_LastN);
            //Serial.println(gcode_N);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          if(strchr(cmdbuffer[bufindw], '*') != NULL)
          {
            byte checksum = 0;
            byte count = 0;
            while(cmdbuffer[bufindw][count] != '*') checksum = checksum^cmdbuffer[bufindw][count++];
            strchr_pointer = strchr(cmdbuffer[bufindw], '*');

            if( (int)(strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)) != checksum) {
              SERIAL_ERROR_START;
              SERIAL_ERRORPGM(MSG_ERR_CHECKSUM_MISMATCH);
              SERIAL_ERRORLN(gcode_LastN);
              FlushSerialRequestResend();
              serial_count = 0;
              return;
            }
            //if no errors, continue parsing
          }
          else
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          gcode_LastN = gcode_N;
          //if no errors, continue parsing
        }
        else  // if we don't receive 'N' but still see '*'
        {
          if((strchr(cmdbuffer[bufindw], '*') != NULL))
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_LINENUMBER_WITH_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            serial_count = 0;
            return;
          }
        }
        if((strchr(cmdbuffer[bufindw], 'G') != NULL)){
          strchr_pointer = strchr(cmdbuffer[bufindw], 'G');
          switch((int)((strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)))){
          case 0:
          case 1:
          case 2:
          case 3:
            if(Stopped == false) { // If printer is stopped by an error the G[0-3] codes are ignored.
          #ifdef SDSUPPORT
              if(card.saving)
                break;
          #endif //SDSUPPORT
              SERIAL_PROTOCOLLNPGM(MSG_OK);
            }
            else {
              SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
              LCD_MESSAGEPGM(MSG_STOPPED);
            }
            break;
          default:
            break;
          }

        }
        bufindw = (bufindw + 1)%BUFSIZE;
        buflen += 1;
      }
      serial_count = 0; //clear buffer
    }
    else
    {
      if(serial_char == ';' ||
		  (int)serial_char < 0 ||
		  ((int)serial_char == 104 && cmdbuffer[bufindw][0] != 'M')
		  ) {
		comment_mode = true;
	  }
      if(!comment_mode) cmdbuffer[bufindw][serial_count++] = serial_char;
    }
  }
}
#endif

float code_value()
{
  return (strtod(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1], NULL));
}

long code_value_long()
{
  return (strtol(&cmdbuffer[bufindr][strchr_pointer - cmdbuffer[bufindr] + 1], NULL, 10));
}

bool code_seen(char code)
{
  strchr_pointer = strchr(cmdbuffer[bufindr], code);
  return (strchr_pointer != NULL);  //Return True if a character was found
}

void command_M81(){
      #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
        st_synchronize();
        suicide();
      #elif defined(PS_ON_PIN) && PS_ON_PIN > -1
        SET_OUTPUT(PS_ON_PIN);
		#ifdef TENLOG_CONTROLLER
		TenlogScreen_println("page shutdown");
		delay(1000);
		#endif
        WRITE(PS_ON_PIN, PS_ON_ASLEEP);
		digitalWrite(HEATER_BED_PIN,HIGH);
        digitalWrite(HEATER_0_PIN,HIGH);
        digitalWrite(HEATER_1_PIN,HIGH);
		#ifdef FAN2_CONTROL
        digitalWrite(FAN2_PIN,HIGH);
		#endif
        digitalWrite(FAN_PIN,HIGH);
        delay(5000);
		digitalWrite(HEATER_BED_PIN,LOW);
        digitalWrite(HEATER_0_PIN,LOW);
        digitalWrite(HEATER_1_PIN,LOW);
      #endif
}

void command_G4(float dwell=0){
  unsigned long codenum; //throw away variable  
  
  LCD_MESSAGEPGM(MSG_DWELL);
  codenum = 0;
  if(code_seen('P')) codenum = code_value(); // milliseconds to wait
  if(code_seen('S')) codenum = code_value() * 1000; // seconds to wait
  if(dwell > 0) codenum = dwell * 1000;

  st_synchronize();
  codenum += millis();  // keep track of when we started waiting
  previous_millis_cmd = millis();
  while(millis()  < codenum ){
    manage_heater();
    if(zyf_HEATER_FAIL){
        card.closefile();
        card.sdprinting = 0;
    }
    manage_inactivity();
    lcd_update();
  }
}

void get_command()
{
  while( MYSERIAL.available() > 0  && buflen < BUFSIZE) {
    serial_char = MYSERIAL.read();
    if(serial_char == '\n' ||
       serial_char == '\r' ||
       (serial_char == ':' && comment_mode == false) ||
       serial_count >= (MAX_CMD_SIZE - 1) )
    {
      if(!serial_count) { //if empty line
        comment_mode = false; //for new command
        return;
      }
      cmdbuffer[bufindw][serial_count] = 0; //terminate string
      if(!comment_mode){
        comment_mode = false; //for new command
        fromsd[bufindw] = false;
        if(strchr(cmdbuffer[bufindw], 'N') != NULL)
        {
          strchr_pointer = strchr(cmdbuffer[bufindw], 'N');
          gcode_N = (strtol(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL, 10));
          if(gcode_N != gcode_LastN+1 && (strstr_P(cmdbuffer[bufindw], PSTR("M110")) == NULL) ) {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_LINE_NO);
            SERIAL_ERRORLN(gcode_LastN);
            //Serial.println(gcode_N);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          if(strchr(cmdbuffer[bufindw], '*') != NULL)
          {
            byte checksum = 0;
            byte count = 0;
            while(cmdbuffer[bufindw][count] != '*') checksum = checksum^cmdbuffer[bufindw][count++];
            strchr_pointer = strchr(cmdbuffer[bufindw], '*');

            if( (int)(strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)) != checksum) {
              SERIAL_ERROR_START;
              SERIAL_ERRORPGM(MSG_ERR_CHECKSUM_MISMATCH);
              SERIAL_ERRORLN(gcode_LastN);
              FlushSerialRequestResend();
              serial_count = 0;
              return;
            }
            //if no errors, continue parsing
          }
          else
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          gcode_LastN = gcode_N;
          //if no errors, continue parsing
        }
        else  // if we don't receive 'N' but still see '*'
        {
          if((strchr(cmdbuffer[bufindw], '*') != NULL))
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_LINENUMBER_WITH_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            serial_count = 0;
            return;
          }
        }
        if((strchr(cmdbuffer[bufindw], 'G') != NULL)){
          strchr_pointer = strchr(cmdbuffer[bufindw], 'G');
          switch((int)((strtod(&cmdbuffer[bufindw][strchr_pointer - cmdbuffer[bufindw] + 1], NULL)))){
          case 0:
          case 1:
          case 2:
          case 3:
            if(Stopped == false) { // If printer is stopped by an error the G[0-3] codes are ignored.
          #ifdef SDSUPPORT
              if(card.saving)
                break;
          #endif //SDSUPPORT
              // SERIAL_PROTOCOLLNPGM(MSG_OK); // Bug fix for USB Printing - levelfifty
            }
            else {
              SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
              LCD_MESSAGEPGM(MSG_STOPPED);
            }
            break;
          default:
            break;
          }

        }
        bufindw = (bufindw + 1)%BUFSIZE;
        buflen += 1;
      }
      serial_count = 0; //clear buffer
    }
    else
    {
      if(serial_char == ';') comment_mode = true;
      if(!comment_mode) cmdbuffer[bufindw][serial_count++] = serial_char;
    }
  }
  #ifdef SDSUPPORT
  if(card.sdprinting == 0 || serial_count!=0){ 
    return;
  }
  while( !card.eof()  && buflen < BUFSIZE) {
    int16_t n=card.get();
    serial_char = (char)n;
    if(serial_char == '\n' ||
       serial_char == '\r' ||
       (serial_char == ':' && comment_mode == false) ||
       serial_count >= (MAX_CMD_SIZE - 1)||n==-1)
    {
      if(card.eof()){
        SERIAL_PROTOCOLLNPGM(MSG_FILE_PRINTED);
        stoptime=millis();
        char time[30];
        unsigned long t=(stoptime-starttime)/1000;
        int hours, minutes;
        minutes=(t/60)%60;
        hours=t/60/60;
        sprintf_P(time, PSTR("%i hours %i minutes"),hours, minutes);
        SERIAL_ECHO_START;
        SERIAL_ECHOLN(time);
        lcd_setstatus(time);
        #ifdef TENLOG_CONTROLLER
        TenlogScreen_println("sleep=0");
        TenlogScreen_println("msgbox.vaFromPageID.val=1");
        TenlogScreen_println("msgbox.vaToPageID.val=1");
        TenlogScreen_println("msgbox.vtOKValue.txt=\"\"");
        String strMessage="";
        bool bAutoOff = false;
        if(languageID==0)
            strMessage="Print finished, " + String(hours) + " hours and " + String(minutes) + " minutes. ";
        else
            strMessage="打印完成！共用了" + String(hours) + "时" + String(minutes) + "分。";
            #ifdef POWER_LOSS_TRIGGER_BY_PIN
        if(zyf_AUTO_OFF == 1){
            if(languageID==0)
                strMessage = strMessage + "Power off in 10 seconds.";
            else
                strMessage = strMessage + "10秒后关机";
            bAutoOff = true;
        }
            #endif
        strMessage = "msgbox.tMessage.txt=\"" + strMessage + "\"";
	    const char*  str0 = strMessage.c_str();
		TenlogScreen_println(str0);
        TenlogScreen_println("page msgbox");
        
        if(bAutoOff){
            command_G4(10.0);
            command_M81();
        }
        #endif //TENLOG_CONTROLLER
        card.printingHasFinished();
        card.checkautostart(true);
      }
      if(!serial_count)
      {
        comment_mode = false; //for new command
        return; //if empty line
      }
      cmdbuffer[bufindw][serial_count] = 0; //terminate string
//      if(!comment_mode){
        fromsd[bufindw] = true;
        buflen += 1;
        bufindw = (bufindw + 1)%BUFSIZE;
//      }
      comment_mode = false; //for new command
      serial_count = 0; //clear buffer
    }
    else
    {
      if(serial_char == ';') comment_mode = true;
      if(!comment_mode) cmdbuffer[bufindw][serial_count++] = serial_char;
    }
  }

  #endif //SDSUPPORT

}


#define DEFINE_PGM_READ_ANY(type, reader)       \
    static inline type pgm_read_any(const type *p)  \
    { return pgm_read_##reader##_near(p); }

DEFINE_PGM_READ_ANY(float,       float);
DEFINE_PGM_READ_ANY(signed char, byte);

#define XYZ_CONSTS_FROM_CONFIG(type, array, CONFIG) \
static const PROGMEM type array##_P[3] =        \
    { X_##CONFIG, Y_##CONFIG, Z_##CONFIG };     \
static inline type array(int axis)          \
    { return pgm_read_any(&array##_P[axis]); }

XYZ_CONSTS_FROM_CONFIG(float, base_min_pos,    MIN_POS);
XYZ_CONSTS_FROM_CONFIG(float, base_max_pos,    MAX_POS);
XYZ_CONSTS_FROM_CONFIG(float, base_home_pos,   HOME_POS);
XYZ_CONSTS_FROM_CONFIG(float, max_length,      MAX_LENGTH);
XYZ_CONSTS_FROM_CONFIG(float, home_retract_mm, HOME_RETRACT_MM);
XYZ_CONSTS_FROM_CONFIG(signed char, home_dir,  HOME_DIR);

#ifdef DUAL_X_CARRIAGE

  #if EXTRUDERS == 1 || defined(COREXY) \
      || !defined(X2_ENABLE_PIN) || !defined(X2_STEP_PIN) || !defined(X2_DIR_PIN) \
      || !defined(X2_HOME_POS) || !defined(X2_MIN_POS) || !defined(X2_MAX_POS)      
    #error "Missing or invalid definitions for DUAL_X_CARRIAGE mode."
	#elif !defined(X_MAX_PIN) || X_MAX_PIN < 0
    #error "Missing or invalid definitions for DUAL_X_CARRIAGE mode X MAX Pin."
  #endif
  #if X_HOME_DIR != -1 || X2_HOME_DIR != 1
    #error "Please use canonical x-carriage assignment" // the x-carriages are defined by their homing directions
  #endif  

#define DXC_FULL_CONTROL_MODE 0
#define DXC_AUTO_PARK_MODE    1
#define DXC_DUPLICATION_MODE  2
#define DXC_MIRROR_MODE  3
int dual_x_carriage_mode = DEFAULT_DUAL_X_CARRIAGE_MODE;
 
static float x_home_pos(int extruder) {
  if (extruder == 0)
    return base_home_pos(X_AXIS) + add_homeing[X_AXIS];
  else
    // In dual carriage mode the extruder offset provides an override of the
    // second X-carriage offset when homed - otherwise X2_HOME_POS is used.
    // This allow soft recalibration of the second extruder offset position without firmware reflash 
    // (through the M218 command).
    #ifdef CONFIG_XYZ
    return (extruder_offset[X_AXIS][1] > 0) ? extruder_offset[X_AXIS][1] : zyf_X2_MAX_POS;
	#else
    return (extruder_offset[X_AXIS][1] > 0) ? extruder_offset[X_AXIS][1] : X2_MAX_POS;
	#endif
}

static int x_home_dir(int extruder) {
  return (extruder == 0) ? X_HOME_DIR : X2_HOME_DIR;
}

#ifdef CONFIG_XYZ
static float inactive_extruder_x_pos = zyf_X2_MAX_POS; // used in mode 0 & 1
#else
static float inactive_extruder_x_pos = X2_MAX_POS; // used in mode 0 & 1
#endif

#endif //DUAL_X_CARRIAGE    

static void axis_is_at_home(int axis) {
#ifdef DUAL_X_CARRIAGE
  if (axis == X_AXIS) {
    if (active_extruder != 0) {
      current_position[X_AXIS] = x_home_pos(active_extruder);
      min_pos[X_AXIS] =          X2_MIN_POS;
	  #ifdef CONFIG_XYZ
	  max_pos[X_AXIS] = max(extruder_offset[X_AXIS][1], zyf_X2_MAX_POS);
	  #else
	  max_pos[X_AXIS] = max(extruder_offset[X_AXIS][1], X2_MAX_POS);
	  #endif
      return;
    }
    else if ((dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE) && active_extruder == 0) {
      current_position[X_AXIS] = base_home_pos(X_AXIS) + add_homeing[X_AXIS];
      min_pos[X_AXIS] = base_min_pos(X_AXIS) + add_homeing[X_AXIS]; 
      #ifdef CONFIG_XYZ
      max_pos[X_AXIS] = min(base_max_pos(X_AXIS) + add_homeing[X_AXIS], max(extruder_offset[X_AXIS][1], zyf_X2_MAX_POS) - duplicate_extruder_x_offset);
      #ifdef ZYF_DEBUG
      ZYF_DEBUG_PRINT("X12_MAX_POS:");
      ZYF_DEBUG_PRINT_LN(max_pos[X_AXIS]);
      #endif
	  #else
      max_pos[X_AXIS] = min(base_max_pos(X_AXIS) + add_homeing[X_AXIS], max(extruder_offset[X_AXIS][1], X2_MAX_POS) - duplicate_extruder_x_offset);
	  #endif
	  return;
    }
  }
#endif  //DUAL_X_CARRIAGE
  current_position[axis] = base_home_pos(axis) + add_homeing[axis];
  min_pos[axis] =          base_min_pos(axis) + add_homeing[axis];
  max_pos[axis] =          base_max_pos(axis) + add_homeing[axis];
  #ifdef ZYF_DEBUG
  ZYF_DEBUG_PRINT(axis);
  ZYF_DEBUG_PRINT("_MAX_POS:");
  ZYF_DEBUG_PRINT_LN(max_pos[axis]);
  #endif
}

static void homeaxis(int axis) {

#define HOMEAXIS_DO(LETTER) \
  ((LETTER##_MIN_PIN > -1 && LETTER##_HOME_DIR==-1) || (LETTER##_MAX_PIN > -1 && LETTER##_HOME_DIR==1))

  if (axis==X_AXIS ? HOMEAXIS_DO(X) :
      axis==Y_AXIS ? HOMEAXIS_DO(Y) :
      axis==Z_AXIS ? HOMEAXIS_DO(Z) :
      0) {
    int axis_home_dir = home_dir(axis);
	#ifdef DUAL_X_CARRIAGE
    if (axis == X_AXIS)
      axis_home_dir = x_home_dir(active_extruder);
	#endif

    // Engage Servo endstop if enabled
		#ifdef SERVO_ENDSTOPS
		if (SERVO_ENDSTOPS[axis] > -1) {
	        servos[servo_endstops[axis]].write(servo_endstop_angles[axis * 2]);
		}
		#endif
      
	int iR0 = 60;
	int iR1 = 100;
	int iR2 = 180;

    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = 1.5 * max_length(axis) * axis_home_dir;
  	feedrate = homing_feedrate[axis];
    //SERIAL_PROTOCOL(destination[axis] ); //By Zyf
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/iR0, active_extruder);
	  //SERIAL_PROTOCOLLNPGM(" Homing 1");
    st_synchronize();
    
	current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = -home_retract_mm(axis) * axis_home_dir;
    //SERIAL_PROTOCOL(destination[axis] );
	plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/iR1, active_extruder);
	//SERIAL_PROTOCOLLNPGM(" Homing 2");
    st_synchronize();

    destination[axis] = 2*home_retract_mm(axis) * axis_home_dir;
    feedrate = homing_feedrate[axis]/2 ;
    //SERIAL_PROTOCOL(destination[axis] );
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/iR2, active_extruder);
	//SERIAL_PROTOCOLLNPGM(" Homing 3");
    st_synchronize();

    axis_is_at_home(axis);
    destination[axis] = current_position[axis];
    feedrate = 0.0;
    endstops_hit_on_purpose();

    // Retract Servo endstop if enabled
    #ifdef SERVO_ENDSTOPS
      if (SERVO_ENDSTOPS[axis] > -1) {
        servos[servo_endstops[axis]].write(servo_endstop_angles[axis * 2 + 1]);
      }
    #endif
  }
}


void command_M605()
{
	st_synchronize();
	
	if (code_seen('S'))
	{
		dual_x_carriage_mode = code_value();
	}

	if (dual_x_carriage_mode == DXC_DUPLICATION_MODE)
	{
	  if (code_seen('X'))
	  {
		duplicate_extruder_x_offset = max(code_value(),X2_MIN_POS - x_home_pos(0));
	  }
	  if (code_seen('R'))
		  duplicate_extruder_temp_offset = code_value();

	  SERIAL_ECHO_START;
	  SERIAL_ECHOPGM(MSG_HOTEND_OFFSET);
	  SERIAL_ECHO(" ");
	  SERIAL_ECHO(extruder_offset[X_AXIS][0]);
	  SERIAL_ECHO(",");
	  SERIAL_ECHO(extruder_offset[Y_AXIS][0]);
	  SERIAL_ECHO(" ");
	  SERIAL_ECHO(duplicate_extruder_x_offset);
	  SERIAL_ECHO(",");
	  SERIAL_ECHOLN(extruder_offset[Y_AXIS][1]);
	}
	else if (dual_x_carriage_mode != DXC_FULL_CONTROL_MODE && dual_x_carriage_mode != DXC_AUTO_PARK_MODE && dual_x_carriage_mode != DXC_DUPLICATION_MODE && dual_x_carriage_mode != DXC_MIRROR_MODE)
	{
	  dual_x_carriage_mode = DEFAULT_DUAL_X_CARRIAGE_MODE;
	}
	
	active_extruder_parked = false;
	extruder_carriage_mode = dual_x_carriage_mode;
	delayed_move_time = 0;
}


#define HOMEAXIS(LETTER) homeaxis(LETTER##_AXIS)

void command_G28(int XHome=0, int YHome=0, int ZHome=0){         //By zyf    

  saved_feedrate = feedrate;
  saved_feedmultiply = feedmultiply;
  feedmultiply = 100;
  previous_millis_cmd = millis();

  enable_endstops(true, -1);

  for(int8_t i=0; i < NUM_AXIS; i++) {
    destination[i] = current_position[i];
  }
    feedrate = 0.0;

    home_all_axis = !((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])) || (code_seen(axis_codes[2]))) && XHome==0 && YHome==0 && ZHome==0;

	bool bSkip = false;
   	#ifdef PRINT_FROM_Z_HEIGHT
	if((home_all_axis || code_seen(axis_codes[2]) || ZHome == 1) && !PrintFromZLevelFound && card.sdprinting == 1){
		bSkip = true;
	}
	bool b_temp_PrintFromZLevelFound = PrintFromZLevelFound;
	#endif


  #if Z_HOME_DIR < 0                     
    if((home_all_axis || ZHome == 1 || code_seen(axis_codes[Z_AXIS])) && !bSkip) {
        
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
        destination[Z_AXIS] = current_position[Z_AXIS] + 7.0;
        feedrate = homing_feedrate[Z_AXIS];
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
        st_synchronize();	
        
        axis_is_at_home(Z_AXIS);
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
        destination[Z_AXIS] = current_position[Z_AXIS];
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
        feedrate = 0.0;
        st_synchronize();
        
		#ifdef ZYF_DUAL_Z		//By zyf
        zyf_RUN_STATUS = 1;
        #endif
    }
  #endif // Z_HOME_DIR < 0    

    #if Z_HOME_DIR > 0                      // If homing away from BED do Z first
    if((home_all_axis) || Zhome == 1 || (code_seen(axis_codes[Z_AXIS]))) {
        HOMEAXIS(Z);
    }
    #endif

  #ifdef PRINT_FROM_Z_HEIGHT
  PrintFromZLevelFound = true;
  #endif

  #ifdef QUICK_HOME
  if((home_all_axis)||( code_seen(axis_codes[X_AXIS]) && code_seen(axis_codes[Y_AXIS])) )  //first diagonal move
  {
    current_position[X_AXIS] = 0;current_position[Y_AXIS] = 0;

   #ifndef DUAL_X_CARRIAGE
    int x_axis_home_dir = home_dir(X_AXIS);
   #else
    int x_axis_home_dir = x_home_dir(active_extruder);
    extruder_carriage_mode = 1;
   #endif
    
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[X_AXIS] = 1.5 * max_length(X_AXIS) * x_axis_home_dir; destination[Y_AXIS] = 1.5 * max_length(Y_AXIS) * home_dir(Y_AXIS);
    feedrate = homing_feedrate[X_AXIS];
    if(homing_feedrate[Y_AXIS]<feedrate)
      feedrate =homing_feedrate[Y_AXIS];
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();

    axis_is_at_home(X_AXIS);
    axis_is_at_home(Y_AXIS);
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[X_AXIS] = current_position[X_AXIS];
    destination[Y_AXIS] = current_position[Y_AXIS];
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    feedrate = 0.0;
    st_synchronize();
    endstops_hit_on_purpose();

    current_position[X_AXIS] = destination[X_AXIS];
    current_position[Y_AXIS] = destination[Y_AXIS];
    current_position[Z_AXIS] = destination[Z_AXIS];
  }
  #endif

  if((home_all_axis) || XHome == 1 || (code_seen(axis_codes[X_AXIS])))
  {	
  #ifdef DUAL_X_CARRIAGE
    int tmp_extruder = active_extruder;
    extruder_carriage_mode = 1;
    active_extruder = !active_extruder;
    HOMEAXIS(X);
    inactive_extruder_x_pos = current_position[X_AXIS];
    active_extruder = tmp_extruder;
    HOMEAXIS(X);
    // reset state used by the different modes
    //memcpy(raised_parked_position, current_position, sizeof(raised_parked_position));   //By Zyf
    raised_parked_position[X_AXIS] = current_position[X_AXIS];														//By zyf
    
    if((home_all_axis) || YHome == 1 || (code_seen(axis_codes[Y_AXIS])))
        raised_parked_position[Y_AXIS] = 0;														//By zyf
    else
        raised_parked_position[Y_AXIS] = current_position[Y_AXIS];

    raised_parked_position[Z_AXIS] = current_position[Z_AXIS];														//By zyf
    raised_parked_position[E_AXIS] = current_position[E_AXIS];														//By zyf

    delayed_move_time = 0;
    active_extruder_parked = true; 
  #else      
    HOMEAXIS(X);
  #endif         
  }

  if((home_all_axis) || YHome == 1 || (code_seen(axis_codes[Y_AXIS]))) {
    HOMEAXIS(Y);
  }
	
  #ifdef PRINT_FROM_Z_HEIGHT
  if(!b_temp_PrintFromZLevelFound)
    command_G1(0,0);
  PrintFromZLevelFound = b_temp_PrintFromZLevelFound;
  #endif

  #if Z_HOME_DIR < 0                      // If homing towards BED do Z last
  if((home_all_axis || ZHome == 1 || code_seen(axis_codes[Z_AXIS])) && !bSkip) {

    #ifdef ZYF_DUAL_Z		//By Zyf
                
        zyf_Y_STEP_PIN = Z2_STEP_PIN;  //65;//60
        zyf_Y_DIR_PIN = Z2_DIR_PIN;    //66;//61
        zyf_Y_MIN_PIN = Z2_MIN_PIN;    //19;//14
        zyf_Y_ENDSTOPS_INVERTING = Z_ENDSTOPS_INVERTING; //LOW
        zyf_INVERT_Y_DIR = INVERT_Z_DIR;
        
        current_position[Z_AXIS] = 0;
        current_position[Y_AXIS] = 0;

        int Y_step_per_unit = axis_steps_per_unit[Y_AXIS];
        axis_steps_per_unit[Y_AXIS] = axis_steps_per_unit[Z_AXIS];

        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
        destination[Z_AXIS] = 1.5 * max_length(Z_AXIS) * home_dir(Z_AXIS); 
        destination[Y_AXIS] = 1.5 * max_length(Z_AXIS) * home_dir(Y_AXIS);

        feedrate = homing_feedrate[Z_AXIS];
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
        st_synchronize();

        axis_is_at_home(Z_AXIS);
        axis_is_at_home(Y_AXIS);
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
        destination[Z_AXIS] = current_position[Z_AXIS];
        destination[Y_AXIS] = current_position[Y_AXIS];
        //feedrate = homing_feedrate[Z_AXIS] * 6;
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
        feedrate = 0.0;
        st_synchronize();
        endstops_hit_on_purpose();

        current_position[X_AXIS] = destination[X_AXIS];
        current_position[Y_AXIS] = destination[Y_AXIS];
        current_position[Z_AXIS] = destination[Z_AXIS];
	
		int temp_feedrate = homing_feedrate[Y_AXIS];
        HOMEAXIS(Z);
        axis_steps_per_unit[Y_AXIS] = Y_step_per_unit;
		homing_feedrate[Y_AXIS] = homing_feedrate[Z_AXIS]*8;
		HOMEAXIS(Y);

        homing_feedrate[Y_AXIS] = temp_feedrate;
		zyf_Y_STEP_PIN = Y_STEP_PIN;  //60;
        zyf_Y_DIR_PIN = Y_DIR_PIN;    //61;
        zyf_Y_MIN_PIN = Y_MIN_PIN;    //14;
        zyf_Y_ENDSTOPS_INVERTING = Y_ENDSTOPS_INVERTING; //HIGH
        zyf_INVERT_Y_DIR = INVERT_Y_DIR;
        HOMEAXIS(Z);

    #else
        HOMEAXIS(Z);
    #endif
  }
  #endif

  if(code_seen(axis_codes[X_AXIS]))
  {
    if(code_value_long() != 0) {
      current_position[X_AXIS]=code_value()+add_homeing[0];
    }
  }

  if(code_seen(axis_codes[Y_AXIS])) {
    if(code_value_long() != 0) {
      current_position[Y_AXIS]=code_value()+add_homeing[1];
    }
  }

  if(code_seen(axis_codes[Z_AXIS])) {
    if(code_value_long() != 0) {
      current_position[Z_AXIS]=code_value()+add_homeing[2];
    }
  }
  plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);

    #ifdef ZYF_DUAL_Z		//By zyf
        zyf_RUN_STATUS = 0;
    #endif
      
  #ifdef ENDSTOPS_ONLY_FOR_HOMING
    enable_endstops(false, -1);
  #endif

  feedrate = saved_feedrate;
  feedmultiply = saved_feedmultiply;
  previous_millis_cmd = millis();
  endstops_hit_on_purpose();
} //command_G28

void command_T(int T01=-1){
    
    if(T01==-1){
        tmp_extruder = code_value();
    }else{
        tmp_extruder = T01;
    }
    
    if(tmp_extruder >= EXTRUDERS) {
      SERIAL_ECHO_START;
      SERIAL_ECHO("T");
      SERIAL_ECHO(tmp_extruder);
      SERIAL_ECHOLN(MSG_INVALID_EXTRUDER);
    } else {
      boolean make_move = false;
      if(code_seen('F')) {
        make_move = true;
        next_feedrate = code_value();
        if(next_feedrate > 0.0) {
          feedrate = next_feedrate;
        }
      }
      #if EXTRUDERS > 1
      if(tmp_extruder != active_extruder) {
        // Save current position to return to after applying extruder offset
        //memcpy(destination, current_position, sizeof(destination));
        destination[X_AXIS] = current_position[X_AXIS];											//By zyf
        destination[Y_AXIS] = current_position[Y_AXIS];											//By zyf
        destination[Z_AXIS] = current_position[Z_AXIS];											//By zyf
        destination[E_AXIS] = current_position[E_AXIS];											//By zyf
      #ifdef DUAL_X_CARRIAGE

        //By zyf go home befor switch
        if(card.sdprinting != 1){ 
            enable_endstops(true, 0);
            HOMEAXIS(X);
            enable_endstops(false, 0);
        }

        if (Stopped == false && 
            (delayed_move_time != 0 || current_position[X_AXIS] != x_home_pos(active_extruder)))
        {
          // Park old head: 1) raise 2) move to park position 3) lower
          plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS] + TOOLCHANGE_PARK_ZLIFT,current_position[E_AXIS], max_feedrate[Z_AXIS], active_extruder);
          plan_buffer_line(x_home_pos(active_extruder), current_position[Y_AXIS], current_position[Z_AXIS] + TOOLCHANGE_PARK_ZLIFT,current_position[E_AXIS], max_feedrate[X_AXIS], active_extruder);
          plan_buffer_line(x_home_pos(active_extruder), current_position[Y_AXIS], current_position[Z_AXIS],current_position[E_AXIS],max_feedrate[Z_AXIS], active_extruder);					
          st_synchronize();
        }

        // apply Y & Z extruder offset (x offset is already used in determining home pos)
        #ifdef CONFIG_E2_OFFSET
        if(zyf_Y2_OFFSET < 0.0 || zyf_Y2_OFFSET > 10.0) zyf_Y2_OFFSET = 4.5;
        if(zyf_Z2_OFFSET < 0.0 || zyf_Z2_OFFSET > 4.0) zyf_Z2_OFFSET = 2.0;
        extruder_offset[Y_AXIS][1] = zyf_Y2_OFFSET - 5.0;			//By Zyf
        extruder_offset[Z_AXIS][0] = 0.0;			                //By Zyf
        extruder_offset[Z_AXIS][1] = 2.0 - zyf_Z2_OFFSET;			//By Zyf
        #else
        //extruder_offset[Y_AXIS][1] = EXTRUDER_OFFSET_Y[1];			//By Zyf
        #endif
        //SERIAL_PROTOCOLPGM("Y2 Offset: "); SERIAL_PROTOCOLLN(extruder_offset[Y_AXIS][1]);
        
        current_position[Y_AXIS] = current_position[Y_AXIS] -
                     extruder_offset[Y_AXIS][active_extruder] +
                     extruder_offset[Y_AXIS][tmp_extruder];

        current_position[Z_AXIS] = current_position[Z_AXIS] -
                     extruder_offset[Z_AXIS][active_extruder] +
                     extruder_offset[Z_AXIS][tmp_extruder];
                     
        active_extruder = tmp_extruder;

        // This function resets the max/min values - the current position may be overwritten below.
        axis_is_at_home(X_AXIS);

        if (dual_x_carriage_mode == DXC_FULL_CONTROL_MODE)
        {
          current_position[X_AXIS] = inactive_extruder_x_pos; 
          inactive_extruder_x_pos = destination[X_AXIS];
        }
        else if (dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE)
        {
          active_extruder_parked = (active_extruder == 0); // this triggers the second extruder to move into the duplication position
          if (active_extruder == 0 || active_extruder_parked)
            current_position[X_AXIS] = inactive_extruder_x_pos;
          else
            current_position[X_AXIS] = destination[X_AXIS] + duplicate_extruder_x_offset; 

          inactive_extruder_x_pos = destination[X_AXIS];
          extruder_carriage_mode = 1;
        }
        else if (dual_x_carriage_mode == DXC_AUTO_PARK_MODE) {
          // record raised toolhead position for use by unpark
          //memcpy(raised_parked_position, current_position, sizeof(raised_parked_position));            
          raised_parked_position[X_AXIS] = current_position[X_AXIS];														//By zyf
          raised_parked_position[Y_AXIS] = current_position[Y_AXIS];											//By zyf
          raised_parked_position[Z_AXIS] = current_position[Z_AXIS];														//By zyf
          raised_parked_position[E_AXIS] = current_position[E_AXIS];														//By zyf

          raised_parked_position[Z_AXIS] += TOOLCHANGE_UNPARK_ZLIFT;
          active_extruder_parked = true;
          delayed_move_time = 0;

          //By Zyf gohome after autopark;
          enable_endstops(true, 0);
          HOMEAXIS(X);
          enable_endstops(false, 0);
        }

      #else    
        // Offset extruder (only by XY)
        int i;
        for(i = 0; i < 2; i++) {
           current_position[i] = current_position[i] -
                                 extruder_offset[i][active_extruder] +
                                 extruder_offset[i][tmp_extruder];
        }
        // Set the new active extruder and position
        active_extruder = tmp_extruder;
      #endif //else DUAL_X_CARRIAGE
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
        // Move to the old position if 'F' was in the parameters
        if(make_move && Stopped == false) {
           prepare_move();
        }
      }
      #endif
      SERIAL_ECHO_START;
      SERIAL_ECHO(MSG_ACTIVE_EXTRUDER);
      SERIAL_PROTOCOLLN((int)active_extruder);
    }
} //T


void PrintStopOrFinished()
{
	#ifdef PRINT_FROM_Z_HEIGHT
	PrintFromZLevelFound = true;
	print_from_z_target = 0.0;
	#endif
    //raised_parked_position[X_AXIS] = current_position[X_AXIS];														//By zyf
    raised_parked_position[Y_AXIS] = 0;														//By zyf
    //raised_parked_position[Z_AXIS] = current_position[Z_AXIS];														//By zyf
    //raised_parked_position[E_AXIS] = current_position[E_AXIS];														//By zyf
    #ifdef POWER_RAIL_RECV
    EEPROM_Write_PLR();
	EEPROM_PRE_Write_PLR();
    #endif
    if (dual_x_carriage_mode == DXC_AUTO_PARK_MODE)
        command_T(0);
}

#ifdef FILAMENT_FAIL_DETECT
bool bFilaFail = false;
void check_filament_fail(){
	bool bRead = digitalRead(FILAMENT_FAIL_DETECT_PIN) == FILAMENT_FAIL_DETECT_TRIGGER;
    if(bRead && !bFilaFail){
		if(card.sdprinting == 1){ 			
			bFilaFail = true;
			#ifdef TENLOG_CONTROLLER
			TenlogScreen_println("sleep=0");							
			TenlogScreen_println("msgbox.vaFromPageID.val=15");
			TenlogScreen_println("msgbox.vaToPageID.val=15");
			TenlogScreen_println("msgbox.vtOKValue.txt=\"\"");
			TenlogScreen_println("msgbox.vtCancelValue.txt=\"\"");
			TenlogScreen_println("msgbox.vtStartValue.txt=\"M1031 O1\"");			
			String strMessage="";			
			if(languageID==0)
				strMessage="Filament runout!";
			else
				strMessage="耗材用尽！";
			strMessage = "msgbox.tMessage.txt=\"" + strMessage + "\"";
			const char* str0 = strMessage.c_str();
			TenlogScreen_println(str0);
			TenlogScreen_println("page msgbox");			
			#endif
		}
	}
}
#endif

void command_G1(float XValue,float YValue,float ZValue,float EValue){
    if(Stopped == false) {
        //BOF By zyf
        if (dual_x_carriage_mode == DXC_AUTO_PARK_MODE){
            float fXMin = X_MIN_POS;
            float fXMax = zyf_X2_MAX_POS;
            if(active_extruder == 0) fXMax = zyf_X2_MAX_POS - X_NOZZLE_WIDTH;
            if(active_extruder == 1) fXMin = X_MIN_POS + X_NOZZLE_WIDTH;

            if(code_seen(axis_codes[X_AXIS])){
                float fXV = code_value();
                float fRate = 1.0;
                if(code_seen('R')) fRate = code_value();
                fXV = fXV / fRate;
                int iMode=0;
                if(code_seen('M')) {
                    iMode=(int)code_value();
                }

                if((fXV > fXMax || fXV < fXMin) && iMode==0){ 
                    #ifdef TENLOG_CONTROLLER
                    TenlogScreen_print("main.sStatus.txt=\"");
                    long lN = current_position[X_AXIS]*10.0; //1
                    String sSend = String(lN);
					const char* str0 = sSend.c_str();
                    TenlogScreen_print(str0);
                    TenlogScreen_print("|");
                    TenlogScreen_print("\"");
                    TenlogScreen_printend();
                    delay(50);
                    TenlogScreen_println("click btReflush,0");
                    #endif
                    return; 
                }
            }				
        }
        //Eof By zyf
		#ifdef FILAMENT_FAIL_DETECT
		if(code_seen('E') || EValue > -99999.0){
			check_filament_fail();
		}
		#endif
        get_coordinates(XValue,YValue,ZValue,EValue); // For X Y Z E F
        prepare_move();
        //ClearToSend();

        return;
    }
}

void command_M190(int SValue=-1){
    #if defined(TEMP_BED_PIN) && TEMP_BED_PIN > -1
    unsigned long codenum; //throw away variable
    LCD_MESSAGEPGM(MSG_BED_HEATING);
    
    if (code_seen('S')) { 
      setTargetBed(code_value());
      CooldownNoWait = true;
    } else if (code_seen('R')) {
      setTargetBed(code_value());
      CooldownNoWait = false;
    } else if(SValue > -1){
      setTargetBed(SValue);
      CooldownNoWait = true;        
    }
    codenum = millis();
    
    target_direction = isHeatingBed(); // true if heating, false if cooling
    card.heating = true;
    while ( target_direction ? (isHeatingBed()) : (isCoolingBed()&&(CooldownNoWait==false)) && card.isFileOpen())
    {
      if(( millis() - codenum) > 1000 ) //Print Temp Reading every 1 second while heating up.
      {
        float tt=degHotend(active_extruder);
        SERIAL_PROTOCOLPGM("T:");
        SERIAL_PROTOCOL(tt);
        SERIAL_PROTOCOLPGM(" E:");
        SERIAL_PROTOCOL((int)active_extruder);
        SERIAL_PROTOCOLPGM(" B:");
        SERIAL_PROTOCOL_F(degBed(),1);
        SERIAL_PROTOCOLLN("");
        codenum = millis();
      
        #ifdef TENLOG_CONTROLLER
        String strSerial2 = getSerial2Data();
        if(strSerial2 != "")
        {
            strSerial2.replace("\r","");
            strSerial2.replace("\n","");
            String strM104 = getSplitValue(strSerial2,' ',0);
            if(strM104 == "M140")
            {
                String strTemp = getSplitValue(strSerial2,' ',1);
                int iTempE;
                if(strTemp.substring(0,1) == "S")
                    iTempE = strTemp.substring(1, strTemp.length()).toInt();
                setTargetBed(iTempE);
            }else if(strSerial2 == "M1033"){
                sdcard_stop();
            }else if(strSerial2 == "M1031"){
                sdcard_pause();
            }else if(strSerial2 == "M1031 O1"){
            }else if(strSerial2.substring(0,5) == "M1032"){
                sdcard_resume();
            }
        }
        #endif
      }
      manage_heater();
      if(zyf_HEATER_FAIL){
        card.closefile();
        card.sdprinting = 0;
      }else{
      }
      manage_inactivity();
      lcd_update();
    }
    card.heating = false;
    LCD_MESSAGEPGM(MSG_BED_DONE);
    previous_millis_cmd = millis();
    #endif
}

void command_M104(int iT=-1, int iS=-1){
    if(setTargetedHotend(104)){
        return;
    }
    int iTempE;
    iTempE = tmp_extruder;
	int iSV = -1;
#ifdef TENLOG_CONTROLLER
    if (code_seen('T')) iTempE = code_value();
	if(iT > -1) iTempE = iT;
#endif
    if (code_seen('S')) iSV = code_value();
	if(iS > -1) iSV = iS;
	setTargetHotend(iSV, iTempE);
	
#ifdef DUAL_X_CARRIAGE
    if ((dual_x_carriage_mode == DXC_DUPLICATION_MODE ||dual_x_carriage_mode == DXC_MIRROR_MODE) && tmp_extruder == 0)
        setTargetHotend1(iSV == 0.0 ? 0.0 : iSV + duplicate_extruder_temp_offset);
#endif          
    setWatch();
}

void command_M109(int SValue=-1){    // M109 - Wait for extruder heater to reach target.
    unsigned long codenum; //throw away variable
    if(setTargetedHotend(109)){
        return;
    }
    if(card.sdprinting == 1){   //
    }
    else{
        LCD_MESSAGEPGM(MSG_HEATING);
    }
    #ifdef AUTOTEMP
    autotemp_enabled=false;
    #endif
    if (code_seen('S')) { 
        setTargetHotend(code_value(), tmp_extruder);
        #ifdef DUAL_X_CARRIAGE
        if ((dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE) && tmp_extruder == 0)
            setTargetHotend1(code_value() == 0.0 ? 0.0 : code_value() + duplicate_extruder_temp_offset);
        #endif          
        CooldownNoWait = true;
    } else if (code_seen('R')) {
        setTargetHotend(code_value(), tmp_extruder);
        #ifdef DUAL_X_CARRIAGE
        if ((dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE) && tmp_extruder == 0)
            setTargetHotend1(code_value() == 0.0 ? 0.0 : code_value() + duplicate_extruder_temp_offset);
        #endif          
        CooldownNoWait = false;
    }

    if(SValue > -1){
        setTargetHotend(SValue, tmp_extruder);
        #ifdef DUAL_X_CARRIAGE
        if ((dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE) && tmp_extruder == 0)
            setTargetHotend1(SValue == 0.0 ? 0.0 : SValue + duplicate_extruder_temp_offset);
        #endif
        CooldownNoWait = true;        
    }

    #ifdef AUTOTEMP
    if (code_seen('S')) autotemp_min=code_value();
    if (SValue > -1) autotemp_min=SValue;
    if (code_seen('B')) autotemp_max=code_value();
    if (code_seen('F'))
    {
        autotemp_factor=code_value();
        autotemp_enabled=true;
    }
    #endif

    setWatch();
    codenum = millis();

    /* See if we are heating up or cooling down */
    target_direction = isHeatingHotend(tmp_extruder); // true if heating, false if cooling
    //By Zyf 
	if(target_direction)
	    card.heating = true;

    #ifdef TEMP_RESIDENCY_TIME
    long residencyStart;
    residencyStart = -1;
      /* continue to loop until we have reached the target temp
       _and_ until TEMP_RESIDENCY_TIME hasn't passed since we reached it */

    while((residencyStart == -1) ||
            (residencyStart >= 0 && (((unsigned int) (millis() - residencyStart)) < (TEMP_RESIDENCY_TIME * 1000UL)))  && card.isFileOpen()) {
    #else
    while ( target_direction ? (isHeatingHotend(tmp_extruder)) : (isCoolingHotend(tmp_extruder)&&(CooldownNoWait==false)) && card.isFileOpen()) {
    #endif //TEMP_RESIDENCY_TIME
        if( (millis() - codenum) > 1000UL )          { //Print Temp Reading and remaining time every 1 second while heating up/cooling down
		#ifndef ZYF_DEBUG
            SERIAL_PROTOCOLPGM("T:");
            SERIAL_PROTOCOL_F(degHotend(tmp_extruder),1);
            SERIAL_PROTOCOLPGM(" E:");
            SERIAL_PROTOCOL((int)tmp_extruder);
            #ifdef TEMP_RESIDENCY_TIME
              SERIAL_PROTOCOLPGM(" W:");
              if(residencyStart > -1)
              {
                 codenum = ((TEMP_RESIDENCY_TIME * 1000UL) - (millis() - residencyStart)) / 1000UL;
                 SERIAL_PROTOCOLLN( codenum );
              }
              else
              {
                 SERIAL_PROTOCOLLN( "?" );
              }
            #else
              SERIAL_PROTOCOLLN("");
            #endif
		#endif
            codenum = millis();
            #ifdef TENLOG_CONTROLLER
            String strSerial2 = getSerial2Data();
            if(strSerial2 != "")
            {
                strSerial2.replace("\r","");
                strSerial2.replace("\n","");
                String strM104 = getSplitValue(strSerial2,' ',0);
                if(strM104 == "M104")
                {
                    String strT01 = getSplitValue(strSerial2,' ',1);
                    String strTemp = getSplitValue(strSerial2,' ',2);
                    int iTempE;
                    int iTemp = 0;
                    if(strT01 == "T0") iTemp = 0; else iTemp = 1;
                    if(strTemp.substring(0,1) == "S")
                        iTempE = strTemp.substring(1, strTemp.length()).toInt();
                    setTargetHotend(iTempE, iTemp);
                    
                    #ifdef DUAL_X_CARRIAGE
                    if ((dual_x_carriage_mode == DXC_DUPLICATION_MODE ||dual_x_carriage_mode == DXC_MIRROR_MODE) && tmp_extruder == 0)
                        setTargetHotend1(iTempE == 0.0 ? 0.0 : iTempE + duplicate_extruder_temp_offset);
                    if ((dual_x_carriage_mode == DXC_DUPLICATION_MODE ||dual_x_carriage_mode == DXC_MIRROR_MODE) && tmp_extruder == 1)
                        setTargetHotend(iTempE == 0.0 ? 0.0 : iTempE - duplicate_extruder_temp_offset, 0);
                    #endif         
                }else if(strSerial2 == "M1033"){
                    sdcard_stop();                    
                }else if(strSerial2 == "M1031"){
                    sdcard_pause();
                }else if(strSerial2 == "M1031 O1"){
                    //sdcard_pause(1);
                }else if(strSerial2.substring(0,5) == "M1032"){
                    sdcard_resume();
                }
            }
            #endif
        }
        manage_heater();
        if(zyf_HEATER_FAIL){
            card.closefile();
            card.sdprinting = 0;
            manage_inactivity();
            lcd_update();
            return;
        }
        manage_inactivity();
        lcd_update();
        #ifdef TEMP_RESIDENCY_TIME
            /* start/restart the TEMP_RESIDENCY_TIME timer whenever we reach target temp for the first time
              or when current temp falls outside the hysteresis after target temp was reached */
        if ((residencyStart == -1 &&  target_direction && (
               degHotend(tmp_extruder) >= (degTargetHotend(tmp_extruder)-TEMP_WINDOW)
               &&
               (
                    dual_x_carriage_mode == DXC_AUTO_PARK_MODE 
                    ||
                    (
                        (dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE) 
                        && tmp_extruder == 0
                        && degHotend(1) >= (degTargetHotend(0)-TEMP_WINDOW)
                    )
                )
           )) ||
           (residencyStart == -1 && !target_direction && (
               degHotend(tmp_extruder) <= (degTargetHotend(tmp_extruder)+TEMP_WINDOW)
               &&
                (
                    dual_x_carriage_mode == DXC_AUTO_PARK_MODE 
                    ||
                    (
                        (dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE) 
                        && tmp_extruder == 0
                        && degHotend(1) <= (degTargetHotend(0)+TEMP_WINDOW)
                    )
                )

           )) ||
           (residencyStart > -1 && labs(degHotend(tmp_extruder) - degTargetHotend(tmp_extruder)) > TEMP_HYSTERESIS) )
        {
            residencyStart = millis();
        }
        #endif //TEMP_RESIDENCY_TIME
        }//while
            
		card.heating = false;
        if(card.sdprinting != 1){
			LCD_MESSAGEPGM(MSG_HEATING_COMPLETE);
        }
		//starttime=millis();													//By Zyf	No need
        previous_millis_cmd = millis();					

}

void command_G92(float XValue=-99999.0, float YValue=-99999.0, float ZValue=-99999.0, float EValue=-99999.0)    //By Zyf
{
    if(!code_seen(axis_codes[E_AXIS]) || EValue > -99999.0)
        st_synchronize();

    for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) {
            if(i == E_AXIS) {
                current_position[i] = code_value();
                plan_set_e_position(current_position[E_AXIS]);
            }
            else {
                current_position[i] = code_value()+add_homeing[i];
                plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
            }
        }
    }

    if(EValue > -99999.0){
        current_position[E_AXIS] = EValue;
        plan_set_e_position(current_position[E_AXIS]);
    }

    if(XValue > -99999.0){
        current_position[X_AXIS] = XValue+add_homeing[X_AXIS];
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);        
    }

    if(YValue > -99999.0){
        current_position[Y_AXIS] = YValue+add_homeing[Y_AXIS];
        plan_set_position(current_position[Y_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);        
    }

    if(ZValue > -99999.0){
        current_position[Z_AXIS] = ZValue+add_homeing[Z_AXIS];
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);        
    }
}

#ifdef POWER_LOSS_RECOVERY
void command_M1003(){
    bool bOK = false;
    
	String sPLR = card.get_PLR();
    String sFileName = getSplitValue(sPLR,'|',0);

    uint32_t lFPos = 0;
    int iTemp0 = 0;
    int iTemp1 = 0;
    int iFan = 0;
    int iT01 = 0;
    int iTempBed = 0;
    float fZ = 0.0;
    float fE = 0.0;
    float fX = 0.0;
    float fY = 0.0; 
    int iMode = 0;
    float fXOffSet = 0.0;

    if(sPLR != ""){
                
        lFPos = atol(const_cast<char*>(getSplitValue(sPLR, '|', 1).c_str()));
        iTemp0 = getSplitValue(sPLR, '|', 2).toInt();
        iTemp1 = getSplitValue(sPLR, '|', 3).toInt();
        iT01 = getSplitValue(sPLR, '|', 4).toInt();
        
        fZ = string2Float(getSplitValue(sPLR, '|', 5));
        fE = string2Float(getSplitValue(sPLR, '|', 6));
        iFan = getSplitValue(sPLR, '|', 7).toInt();
        fX = string2Float(getSplitValue(sPLR, '|', 8));
        fY = string2Float(getSplitValue(sPLR, '|', 9));
                    
        iTempBed = getSplitValue(sPLR, '|', 10).toInt();
        iMode = getSplitValue(sPLR, '|', 11).toInt();
        fXOffSet = string2Float(getSplitValue(sPLR, '|', 12));
        
        if(lFPos > 2048 ){
            bOK = true;
        }        
    }

	#ifdef TENLOG_CONTROLLER
    if(!bOK){
        TenlogScreen_println("page main");            
    }else{
       
        char *fName = const_cast<char*>(sFileName.c_str());
        card.openFile(fName,true,lFPos);
		
		sFileName = "printing.tFileName.txt=\"" + sFileName + "\"";
		const char * str0 = sFileName.c_str();
        TenlogScreen_println(str0);
        TenlogScreen_println("page printing");
        feedrate = 4000;

        card.sdprinting = 2;

        if(iTempBed > 0){
            command_M190(iTempBed);
        }

        #ifdef DUAL_X_CARRIAGE
		
        dual_x_carriage_mode = iMode;
        if(iTemp0 > 0 ){
            if(dual_x_carriage_mode == DXC_DUPLICATION_MODE || dual_x_carriage_mode == DXC_MIRROR_MODE){
                tmp_extruder = 0;
                command_M109(iTemp0);                
            }else if(dual_x_carriage_mode == DXC_AUTO_PARK_MODE){
                setTargetHotend(iTemp0, 0);
            }
        }

        if(iTemp1 > 0 ){
            if(dual_x_carriage_mode == DXC_AUTO_PARK_MODE){
                setTargetHotend(iTemp0, 1);
            }
        }
        
        if(dual_x_carriage_mode == DXC_AUTO_PARK_MODE){
            if(iTemp0 > 0 && iT01 == 0){
                active_extruder = 0;
                command_M109(iTemp0);
            }
            if(iTemp1 > 0 && iT01 == 1){
                active_extruder = 1;
                command_M109(iTemp1);
            }
	        ZYF_DEBUG_PRINT("T");
	        ZYF_DEBUG_PRINT_LN(iT01);
        }

        if(dual_x_carriage_mode == DXC_AUTO_PARK_MODE){
            if(iT01 == 0){
                command_T(0);
            }else{
                command_T(1);
            }
        }
        axis_is_at_home(X_AXIS);

        if (dual_x_carriage_mode == DXC_DUPLICATION_MODE)
        {
            duplicate_extruder_x_offset = fXOffSet;
        }

		if(iT01 == 1 && fX<1) fX = zyf_X2_MAX_POS - 60;

        #else

        if(iTemp0>0){
            command_M109(iTemp0);
        }        
        #endif

        //fZ = fZ + 1.0;
        fanSpeed = iFan;
		command_G92(0.0, 0.0, fZ, fE);
        command_G1(0, 0, fZ+15.0, fE);
        command_G28(1,1,0);
		
		#if defined(POWER_LOSS_SAVE_TO_EEPROM)
		EEPROM_Write_PLR();
		EEPROM_PRE_Write_PLR();
		#elif defined(POWER_LOSS_SAVE_TO_SDCARD)
		card.Write_PLR();
		card.PRE_Write_PLR();
		#endif
        //fZ = fZ - 1.0;
        command_G1(fX,fY,fZ,fE);        
        
        feedrate = string2Float(getSplitValue(sPLR, '|', 13));
		if(feedrate < 2000) feedrate = 4000;
        card.sdprinting = 0;

        card.startFileprint();
        starttime=millis();
    }
	#endif //TENLOG_CONTROLLER
}
#endif

void process_commands()
{
    if(zyf_HEATER_FAIL){
        card.closefile();
        card.sdprinting = 0;
        zyf_HEATER_FAIL = false;
    }

  unsigned long codenum; //throw away variable
  char *starpos = NULL;

  if(code_seen('G'))
  {
    switch((int)code_value())
    {
    case 0: // G0 -> G1
    case 1: // G1
      command_G1();
      break;
    case 2: // G2  - CW ARC
      if(Stopped == false) {
        get_arc_coordinates();
        prepare_arc_move(true);
        return;
      }
    case 3: // G3  - CCW ARC
      if(Stopped == false) {
        get_arc_coordinates();
        prepare_arc_move(false);
        return;
      }
    case 4: // G4 dwell
      command_G4();
      break;
      #ifdef FWRETRACT
      case 10: // G10 retract
      if(!retracted)
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS];
        current_position[Z_AXIS]+=-retract_zlift;
        destination[E_AXIS]=current_position[E_AXIS]-retract_length;
        feedrate=retract_feedrate;
        retracted=true;
        prepare_move();
      }

      break;
      case 11: // G10 retract_recover
      if(!retracted)
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS];

        current_position[Z_AXIS]+=retract_zlift;
        current_position[E_AXIS]+=-retract_recover_length;
        feedrate=retract_recover_feedrate;
        retracted=false;
        prepare_move();
      }
      break;
      #endif //FWRETRACT
    case 28: //G28 Home all Axis one at a time
      command_G28();
      break;
    case 90: // G90
      relative_mode = false;
      break;
    case 91: // G91
      relative_mode = true;
      break;
    case 92: // G92
      command_G92();
      break;
    }//Case G Number
  }//Goce Seen G

  else if(code_seen('M'))
  {
    switch( (int)code_value() )
    {
    case 0: // M0 - Unconditional stop - Wait for user button press on LCD
    case 1: // M1 - Conditional stop - Wait for user button press on LCD
    {
	  /*
      LCD_MESSAGEPGM(MSG_USERWAIT);
      codenum = 0;
      if(code_seen('P')) codenum = code_value(); // milliseconds to wait
      if(code_seen('S')) codenum = code_value() * 1000; // seconds to wait

      st_synchronize();
      previous_millis_cmd = millis();
      if (codenum > 0){
        codenum += millis();  // keep track of when we started waiting
        while(millis()  < codenum && true){
          manage_heater();
          if(zyf_HEATER_FAIL){
              card.closefile();
              card.sdprinting = 0;
          }
          manage_inactivity();
          lcd_update();
        }
      }else{
        while(true){
          manage_heater();
          if(zyf_HEATER_FAIL){ 
              card.closefile();
              card.sdprinting = 0;
          }
          manage_inactivity();
          lcd_update();
        }
      }
      LCD_MESSAGEPGM(MSG_RESUMING);
     */
      sdcard_pause(1);
	}
    break;
    case 17:
        LCD_MESSAGEPGM(MSG_NO_MOVE);
        enable_x();
        enable_y();
        enable_z();
        enable_e0();
        enable_e1();
        enable_e2();
      break;

#ifdef SDSUPPORT
    #ifdef TENLOG_CONTROLLER
    case 19: //M19 tlController list sd file
    {
        if(code_seen('S')) i_print_page_id = code_value();
        sdcard_tlcontroller();
    }
    break;
    #endif
    case 20: // M20 - list SD card
    {
        SERIAL_PROTOCOLLNPGM(MSG_BEGIN_FILE_LIST);
        card.ls();
        SERIAL_PROTOCOLLNPGM(MSG_END_FILE_LIST);
    }
    break;
    case 21: // M21 - init SD card

      card.initsd();

      break;
    case 22: //M22 - release SD card
      card.release();
      break;
    case 23: //M23 - Select file
      starpos = (strchr(strchr_pointer + 4,'*'));
      if(starpos!=NULL)
        *(starpos-1)='\0';
      card.openFile(strchr_pointer + 4,true);
      break;
    case 24: //M24 - Start SD print
      card.startFileprint();
      starttime=millis();
      break;
    case 25: //M25 - Pause SD print
	  sdcard_pause(1);
      break;
    case 26: //M26 - Set SD index
      if(card.cardOK && code_seen('S')) {
        card.setIndex(code_value_long());
      }
      break;
    case 27: //M27 - Get SD status
      card.getStatus();
      break;
    case 28: //M28 - Start SD write
      starpos = (strchr(strchr_pointer + 4,'*'));
      if(starpos != NULL){
        char* npos = strchr(cmdbuffer[bufindr], 'N');
        strchr_pointer = strchr(npos,' ') + 1;
        *(starpos-1) = '\0';
      }
      card.openFile(strchr_pointer+4,false);
      break;
    case 29: //M29 - Stop SD write
      //processed in write to file routine above
      //card,saving = false;
      break;
    case 30: //M30 <filename> Delete File
      if (card.cardOK){
        card.closefile();
        starpos = (strchr(strchr_pointer + 4,'*'));
        if(starpos != NULL){
          char* npos = strchr(cmdbuffer[bufindr], 'N');
          strchr_pointer = strchr(npos,' ') + 1;
          *(starpos-1) = '\0';
        }
        card.removeFile(strchr_pointer + 4);
      }
      break;
    case 32: //M32 - Select file and start SD print
      if(card.sdprinting == 1) { 
        st_synchronize();
        card.closefile();
      }
      starpos = (strchr(strchr_pointer + 4,'*'));
      if(starpos!=NULL)
        *(starpos-1)='\0';
      card.openFile(strchr_pointer + 4,true);
      card.startFileprint();
      starttime=millis();
      break;
    case 928: //M928 - Start SD write
      starpos = (strchr(strchr_pointer + 5,'*'));
      if(starpos != NULL){
        char* npos = strchr(cmdbuffer[bufindr], 'N');
        strchr_pointer = strchr(npos,' ') + 1;
        *(starpos-1) = '\0';
      }
      card.openLogFile(strchr_pointer+5);
      break;

	case 226:
	{
		sdcard_pause(1);
	}
	break;

#endif //SDSUPPORT

    case 31: //M31 take time since the start of the SD print or an M109 command
      {
      stoptime=millis();
      char time[30];
      unsigned long t=(stoptime-starttime)/1000;
      int sec,min;
      min=t/60;
      sec=t%60;
      sprintf_P(time, PSTR("%i min, %i sec"), min, sec);
      SERIAL_ECHO_START;
      SERIAL_ECHOLN(time);
      lcd_setstatus(time);
      autotempShutdown();
      }
      break;
    case 42: //M42 -Change pin status via gcode
      if (code_seen('S'))
      {
        int pin_status = code_value();
        int pin_number = LED_PIN;
        if (code_seen('P') && pin_status >= 0 && pin_status <= 255)
          pin_number = code_value();
        for(int8_t i = 0; i < (int8_t)sizeof(sensitive_pins); i++)
        {
          if (sensitive_pins[i] == pin_number)
          {
            pin_number = -1;
            break;
          }
        }
      #if defined(FAN_PIN) && FAN_PIN > -1
        if (pin_number == FAN_PIN)
          fanSpeed = pin_status;
      #endif
        if (pin_number > -1)
        {
          pinMode(pin_number, OUTPUT);
          digitalWrite(pin_number, pin_status);
          analogWrite(pin_number, pin_status);
        }
      }
     break;
    case 104: // M104 set extruder temp
    {
		command_M104();
    }
      break;
    case 140: // M140 set bed temp
      if (code_seen('S')) setTargetBed(code_value());
      break;
    case 105 : // M105
      if(setTargetedHotend(105)){
        break;
        }
      #if defined(TEMP_0_PIN) && TEMP_0_PIN > -1
        SERIAL_PROTOCOLPGM("ok T:");
        SERIAL_PROTOCOL_F(degHotend(tmp_extruder),1);
        SERIAL_PROTOCOLPGM(" /");
        SERIAL_PROTOCOL_F(degTargetHotend(tmp_extruder),1);
        #if defined(TEMP_BED_PIN) && TEMP_BED_PIN > -1
          SERIAL_PROTOCOLPGM(" B:");
          SERIAL_PROTOCOL_F(degBed(),1);
          SERIAL_PROTOCOLPGM(" /");
          SERIAL_PROTOCOL_F(degTargetBed(),1);
        #endif //TEMP_BED_PIN
      #else
        SERIAL_ERROR_START;
        SERIAL_ERRORLNPGM(MSG_ERR_NO_THERMISTORS);
      #endif

        SERIAL_PROTOCOLPGM(" @:");
        SERIAL_PROTOCOL(getHeaterPower(tmp_extruder));

        SERIAL_PROTOCOLPGM(" B@:");
        SERIAL_PROTOCOL(getHeaterPower(-1));

        SERIAL_PROTOCOLLN("");
      return;
      break;
    case 109:
      command_M109();
      break;
    case 190: // M190 - Wait for bed heater to reach target.
      command_M190();
      break;
    
    #if defined(FAN_PIN) && FAN_PIN > -1
      case 106: //M106 Fan On
        if (code_seen('S')){
           fanSpeed=constrain(code_value(),0,255);
        }
        else {
          fanSpeed=255;
        }
        break;
      case 107: //M107 Fan Off
        fanSpeed = 0;
        break;
    #endif //FAN_PIN
    #ifdef BARICUDA
      // PWM for HEATER_1_PIN
      #if defined(HEATER_1_PIN) && HEATER_1_PIN > -1
        case 126: //M126 valve open
          if (code_seen('S')){
             ValvePressure=constrain(code_value(),0,255);
          }
          else {
            ValvePressure=255;
          }
          break;
        case 127: //M127 valve closed
          ValvePressure = 0;
          break;
      #endif //HEATER_1_PIN

      // PWM for HEATER_2_PIN
      #if defined(HEATER_2_PIN) && HEATER_2_PIN > -1
        case 128: //M128 valve open
          if (code_seen('S')){
             EtoPPressure=constrain(code_value(),0,255);
          }
          else {
            EtoPPressure=255;
          }
          break;
        case 129: //M129 valve closed
          EtoPPressure = 0;
          break;
      #endif //HEATER_2_PIN
    #endif

      #if defined(PS_ON_PIN) && PS_ON_PIN > -1
      case 80: // M80 - ATX Power On
        SET_OUTPUT(PS_ON_PIN); //GND
        WRITE(PS_ON_PIN, PS_ON_AWAKE);
        break;
      #endif

      case 81: // M81 - ATX Power Off
        command_M81();
        break;

    case 82:
      axis_relative_modes[3] = false;
      break;
    case 83:
      axis_relative_modes[3] = true;
      break;
    case 18: //compatibility
    case 84: // M84
      if(code_seen('S')){
        stepper_inactive_time = code_value() * 1000;
      }
      else
      {
        bool all_axis = !((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])) || (code_seen(axis_codes[2]))|| (code_seen(axis_codes[3])));
        if(all_axis)
        {
          st_synchronize();
          disable_e0();
          disable_e1();
          disable_e2();
          finishAndDisableSteppers(true);
        }
        else
        {
          st_synchronize();
          if(code_seen('X')) disable_x();
          if(code_seen('Y')) disable_y();
          if(code_seen('Z')) disable_z();
          #if ((E0_ENABLE_PIN != X_ENABLE_PIN) && (E1_ENABLE_PIN != Y_ENABLE_PIN)) // Only enable on boards that have seperate ENABLE_PINS
            if(code_seen('E')) {
              disable_e0();
              disable_e1();
              disable_e2();
            }
          #endif
        }
      }
      break;
    case 85: // M85
      code_seen('S');
      max_inactive_time = code_value() * 1000;
      break;
    case 92: // M92
    {
      float fRate=1.0;
      #ifdef TENLOG_CONTROLLER
        if(code_seen('R')) {
          fRate=(float)code_value();
        }
      #endif
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        if(code_seen(axis_codes[i]))
        {
          if(i == 3) { // E
            float value = code_value()/fRate;
            if(value < 20.0) {
              float factor = axis_steps_per_unit[i] / value; // increase e constants if M92 E14 is given for netfab.
              max_e_jerk *= factor;
              max_feedrate[i] *= factor;
              axis_steps_per_sqr_second[i] *= factor;
            }
            axis_steps_per_unit[i] = value;
          }
          else {
            axis_steps_per_unit[i] = code_value()/fRate;
          }
        }
      }
      Config_StoreSettings();//By zyf
    }
      break;
    case 115: // M115
      SERIAL_PROTOCOLPGM(MSG_M115_REPORT);
      break;
    case 117: // M117 display message
      starpos = (strchr(strchr_pointer + 5,'*'));
      if(starpos!=NULL)
        *(starpos-1)='\0';
      lcd_setstatus(strchr_pointer + 5);
      break;
    case 114: // M114
      SERIAL_PROTOCOLPGM("X:");
      SERIAL_PROTOCOL(current_position[X_AXIS]);
      SERIAL_PROTOCOLPGM("Y:");
      SERIAL_PROTOCOL(current_position[Y_AXIS]);
      SERIAL_PROTOCOLPGM("Z:");
      SERIAL_PROTOCOL(current_position[Z_AXIS]);
      SERIAL_PROTOCOLPGM("E:");
      SERIAL_PROTOCOL(current_position[E_AXIS]);

      SERIAL_PROTOCOLPGM(MSG_COUNT_X);
      SERIAL_PROTOCOL(float(st_get_position(X_AXIS))/axis_steps_per_unit[X_AXIS]);
      SERIAL_PROTOCOLPGM("Y:");
      SERIAL_PROTOCOL(float(st_get_position(Y_AXIS))/axis_steps_per_unit[Y_AXIS]);
      SERIAL_PROTOCOLPGM("Z:");
      SERIAL_PROTOCOL(float(st_get_position(Z_AXIS))/axis_steps_per_unit[Z_AXIS]);

      SERIAL_PROTOCOLLN("");
      break;
    case 120: // M120
      enable_endstops(false, -1) ;
      break;
    case 121: // M121
      enable_endstops(true, -1) ;
      break;
    case 119: // M119
    SERIAL_PROTOCOLLN(MSG_M119_REPORT);
      #if defined(X_MIN_PIN) && X_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_X_MIN);
        SERIAL_PROTOCOLLN(((READ(X_MIN_PIN)^X_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(X_MAX_PIN) && X_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_X_MAX);
        SERIAL_PROTOCOLLN(((READ(X_MAX_PIN)^X_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Y_MIN_PIN) && Y_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Y_MIN);
        SERIAL_PROTOCOLLN(((READ(Y_MIN_PIN)^Y_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Y_MAX_PIN) && Y_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Y_MAX);
        SERIAL_PROTOCOLLN(((READ(Y_MAX_PIN)^Y_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Z_MIN_PIN) && Z_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Z_MIN);
        SERIAL_PROTOCOLLN(((READ(Z_MIN_PIN)^Z_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Z_MAX_PIN) && Z_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Z_MAX);
        SERIAL_PROTOCOLLN(((READ(Z_MAX_PIN)^Z_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      break;
      //TODO: update for all axis, use for loop
    case 201: // M201
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        if(code_seen(axis_codes[i]))
        {
          max_acceleration_units_per_sq_second[i] = code_value();
        }
      }
      // steps per sq second need to be updated to agree with the units per sq second (as they are what is used in the planner)
      reset_acceleration_rates();
      break;
    #if 0 // Not used for Sprinter/grbl gen6
    case 202: // M202
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) axis_travel_steps_per_sqr_second[i] = code_value() * axis_steps_per_unit[i];
      }
      break;
    #endif
    case 203: // M203 max feedrate mm/sec
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) max_feedrate[i] = code_value();
      }
      break;
    case 204: // M204 acclereration S normal moves T filmanent only moves
      {
        if(code_seen('S')) acceleration = code_value() ;
        if(code_seen('T')) retract_acceleration = code_value() ;
      }
      break;
    case 205: //M205 advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk
    {
      if(code_seen('S')) minimumfeedrate = code_value();
      if(code_seen('T')) mintravelfeedrate = code_value();
      if(code_seen('B')) minsegmenttime = code_value() ;
      if(code_seen('X')) max_xy_jerk = code_value() ;
      if(code_seen('Z')) max_z_jerk = code_value() ;
      if(code_seen('E')) max_e_jerk = code_value() ;
    }
    break;
    case 206: // M206 additional homeing offset
      for(int8_t i=0; i < 3; i++)
      {
        if(code_seen(axis_codes[i])) add_homeing[i] = code_value();
      }
      break;
    #ifdef FWRETRACT
    case 207: //M207 - set retract length S[positive mm] F[feedrate mm/sec] Z[additional zlift/hop]
    {
      if(code_seen('S'))
      {
        retract_length = code_value() ;
      }
      if(code_seen('F'))
      {
        retract_feedrate = code_value() ;
      }
      if(code_seen('Z'))
      {
        retract_zlift = code_value() ;
      }
    }break;
    case 208: // M208 - set retract recover length S[positive mm surplus to the M207 S*] F[feedrate mm/sec]
    {
      if(code_seen('S'))
      {
        retract_recover_length = code_value() ;
      }
      if(code_seen('F'))
      {
        retract_recover_feedrate = code_value() ;
      }
    }break;
    case 209: // M209 - S<1=true/0=false> enable automatic retract detect if the slicer did not support G10/11: every normal extrude-only move will be classified as retract depending on the direction.
    {
      if(code_seen('S'))
      {
        int t= code_value() ;
        switch(t)
        {
          case 0: autoretract_enabled=false;retracted=false;break;
          case 1: autoretract_enabled=true;retracted=false;break;
          default:
            SERIAL_ECHO_START;
            SERIAL_ECHOPGM(MSG_UNKNOWN_COMMAND);
            SERIAL_ECHO(cmdbuffer[bufindr]);
            SERIAL_ECHOLNPGM("\"");
        }
      }

    }break;
    #endif // FWRETRACT
    #if EXTRUDERS > 1
    case 218: // M218 - set hotend offset (in mm), T<extruder_number> X<offset_on_X> Y<offset_on_Y>
    {
      if(setTargetedHotend(218)){
        break;
      }
      if(code_seen('X'))
      {
        extruder_offset[X_AXIS][tmp_extruder] = code_value();
      }
      if(code_seen('Y'))
      {
        extruder_offset[Y_AXIS][tmp_extruder] = code_value();
      }
      #ifdef DUAL_X_CARRIAGE
      if(code_seen('Z'))
      {
        extruder_offset[Z_AXIS][tmp_extruder] = code_value();
      }
      #endif       
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM(MSG_HOTEND_OFFSET);
      for(tmp_extruder = 0; tmp_extruder < EXTRUDERS; tmp_extruder++)
      {
         SERIAL_ECHO(" ");
         SERIAL_ECHO(extruder_offset[X_AXIS][tmp_extruder]);
         SERIAL_ECHO(",");
         SERIAL_ECHO(extruder_offset[Y_AXIS][tmp_extruder]);
      #ifdef DUAL_X_CARRIAGE
         SERIAL_ECHO(",");
         SERIAL_ECHO(extruder_offset[Z_AXIS][tmp_extruder]);
      #endif
      }
      SERIAL_ECHOLN("");
    }break;
    #endif
    case 220: // M220 S<factor in percent>- set speed factor override percentage
    {
      if(code_seen('S'))
      {
        feedmultiply = code_value() ;
      }
    }
    break;
    case 221: // M221 S<factor in percent>- set extrude factor override percentage
    {
      if(code_seen('S'))
      {
        extrudemultiply = code_value() ;
      }
    }
    break;

    #if NUM_SERVOS > 0
    case 280: // M280 - set servo position absolute. P: servo index, S: angle or microseconds
      {
        int servo_index = -1;
        int servo_position = 0;
        if (code_seen('P'))
          servo_index = code_value();
        if (code_seen('S')) {
          servo_position = code_value();
          if ((servo_index >= 0) && (servo_index < NUM_SERVOS)) {
            servos[servo_index].write(servo_position);
          }
          else {
            SERIAL_ECHO_START;
            SERIAL_ECHO("Servo ");
            SERIAL_ECHO(servo_index);
            SERIAL_ECHOLN(" out of range");
          }
        }
        else if (servo_index >= 0) {
          SERIAL_PROTOCOL(MSG_OK);
          SERIAL_PROTOCOL(" Servo ");
          SERIAL_PROTOCOL(servo_index);
          SERIAL_PROTOCOL(": ");
          SERIAL_PROTOCOL(servos[servo_index].read());
          SERIAL_PROTOCOLLN("");
        }
      }
      break;
    #endif // NUM_SERVOS > 0

    #if LARGE_FLASH == true && ( BEEPER > 0 || defined(ULTRALCD) )
    case 300: // M300
    {
      int beepS = code_seen('S') ? code_value() : 110;
      int beepP = code_seen('P') ? code_value() : 1000;
      if (beepS > 0)
      {
        #if BEEPER > 0
          tone(BEEPER, beepS);
          delay(beepP);
          noTone(BEEPER);
        #elif defined(ULTRALCD)
          lcd_buzz(beepS, beepP);
        #endif
      }
      else
      {
        delay(beepP);
      }
    }
    break;
    #endif // M300

    #ifdef PIDTEMP
    case 301: // M301
      {
        if(code_seen('P')) Kp = code_value();
        if(code_seen('I')) Ki = scalePID_i(code_value());
        if(code_seen('D')) Kd = scalePID_d(code_value());

        #ifdef PID_ADD_EXTRUSION_RATE
        if(code_seen('C')) Kc = code_value();
        #endif

        updatePID();
        SERIAL_PROTOCOL(MSG_OK);
        SERIAL_PROTOCOL(" p:");
        SERIAL_PROTOCOL(Kp);
        SERIAL_PROTOCOL(" i:");
        SERIAL_PROTOCOL(unscalePID_i(Ki));
        SERIAL_PROTOCOL(" d:");
        SERIAL_PROTOCOL(unscalePID_d(Kd));
        #ifdef PID_ADD_EXTRUSION_RATE
        SERIAL_PROTOCOL(" c:");
        //Kc does not have scaling applied above, or in resetting defaults
        SERIAL_PROTOCOL(Kc);
        #endif
        SERIAL_PROTOCOLLN("");
      }
      break;
    #endif //PIDTEMP
    #ifdef PIDTEMPBED
    case 304: // M304
      {
        if(code_seen('P')) bedKp = code_value();
        if(code_seen('I')) bedKi = scalePID_i(code_value());
        if(code_seen('D')) bedKd = scalePID_d(code_value());

        updatePID();
        SERIAL_PROTOCOL(MSG_OK);
        SERIAL_PROTOCOL(" p:");
        SERIAL_PROTOCOL(bedKp);
        SERIAL_PROTOCOL(" i:");
        SERIAL_PROTOCOL(unscalePID_i(bedKi));
        SERIAL_PROTOCOL(" d:");
        SERIAL_PROTOCOL(unscalePID_d(bedKd));
        SERIAL_PROTOCOLLN("");
      }
      break;
    #endif //PIDTEMP
    case 240: // M240  Triggers a camera by emulating a Canon RC-1 : http://www.doc-diy.net/photo/rc-1_hacked/
     {
      #if defined(PHOTOGRAPH_PIN) && PHOTOGRAPH_PIN > -1
        const uint8_t NUM_PULSES=16;
        const float PULSE_LENGTH=0.01524;
        for(int i=0; i < NUM_PULSES; i++) {
          WRITE(PHOTOGRAPH_PIN, HIGH);
          _delay_ms(PULSE_LENGTH);
          WRITE(PHOTOGRAPH_PIN, LOW);
          _delay_ms(PULSE_LENGTH);
        }
        delay(7.33);
        for(int i=0; i < NUM_PULSES; i++) {
          WRITE(PHOTOGRAPH_PIN, HIGH);
          _delay_ms(PULSE_LENGTH);
          WRITE(PHOTOGRAPH_PIN, LOW);
          _delay_ms(PULSE_LENGTH);
        }
      #endif
     }
    break;
#ifdef DOGLCD
    case 250: // M250  Set LCD contrast value: C<value> (value 0..63)
     {
	  if (code_seen('C')) {
	   lcd_setcontrast( ((int)code_value())&63 );
          }
          SERIAL_PROTOCOLPGM("lcd contrast value: ");
          SERIAL_PROTOCOL(lcd_contrast);
          SERIAL_PROTOCOLLN("");
     }
    break;
#endif
    #ifdef PREVENT_DANGEROUS_EXTRUDE
    case 302: // allow cold extrudes, or set the minimum extrude temperature
    {
	  float temp = .0;
	  if (code_seen('S')) temp=code_value();
      set_extrude_min_temp(temp);
    }
    break;
	#endif
    case 303: // M303 PID autotune
    {
      float temp = 150.0;
      int e=0;
      int c=5;
      if (code_seen('E')) e=code_value();
        if (e<0)
          temp=70;
      if (code_seen('S')) temp=code_value();
      if (code_seen('C')) c=code_value();
      PID_autotune(temp, e, c);
    }
    break;
    case 400: // M400 finish all moves
    {
      st_synchronize();
    }
    break;
    case 500: // M500 Store settings in EEPROM
    {
        Config_StoreSettings();
    }
    break;
    case 501: // M501 Read settings from EEPROM
    {
        Config_RetrieveSettings();
    }
    break;
    case 502: // M502 Revert to default settings
    {
        Config_ResetDefault();
        #ifdef TENLOG_CONTROLLER
        Init_TenlogScreen();
        #endif
    }
    break;
    case 503: // M503 print settings currently in memory
    {
        Config_PrintSettings();
    }
    break;
    #ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
    case 540:
    {
        if(code_seen('S')) abort_on_endstop_hit = code_value() > 0;
    }
    break;
    #endif
    #ifdef FILAMENTCHANGEENABLE
    case 600: //Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]
    {
        float target[4];
        float lastpos[4];
        target[X_AXIS]=current_position[X_AXIS];
        target[Y_AXIS]=current_position[Y_AXIS];
        target[Z_AXIS]=current_position[Z_AXIS];
        target[E_AXIS]=current_position[E_AXIS];

        lastpos[X_AXIS]=current_position[X_AXIS];
        lastpos[Y_AXIS]=current_position[Y_AXIS];
        lastpos[Z_AXIS]=current_position[Z_AXIS];
        lastpos[E_AXIS]=current_position[E_AXIS];
        //retract by E
        if(code_seen('E'))
        {
          target[E_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FIRSTRETRACT
            target[E_AXIS]+= FILAMENTCHANGE_FIRSTRETRACT ;
          #endif
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        //lift Z
        if(code_seen('Z'))
        {
          target[Z_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_ZADD
            target[Z_AXIS]+= FILAMENTCHANGE_ZADD ;
          #endif
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        //move xy
        if(code_seen('X'))
        {
          target[X_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_XPOS
            target[X_AXIS]= FILAMENTCHANGE_XPOS ;
          #endif
        }
        if(code_seen('Y'))
        {
          target[Y_AXIS]= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_YPOS
            target[Y_AXIS]= FILAMENTCHANGE_YPOS ;
          #endif
        }

        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        if(code_seen('L'))
        {
          target[E_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FINALRETRACT
            target[E_AXIS]+= FILAMENTCHANGE_FINALRETRACT ;
          #endif
        }

        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        //finish moves
        st_synchronize();
        //disable extruder steppers so filament can be removed
        disable_e0();
        disable_e1();
        disable_e2();
        delay(100);
        LCD_ALERTMESSAGEPGM(MSG_FILAMENTCHANGE);
        uint8_t cnt=0;
        while(!lcd_clicked()){
          cnt++;
          manage_heater();
          if(zyf_HEATER_FAIL){ 
              card.closefile;
              card.sdprinting = 0;
          }
          manage_inactivity();
          lcd_update();
          if(cnt==0)
          {
          #if BEEPER > 0
            SET_OUTPUT(BEEPER);

            WRITE(BEEPER,HIGH);
            delay(3);
            WRITE(BEEPER,LOW);
            delay(3);
          #else 
            lcd_buzz(1000/6,100);
          #endif
          }
        }

        //return to normal
        if(code_seen('L'))
        {
          target[E_AXIS]+= -code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FINALRETRACT
            target[E_AXIS]+=(-1)*FILAMENTCHANGE_FINALRETRACT ;
          #endif
        }
        current_position[E_AXIS]=target[E_AXIS]; //the long retract of L is compensated by manual filament feeding
        plan_set_e_position(current_position[E_AXIS]);
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //should do nothing
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //move xy back
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], lastpos[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //move z back
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], lastpos[Z_AXIS], lastpos[E_AXIS], feedrate/60, active_extruder); //final untretract
    }
    break;//M600
    #endif //FILAMENTCHANGEENABLE 
    #ifdef DUAL_X_CARRIAGE
    case 605: // Set dual x-carriage movement mode:
              //    M605 S0: Full control mode. The slicer has full control over x-carriage movement
              //    M605 S1: Auto-park mode. The inactive head will auto park/unpark without slicer involvement
              //    M605 S2 [Xnnn] [Rmmm]: Duplication mode. The second extruder will duplicate the first with nnn
              //                         millimeters x-offset and an optional differential hotend temperature of 
              //                         mmm degrees. E.g., with "M605 S2 X100 R2" the second extruder will duplicate
              //                         the first with a spacing of 100mm in the x direction and 2 degrees hotter.
              //
              //    Note: the X axis should be homed after changing dual x-carriage mode.
    command_M605();
	break;
    #endif //DUAL_X_CARRIAGE         

    case 907: // M907 Set digital trimpot motor current using axis codes.
    {
      #if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
        for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) digipot_current(i,code_value());
        if(code_seen('B')) digipot_current(4,code_value());
        if(code_seen('S')) for(int i=0;i<=4;i++) digipot_current(i,code_value());
      #endif
    }
    break;
    case 908: // M908 Control digital trimpot directly.
    {
      #if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
        uint8_t channel,current;
        if(code_seen('P')) channel=code_value();
        if(code_seen('S')) current=code_value();
        digitalPotWrite(channel, current);
      #endif
    }
    break;
    case 350: // M350 Set microstepping mode. Warning: Steps per unit remains unchanged. S code sets stepping mode for all drivers.
    {
      #if defined(X_MS1_PIN) && X_MS1_PIN > -1
        if(code_seen('S')) for(int i=0;i<=4;i++) microstep_mode(i,code_value());
        for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_mode(i,(uint8_t)code_value());
        if(code_seen('B')) microstep_mode(4,code_value());
        microstep_readings();
      #endif
    }
    break;
    case 351: // M351 Toggle MS1 MS2 pins directly, S# determines MS1 or MS2, X# sets the pin high/low.
    {
      #if defined(X_MS1_PIN) && X_MS1_PIN > -1
      if(code_seen('S')) switch((int)code_value())
      {
        case 1:
          for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_ms(i,code_value(),-1);
          if(code_seen('B')) microstep_ms(4,code_value(),-1);
          break;
        case 2:
          for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_ms(i,-1,code_value());
          if(code_seen('B')) microstep_ms(4,-1,code_value());
          break;
      }
      microstep_readings();
      #endif
    }
    break;
    #ifdef TENLOG_CONTROLLER
    case 1001: //M1001 languange id
    {
        if(code_seen('S'))
        {
            languageID=code_value();
            if(languageID>1) languageID=1;
            if(languageID<0) languageID=0;
            Config_StoreSettings();
        }
    }
    break;
    case 1002: //M1002 Calibrate 
    {
        //Init_TenlogScreen();
        //TenlogScreen_println("page main");
    }
    break;
    #ifdef POWER_LOSS_RECOVERY
    case 1003: //M1003 Power fail resume. //by zyf
    command_M1003();
    break;
    case 1004: //M1004 cancel power fail recovery
		{
			#if defined(POWER_LOSS_SAVE_TO_EEPROM)
			EEPROM_Write_PLR();
			EEPROM_PRE_Write_PLR();
			#elif defined(POWER_LOSS_SAVE_TO_SDCARD)
			card.Write_PLR();
			card.PRE_Write_PLR();
			#endif
		}
	break;
    #endif //POWER_LOSS_RECOVERY
    case 1011: //M1011 X2 Max mm
    {
        float fRate=1.0;
        if(code_seen('R')) {
          fRate=(float)code_value();
        }
        if(code_seen('S'))
        {
            zyf_X2_MAX_POS=(float)code_value()/fRate;
            Config_StoreSettings();
        }
    }
    break;
    case 1012: //M1012 Y2 mm
    {
        float fRate=1.0;
        if(code_seen('R')) {
          fRate=(float)code_value();
        }
        if(code_seen('S'))
        {
            zyf_Y2_OFFSET=(float)code_value()/fRate;;
            Config_StoreSettings();
        }
    }
    break;
    case 1013: //M1013 Z2 mm
    {
        float fRate=1.0;
        if(code_seen('R')) {
          fRate=(float)code_value();
        }
        if(code_seen('S'))
        {
            zyf_Z2_OFFSET=(float)code_value()/fRate;
            Config_StoreSettings();
        }
    }
    break;
	#ifdef FAN2_CONTROL
    case 1014: //M1014 Fan2 Value
    {
        if(code_seen('S'))
        {
            int iGet = (int)code_value();
            if(iGet > 255) iGet = 255;
            if(iGet < 0) iGet = 0;
            zyf_FAN2_VALUE = iGet;
            Config_StoreSettings();
        }
    }
    break;
    case 1015: //M1015 Fan2 Temp
    {
        if(code_seen('S'))
        {
            int iGet = (int)code_value();
            if(iGet > 100) iGet = 100;
            if(iGet < 30) iGet = 30;
            zyf_FAN2_START_TEMP=iGet;
            Config_StoreSettings();
        }
    }
    break;
	#endif
    #ifdef POWER_LOSS_TRIGGER_BY_PIN
    case 1016: //M1016 Auto Off
    {
        if(code_seen('S'))
        {
            int iGet = (int)code_value();
            if(iGet != 1) iGet = 0;
            zyf_AUTO_OFF = iGet;
            Config_StoreSettings();
        }
    }
    break;
    #endif 

    case 1017: //M1017 sleep time
    {
        if(code_seen('S'))
        {
            int iGet = (int)code_value();
            if(iGet > 60) iGet = 60;
            if(iGet < 0) iGet = 0;
            zyf_SLEEP_TIME = iGet;
            Config_StoreSettings();
        }
    }
	break;

    case 1021: //M1021   
    {
        if(code_seen('S'))
        {
            int iTemp=code_value();
            if(iTemp == 0)
                lcd_preheat_abs();
            else if(iTemp == 1)
                lcd_preheat_pla();
            else if(iTemp == 2)
                lcd_cooldown();
            else if(iTemp == 3)
            {
                //disable all steppers and cool down.
                lcd_cooldown();
                st_synchronize();
                disable_e0();
                disable_e1();
                disable_e2();
                finishAndDisableSteppers(false);
            }
        }
    }
    break;
    case 1022: //M1022   
    {
        if(code_seen('S'))
        {
            int iTemp = code_value();
			int iTemp1 = -1;
			if(code_seen('T')){
				iTemp1 = code_value();
			}
            if(iTemp == 0 || iTemp == 1)
                load_filament(iTemp, iTemp1);
        }
    }
    break;
    case 1031: //M1031
    {		
        sdcard_pause();
    }
    break;
    case 1032: //M1032   
    {
        sdcard_resume();
    }
    break;
    case 1033: //M1033   
    {
        sdcard_stop();
    }
    break;
    #endif //TENLOG_CONTROLLER

	#ifdef PRINT_FROM_Z_HEIGHT
	case 1040:
	{
		if(code_seen('S'))
		{
			float fValue = code_value();
			if(fValue == 0){
				PrintFromZLevelFound = true;
			}else{
				print_from_z_target = fValue / 10.0;
				PrintFromZLevelFound = false;
				//if(dual_x_carriage_mode == 2)
				//	dual_x_carriage_mode = 1;
			}
		}
	}
	break;
	#endif //PRINT_FROM_Z_HEIGHT

	#ifndef TENLOG_CONTROLLER
	case 1050:
	{
		pinMode(16, OUTPUT);
		pinMode(17, OUTPUT);
		if (code_seen('S'))
		{
			int iOut = code_value();
			if(iOut == 0){
				digitalWrite(16, HIGH);
				digitalWrite(17, LOW);
			}else if(iOut == 1){
				digitalWrite(17, HIGH);
				digitalWrite(16, LOW);
			}
		}
	}
	break;
	#endif

    #ifdef ENGRAVE
    case 2000: //M2000
    {
        if(code_seen('S'))
        {
            int iS=code_value();
            if(iS == 1)
                digitalWrite(ENGRAVE_PIN, ENGRAVE_ON);
            else if(iS == 0)
                digitalWrite(ENGRAVE_PIN, ENGRAVE_OFF);
        }
    }
    break;
    #endif

    case 999: // M999: Restart after being stopped
      Stopped = false;
      lcd_reset_alert_level();
      gcode_LastN = Stopped_gcode_LastN;
      FlushSerialRequestResend();
    break;
    }
  }//code_seen("M") line 1328

  else if(code_seen('T')) //Switch T0T1
    command_T();
  else
  {
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM(MSG_UNKNOWN_COMMAND);
    SERIAL_ECHO(cmdbuffer[bufindr]);
    SERIAL_ECHOLNPGM("\"");
  }
  ClearToSend();
}


void FlushSerialRequestResend()
{
  //char cmdbuffer[bufindr][100]="Resend:";
  MYSERIAL.flush();
  SERIAL_PROTOCOLPGM(MSG_RESEND);
  SERIAL_PROTOCOLLN(gcode_LastN + 1);
  ClearToSend();
}

void ClearToSend()
{
  previous_millis_cmd = millis();
  #ifdef SDSUPPORT
  if(fromsd[bufindr])
    return;
  #endif //SDSUPPORT
  SERIAL_PROTOCOLLNPGM(MSG_OK);
}

#ifdef POWER_LOSS_RECOVERY
void Save_Power_Off_Status(){

	uint32_t lFPos = card.sdpos;
	int iTPos = degTargetHotend(0) + 0.5;
	int iTPos1 = degTargetHotend(1) + 0.5;
	int iFanPos = fanSpeed;
	int iT01 = active_extruder == 0 ? 0:1;
	int iBPos = degTargetBed() + 0.5;    				
	float fZPos = current_position[Z_AXIS];				
	float fEPos = current_position[E_AXIS];
	float fXPos = current_position[X_AXIS];
	float fYPos = current_position[Y_AXIS];
	float f_feedrate = feedrate;

	#if defined(POWER_LOSS_SAVE_TO_EEPROM)
	EEPROM_Write_PLR(lFPos,iTPos,iTPos1,iT01,fZPos,fEPos);
	#elif defined(POWER_LOSS_SAVE_TO_SDCARD)
	card.Write_PLR(lFPos,iTPos,iTPos1,iT01,fZPos,fEPos);
	#endif
}
#endif

#ifdef POWER_LOSS_TRIGGER_BY_Z_LEVEL
float fLastZ = 0.0;
#endif

#ifdef POWER_LOSS_TRIGGER_BY_E_COUNT
	long lECount = POWER_LOSS_E_COUNT;
#endif

void get_coordinates(float XValue=-99999.0,float YValue=-99999.0,float ZValue=-99999.0,float EValue=-99999.0) //By zyf 20190716
{

  float fRate=1.0;
  int iMode=0;
  #ifdef TENLOG_CONTROLLER
    if(code_seen('R')) {            //倍率
        fRate=(float)code_value();
    }
    if(code_seen('M')) {            //绝对坐标，相对坐标
        iMode=(int)code_value();
    }
  #endif

  bool seen[4]={false,false,false,false};
  for(int8_t i=0; i < NUM_AXIS; i++) {
    if(code_seen(axis_codes[i]))
    {
        if(iMode == 1){
            destination[i] = (float)code_value()/fRate + current_position[i];
        }
        else
            destination[i] = (float)code_value()/fRate + (axis_relative_modes[i] || relative_mode)*current_position[i];
        seen[i]=true;
    }
    else destination[i] = current_position[i]; //Are these else lines really needed?
  }

  if(!seen[0] && !seen[1] && !seen[2] && !seen[3]){
      if(XValue > -99999.0){
        destination[X_AXIS] = XValue/fRate + (axis_relative_modes[X_AXIS] || relative_mode)*current_position[X_AXIS];  
      }else{
        destination[X_AXIS] = current_position[X_AXIS];
      }

      if(YValue > -99999.0){
        destination[Y_AXIS] = YValue/fRate + (axis_relative_modes[Y_AXIS] || relative_mode)*current_position[Y_AXIS];  
      }else{
        destination[Y_AXIS] = current_position[Y_AXIS];
      }

      if(ZValue > -99999.0){
        destination[Z_AXIS] = ZValue/fRate + (axis_relative_modes[Z_AXIS] || relative_mode)*current_position[Z_AXIS];  
      }else{
        destination[Z_AXIS] = current_position[Z_AXIS];
      }

      if(EValue > -99999.0){
        destination[E_AXIS] = EValue/fRate + (axis_relative_modes[E_AXIS] || relative_mode)*current_position[E_AXIS];  
      }else{
        destination[E_AXIS] = current_position[E_AXIS];
      }
  }
  
  #ifdef POWER_LOSS_TRIGGER_BY_Z_LEVEL
  if(destination[Z_AXIS] > fLastZ && card.sdprinting == 1 && card.sdpos > 2048) {
	  fLastZ = destination[Z_AXIS];
	  Save_Power_Off_Status();
  }
  #endif

  #ifdef POWER_LOSS_TRIGGER_BY_E_COUNT
  if(destination[E_AXIS] != current_position[E_AXIS] && card.sdprinting == 1) lECount--;
  if(lECount <=0 && card.sdprinting == 1 && card.sdpos > 2048) {
	  lEcount = POWER_LOSS_E_COUNT;
	  Save_Power_Off_Status();
  }
  #endif

  if(code_seen('F')) {
    next_feedrate = code_value();
    if(next_feedrate > 0.0) feedrate = next_feedrate;
  }

  #ifdef FWRETRACT
  if(autoretract_enabled)
  if( !(seen[X_AXIS] || seen[Y_AXIS] || seen[Z_AXIS]) && seen[E_AXIS])
  {
    float echange=destination[E_AXIS]-current_position[E_AXIS];
    if(echange<-MIN_RETRACT) //retract
    {
      if(!retracted)
      {

      destination[Z_AXIS]+=retract_zlift; //not sure why chaninging current_position negatively does not work.
      //if slicer retracted by echange=-1mm and you want to retract 3mm, corrrectede=-2mm additionally
      float correctede=-echange-retract_length;
      //to generate the additional steps, not the destination is changed, but inversely the current position
      current_position[E_AXIS]+=-correctede;
      feedrate=retract_feedrate;
      retracted=true;
      }

    }
    else
      if(echange>MIN_RETRACT) //retract_recover
    {
      if(retracted)
      {
      //current_position[Z_AXIS]+=-retract_zlift;
      //if slicer retracted_recovered by echange=+1mm and you want to retract_recover 3mm, corrrectede=2mm additionally
      float correctede=-echange+1*retract_length+retract_recover_length; //total unretract=retract_length+retract_recover_length[surplus]
      current_position[E_AXIS]+=correctede; //to generate the additional steps, not the destination is changed, but inversely the current position
      feedrate=retract_recover_feedrate;
      retracted=false;
      }
    }

  }
  #endif //FWRETRACT
}

void get_arc_coordinates()
{
#ifdef SF_ARC_FIX
   bool relative_mode_backup = relative_mode;
   relative_mode = true;
#endif
   get_coordinates();
#ifdef SF_ARC_FIX
   relative_mode=relative_mode_backup;
#endif

   if(code_seen('I')) {
     offset[0] = code_value();
   }
   else {
     offset[0] = 0.0;
   }
   if(code_seen('J')) {
     offset[1] = code_value();
   }
   else {
     offset[1] = 0.0;
   }
}

void clamp_to_software_endstops(float target[3])
{
  if (min_software_endstops) {
    if (target[X_AXIS] < min_pos[X_AXIS]) target[X_AXIS] = min_pos[X_AXIS];
    if (target[Y_AXIS] < min_pos[Y_AXIS]) target[Y_AXIS] = min_pos[Y_AXIS];
    if (target[Z_AXIS] < min_pos[Z_AXIS]) target[Z_AXIS] = min_pos[Z_AXIS];
  }

  if (max_software_endstops) {
    if (target[X_AXIS] > max_pos[X_AXIS]) target[X_AXIS] = max_pos[X_AXIS];
    if (target[Y_AXIS] > max_pos[Y_AXIS]) target[Y_AXIS] = max_pos[Y_AXIS];
    if (target[Z_AXIS] > max_pos[Z_AXIS]) target[Z_AXIS] = max_pos[Z_AXIS];
  }

  if(dual_x_carriage_mode == DXC_MIRROR_MODE) //protect headers from hitting each other when mirror mode print
    if(target[X_AXIS] > (zyf_X2_MAX_POS - X_NOZZLE_WIDTH * 2)/2)
        target[X_AXIS] = (zyf_X2_MAX_POS - X_NOZZLE_WIDTH * 2)/2;
  else if(dual_x_carriage_mode == DXC_DUPLICATION_MODE) //protect headers from hitting each other when DUPLICATION mode print
    if(target[X_AXIS] > zyf_X2_MAX_POS/2)
        target[X_AXIS] = zyf_X2_MAX_POS/2;
  			
}

#ifdef DELTA
void calculate_delta(float cartesian[3])
{
  delta[X_AXIS] = sqrt(sq(DELTA_DIAGONAL_ROD)
                       - sq(DELTA_TOWER1_X-cartesian[X_AXIS])
                       - sq(DELTA_TOWER1_Y-cartesian[Y_AXIS])
                       ) + cartesian[Z_AXIS];
  delta[Y_AXIS] = sqrt(sq(DELTA_DIAGONAL_ROD)
                       - sq(DELTA_TOWER2_X-cartesian[X_AXIS])
                       - sq(DELTA_TOWER2_Y-cartesian[Y_AXIS])
                       ) + cartesian[Z_AXIS];
  delta[Z_AXIS] = sqrt(sq(DELTA_DIAGONAL_ROD)
                       - sq(DELTA_TOWER3_X-cartesian[X_AXIS])
                       - sq(DELTA_TOWER3_Y-cartesian[Y_AXIS])
                       ) + cartesian[Z_AXIS];
  /*
  SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(cartesian[X_AXIS]);
  SERIAL_ECHOPGM(" y="); SERIAL_ECHO(cartesian[Y_AXIS]);
  SERIAL_ECHOPGM(" z="); SERIAL_ECHOLN(cartesian[Z_AXIS]);

  SERIAL_ECHOPGM("delta x="); SERIAL_ECHO(delta[X_AXIS]);
  SERIAL_ECHOPGM(" y="); SERIAL_ECHO(delta[Y_AXIS]);
  SERIAL_ECHOPGM(" z="); SERIAL_ECHOLN(delta[Z_AXIS]);
  */
}
#endif

void prepare_move()
{
  clamp_to_software_endstops(destination);
  previous_millis_cmd = millis();

#ifdef DUAL_X_CARRIAGE
  if (active_extruder_parked)
  {
	float fZRaise = 15;
    if (dual_x_carriage_mode == DXC_DUPLICATION_MODE && active_extruder == 0)
    {
        // move duplicate extruder into correct duplication position.				//By Zyf Add 15mm Z
		plan_set_position(inactive_extruder_x_pos, current_position[Y_AXIS], current_position[Z_AXIS] - fZRaise, current_position[E_AXIS]);
		plan_buffer_line(current_position[X_AXIS] + duplicate_extruder_x_offset, current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], max_feedrate[X_AXIS], 1);	  	  
		plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS] + fZRaise, current_position[E_AXIS]);
		st_synchronize();
		extruder_carriage_mode = 2;
	    active_extruder_parked = false;
    }
    else if (dual_x_carriage_mode == DXC_MIRROR_MODE)
    {
      // move duplicate extruder into correct duplication position.				//By Zyf Add 15mm Z
      plan_set_position(inactive_extruder_x_pos, current_position[Y_AXIS], current_position[Z_AXIS] - fZRaise, current_position[E_AXIS]);
      plan_buffer_line(zyf_X2_MAX_POS, current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], max_feedrate[X_AXIS], 1);
      plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS] + fZRaise, current_position[E_AXIS]);
      st_synchronize();
      extruder_carriage_mode = 3;
      active_extruder_parked = false;
    }  
    else if (dual_x_carriage_mode == DXC_AUTO_PARK_MODE) // handle unparking of head
    {
      if (current_position[E_AXIS] == destination[E_AXIS])
      {
        // this is a travel move - skit it but keep track of current position (so that it can later
        // be used as start of first non-travel move)
        if (delayed_move_time != 0xFFFFFFFFUL)
        {
          //memcpy(current_position, destination, sizeof(current_position)); 
          current_position[X_AXIS] = destination[X_AXIS];											//By zyf
          current_position[Y_AXIS] = destination[Y_AXIS];											//By zyf
          current_position[Z_AXIS] = destination[Z_AXIS];											//By zyf
          current_position[E_AXIS] = destination[E_AXIS];											//By zyf

          if (destination[Z_AXIS] > raised_parked_position[Z_AXIS])
            raised_parked_position[Z_AXIS] = destination[Z_AXIS];

          delayed_move_time = millis();
          //return; 
        }
      }

      delayed_move_time = 0;
      plan_buffer_line(raised_parked_position[X_AXIS], raised_parked_position[Y_AXIS], raised_parked_position[Z_AXIS], current_position[E_AXIS], max_feedrate[Z_AXIS], active_extruder);
      plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], raised_parked_position[Z_AXIS], current_position[E_AXIS], min(max_feedrate[X_AXIS],max_feedrate[Y_AXIS]), active_extruder);
      plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], max_feedrate[Z_AXIS], active_extruder);
      st_synchronize();
      extruder_carriage_mode = 1;
      active_extruder_parked = false;
    }
  }
#endif //DUAL_X_CARRIAGE

  // Do not use feedmultiply for E or Z only moves
  if( (current_position[X_AXIS] == destination [X_AXIS]) && (current_position[Y_AXIS] == destination [Y_AXIS])) {
      plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
  }
  else {
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate*feedmultiply/60/100.0, active_extruder);
  }

  for(int8_t i=0; i < NUM_AXIS; i++) {
    current_position[i] = destination[i];
  }
} //prepare_move

void prepare_arc_move(char isclockwise) {
  float r = hypot(offset[X_AXIS], offset[Y_AXIS]); // Compute arc radius for mc_arc

  // Trace the arc
  mc_arc(current_position, destination, offset, X_AXIS, Y_AXIS, Z_AXIS, feedrate*feedmultiply/60/100.0, r, isclockwise, active_extruder);

  // As far as the parser is concerned, the position is now == target. In reality the
  // motion control system might still be processing the action and the real tool position
  // in any intermediate location.
  for(int8_t i=0; i < NUM_AXIS; i++) {
    current_position[i] = destination[i];
  }
  previous_millis_cmd = millis();
}

#if defined(CONTROLLERFAN_PIN) && CONTROLLERFAN_PIN > -1

#if defined(FAN_PIN)
  #if CONTROLLERFAN_PIN == FAN_PIN 
    #error "You cannot set CONTROLLERFAN_PIN equal to FAN_PIN"
  #endif
#endif  

unsigned long lastMotor = 0; //Save the time for when a motor was turned on last
unsigned long lastMotorCheck = 0;

void controllerFan()
{
  if ((millis() - lastMotorCheck) >= 2500) //Not a time critical function, so we only check every 2500ms
  {
    lastMotorCheck = millis();

    if(!READ(X_ENABLE_PIN) || !READ(Y_ENABLE_PIN) || !READ(Z_ENABLE_PIN)
    #if EXTRUDERS > 2
       || !READ(E2_ENABLE_PIN)
    #endif
    #if EXTRUDER > 1
      #if defined(X2_ENABLE_PIN) && X2_ENABLE_PIN > -1
       || !READ(X2_ENABLE_PIN)
      #endif
       || !READ(E1_ENABLE_PIN)
    #endif
       || !READ(E0_ENABLE_PIN)) //If any of the drivers are enabled...
    {
      lastMotor = millis(); //... set time to NOW so the fan will turn on
    }
    
    if ((millis() - lastMotor) >= (CONTROLLERFAN_SECS*1000UL) || lastMotor == 0) //If the last time any driver was enabled, is longer since than CONTROLLERSEC...   
    {
        digitalWrite(CONTROLLERFAN_PIN, 0); 
        analogWrite(CONTROLLERFAN_PIN, 0); 
    }
    else
    {
        // allows digital or PWM fan output to be used (see M42 handling)
        digitalWrite(CONTROLLERFAN_PIN, CONTROLLERFAN_SPEED);
        analogWrite(CONTROLLERFAN_PIN, CONTROLLERFAN_SPEED); 
    }
  }
}
#endif

 //By Zyf
#ifdef POWER_LOSS_TRIGGER_BY_PIN
bool bPowerOned = false;
bool bDetected = false;

bool Check_Power_Loss(){

	if(bDetected) return true;

	bool bRet = false;
	int iPowerLoss = digitalRead(POWER_LOSS_DETECT_PIN);
	if( iPowerLoss == 0){
		bPowerOned = true;
	}else{
		if(bPowerOned && !bDetected){
			bDetected = true;
			bRet = true;
			Power_Off_Handler();
		}
	}
	return bRet;
}

void Power_Off_Handler(bool MoveX){

	if(card.sdprinting == 1){ 
		cli(); // Stop interrupts												
				
		digitalWrite(HEATER_BED_PIN,LOW);
		digitalWrite(HEATER_0_PIN,LOW);
		digitalWrite(HEATER_1_PIN,LOW);
		digitalWrite(FAN2_PIN,LOW);
		digitalWrite(FAN_PIN,LOW);
		digitalWrite(PS_ON_PIN, PS_ON_ASLEEP);

		disable_x();
		disable_y();
		disable_z();
		disable_e0();
		disable_e1();
		
		Save_Power_Off_Status();
				
		if(MoveX){
			enable_x();
			for (int i=0; i<axis_steps_per_unit[X_AXIS]*20; i++){

				bool bWrite = false;
				if(i%2 == 1){
					bWrite = false;
				}else{
					bWrite = true;
				}

				#ifdef DUAL_X_CARRIAGE
				if (extruder_carriage_mode == 2 || extruder_carriage_mode == 3){
				  WRITE(X_STEP_PIN, bWrite);
				  WRITE(X2_STEP_PIN, bWrite);
				}
				else {
				  if (active_extruder != 0)
					WRITE(X2_STEP_PIN, bWrite);
				  else
					WRITE(X_STEP_PIN, bWrite);
				}
				#else
				WRITE(X_STEP_PIN, bWrite);
				#endif

				delayMicroseconds(120);
			}
		}
	}

	#ifdef TENLOG_CONTROLLER
	TenlogScreen_println("page shutdown");
	delay(1000);
	#endif
	digitalWrite(PS_ON_PIN, PS_ON_ASLEEP);
	digitalWrite(HEATER_BED_PIN,HIGH);
	digitalWrite(HEATER_0_PIN,HIGH);
	digitalWrite(HEATER_1_PIN,HIGH);
	digitalWrite(FAN2_PIN,HIGH);
	digitalWrite(FAN_PIN,HIGH);
	delay(5000);
	digitalWrite(HEATER_BED_PIN,LOW);
	digitalWrite(HEATER_0_PIN,LOW);
	digitalWrite(HEATER_1_PIN,LOW);
	digitalWrite(FAN2_PIN,LOW);
	digitalWrite(FAN_PIN,LOW);
}

#endif //POWER_LOSS_TRIGGER_BY_PIN

void manage_inactivity()
{
    if( (millis() - previous_millis_cmd) >  max_inactive_time )
        if(max_inactive_time)
            kill();
    if(stepper_inactive_time){
        if( (millis() - previous_millis_cmd) >  stepper_inactive_time ){
            if(blocks_queued() == false) {
	            if(!card.isFileOpen()){						//By zyf not disable steppers when pause.
			        disable_x();
				    disable_y();
				    //disable_z();									//By zyf always lock Z;
				    disable_e0();
				    disable_e1();
				    disable_e2();
			    }
            }
        }
    }
  
	#if defined(CONTROLLERFAN_PIN) && CONTROLLERFAN_PIN > -1
    controllerFan(); //Check if fan should be turned on to cool stepper drivers down
    #endif
    #ifdef EXTRUDER_RUNOUT_PREVENT
    if( (millis() - previous_millis_cmd) >  EXTRUDER_RUNOUT_SECONDS*1000 )
    if(degHotend(active_extruder)>EXTRUDER_RUNOUT_MINTEMP)
    {
     bool oldstatus=READ(E0_ENABLE_PIN);
     enable_e0();
     float oldepos=current_position[E_AXIS];
     float oldedes=destination[E_AXIS];
     plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS],
                      current_position[E_AXIS]+EXTRUDER_RUNOUT_EXTRUDE*EXTRUDER_RUNOUT_ESTEPS/axis_steps_per_unit[E_AXIS],
                      EXTRUDER_RUNOUT_SPEED/60.*EXTRUDER_RUNOUT_ESTEPS/axis_steps_per_unit[E_AXIS], active_extruder);
     current_position[E_AXIS]=oldepos;
     destination[E_AXIS]=oldedes;
     plan_set_e_position(oldepos);
     previous_millis_cmd=millis();
     st_synchronize();
     WRITE(E0_ENABLE_PIN,oldstatus);
    }
    #endif
    #if defined(DUAL_X_CARRIAGE)
    // handle delayed move timeout
    if (delayed_move_time != 0 && (millis() - delayed_move_time) > 1000 && Stopped == false)
    {
      // travel moves have been received so enact them
      delayed_move_time = 0xFFFFFFFFUL; // force moves to be done
      memcpy(destination,current_position,sizeof(destination));
      prepare_move(); 
    }
    #endif  
    check_axes_activity();
}

void kill()
{
  cli(); // Stop interrupts
  disable_heater();

  disable_x();
  disable_y();
  disable_z();
  disable_e0();
  disable_e1();
  disable_e2();

#if defined(PS_ON_PIN) && PS_ON_PIN > -1
  //pinMode(PS_ON_PIN,INPUT);
#endif  
  SERIAL_ERROR_START;
  SERIAL_ERRORLNPGM(MSG_ERR_KILLED);
  LCD_ALERTMESSAGEPGM(MSG_KILLED);
  suicide();
  while(1) { /* Intentionally left empty */ } // Wait for reset
}

void Stop()
{
  disable_heater();
  if(Stopped == false) {
    Stopped = true;
    Stopped_gcode_LastN = gcode_LastN; // Save last g_code for restart
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
    LCD_MESSAGEPGM(MSG_STOPPED);
  }
}

bool IsStopped() { return Stopped; };

#ifdef FAST_PWM_FAN
void setPwmFrequency(uint8_t pin, int val)
{
  val &= 0x07;
  switch(digitalPinToTimer(pin))
  {

    #if defined(TCCR0A)
    case TIMER0A:
    case TIMER0B:
//         TCCR0B &= ~(_BV(CS00) | _BV(CS01) | _BV(CS02));
//         TCCR0B |= val;
         break;
    #endif

    #if defined(TCCR1A)
    case TIMER1A:
    case TIMER1B:
//         TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
//         TCCR1B |= val;
         break;
    #endif

    #if defined(TCCR2)
    case TIMER2:
    case TIMER2:
         TCCR2 &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
         TCCR2 |= val;
         break;
    #endif

    #if defined(TCCR2A)
    case TIMER2A:
    case TIMER2B:
         TCCR2B &= ~(_BV(CS20) | _BV(CS21) | _BV(CS22));
         TCCR2B |= val;
         break;
    #endif

    #if defined(TCCR3A)
    case TIMER3A:
    case TIMER3B:
    case TIMER3C:
         TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
         TCCR3B |= val;
         break;
    #endif

    #if defined(TCCR4A)
    case TIMER4A:
    case TIMER4B:
    case TIMER4C:
         TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));
         TCCR4B |= val;
         break;
   #endif

    #if defined(TCCR5A)
    case TIMER5A:
    case TIMER5B:
    case TIMER5C:
         TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52));
         TCCR5B |= val;
         break;
   #endif

  }
}
#endif //FAST_PWM_FAN

bool setTargetedHotend(int code){
  tmp_extruder = active_extruder;
  if(code_seen('T')) {
    tmp_extruder = code_value();
    if(tmp_extruder >= EXTRUDERS) {
      SERIAL_ECHO_START;
      switch(code){
        case 104:
          SERIAL_ECHO(MSG_M104_INVALID_EXTRUDER);
          break;
        case 105:
          SERIAL_ECHO(MSG_M105_INVALID_EXTRUDER);
          break;
        case 109:
          SERIAL_ECHO(MSG_M109_INVALID_EXTRUDER);
          break;
        case 218:
          SERIAL_ECHO(MSG_M218_INVALID_EXTRUDER);
          break;
      }
      SERIAL_ECHOLN(tmp_extruder);
      return true;
    }
  }
  return false;
}

float feedrate_pause = 0;
float ePos_pause = 0.0;

void raise_Z_E(int Z, int E){
    float x = current_position[X_AXIS];
    float y = current_position[Y_AXIS];
    float z = current_position[Z_AXIS] + Z;
    float e = current_position[E_AXIS] + E;
	feedrate = 6000;
    command_G1(x,y,z,e);
}


#ifdef SDSUPPORT
void sdcard_pause(int OValue)
{
	#ifdef PRINT_FROM_Z_HEIGHT
	if(!PrintFromZLevelFound)
		return;
	#endif

	card.pauseSDPrint();
	#ifdef TENLOG_CONTROLLER
	TenlogScreen_println("reload.vaFromPageID.val=6");
	String strMessage = "reload.sT1T2.txt=\"" + String(active_extruder + 1) + "\"";
    const char*  str0 = strMessage.c_str();
	TenlogScreen_println(str0);
	strMessage = "reload.vaTargetTemp0.val=" + String(target_temperature[0]) + "";
    str0 = strMessage.c_str();
	TenlogScreen_println(str0);
	strMessage = "reload.vaTargetTemp1.val=" + String(target_temperature[1]) + "";
    str0 = strMessage.c_str();
	TenlogScreen_println(str0);
	strMessage = "reload.vaTargetBed.val=" + String(int(degTargetBed()+0.5)) + "";
    str0 = strMessage.c_str();
	TenlogScreen_println(str0);
	strMessage = "reload.vaMode.val=" + String(dual_x_carriage_mode) + "";
    str0 = strMessage.c_str();
	TenlogScreen_println(str0);

	if(duplicate_extruder_x_offset != DEFAULT_DUPLICATION_X_OFFSET){	
		strMessage = "reload.vaMode2Offset.val=" + String(duplicate_extruder_x_offset) + "";
        str0 = strMessage.c_str();
		TenlogScreen_println(str0);
	}else
		TenlogScreen_println("reload.vaMode2Offset.val=-1");
	#endif

	bool isFF = false;
	if(code_seen('O') || OValue==1){
		isFF = true;
	}
	if(isFF){
		setTargetHotend(0,0);				//By Zyf
		setTargetHotend(0,1);				//By Zyf
		//setTargetBed(0);	//Do not cool down bed
	}

	ePos_pause = current_position[E_AXIS];
	feedrate_pause = feedrate;	
    #ifdef PAUSE_RAISE_Z	//By Zyf
    raise_Z_E(15, -5);
    #endif
}

void sdcard_resume()
{
	#ifdef PRINT_FROM_Z_HEIGHT
	if(!PrintFromZLevelFound)
		return;
	#endif

    card.sdprinting = 2;
	if(code_seen('T'))
	{
		int iT0 = code_value();
		int iT1 = (iT0==0?1:0);
		int iF = 0;
		int iS = 0;
		
		if(code_seen('H')){
			iF = code_value();
		}
		if(code_seen('I')){
			iS = code_value();
		}

		if(iS > 0){
			command_M104(iT1, iS);
		}
		if(iF >0){
			command_M109(iF);
		}
	}
    #ifdef PAUSE_RAISE_Z		//By Zyf
    raise_Z_E(-15, 0);
    #endif
	if(feedrate_pause > 1000) feedrate = feedrate_pause; else feedrate = 4000;
	command_G92(-99999.0,-99999.0,-99999.0,ePos_pause);
	#ifdef FILAMENT_FAIL_DETECT
	bFilaFail = false;
	#endif
    card.sdprinting = 0;
    card.startFileprint();
}

void sdcard_stop()
{
    card.sdprinting = 0;
		
    setTargetHotend(0,0);				//By Zyf
    setTargetHotend(0,1);				//By Zyf
    setTargetBed(0);						//By Zyf
    enquecommand_P((PSTR("G28 X"))); // axis home
    card.closefile();
    quickStop();

    finishAndDisableSteppers(false);										//By Zyf
    autotempShutdown();    
}
#endif //SDSUPPORT

void load_filament(int LoadUnload, int TValue){

	//bool bChanged = false;
	int iTempE = active_extruder;
	//float fX = current_position[X_AXIS];
	if(TValue != iTempE && TValue != -1){
		command_T(TValue);
		//bChanged = true;
	}

	if(LoadUnload == 0){
		current_position[E_AXIS] += 90;
		plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 10, active_extruder); //20
		current_position[E_AXIS] += 20;
		plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 2, active_extruder); //20
	}else if(LoadUnload == 1){
		current_position[E_AXIS] += 30;
		plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 2, active_extruder); //20
		current_position[E_AXIS] -= 120;
		plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 50, active_extruder); //20	
	}

}
