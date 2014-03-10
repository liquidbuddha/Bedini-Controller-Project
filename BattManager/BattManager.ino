/*
  Bedini Auto-tuner Web Server
  Author: Goa Lobaugh

 A web server that shows status and logging of the Bedini 3-pole monopole.
 Arduino Mega using an Arduino Wiznet Ethernet shield. 
 
 Battery Swapper - 2 Batt banks, flip-flop after duration
 
 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional) 

 */
//************************************************************************************************
// PIN LAYOUT
  unsigned char relayPin[4] = {4,5,6,7};       //  relay shield pinouts
  int BattA = 2;                              // Analog input for Battery Bank A
  int BattB = 3;                              // Analog input for Battery Bank B

// VARIABLES SET BY USER - DEFAULTS
  long SwapDuration = 2;                      //  Duration in # of MINUTES, determining how long one battery cycle will last, before swapping                                            
 
// VARIALBLES used by program

  int     BattSwapStatus = 0;                       // variable used to track status of battery swap
  long    BattSwapFullDur = SwapDuration * 60000;  // convertion from millis to minutes
  char*   MsgState[]={" mins  <<<<<<<  Battery Bank A is Source", " mins           Battery Bank B is Source  >>>>>>"};
  long    TimeStop = millis() + BattSwapFullDur;

  float BattAvout = 0.0;
  float BattAvin = 0.0;
  float BattBvout = 0.0;
  float BattBvin = 0.0;
  float R1 = 50000.0;    // !! resistance of R1 !!
  float R2 = 4300.0;     // !! resistance of R2 !!


// Ethernet and webserver setup
   #include <SPI.h>
   #include <Ethernet.h>

// MAC address and IP address for controller.
   byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x14, 0x70 };    //  MAC Addy here
   IPAddress ip(10,0,1,18);                   //  Set the IP Address HERE

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
   EthernetServer server(80);
   EthernetClient client = server.available();

//************************************************************************************************

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  delay(100);

 // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
 // setup the pin outs and startup the comms
    int i;                                    // setup the four Relay switch Pins, pin # set above
        for(i = 0; i < 4; i++)
        { 
          pinMode(relayPin[i],OUTPUT);
        }
  pinMode(BattA, INPUT);
  pinMode(BattB, INPUT);

}

//************************************************************************************************
void loop() {

  // measure and calculate the battery voltages
  
  int BattStatA  = analogRead(BattA);
  int BattStatB  = analogRead(BattB);

  BattAvout = (BattStatA * 5.0) / 1024.0;
  BattAvin = BattAvout / (R2/(R1+R2));
  Serial.print(BattAvin);
  Serial.println(" volts");

  BattBvout = (BattStatB * 5.0) / 1024.0;
  BattBvin = BattBvout / (R2/(R1+R2));
  Serial.print(BattBvin);
  Serial.println(" volts");

  // listen for incoming clients
  if (client) {
  
// Webserver to show battery monitor results, etc

    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<meta http-equiv=\"refresh\" content=\"15\">");         // meta refresh tag, so browser pulls again every 15 seconds:
          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {       // output the value of each analog input pin
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");       
            }
          client.println("</html>");
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
}
  
//************************************************************************************************

void SwapBatts(){
  
    int i=0;
    if (BattSwapStatus == 0)  
      {
          for(i = 0; i < 4; i++)  {            // set the pins HIGH, Switches to position B
          digitalWrite(relayPin[i],HIGH);
           }
          BattSwapStatus = 1;
      }
    else 
      {
          for(i = 0; i < 4; i++)  {            // set the pins LOW, Switches to position A
            digitalWrite(relayPin[i],LOW);
          }
          BattSwapStatus = 0;

      }
    DisplayInfo();    
}

//************************************************************************************************

void DisplayInfo() { 
    Serial.print(" time: ");
    Serial.print(millis()/1000/60);
   // Serial.print(" min SwapStatus: ");
   // Serial.println(BattSwapStatus);
    Serial.println(MsgState[BattSwapStatus]);
}
//************************************************************************************************
