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

const unsigned long writeInterval = 1200000 ; //v 20 minutes in miliseconds
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
  
    
    // Connect to RTC
    Wire.begin();  
    if (!RTC.begin()) {
        Serial.println("RTC failed");
    } else {
        Serial.println("RTC OK");
        RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Sync RTC time to compile time
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
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(437.f); // this value is obtained by calibrating the scale with known weights
    delay(2000);
}

void loop() {
    static float errorValue = 1017+480.04;
    static float targetWeight = 0;
    static float currentWeight = 0;
    static bool buttonPressed = false;
    static float remainingWeight = 0;
    static bool buttonState = false;
    static bool lastButtonState = false;

    int value = digitalRead(buttonPin);
    buttonState = (value == 0); // Button pressed when LOW

    if (buttonState && !lastButtonState) {
        buttonPressed = true; // Set flag when button is pressed
        targetWeight = 1000.0 +(1000-currentWeight); // Set target weight to 1000 when button is pressed
        Serial.print("buttonPressed: ");
        Serial.println(targetWeight);
    }

    lastButtonState = buttonState; // Update lastButtonState
    currentWeight = scale.get_units(5) - errorValue;
    if (buttonPressed) {
        remainingWeight = targetWeight - (1000 - currentWeight);
      } else {
        remainingWeight = currentWeight; // Show current weight when button hasn't been pressed
      }
    Serial.print("Measured Weight:");
    Serial.println(remainingWeight);
    // Fetch the time
    DateTime now = RTC.now();


    // Create the filename using the current date
    String filename = "";
    filename += String(now.day(), DEC);
    filename += String(now.month(), DEC);
    filename += String(now.year(), DEC) + ".csv";
    //Serial.print("Generated Filename: ");
    //Serial.println(filename);  // Print the full filename to the serial monitor

    
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
        logfile = SD.open(filename, FILE_WRITE); // Open the file for writing
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