/*
      Bedini Auto Tuner - Cap Charger v 1
      by Goa Lobaugh - aka liquidbuddha
      
      Arduin CAP Discharger -  Module 4  , for use with 3-pole monopole kit

      *  Arduino Digital Output - CAP 132
      *  Arduino Digital Output - LED 12
   
      *  License:
      *  This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as 
      *  published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
      *  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
      *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.  
      *  <http://www.gnu.org/licenses/>
      *
      *  This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License. 
      *  To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or
      *  send a letter to Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
      
   */  
 
int led = 12;
int CAP = 13;
int DischargeTime = 01;   // in millis
int WaitTime = 499;      // in millis


void setup() {                
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);   
  pinMode(CAP, OUTPUT);   
  
}

// simply dump the Cap, periods determined by WaitTime, duration by DischargeTime

void loop() {
  // turn them ON, CAP in Dump state
  digitalWrite(CAP, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)

  delay(DischargeTime);               // wait for a second

  // turn them OFF, CAP in charge state
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(CAP, LOW);    // turn the LED off by making the voltage LOW

  delay(WaitTime);               // wait for a second
  Serial.println ("..discharge the cap...");
}
