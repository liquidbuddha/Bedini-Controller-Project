/*

next steps... it's bugging out when the update is received when updating the data card. far as I can tell
so, let's try disabling interrupts when logging the data.

-runtime on webpage not reporting beyond decimel

*/


    /*
      Bedini Auto Tuner - Battery Manager v 5
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
 
      * SD card attached to SPI bus as follows:
         ** MOSI - pin 11
         ** MISO - pin 12
         ** CLK - pin 13
         ** CS - pin 4

      *  Arduino analog input 4 - I2C SDA
      *  Arduino analog input 5 - I2C SCL
      *  Arduino Mega analog input 20 - I2C SDA
      *  Arduino Mega analog input 21 - I2C SCL
    
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
    
    //************************************************************************************************
    // INCLUDES
    
    #include <SPI.h>
    #include <Ethernet.h>
    #include <Wire.h>
    #include <EasyTransferI2C.h>
    #include <SD.h>
    
    
    // PIN LAYOUT
      unsigned char relayPin[4]  = {31,33,35,37};       //  relay shield pinouts
      int SourceBatt             = A15;                 //  Analog input for SOURCE Bank, referred to as "A" or "0"
      int ChargeBatt             = A11;                 //  Analog input for CHARGE Bank, referred to as "B" or "1"
      
    // VARIABLES SET BY USER - DEFAULTS
    // ***************************************************
    // ***************************************************
      double SwapDuration   = 0;                       //  Duration in # of MINUTES, how long one battery cycle will last before swapping
                                                       //  *** Leave as ZERO for Swap Disable ***
      double LogDuration    = 1;                      // in Minutes
   
      char LogFileName[] = "newlog.txt";
    // ***************************************************
    // ***************************************************

    // VARIALBLES used by program
    
      boolean BattSwapStatus = 0;                      // variable used to track status of battery swap
      char*   MsgState[]={"Bank A", "Bank B"};         // State 0 = source is A, 1 = source is B
      double  BattSwapFullDur = SwapDuration * 60000;  // convertion from millis to minutes
      double  LogTimeDuration = LogDuration  * 60000;  // convertion from millis to seconds
      
      double  TimeNow = millis();
      double  TimeStop = TimeNow + BattSwapFullDur;
      double  TimeLog = TimeNow + LogTimeDuration;
      double  Time2Log = (TimeLog - TimeNow)/60000;
      double  TimetoSwap = (TimeStop - TimeNow)/60000;
      int coilValue;
      int Threshold;
      int RPM;
      int CurrentValue;
      char Mode;
      
      float SourceBattvout = 0.0;                      // variables for tracking voltages
      float SourceBattvin = 0.0;
      float ChargeBattvout = 0.0;
      float ChargeBattvin = 0.0;
      
      String dataString = "";                          // container to log data to SD card

      // actual resistance measured from the resistoors in the voltage dividers 
      float vdSrcR1  = 4400.0  ;     // !! resistance of R1 measured 4320 Ohms
      float vdSrcR2  = 990.0   ;     // !! resistance of R2 measured 939 Ohms
      float vdChrgR1 = 4400.0  ;     // !! resistance of R1 measured 4320 Ohms
      float vdChrgR2 = 990.0   ;     // !! resistance of R2 measured 990 Ohms
    //  double alpha = .9;    // leaky ingtegration method variable, to smooth batt readings
      boolean reading = false;    // used by webpage, input from user
    
    
    // Enter a MAC address and IP address for your controller below.
    // The IP address will be dependent on your local network:
       byte mac[] = {0x90,0xA2,0xDA,0x0D,0x14,0x70};    //  MAC Addy here
       IPAddress ip (10,0,1,21);                        //  Set the IP Address HERE
    
    // Initialize the Ethernet server library
    // with the IP address and port you want to use 
    // (port 80 is default for HTTP):
      EthernetServer server(80);
    
    // Setup the EasyTranfer structures etc.
    
      EasyTransferI2C ETin;        //create the two ET objects
    
      struct DATA_STRUCTURE{       // data in communicatiosn - MUST BE EXACTLY THE SAME ON THE OTHER ARDUINO
            int etThreshold;
            int etCoilValue;
            int etPulseDelay;
            int etCurrentValue;
            int etRPM;
            int etBattState;
            char etMode;
           };

 
      DATA_STRUCTURE Cmdata;    //give a name to the groups of data
    
    #define I2C_SLAVE  9
    #define I2C_Master 10
    #define I2C_BATMan 11
    
    // On the Ethernet Shield, CS is pin 4. Note that even if it's not
    // used as the CS pin, the hardware CS pin (10 on most Arduino boards,
    // 53 on the Mega) must be left as an output or the SD library
    // functions will not work.

    const int chipSelect = 4;

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
      Wire.onReceive(receiveEvent);           //define handler function on receiving data
      
      ETin.begin(details(Cmdata), &Wire);
    
      // setup the SD card
      Serial.print("Initializing SD card...");
      // make sure that the default chip select pin is set to
      // output, even if you don't use it:
      pinMode(53, OUTPUT);                    //10 for UNO, 53 for Mega (in this case we're using a Mega)
      
      // see if the card is present and can be initialized:
      if (!SD.begin(chipSelect)) {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        //return;
      }
      Serial.println("\nCard initialized.");
    
      int i=0;                                // Setup the initial state for the relays, based on defaults
      for(i = 0; i < 4; i++)  {               // set the pins HIGH, Switches to position B, RELAYS OFF
            pinMode(relayPin[i],OUTPUT);
            }
            
      SetBatts(BattSwapStatus);               // call the Set Battery routine
        
    //  analogReference(EXTERNAL);            // sets the analog reference to EXTERNAL, 
    
    }
    
    // *******************************************************************************************************
    // *******************************************************************************************************
    // *******************************************************************************************************
    
    void loop() {
    
      checkBattSwapTime();
  
  
 //     noInterrupts();  // **** disable interrupts to insure clean write to card
 //     interrupts();  // **** disable interrupts to insure clean write to card

    // ****************  Receive Data 
      if(ETin.receiveData())          // if a someone sent us an update, update the variables
          {   
          Serial.println("Update received");        
          coilValue = Cmdata.etCoilValue;
          Threshold = Cmdata.etThreshold;
          RPM = Cmdata.etRPM;
          CurrentValue = Cmdata.etCurrentValue;
          Mode = Cmdata.etMode;
       // ****************  time to Log the Data ?
      checkDataLogTime();
         }    
         
     //******************** SERVE WEBPAGE    
       EthernetClient client = server.available();
       if (client) {    ServeWebPage(client);   }
      
    } 
    
    //*******************************************************************************************************
    //*******************************************************************************************************
    //*******************************************************************************************************
    void receiveEvent(int numBytes) { }
    
    // ************************************************************************************************
    // Check if it's time to swap the batteries 
    // ************************************************************************************************
   
    void checkBattSwapTime(){
    
      if (SwapDuration != 0 ) {      // check if the swap is on or not
          
      // this is for timing by duration ... ie, swap the battery every x minutes, regardless of battery levels
      TimeNow = millis();                          // get the time
      if ( TimeNow > TimeStop ){                          // check if the time matches the Time to stop
        Serial.println("Swapping the Battery bank ");
        SetBatts(!BattSwapStatus);                                      // call the swap routine
        TimeStop = TimeNow + BattSwapFullDur;             // setup the new time to stop
         }
       TimetoSwap = (TimeStop - TimeNow)/60000;
      }
      
    }
    
    //******************************************************************************************************
    //  check if it's time to log the data
   // ************************************************************************************************
     void checkDataLogTime(){
      
      // this is for timing by duration ... ie, log the data every x minutes
      TimeNow = millis();                          // get the time
      if ( TimeNow > TimeLog ){                          // check if the time matches the Time to stop
        Serial.println("loggin the data ");
        LogTheData();
        TimeLog = TimeNow + LogTimeDuration;
         }
       Time2Log = (TimeLog - TimeNow)/60000;
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
    
                client.println("<b>   Bedini System Manager v1.1  </b><br />");         
                client.println("<br />");         
                double RunTime = (millis()/1000/60);
                client.print("Mode    :");
                client.println(Mode);
                client.print("<br /><br />Run Time    :");
                client.println(RunTime);
                client.print("<br />Swap in :");
                client.print(TimetoSwap);
                client.println(" minutes<br />");
                client.println("<br />");  
                
              // BANK Status Area
                client.print("SOURCE Battery  :");         
                client.println(MsgState[BattSwapStatus]);
                client.println("<br />");         
                client.println("<br />");        
                
              // Battery Status Area
                client.print("Source Battery  : ");         
                client.println(SourceBattvin);
                client.println(" v<br />");         
                client.print("Charge Battery  : ");         
                client.println(ChargeBattvin);
                client.println(" v (INOP)<br />");         
                client.println("<br />");         
    
              // Bedini Status Area
                client.print("  RPM           :");         
                client.println(RPM);
                client.println("<br />");         
                client.print("  Threshold     :");         
                client.println(Threshold);
                client.println("<br />");         
                client.print("  Current       :");         
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
    
        Serial.print("..SetBatts.Status.");
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
    
        Serial.print("\n time: ");
        Serial.print(millis()/1000/60);
        Serial.print(" min SS: ");
        Serial.print(BattSwapStatus);
        Serial.print(" SwapStatus: ");
        Serial.println(MsgState[BattSwapStatus]);
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
      
    //  smoothedBattStatSource = alpha * BattStatSourceold + (1-alpha) * rawBattStatSource; // leaky integration smoothing
    //  smoothedBattStatCharge = alpha * BattStatChargeold + (1-alpha) * rawBattStatCharge; // leaky integration smoothing
      BattStatSourceold = BattStatSource;
      BattStatChargeold = BattStatCharge;
    
      SourceBattvout = (rawBattStatSource * 5.00) / 1024.00;
      SourceBattvin = SourceBattvout / (vdSrcR2/(vdSrcR1+vdSrcR2));
    /*  Serial.print(rawBattStatSource);
      Serial.print(" Battery A ");
      Serial.print(SourceBattvin);
      Serial.print(" v");
    */
      ChargeBattvout = (rawBattStatCharge * 5.00) / 1024.00;
      ChargeBattvin = ChargeBattvout / (vdChrgR2/(vdChrgR1+vdChrgR2));
    /*  Serial.print("   Battery B ");
      Serial.print(ChargeBattvin);
      Serial.println(" v");
    */
  
/*
          Serial.print("Source Battery ");
          Serial.print(String(findDouble(SourceBattvin,2)));
          Serial.println(" v");
          Serial.print("Charge Battery ");
          Serial.print(String(findDouble(ChargeBattvin,2)));
          Serial.println(" v");
*/
    }
    //************************************************************************************************
    
    String findDouble(double val, byte precision){
      
     //           Serial.print("Finding DOUBLE");

      
          // prints val with number of decimal places determine by precision
          // NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
          // example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)
          String data = "";
          
           if(val < 0.0){
//              Serial.print('-');
                data += "-";
             val = -val;
            }
          
//            Serial.print (int(val));  //prints the int part
              data += String(int(val));

            if( precision > 0) {
//              Serial.print(".");  // print the decimal point
                data += ".";
              unsigned long frac;
              unsigned long mult = 1;
              byte padding = precision -1;
              while(precision--)
                mult *=10;
           
              if(val >= 0)
                 frac = (val - int(val)) * mult;
              else
                 frac = (int(val)- val ) * mult;
                  unsigned long frac1 = frac;
                  while( frac1 /= 10 )
                     padding--;
                  while(  padding--)  {
//                  Serial.print("0");
                  data += "0";
                   }
//              Serial.println(frac,DEC) ;
                data += String(frac,DEC);
                 }
                 
        //  Serial.print(data);

          return data;
    }
   //************************************************************************************************

    void LogTheData() {
    //      noInterrupts();  // **** disable interrupts to insure clean write to card

            checkBatts();
        Serial.print("Loggin data: ");

          dataString += String(millis());
          dataString += ","; 
          dataString += String(coilValue);
          dataString += ","; 
          dataString += String(Threshold);
          dataString += ","; 
          dataString += String(RPM);
          dataString += ","; 
          dataString += String(CurrentValue);
          dataString += ","; 
          dataString += String(BattSwapStatus);
          dataString += ","; 

          dataString += String(findDouble(SourceBattvin,2));
          dataString += ";"; 
          dataString += String(findDouble(ChargeBattvin,2));
          dataString += ";"; 
          
          Serial.println(dataString);
          dataString = "";
          

          // open the file. note that only one file can be open at a time,
          // so you have to close this one before opening another.
          File dataFile = SD.open(LogFileName, FILE_WRITE);
        
          // if the file is available, write to it:
          if (dataFile) {
            dataFile.println(dataString);
            dataFile.close();
            // print to the serial port too:
  //          Serial.println(dataString);
            dataString = "";
          }  
          // if the file isn't open, pop up an error:
          else {
            Serial.println("error opening datalog.txt");
            
    //     interrupts();    // **** restore normal interrupts
     
         } 

    
    }
