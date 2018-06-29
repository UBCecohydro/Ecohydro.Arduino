# Arduino Sketch Descriptions
## EC-GPS Sketches Created by Teddy Eyster Msc 2018

EC_GPS_v01.ino:

    This is the sketch to run the EC-GPS Logger funded by Mitacs and created by Teddy Eyster and Tom Keddie for the Decolonizing Water Project
    The sketch allows the user to set the time interval at which the logger functions. At this specified interval,
      the sketch collects GPS information from an adafruit featherwing, EC and Temp from a GS3 (via SDI21), and writes them to an SD Card. 

EC_GPS_debog_v01.ino:

    This sketch does the same things as "EC_GPS_v01.ino" but allows the user to view data through the serial terminal. 

SDI12_debug_v01.ino:

    This sketch allows the user to troubleshoot GS3 data through the serial terminal. 
