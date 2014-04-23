
/*
   Bedini Auto Tuner - User Interface v 4.2
   by Goa Lobaugh - aka liquidbuddha

   Arduino / Bedini Pulse Controller, for use with 3-pole monopole kit
   Replacing the control POT circuit with Arduino. Circuit changes include 3 Motorola H11D2 optocouplers, 
   to isolate the Arduinos from the potential high-volgates.
  
   System includes (1) Master Controller w/ UI, and a (1) SLAVE which is the pulse controller and sensor.
   
   This sketch is the MASTER. It controls the 
       User Interface via JS, LCD, @ two buttons
       AutoTune dataTable @ algorithm
       communications with the SLAVE 

   I2C is used for 2-way communications between Ardiuno
   Tx:   Changes to Threshold & Current, via input from the Joystick & Autotuner Data Table lookups
   Rx:   Coil sense data & other values from Slave
  
   Hardware in system: 
     Arduino Uno
     LCD Display (from seeed studio)
     2-axis Joystick (from parallax)
     Optical Tachometer (IR LED and IR phototransistor)
     (2) buttons
     Parallax 2-axis Joystick: (http://learn.parallax.com/KickStart/27800)
   
   *  Arduino analog input 4 - I2C SDA
   *  Arduino analog input 5 - I2C SCL

*/
 
// ************************************************************************************************

// Includes
#include <Wire.h>
#include <EasyTransferI2C.h>
#include <SoftwareSerial.h>


// PIN LAYOUT
  int LCDTxPin           = 8;            //  LCD transfer pin
  int UDPin              = A1;           //  Joystick Up & Down  
  int LRPin              = A2;           //  Joystick Right & Left
  int UpdateButton       = 12;           //  Button to send updates to slave
  int HoldButton         = 13;           //  button to suspend automatic updates from Joystick
  int GRNLed             = 11;           //  Green ON means we're in AUTO mode
  int REDLed             = 10;           //  Ren ON means we're in Manual mode
  int IRLed              = 9;            //  *** Keep in mind the IR Phototransistor must operate from D2 ***
                                         //  ***        which is the 0 interrupt, on Arduino Uno          ***
                                         
// ************************************************************************************************

// VARIABLES  - Startup DEFAULTS
  int ThresholdDefault      = 100 ;      //  sets starting variable for the Threshold value =  coil sensitivity
  int CurrentDefault        = 0 ;        //  sets Default for CURRENT, which is the pulse length (to COILs)
  int ThresholdLowerLimit   = 01;        //  sets lower limit of the Threshold
  int CurrentLowerLimit     = 10;        //  sets lower limit for the Current pulse length
  int MasterUIRefreshRate   = 750;       //  delay in microseconds before the LCD is refreshed.
  int BatManRefreshRate     = 2500;      //  delay in microseconds before the LCD is refreshed.
  int NumberBlades          = 4;         //  number of counts in each full rotation (ie # fan blades)
  boolean AutoTuneState     = 1;         //  set the initial state of the AutoTuner ( ON = 1, OFF = 0) 
  boolean BattState         = 0;      
// ************************************************************************************************
 
// VARIALBLES used by sketch
  int   Threshold     = ThresholdDefault;  // varaible used to track the Theshold, Tx to slave
  int   CurrentValue  = CurrentDefault;    // var used to track Current, Tx to slave
  int   coilValue     = 0;                 // Rx from Slave
  int   PPM           = 0;                 // Rx from Slave
  long  EndTime ;                          // EndTime to find time delta
  int   RPMCounter ;                       // used to calculate the RPM
  int   RPM ;                              // used for RPM
  int   CycleCounter  = 0;
  int   UD ;
  int   LR ;
  boolean LCDBrightness = false;
  

// Communications
  
  SoftwareSerial lcdSerial     = SoftwareSerial(255, LCDTxPin);                      // RX, TX  for the LCD display
//  SoftwareSerial SlaveSerial   = SoftwareSerial(EtSlvRxPin, EtSlvTxPin);           // RX, TX for the SLAVE SoftEastTransfer
//  SoftwareSerial BatManSerial  = SoftwareSerial(EtBatManRxPin, EtBatManTxPin);     // RX, TX for the Batt Manager SoftEastTransfer

  EasyTransferI2C SlaveIn,  SlaveOut, BatManIn, BatManOut;                           //create the two ET objects
  
  struct Slave_RECEIVE_DATA_STRUCTURE{
        int etThreshold;        // data to receive - MUST BE EXACTLY THE SAME ON OTHER ARDUINO
        int etCoilValue;
        int etCurrentValue;
        int etRPM;
        };
  
  struct Slave_SEND_DATA_STRUCTURE{
        int etThreshold;        // data to send - MUST BE EXACTLY THE SAME ON OTHER ARDUINO
        int etCoilValue;
        int etCurrentValue;
        int etRPM;
        };
               
  struct BatMan_RECEIVE_DATA_STRUCTURE{
        int etThreshold;        // data to receive - MUST BE EXACTLY THE SAME ON OTHER ARDUINO
        int etCoilValue;
        int etCurrentValue;
        int etRPM;
        int etBattState;
        };
  
  struct BatMan_SEND_DATA_STRUCTURE{
        int etThreshold;        // data to send - MUST BE EXACTLY THE SAME ON OTHER ARDUINO
        int etCoilValue;
        int etCurrentValue;
        int etRPM;
        int etBattState;
        };

//give a name to the group of data
  Slave_RECEIVE_DATA_STRUCTURE Srxdata;
  Slave_SEND_DATA_STRUCTURE Stxdata;
  BatMan_RECEIVE_DATA_STRUCTURE BMrxdata;
  BatMan_SEND_DATA_STRUCTURE BMtxdata;

#define I2C_SLAVE  9
#define I2C_Master 10
#define I2C_BATMan 11

// ************************************************************************************************
// ************************************************************************************************
// ************************************************************************************************

void setup() {
    Serial.begin(57600);                       // activate the serial port for monitoring
    Serial.println("Starting up..");

  // setup the pin outs and startup the comms
    pinMode(LCDTxPin, OUTPUT);                 // sets LCDTxPin as an OUTPUT
    digitalWrite(LCDTxPin, HIGH);              // sets LCD as pullup resistor as HIGH
    
    pinMode(GRNLed, OUTPUT);                   // sets the pin for the Green Led
    digitalWrite(GRNLed, HIGH);                // turn it ON
    pinMode(REDLed, OUTPUT);                   // sets the pin for the RED Led
    digitalWrite(REDLed, HIGH);                // turn it ON
    pinMode(IRLed, OUTPUT);                    // sets the pin for the IR Led
    digitalWrite(IRLed, HIGH);                 // turn it ON
    
    pinMode(UpdateButton, INPUT);              // sets the Update button pin to INPUT
    digitalWrite(UpdateButton, HIGH);          // enable the pull-up resistor
    pinMode(HoldButton, INPUT);                // sets the HOLD button pin to INPUT
    digitalWrite(UpdateButton, HIGH);          // enable the pull-up resistor
    
    lcdSerial.begin(9600);                     // activate the Serial connection, sets baud rate
        
    Wire.begin(I2C_Master);                    // Startup the I2C comms
    Wire.onReceive(receiveEvent);              //define handler function on receiving data

    SlaveIn.begin(details(Srxdata), &Wire);    // start ET library, pass data details and serial port.
    SlaveOut.begin(details(Stxdata), &Wire);
    
    BatManIn.begin(details(BMrxdata), &Wire);    // start ET library, pass data details and serial port.
    BatManOut.begin(details(BMtxdata), &Wire);
    
    Serial.println("Comms complete");

    attachInterrupt(0, RPMCountIt, FALLING);   // Tacholmeter Interrupt 0 is digital pin 2, 
                                               // so that is where the IR detector is connected
                                               // Triggers on FALLING (change from HIGH to LOW) (could be either)
    RPMCounter = 0;
    RPM = 0;
    EndTime = 0;

    SplashScreen();                            // call the SplashScreen, intro for LCD instructions
    UpdateLEDs(LOW, LOW);
    
    Serial.println(" END OF SETUP >>> ");              

}
// ************************************************************************************************
// ************************************************************************************************
// ************************************************************************************************

void loop() {                                

    delay(MasterUIRefreshRate);
    Serial.println(".Loop.");

    detachInterrupt(0);                          //  stop the interrupt to run the RPM code
  
    // RPM Calculation:
    float TimeDelta = millis() - EndTime;
    RPM = (((RPMCounter/NumberBlades)/TimeDelta)*60000);
    EndTime = millis();

    ReadJoystick();
    
    if(!digitalRead(UpdateButton))              // if the UPDATE button is pressed
      {   ForceUpdateSlave();   }
    
    if(!digitalRead(HoldButton))                // if the HOLD button is pressed
      {   HoldPress();          }
    
    if(SlaveIn.receiveData())                   // if a SLAVE sent us an update, update the variables
      {                    
        Serial.println(".........received from slave......");
        coilValue     =   Srxdata.etCoilValue;    
        CycleCounter  =   Srxdata.etCurrentValue;
//        BattState     =   rxdata.etBattState;
        if ( AutoTuneState = false ) Threshold = Srxdata.etThreshold;
      }
  
    CalculateCurrent(CycleCounter);
    DisplayInfo();                              // display the results
    RPMCounter = 0;

    attachInterrupt(0, RPMCountIt, FALLING);    // Restart the interrupt processing

}

// ************************************************************************************************
// ************************************************************************************************
// ************************************************************************************************
void receiveEvent(int numBytes) {
  Serial.println(".receive.");
}
// ************************************************************************************************

void CalculateCurrent(double RawValue){
  Serial.println(".CalculateCurrent.");

  double period = 10000;        // defined by the UIRefreshRate of Slave, NOT milliseconds, but the cycle number before updates
  CurrentValue =  period / RawValue ;
  
}

// ************************************************************************************************

void ForceUpdateSlave(){
      Serial.println(".Force Update Slave.");
      Stxdata.etThreshold = Threshold;
      SlaveOut.sendData(I2C_SLAVE);                            //  send that data out
      UpdateMessage();
      Serial.print("******* UPDATE SLAvE ********");    
}
// ************************************************************************************************

void UpdateBattManager (){
   Serial.println(".Update Batt Manager.");

      BMtxdata.etThreshold = Threshold;
      BMtxdata.etRPM = RPM;
      BMtxdata.etCurrentValue = CurrentValue;
   Serial.print("..sending Data..");    
      BatManOut.sendData(I2C_BATMan);                           //  send that data out
   Serial.println("UPDATED BattManager");    
}

// ************************************************************************************************

void RPMCountIt (){                              // this is called when the interupt is triggered
  RPMCounter++;    
} 

// ************************************************************************************************

void HoldPress() {                               // switch is pressed - pullup keeps pin high normally
  Serial.println(".HoldPress.");
    delay(100);                                    // delay to debounce switch
    LCDBrightness = !LCDBrightness;
    if (LCDBrightness == false )     lcdSerial.write(17);                       // Turn backlight OFF
    if (LCDBrightness == true )      lcdSerial.write(18);                       // Turn backlight OFF
}


// ************************************************************************************************

void UpdateLEDs(boolean GreenState, boolean RedState){
  Serial.println(".UpdateLEDs.");
    digitalWrite(GRNLed, GreenState);       // Green LED is OFF
    digitalWrite(REDLed, RedState  );       // Red LED is ON
}

// ************************************************************************************************
void ReadJoystick(){
//   Serial.println(".ReadJoystick.");
/* 
    This section to read the joystick and adjust Threshold or Current accordinly
    joystick sends a signal from 0-1000, 500 being zero or neutral on the stick
    this approach adjusts the increment depending on how far it is depressed,
    dampened by the factor appropriate for the variable being manipulated. 
    Limiting the trigger-zone by 100, to allow for false reads from the pots.
*/

        UD = analogRead(UDPin);
        LR = analogRead(LRPin);
        
      if (UD < 480)     {
            Threshold = (abs(Threshold + (UD-480)/25))+1; // reduced by factor of 25, +1 to keep it positive 
        }
      else if (UD > 520) {
           Threshold = Threshold + ((UD-520)/25);
        }
     if (LR < 480)     {
           CurrentValue = (abs(CurrentValue + (LR-480)/10))+1;     // reduced by factor of 10, abssolute = keep it positive
        }
      else if (LR > 520) {
           CurrentValue = CurrentValue + ((LR-520)/10);
        }
          
      if (Threshold <=ThresholdLowerLimit) Threshold = ThresholdLowerLimit;  //  LIMITs checks 
      if (Threshold > 1024) Threshold = 1024;
      if (CurrentValue <= CurrentLowerLimit) CurrentValue = CurrentLowerLimit;
}

// ************************************************************************************************

void DisplayInfo() { 
//   Serial.println(".DisplayInfo.");

// update the LCD display with relavant data
         lcdSerial.write(12);                    // clear the display
         delay(15);                              // Required delay
         lcdSerial.print("RPM: ");
         lcdSerial.print(RPM);                   // display the RPM
         lcdSerial.write(13);                    // Form feed
         lcdSerial.print("Th:");            
         lcdSerial.print(Threshold);             // display active Threshold value
         lcdSerial.print("  Cu:");            
         lcdSerial.print(CurrentValue);             // display active Threshold value

  // update the BatMan / webserver
         UpdateBattManager();

  //  activate this segment for logging via the serial port
          Serial.print(" coil:");     
          Serial.print(coilValue);               // display coil value
          Serial.print("   RPM:");
          Serial.print(RPM);                     // display the RPM
          Serial.print("   Cu: ");
          Serial.print(CurrentValue);
          Serial.print("   Th: ");
          Serial.println(Threshold);
}

// ************************************************************************************************

void SplashScreen(){                             // ... Splash screen & user startup instructions ...
//     Serial.println(".SplashScreen.");
      lcdSerial.write(17);                       // Turn backlight on
      lcdSerial.write(12);                       // Clear             
      delay(5);                                  // Required delay
      lcdSerial.print("Bedini 3-pole ");         // print title
      lcdSerial.write(13);                       // Form feed
      lcdSerial.print("Autotuner v2.0");         // print version
      delay(1000);                               // vanity pause
}
  
// ************************************************************************************************
// ************************************************************************************************


void UpdateMessage(){  
//   Serial.println(".UpdateMessage.");

    lcdSerial.write(12);                         // Clear display
    lcdSerial.write(12);             
//    for (int i=0; i <= 1; i++)                   // display flashing spin-up instructions
        {
          delay(10);                       
          lcdSerial.print(" -- UPDATED --");      
          delay(500);
        }
    lcdSerial.write(12);                         // Clear             
    delay(5);                                    // Required delay

}
