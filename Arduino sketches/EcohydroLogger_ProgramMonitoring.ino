/*
 * File                 EcohydroLogger_ProgramMonitoring.ino
 *
 * Synopsis             Connects to SDI-12 sensor (here with SDI-ID "0") for data logging in 10 minutes (can be adjusted) intervals. 
 *                      With powerdown between measurements for energy conservation.
 *                      Protocol used with CTD Degacon Devices Sensor (Conductivity, Temperature, Depth), battery & solar panel for logging hydrological data 
 *                      Data is saved on SD card as txt files (saved daily as separate files to minimize data lost in case of corrput files) 
 *                      Includes DS1306 for waking up Arduino processor after powerdown intervals.
 *                      
 *                      Technically, several sensors can be connected to the Ecohydro Logger. Currently we are using only one sensor connected to A14.
 * 
 * Board                Arduino Mega 2560 with Ecohydro Shield V1
 *
 * Author               Silja Hund, Tom Keddie
 *
 * Attribution          ds1306.h library (Chris Bearman), SDI-12 library (SDI12.h, StroudCenter), and LowPower library (rocketscream)
 *
 *
 * Version              1.0
 *
 * License              This software is released under the terms of the Mozilla Public License (MPL) version 2.0
 *                      Full details of licensing terms can be found in the "LICENSE" file, distributed with this code
 *
 * Instructions         Connect Ecohydro Logger. The program lets the Arduino wake up in 10 minute intervals, starting to count after upload of program. 
 *                      After uploading program and disconnecting from computer, insert jumper shunt for disabling USB port, this saves significant amounts of power. 
 *                      
 *                      Connecting Decagon CTD sensor to Ecohydro Logger:
 *                      RED CTD WIRE (communications/signal) - connects to 14
 *                      WHITE CTD WIRE (power / excitation) - connects to +
 *                      BLACK / BARE CTD WIRE (ground) - connects to G (ground)
 *                      
 *                      Important: Wihtin the code, the site ID can be determined, so that each .txt file (that is saved everyday), contains both date (ddmmyy),
 *                      plus a 2 character site ID. This site ID cannot be longer than 2 characters (total name length is 8 characters), otherwise the program will not run 
 *                      for dates after October 10 (1010yy), in contrast to (910yy), when exceeding the 8 character limit.
 *                      
 * 
 * Data                 Water Depth in mm, Water temperature in degrees Celsius, Electrical Conductivity in dS/m
 *
 */

//Libraries
#include <avr/power.h>
#include <avr/sleep.h>
#include "SD.h"
#include "SPI.h"
// https://github.com/StroudCenter/Arduino-SDI-12
#include "SDI12.h"
// https://github.com/cjbearman/ds1306-arduino
#include "DS1306.h"
// https://github.com/rocketscream/Low-Power
#include "LowPower.h"

 
//////////Setting variables //////////////////////////////////////////////////////////////////////////////////////////////
//Set Time Interval (in Minutes) for alarm to go off & measurement to be taken 
static const uint8_t SAMPLE_INTERVAL_MINUTES = 10; //TEN MINUTE TIME INTERVAL. To change logging interval, change minutes here.

///////////// Set Pins
// control pins
static const uint8_t SD_CS_PIN = 49; //SD Pin 
static const uint8_t RTC_CS_PIN = 48; // RTC pin 
static const uint8_t RTC_INT_PIN = 20; // RTC alarm pin

// data acquistion pins (only 1 sensor connected in this protocol, technically, other SDI sensors can be connected to A8-A15).
static const uint8_t POWERPIN_A15 = 40;
static const uint8_t POWERPIN_A14 = 47; //Currently we are using A14 for CTD sensor
static const uint8_t POWERPIN_A13 = 46;
static const uint8_t POWERPIN_A12 = 45;

static const uint8_t POWERPIN_A11 = 44;
static const uint8_t POWERPIN_A10 = 43;
static const uint8_t POWERPIN_A9 = 42;
static const uint8_t POWERPIN_A8 = 41;


///////////// set alarm
DS1306 rtc; // create a new object to interact with the RTC
volatile ds1306time time;    
// volatile because they are modified in the interrupt
volatile bool alarm_enabled = false;

/////////// VOIDS  ////////////////////////////////////////////////////////////////////////////////////////////////////

//Turn SPI ON & OFF
void spi_sleep()
{
    pinMode(SD_CS_PIN, INPUT);
    digitalWrite(SD_CS_PIN, 1);
    pinMode(RTC_CS_PIN, INPUT);
    pinMode(SPI_MOSI_PIN, INPUT);
    pinMode(SPI_SCK_PIN, INPUT);
}

void spi_wake()
{
    digitalWrite(SD_CS_PIN, 1);
    pinMode(SD_CS_PIN, OUTPUT);
    pinMode(RTC_CS_PIN, OUTPUT);
    pinMode(SPI_MOSI_PIN, OUTPUT);
    pinMode(SPI_SCK_PIN, OUTPUT);
    power_spi_enable();
}


//Turn Sensor ON/OFF
void sensor14_sleep()
{
    digitalWrite(POWERPIN_A14, 0);
    pinMode(POWERPIN_A14, INPUT);
}

void sensor14_wake()
{
    pinMode(POWERPIN_A14, OUTPUT);
    digitalWrite(POWERPIN_A14, 1);
}


// Set Alarm
void rtc_set_alarm(bool init = false)
{
    ds1306alarm alarm;
    rtc.getAlarm(0, &alarm);
        
    alarm.dow = DS1306_ANY;
    alarm.hours = DS1306_ANY;
    alarm.seconds = 00;
    if (init)
    {
        ds1306time* time_p = (ds1306time*) &time;
        rtc.getTime(time_p);
        alarm.minutes = (time.minutes + SAMPLE_INTERVAL_MINUTES) % 60;
    }
    else
    {
        alarm.minutes = (alarm.minutes + SAMPLE_INTERVAL_MINUTES) % 60;
    }
    rtc.setAlarm(0, &alarm);
    rtc.enableAlarm(0);
    alarm_enabled = true;
}

volatile int counter = 0;

// DS1306 interrupt is connected to digital pin 20 (int.3)
void rtc_int_handler()
{
    ds1306time* time_p = (ds1306time*) &time;
    spi_wake();
    delay(300);
    rtc.disableAlarm(0);
    rtc.getTime(time_p);
    counter++;
    alarm_enabled = false;
}


///////////// SDI-12 Commands - Connect to sensor, measure & send data
/* "0R0!"
   '0' CTD default SDI sensor address is "0" (if there are several sensors connected, different SDI names have to be given, and addressed specifically)
   'R0!' Measure and Send Data -> this gives the 3 values (water depth, temp, EC) as an output
*/

static const String myCommand = "0R0!";  

///////////// Read and Log Data
void
read_and_log_data()
{
    String dataString;
    dataString += time.day;
    dataString += "/";
    dataString += time.month;
    dataString += "/";
    dataString += time.year; 
    dataString += " ";
    dataString += time.hours; 
    dataString += ":";
    dataString += time.minutes;
    dataString += ":";
    dataString += time.seconds;
    dataString += " ";

    sensor14_wake();
    delay(300);

    SDI12 sdi12_a14(A14); 
    sdi12_a14.begin(); 
    sdi12_a14.sendCommand(myCommand); 
    delay(300);                     // wait a while for a response
    while(sdi12_a14.available())
    {   
        dataString += (char) sdi12_a14.read();
    }
    sdi12_a14.end();

    sensor14_sleep();
   
    Serial.begin(115200);
    Serial.println(dataString); //print output to monitor

    ////// log data
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.

    //Adds date: ddmmyy + ID as file name for .txt file  
    //IMPORTANT: ID cannot be longer then 2 characters, as total character space is only 8 characters. 
    String filename ;
    filename += time.day;
    filename += time.month;
    filename += time.year; 
    
    filename += "X1.txt"; //This can be used to indicate site ID. IMPORTANT: Site ID cannot be longer than 2 characters, otherwise no data will be recorded. 
    File dataFile = SD.open(filename.c_str(), FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile) 
    {
        dataFile.println(dataString);
        dataFile.close();
    }  
    // if the file isn't open, pop up an error:
    else 
    {
        Serial.println("error opening datalog.txt");
    } 
    delay(100);
    Serial.end();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////// VOID SETUP
void setup()
{
    Serial.begin(115200); //set serial monitor to 115200
    Serial.println("Starting");
    delay(100);
    
  // RTC
    rtc.init(RTC_CS_PIN);
    pinMode(RTC_INT_PIN, INPUT);
    attachInterrupt(3, rtc_int_handler, LOW);
    Serial.println("RTC Init done");
    
    SD.begin(SD_CS_PIN);
    Serial.println("SD Init done");

    delay(100);
    Serial.end();
    rtc_set_alarm(true);
    
}
    


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////// VOID LOOP
void loop()
{
      Serial.begin(115200);
      Serial.println("Sleeping");
      delay(100);
    Serial.end();
    
    // shutdown until ds1306 alarm expires
    spi_sleep();
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 

     // restart alarm once isr is done
    while (alarm_enabled);
    rtc_set_alarm();

    // just woke up, need to log data
    read_and_log_data();

    // usb serial port
    Serial.begin(115200);
      Serial.print("Woke at ");
      Serial.print(time.hours);
      Serial.print(":");
      Serial.print(time.minutes);
      Serial.print(":");
      Serial.print(time.seconds);
      Serial.print(" ");
      Serial.println(counter);
      delay(100);
    Serial.end();
}

