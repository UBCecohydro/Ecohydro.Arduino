/*
 * File                 EcohydroLogger_set_time.ino
 *
 * Synopsis             This sketch allows to set the DS1306 Real Time Clock, implemented on the Ecohydro Logger.
 * 
 * Board                Arduino Mega 2560 with Ecohydro Shield V1
 *
 * Author               Silja Hund, Tom Keddie
 *
 * Attribution          Script parts taken from ds1306_example from ds1306.h library (Chris Bearman)
 *
 *
 * Version              1.0
 *
 * License              This software is released under the terms of the Mozilla Public License (MPL) version 2.0
 *                      Full details of licensing terms can be found in the "LICENSE" file, distributed with this code
 *
 * Instructions         Connect Ecohydro Logger to computer. Set time in code to approximately 2 minutes ahead of current time. Upload program.
 *                      Wait to connect to serial monitor until exact time (that was added in code) is reached, then connect to serial monitor and time will be set & displayed.
 *                      No other steps need to be taken to set the time. Now the logging sketch (EcohydroLogger_ProgramMonitoring.ino) can be uploaded to start data recording.
 */

//libraries
#include <DS1306.h>
#include <SPI.h>

//Set Pin for RTC connection: RTC connected to Pin 48 in Ecohydro Logger
static const uint8_t RTC_CS_PIN = 48;

// Create a new object to interact with the RTC
DS1306 rtc;


//////VOID SETUP ////
void setup()
{
  // Initialize serial monitor for 115200 baud
  Serial.begin(115200);
  
  // Initialize RTC
     rtc.init(RTC_CS_PIN); 
     
    Serial.println("RTC Init done");
    delay(100);
    
  //Set time on clock to current time
 ds1306time time;
 
  time.day = 29; //dd 
  time.month = 2; //mm
  time.year = yy; //yy - only 2 digits
  time.hours = hh; //hh
  time.minutes =18; //min
  time.seconds = 30; // ss
 
  rtc.setTime(&time);
}

void loop()
{
  // Get the time
  ds1306time time;
  rtc.getTime(&time);
  
  //Print to Serial Monitor to check
      
  Serial.print(time.day);
      Serial.print("/");
      Serial.print(time.month);
      Serial.print("/");
      Serial.print(time.year);
      Serial.print(" ");
  
  Serial.print(time.hours);
      Serial.print(":");
      Serial.print(time.minutes);
      Serial.print(":");
      Serial.print(time.seconds);
      Serial.print(" ");
  
  // Wait for a second before allowing loop to restart
  delay(1000);
  
}



