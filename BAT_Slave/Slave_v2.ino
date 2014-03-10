/*
   Bedini Pulse Controller v 2.3
   by Goa Lobaugh - aka liquidbuddha

   Arduino ~ Bedini Pulse Controller, for use with 3-pole monopole kit, replacing the control POT circuit with Arduino.
   circuit changes include 3 Motorola H11D2 optocouplers.
   
   Sensor coil is wired to analog pin 0 and turning on and off a opto-couplers connected to digital pin 13, 12 & 11. 
   The amount of time the opto-coupler will be on and off depends on the value obtained by analogRead() compated with the 
   Threshold value. Optocouplers act as an safety isolators from the Arduino circuits.  
   Optocouplers are connected to Base and Ground.
   
   EasyTransfer is used for 2-way communications between Ardiuno
   
   Notes on Variables and States of operation:
       cState true  1  is for Automatic CURRENT timing  / Auto
       cState false 0  is for Defined CURRENT timing    / Manual

  */
// ************************************************************************************************
// Includes 
  #include <SoftwareSerial.h>                  // library for the Software ports
  #include <SoftEasyTransfer.h>


// PIN LAYOUT
  int sensorPin = 0;                           //  Sensing Coil Input pin
  int DriveCoil01 = 13;                        //  assigns variable 'DriveCoil01' to pin 13
  int DriveCoil02 = 12;                        //  assigns variable 'DriveCoil01' to pin 13
  int DriveCoil03 = 11;                        //  assigns variable 'DriveCoil01' to pin 13
  int EtTxPin = 7;                             //  SoftEasyTransfer pins
  int EtRxPin = 6;

// ************************************************************************************************
// VARIABLES  - Startup DEFAULTS
  long UIRefreshRate = 10000;                   //  default is 10000 (1 s) variable used to # of itterations before the LCD is refreshed. //  Also used for PPM (# Pulses per minute) calculation

// ************************************************************************************************
 
// VARIALBLES used by program
  int       Threshold = 100 ;                        //  sets variable for the Threshold value, manipulates coil sensitivity)
  int       Current = 0 ;                            //  sets variable for the Current value, which manipulate pulse length to coils in miliseconds
  int       coilValue = 0;                           //  variable to store the value coming from the sensing coil
  int       CheckInCounter = 0;                      //  variable used to track how many cycles before the LCD is refrehed, etc.
  int       CycleCounter = 0;                        //  used to calc RPM
  float     PulseCounter = 0;
  int       coilLast;                              // used to track the coil value between steps
  boolean   cState = true;                         // used to track the state of autotuning.
  boolean   DriveCoilState = false;                 // used to track the state of the coils beyond the loop
  boolean   DriveCoilOldState = DriveCoilState;     // setup to be the same

// Setup the EasyTranfer structures etc.

  SoftwareSerial EtSerial = SoftwareSerial(EtRxPin, EtTxPin);   // RX, TX  setup the EasyTransfer

  SoftEasyTransfer ETin, ETout;                //create the two ET objects

  struct RECEIVE_DATA_STRUCTURE{
    //put your variable definitions here for the data you want to receive //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
      int etThreshold;
      int etCoilValue;
      int etCurrentValue;
      int etRPM;
      int etBattState;
 };
    
  struct SEND_DATA_STRUCTURE{
    //put your variable definitions here for the data you want to receive //THIS MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
      int etThreshold;
      int etCoilValue;
      int etCurrentValue;
      int etRPM;
      int etBattState;
  };

  RECEIVE_DATA_STRUCTURE rxdata;    //give a name to the groups of data
  SEND_DATA_STRUCTURE txdata;

// ************************************************************************************************

void setup() {
    // setup the pin outs and startup the comms
    pinMode(DriveCoil01, OUTPUT);              // sets DriveCoil01 (pin 13) as an OUTPUT
    pinMode(DriveCoil02, OUTPUT);              // sets DriveCoil02 (pin 12) as an OUTPUT
    pinMode(DriveCoil03, OUTPUT);              // sets DriveCoil03 (pin 11) as an OUTPUT

    // setup COMMs
    Serial.begin(57600);                        // activate the Serial connection, sets baud rate
    delay(100);
    
    //start the ET library, pass in the data details and the name of the serial port. (Can be Serial, Serial1, Serial2, etc.)
    EtSerial.begin(57600);
    ETin.begin(details(rxdata), &EtSerial);
    ETout.begin(details(txdata), &EtSerial);
    delay(100);
    
    LogData();                                 // initiate the first call to the Master... 
    
}
// ************************************************************************************************

void loop() {                                
  
  SensorCoilCheck();
 
  if(ETin.receiveData())
    {                       // if the Master has sent us an update, make the changes
      Serial.println(" *********** UPADATE RECEIVED");
      Threshold = rxdata.etThreshold;
    }

}
// ************************************************************************************************

void SensorCoilCheck() {  
  
  
  coilValue = analogRead(sensorPin);          // declares 'coilValue' by the value from the 'coilPin':
  CheckInCounter++;                           // increments LCD refresh counter
  

  if (coilValue >= Threshold   // if coilValue is greater or equal to threshold   &&
      )   {
      digitalWrite(DriveCoil01, HIGH);        // turns ON the DriveCoils
      digitalWrite(DriveCoil02, HIGH);
      digitalWrite(DriveCoil03, HIGH);
      DriveCoilState = true;        
     }
   else                                       // REST the coils
      {
      digitalWrite(DriveCoil01, LOW);         // turn OFF the DriveCoils
      digitalWrite(DriveCoil02, LOW);
      digitalWrite(DriveCoil03, LOW);
      DriveCoilState = false;        
}

  if ( DriveCoilState != DriveCoilOldState) {      // if they DO NOT match then the state has changed, make a note!
       CycleCounter++;                             // increment the counter
       DriveCoilOldState = DriveCoilState ;        // match them up
  }

  
  if (CheckInCounter >= UIRefreshRate) {      // check to see if it's time to Tx to MASTER
      LogData();                              // logo data, and send updates to the MasterController
      CheckInCounter = 0;                     // reset the counter
      CycleCounter = 0;                       // zero the PPM counter
    }
  
}

// ************************************************************************************************

void LogData() { 
   
//    txdata.etThreshold = Threshold;
//    txdata.etCoilValue = coilValue;
    txdata.etCurrentValue = CycleCounter;
    ETout.sendData();                           //then we will go ahead and send that data out
    
//   logging via the serial port
      Serial.print(" c:");     
      Serial.print(coilValue);               // display coil value
      Serial.print(" Th: ");
      Serial.print(Threshold);
      Serial.print("  Cycle Counter:");
      Serial.println(CycleCounter);

}

  

