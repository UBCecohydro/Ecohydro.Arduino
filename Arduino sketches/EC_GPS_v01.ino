//EC_GPS_v01.ino
// Initial program to log EC and temperature (from GS3) and GPS to SD Card
// Drafted by Tom Keddie Apr 23, 2017
// Finalized by Teddy Eyster May 14, 2018. tdeyster@gmail.com

// this program begins when the device is powered on.

//Blinking Error Codes
// 2: SD Card missing/failed
// 3: Logger not recieving data from GS3
// 4: Log file error

//include libraries
#include <SPI.h>
#include <SD.h>
// https://github.com/arduino-libraries/RTCZero
#include "RTCZero.h"
// https://github.com/adafruit/Adafruit_GPS; good resource https://codebender.cc/example/Adafruit_GPS/due_parsing#due_parsing.ino
#include "Adafruit_GPS.h"
// https://github.com/ManuelJimenezBuendia/Arduino-SDI-12
#include "SDI12_ARM.h"

//SET MEASUREMENT INTERVAL HERE in minutes (2 to 59)
static uint8_t meas_int = 2; //set number of minutes between measurements (2 to 59)

//SET SINGLE FILE OR DAILY FILES
boolean daily = false; //if daily=true, a new file will be created for each day; if daily=false a single file will be created named "LOG_DATA.TXT"

// Set the pins used on main control board
static const uint16_t pin_cardSelect = 4;
static const uint16_t pin_redLed = 13;
static const uint16_t pin_greenLed = 8;
static const uint16_t pin_batterySense = A7;
static const uint16_t pin_gpsEnable = 5;
static const uint16_t pin_gpsFix = 6;
static const uint16_t pin_GS3_SDI12 = 11;
static const uint16_t pin_fiveVoltEnable = 12;

// Create an rtc (real time clock) object
static RTCZero rtc;

// Create a GPS object
Adafruit_GPS gps(&Serial1); //'Serial' is the name of the hardware Serial port

// Create an SDI-12 object and commands; can replace '?' with specific address ie "9"
SDI12 GS3_SDI12(pin_GS3_SDI12);
//static const String myCommandInfo = "?I!"; //"?I!" = get info;      // returns address and unit specs ie "913DECAGON GS3 335"
static const String myCommandMeas = "?M!";//"aM!" = take measurement; // returns address code for measurment ie "9 001 39"
static const String myCommandData = "?D0!";//"aD0!" = read data;      // returns "address+dielectirc+temp+EC"

// Create filename object
static char filename[15]="LOG_DATA.TXT";//create log filename; will be overwritten if daily = true

// Create a file object
static File logfile;

// Create object for the minute it wakes up
static uint8_t wakeMinute = 0;

// timer for gps loop
uint32_t timer = millis();

/////////////////
// SET UP
/////////////////
void setup()
{
  pinMode(pin_redLed, OUTPUT);
  digitalWrite(pin_redLed, LOW);//turn off red light; can be commented to turn Red LED on Feather M0 on unless error
  pinMode(pin_greenLed, OUTPUT);
  pinMode(pin_fiveVoltEnable, OUTPUT);
  pinMode(pin_gpsEnable, OUTPUT);
  pinMode(pin_gpsFix, INPUT);

  // turn on
  digitalWrite(pin_gpsEnable, 0);//power off gps
  digitalWrite(pin_fiveVoltEnable, 1);//power on power booster

  // USB serial for debug only
  Serial.begin(115200);
  
  // Check for SD
  if (!SD.begin(pin_cardSelect))
  {
    error(2);
  }

  // start GPS
  gps.begin(9600);
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // comment this line to turn off RMC (recommended minimum) and GGA (fix data) including altitude
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);   // uncomment this line to turn on only the "minimum recommended" data
  // For parsing data, anything but RMC only or RMC+GGA is not recommended since the parser doesn't care about other sentences
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // set update rate to 1 Hz
  //gps.sendCommand(PGCMD_ANTENNA);//antenna status update
  delay(1000);
  Serial1.println(PMTK_Q_RELEASE); //get firmware v.

  // start the rtc "clock"
  rtc.begin(true);
}

/////////////////
// MAIN LOOP
/////////////////
void loop()
{
  digitalWrite(pin_greenLed, HIGH);//green light turns on when it wakes up

  wakeMinute = rtc.getMinutes();
  
  // GPS
  timer = millis();
  while (true)
  {
    char c = gps.read();//read data char from GPS
    if(gps.newNMEAreceived())  //wait until a new sentance is recieved, then read it (1x per sec)
    // If sentance parse fails start loop over
    if (!gps.parse(gps.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      continue;  

    // if millis() or timer wraps around, we'll just reset it
    if (timer > millis()) timer = millis();

    // every three seconds check if fix has been achieved
    if (millis() - timer > 3000) {
      timer = millis(); // reset the timer
      //condition when fix is obtained
      if ((gps.fix and gps.fixquality >0) and gps.fixquality < 5)
      {
        break;
      }
      else{
      //condition if fix isn't obtained after 1/2 measurement interval take reading anyway
      if (rtc.getMinutes()== (wakeMinute + meas_int/2)%60){
        break;}
      }
    }
  }

  // read from decagon GS3
  String dataString;
  GS3_SDI12.begin();
  GS3_SDI12.sendCommand(myCommandMeas);// start meas; returns something like "9 001 39"
  delay(50);
  while (GS3_SDI12.available())GS3_SDI12.read();
  delay(1000);                     // wait a "001" second to measure 
  GS3_SDI12.sendCommand(myCommandData);
  delay(50);
  while (GS3_SDI12.available())
  {
    uint8_t data = GS3_SDI12.read();
    if (data != '\r' && data != '\n')
      dataString += (char) data;
  }
  GS3_SDI12.end();
  if (dataString.length()< 4)
    {
      error(3);
    }

  // read battery a2d converter
  uint16_t battery = analogRead(pin_batterySense);

  // create filename and header
  if (daily){
    sprintf(filename, "LOG_%02d%02d.TXT", gps.month, gps.day); //create a new log file for each day
  }
  if (!SD.exists(filename)){
    //open or create file for logging
    logfile = SD.open(filename, FILE_WRITE);
      if ( ! logfile )
      {
        error(4);
      }
      // write header file if first time through loop
      logfile.println("year,month,day,hour_UTC,minute,second,lat,lon,altitude,satellites,gps_fix,gps_fix_qual,batt,GS3_data");
  }
  else {
    logfile = SD.open(filename, FILE_WRITE);  
    if ( ! logfile )
    {
      error(4);
    }
  }
  
  // write data to file
  logfile.print("20"); logfile.print(gps.year); logfile.print(",");
  logfile.print(gps.month);  logfile.print(",");
  logfile.print(gps.day);    logfile.print(",");
  logfile.print(gps.hour);   logfile.print(",");
  logfile.print(gps.minute); logfile.print(",");
  logfile.print(gps.seconds);logfile.print(",");
  logfile.print(gps.latitude, 8);
  logfile.print(gps.lat);    logfile.print(",");
  logfile.print(gps.longitude, 8);
  logfile.print(gps.lon);    logfile.print(",");
  logfile.print(gps.altitude);         logfile.print(",");
  logfile.print((int)gps.satellites);  logfile.print(",");
  logfile.print(gps.fix);    logfile.print(",");
  logfile.print(gps.fixquality);       logfile.print(",");
  logfile.print(battery);    logfile.print(",\"");  
  logfile.print(dataString); logfile.println("\"");
  logfile.close();

  digitalWrite(pin_greenLed, LOW);//turn off green light

  rtc.setAlarmTime(00, (wakeMinute + meas_int) % 60, 00);//handle rollover condition
  rtc.enableAlarm(rtc.MATCH_MMSS);
  rtc.standbyMode(); //put unit to sleep
}

/////////////////
// ERROR CODES
/////////////////
// blink out an error code with red light on Feather M0 board (if error(3)->3-blink,pause)
void error(uint8_t errno)
{
  while (1)
  {
    uint8_t i;
    for (i = 0; i < errno; i++)
    {
      digitalWrite(pin_redLed, HIGH);
      delay(200);
      digitalWrite(pin_redLed, LOW);
      delay(200);
    }
    for (i = errno; i < 10; i++)
    {
      delay(200);
    }
  }
}
