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
const int buttonPin = 7; // digital input pin 7 for button
bool buttonPressed = false;
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
String header = "date,time,remainingWeight";

const unsigned long writeInterval = 1200000 ; // 20 minutes in milliseconds
unsigned long lastWriteTime = 0; // Store the last write time

void error(char *str) {
    Serial.print("error: ");
    Serial.println(str);
    while (1); // Loop forever on error
}

void setup() {
    Serial.begin(9600);
    Serial.println("Serial OK");

    pinMode(buttonPin, INPUT);

    // Initialize the SD card
    Serial.print("Initializing SD card...");
    pinMode(SD_CS, OUTPUT);
    
    // see if the card is present and can be initialized:
    if (!SD.begin(SD_CS)) {
        error("Card failed, or not present");
    }
    Serial.println("card initialized.");
    
    // Create a new file
    char filename[] = "LOGGER00.CSV";
    for (uint8_t i = 0; i < 100; i++) {
        filename[6] = i / 10 + '0';
        filename[7] = i % 10 + '0';
        if (!SD.exists(filename)) {
            // only open a new file if it doesn't exist
            logfile = SD.open(filename, FILE_WRITE); 
            break;  // leave the loop!
        }
    }
    
    if (!logfile) {
        error("couldn't create file");
    }
    
    Serial.print("Logging to: ");
    Serial.println(filename);

    // Connect to RTC
    Wire.begin();  
    if (!RTC.begin()) {
        Serial.println("RTC failed");
    } else {
        Serial.println("RTC OK");
    }

    lcd.begin();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(1, 0);
    lcd.print("DataLogger");
    lcd.setCursor(1, 1);
    lcd.print("(c) ChB 2024");

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

void loop() {
    static float errorValue = 480;
    static float targetWeight = 0;
    static float currentWeight = 0;
    static float remainingWeight = 0;
    static bool buttonState = false;
    static bool lastButtonState = false;

    int value = digitalRead(buttonPin);
    buttonState = (value == 0); // Button pressed when LOW

    if (buttonState && !lastButtonState) {
        targetWeight = 1000.0 + abs(currentWeight); // Set target weight to 1000 when button is pressed
        Serial.print("buttonPressed: ");
        Serial.println(targetWeight);
    }

    lastButtonState = buttonState; // Update lastButtonState
    currentWeight = scale.get_units(5) - errorValue;
    remainingWeight = targetWeight - currentWeight;

  Serial.print("ValueButton:");
  Serial.println(value);

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
    
    // Print time to Serial
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());

    // Display time on LCD
    lcd.setCursor(0, 1);
    lcd.print(now.hour());
    lcd.print(":");
    lcd.print(now.minute());
    lcd.print(":");
    lcd.print(now.second());

    lcd.setCursor(9, 1);
    lcd.print("       ");
    lcd.setCursor(10, 1);
    lcd.print(remainingWeight, 1);

    // Check if 20 minutes have passed
    unsigned long currentMillis = millis();
    if (currentMillis - lastWriteTime >= writeInterval) {
        // Write to SD card
        Serial.println("WritingData to SD Card:");
        logfile = SD.open("LOGGER00.CSV", FILE_WRITE);
        if (logfile) {
            logfile.print(now.hour());
            logfile.print(":");
            logfile.print(now.minute());
            logfile.print(":");
            logfile.print(now.second());
            logfile.print(",");
            logfile.print(remainingWeight);
            logfile.println();
            logfile.close(); // Close the file
            Serial.println("Data written to file.");
        } else {
            Serial.println("Error opening file!");
        }
        lastWriteTime = currentMillis; // Update the last write time
    }

    delay(3000); // Delay for readability
}