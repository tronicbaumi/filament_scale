/*
 *
 * Copyright (c) 2016 seeed technology inc.
 * Website    : www.seeedstudio.com
 * Author     : Lambor
 * Create Time:
 * Change Log :
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 #include <SPI.h>
#include <SD.h>
#include <Wire.h>

#include "HX711.h"
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 9;
const int LOADCELL_SCK_PIN = 8;
HX711 scale;

#include <LiquidCrystal_I2C.h>
// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
 

#include "RTClib.h"
RTC_DS1307 RTC; // define the Real Time Clock object



// for the data logging shield, we use digital pin 10 for the SD cs line
const int SD_CS = 10;

// the logging file
File logfile;

String CSVline;
String header = "date,time,voltage,temperature,supplyvoltage";

void error(char *str)
{
    Serial.print("error: ");
    Serial.println(str);
    // red LED indicates error
    //digitalWrite(redLEDpin, HIGH);
    while(1);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Serial OK");


   // initialize the SD card
    Serial.print("Initializing SD card...");
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    pinMode(SD_CS, OUTPUT);
    
    // see if the card is present and can be initialized:
    if (!SD.begin(SD_CS)) {
      error("Card failed, or not present");
    }
    Serial.println("card initialized.");
    
    // create a new file
    char filename[] = "LOGGER00.CSV";
    for (uint8_t i = 0; i < 100; i++) {
      filename[6] = i/10 + '0';
      filename[7] = i%10 + '0';
      if (!SD.exists(filename)) {
        // only open a new file if it doesn't exist
        logfile = SD.open(filename, FILE_WRITE); 
        break;  // leave the loop!
      }
    }
    
    if (! logfile) {
      error("couldnt create file");
    }
    
    Serial.print("Logging to: ");
    Serial.println(filename);




      // connect to RTC
    Wire.begin();  
    if (!RTC.begin()) {
      Serial.println("RTC failed");
    }
    else{
      Serial.println("RTC OK");
      RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(1,0);
  lcd.print("DataLogger");
  lcd.setCursor(1,1);
  lcd.print("(c) ChB 2021");

  Serial.println("Initializing the scale");

  // Initialize library with data output pin, clock input pin and gain factor.
  // Channel selection is made by passing the appropriate gain:
  // - With a gain factor of 64 or 128, channel A is selected
  // - With a gain factor of 32, channel B is selected
  // By omitting the gain factor parameter, the library
  // default "128" (Channel A) is used here.
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());			// print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));  	// print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));		// print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);	// print the average of 5 readings from the ADC minus tare weight (not set) divided
						// by the SCALE parameter (not set yet)

  scale.set_scale(437.f);    // this value is obtained by calibrating the scale with known weights; see the README for details
  //scale.tare();				        // reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());                 // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));		// print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
						// by the SCALE parameter set with set_scale

  Serial.println("Readings:");



  delay(2000);
}

void loop()
{


  // Serial.print("one reading:\t");
  // Serial.print(scale.get_units(), 1);
  // Serial.print("\t| average:\t");
  // Serial.println(scale.get_units(10), 1);

  //Serial.print("one reading:\t");
  //Serial.print(scale.get_units(50), 2);
  Serial.print("\t| average:\t");
  Serial.println(scale.get_value(5)/437, 2);

  


  // fetch the time
  DateTime now = RTC.now();
  
 
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());

  lcd.setCursor(0,1);
  lcd.print(now.hour());lcd.print(":");lcd.print(now.minute());lcd.print(":");lcd.print(now.second());

  lcd.setCursor(9,1);
  lcd.print("       ");
  lcd.setCursor(10,1);
  lcd.print(scale.get_units(5)-478.6,1);
  

  delay(3000);
}
