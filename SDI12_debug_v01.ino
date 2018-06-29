// SDI12_debug_v01.ino
// file to check SDI communication between logger and GS3 sensor.  
// Written by Teddy Eyster Jun 15, 2017

//include libraries
// https://github.com/ManuelJimenezBuendia/Arduino-SDI-12
#include "SDI12_ARM.h"

// Set the pins used on main control board
static const uint16_t pin_redLed = 13;
static const uint16_t pin_greenLed = 8;
static const uint16_t pin_GS3_SDI12 = 11;
static const uint16_t pin_fiveVoltEnable = 12;

// Define if the serial interface should be used
static bool debug = true;

// Create an SDI-12 object and commands; can replace '?' with specific address ie "9"
SDI12 GS3_SDI12(pin_GS3_SDI12);
//static const String myCommandInfo = "?I!"; //"?I!" = get info; returns "913DECAGON GS3 335"
static const String myCommandMeas = "?M!";//"aM!" = take measurement; returns 900139
static const String myCommandData = "?D0!";//"aD0!" = read data; returns "address+dielectirc+temp+EC"

void setup()
{
  pinMode(pin_redLed, OUTPUT);
  pinMode(pin_greenLed, OUTPUT);
  pinMode(pin_fiveVoltEnable, OUTPUT);

  // turn on
  digitalWrite(pin_fiveVoltEnable, 1);//power on power booster

  // USB serial for debug only
  Serial.begin(115200);
  if (debug){
    while (!Serial);} //waits to start 

}

void loop() {
   digitalWrite(pin_greenLed, HIGH); //turn on green light
 
  // Read from decagon
//String infoString; // Uncomment if requesting sensor info below
  String measString;
  String dataString;
  GS3_SDI12.begin();
  
// Uncomment to get sensor address and other info
//  GS3_SDI12.sendCommand(myCommandInfo);
//  delay(300);
//  while (GS3_SDI12.available())
//  {
//    uint8_t info = GS3_SDI12.read();
//    if (info != '\r' && info != '\n')
//      infoString += (char) info;
//  }
//  GS3_SDI12.end();
//  Serial.print("GS3 Info: "); Serial.println(infoString);

  // Request measurements from sensor
  GS3_SDI12.sendCommand(myCommandMeas);// start meas "900139"
  delay(50);//may need to increase to isolate data from meas return
  while (GS3_SDI12.available())
    {GS3_SDI12.read();}
//Uncomment to display meas return
//  {
//    uint8_t meas = GS3_SDI12.read();
//    if (meas != '\r' && meas != '\n')
//      measString += (char) meas;
//  }
//  Serial.print("GS3 Meas: "); Serial.println(measString);

  // Listen to response from sensor
  delay(1000);                     // wait a "001"+0.05 seconds for a response 
  GS3_SDI12.sendCommand(myCommandData);
  delay(50);
  while (GS3_SDI12.available())
  {
    uint8_t data = GS3_SDI12.read();
    if (data != '\r' && data != '\n')
      dataString += (char) data;
  }
  GS3_SDI12.end();
  // Print data to Serial Monitor
  Serial.print("GS3 Data: "); Serial.println(dataString);
   
delay(5000); //10 secs
}
