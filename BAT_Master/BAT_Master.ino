/*
 Bedini 3-Pole monopole AutoTuner v 1.6
 Author: Goa Lobaugh

 Arduino / Bedini Pulse Controller, for use with 3-pole monopole kit
 Replacing the control POT circuit with Arduino. Circuit changes include 3 Motorola H11D2 optocouplers, 
 to isolate the Arduinos from the potential high-volgates.

 System includes (1) Master Controller w/ UI, and a (1) SLAVE which is the pulse controller and sensor.
 
 This sketch is the MASTER. It controls the 
     User Interface via JS, LCD, @ two buttons
     AutoTune dataTable @ algorithm
     communications with the SLAVE 

 Tx:   Changes to Threshold & Current, via input from the Joystick & Autotuner Data Table lookups
 Rx:   Coil sense data & other values from Slaves

 Hardware in system: 
   LCD Display (seed studio)
   2-axis Joystick
   Optical Tachometer (IR led and phototransistor)
   (2) buttons

 SoftEasyTransfer is used for 2-way communication between arduinos.
 Parallax 2-axis Joystick: (http://learn.parallax.com/KickStart/27800)

 */
 
// ************************************************************************************************
// PIN LAYOUT
  int LCDTxPin           = 8;            //  LCD transfer pin
  int EtTxPin            = 7;            //  SoftEasyTransfer pins
  int EtRxPin            = 6;
  int UDPin              = A1;           //  Joystick Up & Down  
  int LRPin              = A2;           //  Joystick Right & Left
  int UpdateButton       = 12;           //  Button to send updates to slave
  int HoldButton         = 13;           //  button to suspend automatic updates from Joystick
  int GRNLed             = 11;
  int REDLed             = 10; 
  int IRLed              = 4;            // Keep in mind the IR Phototransistor will operate from D2, 
                                         // which is the 0 interrupt, or Arduino Uno
// ************************************************************************************************

// Includes
  #include <SoftwareSerial.h>          // for using software ports
  #include <SoftEasyTransfer.h>        // for using for communications between Arduinos

// VARIABLES  - Startup DEFAULTS
  int ThresholdDefault      = 0 ;        //  sets starting variable for the Threshold value =  coil sensitivity
  int CurrentDefault        = 0 ;        //  sets Default for CURRENT, which is the pulse length (to COILs)
  int ThresholdLowerLimit   = 21;        //  sets lower limit of the Threshold
  int CurrentLowerLimit     = 650;       //  sets lower limit for the Current pulse length
  int MasterUIRefreshRate   = 500;       //  delay in microseconds before the LCD is refreshed.
  int NumberBlades          = 4;         //  number of counts in each full rotation (ie # fan blades)
  
  // ************************************************************************************************
 
// VARIALBLES used by sketch
  int Threshold     = ThresholdDefault;  // varaible used to track the Theshold, Tx to slave
  int Current       = CurrentDefault;    // var used to track Current, Tx to slave
  int coilValue     = 0;                 // Rx from Slave
  int PPM           = 0;                 // Rx from Slave
  int CTime         = 0;                 // Rx from Slave

  long  EndTime ;                        // EndTime to find time delta
  int   RPMCounter;                      // used to calculate the RPM
  int   RPM;                             // used for RPM
  
  int CycleCounter  = 0;
  int UD ;
  int LR ;
  boolean HoldState = false;    

// Lookup Table for AutoTuner

  byte xIndex = 0;
  byte yIndex = 0;
  int TotalRows = 17;          // be sure this is the same as the table lookup # of rows

/*
      This data table was derived through experimentation 
      General setup with input of 12v 16aH w/ Source attached
      sensor and drive coil distances of 3 mm each.
      top load coil installed and powering 18 red LEDs
*/
  int ThresholdLookup [20][4] = 
      {
//    {RPMLowerLimit,        RPM Upper limit      Threshold,        Current     },     
      {0,                    250,                 30,               1300        },
      {251,                  1400,                50,               1400        },
      {1401,                 1600,                75,               1300        },
      {1601,                 1800,                100,              1300        },
      {1801,                 2000,                200,              1300        },
      {2001,                 2500,                400,              1300        },
      {2501,                 2800,                300,              1300        },
      {2801,                 3200,                300,              1200        },
      {3201,                 3400,                250,              1200        },
      {3401,                 3600,                225,              1200        },
      {3601,                 3800,                150,              1200        },
      {3801,                 4100,                100,              1300        },
      {4101,                 4200,                75,               1300        },
      {4201,                 4500,                50,               1400        },
      {4501,                 5100,                30,               1350        },
      {5101,                 5400,                30,               1250        },
      {5401,                 5600,                30,               1175        },
      {5601,                 5800,                25,               1150        },
      {5801,                 6000,                24,               1176        },
      };

// Communications
  
  SoftwareSerial lcdSerial = SoftwareSerial(255, LCDTxPin);     // RX, TX  for the LCD display
  SoftwareSerial EtSerial = SoftwareSerial(EtRxPin, EtTxPin);   // RX, TX for the SoftEastTransfer

  SoftEasyTransfer ETin, ETout;                                 //create the two ET objects
  
  struct RECEIVE_DATA_STRUCTURE{
        int etThreshold;        // data to receive - MUST BE EXACTLY THE SAME ON OTHER ARDUINO
        int etCurrent;
        int etCoilValue;
        int etCTime;
        int etPulseCount;
        int etPPM;
  };
  
  struct SEND_DATA_STRUCTURE{
        int etThreshold;        // data to send - MUST BE EXACTLY THE SAME ON OTHER ARDUINO
        int etCurrent;
        int etCoilValue;
        int etCTime;
        int etPulseCount;
        int etPPM;
  };

//give a name to the group of data
RECEIVE_DATA_STRUCTURE rxdata;
SEND_DATA_STRUCTURE txdata;

// ************************************************************************************************
// ************************************************************************************************

void setup() {
    // setup the pin outs and startup the comms
    pinMode(LCDTxPin, OUTPUT);                 // sets LCDTxPin as an OUTPUT
    digitalWrite(LCDTxPin, HIGH);              // sets LCD as pullup resistor as HIGH
    
    pinMode(GRNLed, OUTPUT);                  // sets the pin for the Green Led
    digitalWrite(GRNLed, HIGH);
    pinMode(REDLed, OUTPUT);                   // sets the pin for the RED Led
    digitalWrite(REDLed, LOW);
    pinMode(IRLed, OUTPUT);                    // sets the pin for the IR Led
    digitalWrite(IRLed, HIGH);
    
    pinMode(UpdateButton, INPUT);              // sets the Update button pin to INPUT
    digitalWrite(UpdateButton, HIGH);          // enable the pull-up resistor
    pinMode(HoldButton, INPUT);                // sets the HOLD button pin to INPUT
    digitalWrite(UpdateButton, HIGH);          // enable the pull-up resistor
    
    lcdSerial.begin(9600);                     // activate the Serial connection, sets baud rate
    delay(100); 
    
    Serial.begin(57600);                       // activate the serial port for monitoring

    EtSerial.begin(57600);                     // activate the EasyTranfer software port
    ETin.begin(details(rxdata), &EtSerial);    // start ET library, pass data details and serial port.
    ETout.begin(details(txdata), &EtSerial);
    delay(100);
  
    attachInterrupt(0, RPMCountIt, FALLING);   // Tacholmeter Interrupt 0 is digital pin 2, 
                                               // so that is where the IR detector is connected
                                               // Triggers on FALLING (change from HIGH to LOW)
    RPMCounter = 0;
    RPM = 0;
    EndTime = 0;
    
    SplashScreen();                            // call the SplashScreen, intro for LCD instructions

}
// ************************************************************************************************
// ************************************************************************************************
// ************************************************************************************************

void loop() {                                

    delay(MasterUIRefreshRate);
    
    detachInterrupt(0);                         //  stop the interrupt to run the RPM code

    // RPM Calculation:
    float TimeDelta = millis() - EndTime;
    RPM = (((RPMCounter/NumberBlades)/TimeDelta)*60000);
    EndTime = millis();

    if (RPM == 0) {SpinUpMessage();}            // if the wheel is not spinning, alert the user

        ReadJoystick();                         
        DisplayInfo();                          // display the results
    
    if(!digitalRead(UpdateButton))              // if the UPDATE button is pressed
      {   ForceUpdateSlave();   }
    
    if(!digitalRead(HoldButton))                // if the HOLD button is pressed
      {   HoldPress();          }
    
    if(ETin.receiveData())                      // if the SLAVE sent us an update, update the variables
      {                    
      PPM = rxdata.etPPM ;                      
      CTime = rxdata.etCTime ;
      coilValue = rxdata.etCoilValue;
      }

    RPMCounter = 0;

    attachInterrupt(0, RPMCountIt, FALLING);    // Restart the interrupt processing

}

// ************************************************************************************************
// ************************************************************************************************
// ************************************************************************************************

void AutoTune(){              // use loopup table to match RPM zones. Set appropriate Th and Cu

for (yIndex=0; yIndex<TotalRows; yIndex++){
  
      if (  RPM       >=   ThresholdLookup[yIndex][0]  &&   
            RPM       <    ThresholdLookup[yIndex][1]  &&
            HoldState ==   false                           )
          {
           Threshold = ThresholdLookup[yIndex][2]; 
           Current   = ThresholdLookup[yIndex][3]; 
          }
      }
      
}  

// ************************************************************************************************

void UpdateSlave(){
   if (HoldState == false ) {
      txdata.etThreshold = Threshold;
      txdata.etCurrent = Current;
      ETout.sendData();                            // then go ahead and send that data out
   }
 }
// ************************************************************************************************

void ForceUpdateSlave(){
      txdata.etThreshold = Threshold;
      txdata.etCurrent = Current;
      ETout.sendData();                            //  send that data out
}
// ************************************************************************************************

void RPMCountIt (){ 
  RPMCounter++;    
} 

// ************************************************************************************************

void HoldPress() {                      // switch is pressed - pullup keeps pin high normally
    delay(100);                         // delay to debounce switch
    digitalWrite(GRNLed, HoldState);    // indicate via LED
    HoldState = !HoldState;             // toggle HoldState variable
    digitalWrite(REDLed, HoldState);    // indicate via LED
}

// ************************************************************************************************

void ReadJoystick(){
/* 
    This section to read the joystick and adjust Threshold or Current accordinly
    joystick sends a signal from 0-1000, 500 being zero or neutral on the stick
    this approach adjusts the increment depending on how far it is depressed,
    dampened by the factor appropriate for the variable being manipulated. 
    Limiting the trigger-zone by 100, to allow for false reads from the pots.
*/

        UD = analogRead(UDPin);
        LR = analogRead(LRPin);
        
      if (UD < 450)     {
            Threshold = (abs(Threshold + (UD-450)/25))+1; // reduced by factor of 25, +1 to keep it positive 
            UpdateSlave();
        }
      else if (UD > 550) {
           Threshold = Threshold + ((UD-550)/25);
           UpdateSlave();
        }
       else {
           AutoTune();
           UpdateSlave();
        }
     if (LR < 450)     {
           Current = (abs(Current + (LR-450)/10))+1;     // reduced by factor of 10, abssolute = keep it positive
           UpdateSlave();
        }
      else if (LR > 550) {
           Current = Current + ((LR-550)/10);
           UpdateSlave();
        }
      else {
           AutoTune();
           UpdateSlave();
        }
          
      if (Threshold <=ThresholdLowerLimit) Threshold = ThresholdLowerLimit;  //  LIMITs checks 
      if (Current <= CurrentLowerLimit) Current = CurrentLowerLimit;
}

// ************************************************************************************************

void DisplayInfo() { 

// update the LCD display with relavant data
         lcdSerial.write(12);                    // clear the display
         delay(5);                               // Required delay
         lcdSerial.print("c:");     
         lcdSerial.print(coilValue);             // display coil value
         lcdSerial.print(" rpm:");
         lcdSerial.print(RPM);                   // display the RPM
         lcdSerial.write(13);                    // Form feed
         lcdSerial.print("Th:");            
         lcdSerial.print(Threshold);             // display active Threshold value
         lcdSerial.print(" Cu:");            
         lcdSerial.print(Current);               // displays active Current value

  //  activate this segment for logging via the serial port

          Serial.print("    c:");     
          Serial.print(coilValue);               // display coil value
          Serial.print("   PPM: ");              
          Serial.print(PPM);
          Serial.print("   RPM:");
          Serial.print(RPM);                     // display the RPM
          Serial.print("   Th: ");
          Serial.print(Threshold);
          Serial.print("   C:");            
          Serial.println(Current);               // displays active Current value

}

// ************************************************************************************************

void SplashScreen(){                             // ... Splash screen & user startup instructions ...
      lcdSerial.write(17);                       // Turn backlight on
      lcdSerial.write(12);                       // Clear             
      delay(5);                                  // Required delay
      lcdSerial.print("Bedini 3-pole ");         // print title
      lcdSerial.write(13);                       // Form feed
      lcdSerial.print("Autotuner v1.6");         // print version
      delay(3000);                               // vanity pause

//    lcdSerial.write(18);                       // Turn backlight OFF

}
  
// ************************************************************************************************
// ************************************************************************************************


void SpinUpMessage(){  
    lcdSerial.write(12);                         // Clear display
    lcdSerial.write(12);             
    for (int i=0; i <= 3; i++)                   // display flashing spin-up instructions
        {
          lcdSerial.write(12);                              
          delay(250);                       
          lcdSerial.print(" SPIN UP NOW  ");      
          delay(250);
        }
    lcdSerial.write(12);                         // Clear             
    delay(5);                                    // Required delay

}
