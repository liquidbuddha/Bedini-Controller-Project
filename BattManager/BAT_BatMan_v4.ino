/*
  Bedini Auto Tuner - Battery Manager v 4.0
  by Goa Lobaugh - aka liquidbuddha
  
  Arduino / Bedini Battery Monitor & Controller, for use with 3-pole monopole kit

  I2C is used for 2-way communications between Ardiuno

  Jobs:
    Senses Source Battery value
    Rx data from Master, for Webpaage update
    Rx data from webserver to update Master / Slave
    Tx data to Master, after rx updates
    Tx webpage etc on webrequest
    Swap Batteries via the relay bank  (flip flops SOURCE and CHARGE, 2 battery bank system)
    
  Hardware:
    Arduino Mega 2560
    4-relay  shield
    Ethernet Shield  
    
   *  Arduino analog input 4 - I2C SDA
   *  Arduino analog input 5 - I2C SCL

*/

//************************************************************************************************
// INCLUDES

#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <EasyTransferI2C.h>

// PIN LAYOUT
  unsigned char relayPin[4]  = {31,33,35,37};       //  relay shield pinouts
  int SourceBatt             = A15;                 //  Analog input for SOURCE Bank, referred to as "A" or "0"
  int ChargeBatt             = A13;                 //  Analog input for CHARGE Bank, referred to as "B" or "1"
//  int EtTxPin                = 44;                  //  SoftEasyTransfer pin Tx
//  int EtRxPin                = 45;                  //  SoftEasyTransfer pin Rx
  
// VARIABLES SET BY USER - DEFAULTS
  double SwapDuration = 60;                      //  Duration in # of MINUTES, determining how long one battery cycle will last, before swapping                                            
  
// VARIALBLES used by program

  boolean BattSwapStatus = 0;                      // variable used to track status of battery swap
  char*   MsgState[]={"Bank A", "Bank B"};         // State 0 = source is A, 1 = source is B
  double  BattSwapFullDur = SwapDuration * 60000;  // convertion from millis to minutes
  double  TimeNow = millis();
  double  TimeStop = TimeNow + BattSwapFullDur;
  double  TimetoSwap = (TimeStop - TimeNow)/60000;
  int coilValue;
  int Threshold;
  int RPM;
  int CurrentValue;
  
  float SourceBattvout = 0.0;                    // variables for 
  float SourceBattvin = 0.0;
  float ChargeBattvout = 0.0;
  float ChargeBattvin = 0.0;
  float vdSrcR1  = 4400.0  ;     // !! resistance of R1 measured 4320 Ohms
  float vdSrcR2  = 990.0   ;     // !! resistance of R2 measured 939 Ohms
  float vdChrgR1 = 4400.0  ;     // !! resistance of R1 measured 4320 Ohms
  float vdChrgR2 = 990.0   ;     // !! resistance of R2 measured 990 Ohms
//  double alpha = .9;    // leaky ingtegration method variable, to smooth batt readings
  boolean reading = false;    // used by webpage, input from user


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
   byte mac[] = {0x90,0xA2,0xDA,0x0D,0x14,0x70};    //  MAC Addy here
   IPAddress ip (10,0,1,18);                        //  Set the IP Address HERE

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
  EthernetServer server(80);

// Setup the EasyTranfer structures etc.

  EasyTransferI2C ETin, ETout;                                  //create the two ET objects

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

#define I2C_SLAVE  9
#define I2C_Master 10
#define I2C_BATMan 11


// ************************************************************************************************
// ************************************************************************************************
// ************************************************************************************************

void setup() {

  pinMode(SourceBatt, INPUT);                        // manually setup the Analog pins for Battery monitoring
  pinMode(ChargeBatt, INPUT);

  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  Serial.print(".. starting serial ..");
  
  Ethernet.begin(mac, ip);                    // start the Ethernet connection and the server:
  server.begin();
  Serial.print("server is at ");              
  Serial.println(Ethernet.localIP());

  //start the I2C library, pass in the data details and the name of the serial port. (Can be Serial, Serial1, Serial2, etc.)
  Wire.begin(I2C_BATMan);
  Wire.onReceive(receiveEvent);        //define handler function on receiving data

  ETin.begin(details(rxdata), &Wire);
  ETout.begin(details(txdata), &Wire);

    int i=0;                              // Setup the initial state for the relays, based on defaults
    for(i = 0; i < 4; i++)  {             // set the pins HIGH, Switches to position B, RELAYS OFF
          pinMode(relayPin[i],OUTPUT);
          }
    SetBatts(BattSwapStatus);             // call the Set Battery routine
    
//  analogReference(EXTERNAL);            // sets the analog reference to EXTERNAL, 

}

// *******************************************************************************************************
// *******************************************************************************************************
// ************************************************************************************************

void loop() {

  checkTime();
 
  // ****************  Receive Data 
   if(ETin.receiveData())          // if a someone sent us an update, update the variables
      {   
      Serial.println(" ***   Update received");        
      coilValue = rxdata.etCoilValue;
      Threshold = rxdata.etThreshold;
      RPM = rxdata.etRPM;
      CurrentValue = rxdata.etCurrentValue;
      }
      
 //******************** SERVE WEBPAGE    
   EthernetClient client = server.available();
   if (client) {    ServeWebPage(client);   }
  
} 

//*******************************************************************************************************
// **********************************************
void receiveEvent(int numBytes) {  Serial.println(".receive.");
}
// ************************************************************************************************
// Check if it's time to swap the batteries 
void checkTime(){

  // this is for timing by duration ... ie, swap the battery every x minutes, regardless of battery levels
  TimeNow = millis();                          // get the time
  if ( TimeNow > TimeStop ){                          // check if the time matches the Time to stop
    Serial.println("Swapping the Battery bank ");
    SetBatts(!BattSwapStatus);                                      // call the swap routine
    TimeStop = TimeNow + BattSwapFullDur;             // setup the new time to stop
     }
   TimetoSwap = (TimeStop - TimeNow)/60000;
}

//*******************************************************************************************************
//  Measure and calculate the battery voltages
// **********************************************

void checkBatts(){
  double BattStatSource = 0;
  double BattStatCharge = 0;
  int rawBattStatSource  = analogRead(SourceBatt);
  int rawBattStatCharge  = analogRead(ChargeBatt);
  int BattStatSourceold = 0;
  int BattStatChargeold = 0;
  
//  smoothedBattStatSource = alpha * BattStatSourceold + (1-alpha) * rawBattStatSource;      // leaky integration smoothing
//  smoothedBattStatCharge = alpha * BattStatChargeold + (1-alpha) * rawBattStatCharge;      // leaky integration smoothing
  BattStatSourceold = BattStatSource;
  BattStatChargeold = BattStatCharge;

  SourceBattvout = (rawBattStatSource * 5.0) / 1024.0;
  SourceBattvin = SourceBattvout / (vdSrcR2/(vdSrcR1+vdSrcR2));
/*  Serial.print(rawBattStatSource);
  Serial.print(" Battery A ");
  Serial.print(SourceBattvin);
  Serial.print(" v");
*/
  ChargeBattvout = (rawBattStatCharge * 5.0) / 1024.0;
  ChargeBattvin = ChargeBattvout / (vdChrgR2/(vdChrgR1+vdChrgR2));
/*  Serial.print("   Battery B ");
  Serial.print(ChargeBattvin);
  Serial.println(" v");
*/
}

//*******************************************************************************************************
//  Serve the webpage
// **********************************************

void ServeWebPage(EthernetClient client){
   
    // listen for incoming clients
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    boolean sentHeader = false;
    while (client.connected()) {
      if (client.available()) {
        
      if(!sentHeader){
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          sentHeader = true;
        }

        char c = client.read();
        Serial.write(c);
  
        // check for a command from the user
        if(c == '?') reading = true; //found the ?, begin reading the info
        if(reading){
          //Serial.print("Message Received: ");
          Serial.println(c);
           switch (c) {
            case 'A':
              //add code here
              SetBatts(0);                            // Set the A bank as Source
              delay(100);
              break;
            case 'B':
              //add code here
              SetBatts(1);                            // Set the B bank as Source
              delay(100);
              break;
           }
         }
        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
//          client.println("HTTP/1.1 200 OK");
//          client.println("Content-Type: text/html");
//          client.println("Connnection: close");
//          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
                    // add a meta refresh tag, so the browser pulls again every 15 seconds:
          client.println("<meta http-equiv=\"refresh\" content=\"30\">");
          // WEB PAGE STARTS HERE
          // Header Area

            checkBatts();    // check the battery before we update the page

            client.println("<b>   Bedini System Manager v1.0  </b><br />");         
            client.println("<br />");         
            double RunTime = (millis()/1000/60);
            client.print("Run Time: ");
            client.println(RunTime);
            client.print("<br />  Swap in: ");
            client.print(TimetoSwap);
            client.println(" minutes<br />");
            client.println("<br />");  
            
          // Bank Status Area
            client.print("SOURCE Battery:  ");         
            client.println(MsgState[BattSwapStatus]);
            client.println("<br />");         
            client.println("<br />");        
            
          // Battery Status Area
            client.print("Source Battery : ");         
            client.println(SourceBattvin);
            client.println(" v<br />");         
            client.print("Charge Battery : ");         
            client.println(ChargeBattvin);
            client.println(" v (INOP)<br />");         
            client.println("<br />");         

          // Bedini Status Area
            client.print("RPM : ");         
            client.println(RPM);
            client.println("<br />");         
            client.print("Threshold : ");         
            client.println(Threshold);
            client.println("<br />");         
            client.print("Current : ");         
            client.println(CurrentValue);
            client.println("<br />");         
            client.println("<br />");         


          // Sensory Output Area
          // output the value of each analog input pin
/*          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("Analog Input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");         
          }
*/
          client.println("<br />");         

          client.println("</html>");      // WEB PAGE ENDS HERE
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
 
   
 }

//************************************************************************************************
void SetBatts(boolean BattToSet){
//********************************

    Serial.print(".......................Status  ");
    Serial.print(BattSwapStatus);
    Serial.print(BattToSet);
    int i=0;
    if (BattToSet == 1)  
      {
          for(i = 0; i < 4; i++)  {            // set the pins LOW, Switches to position B - LOW is relay ON
          digitalWrite(relayPin[i],LOW);
           }
          BattSwapStatus = BattToSet;
      }
    else if (BattToSet == 0)
      {
          for(i = 0; i < 4; i++)  {            // set the pins HIGH, Switches to position A
            digitalWrite(relayPin[i],HIGH);
          }
          BattSwapStatus = BattToSet;
      }
    DisplayInfo();    
}

//************************************************************************************************
void DisplayInfo() { 
// ***************************

    Serial.print(" time: ");
    Serial.print(millis()/1000/60);
    Serial.print(" min SS: ");
    Serial.print(BattSwapStatus);
    Serial.print(" SwapStatus: ");
    Serial.println(MsgState[BattSwapStatus]);
}
//************************************************************************************************
