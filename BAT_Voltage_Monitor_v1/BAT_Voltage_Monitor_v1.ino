

#include <Wire.h>
#include <SoftwareSerial.h>


float VoltageCharge;
float VoltageSource;
float VoltageCAP;
float VoltageDelta;
float VoltagesStart  [3] = {0,0,0};    // array number should match NumberOfSensors
float VoltagesBank   [3] = {0,0,0};
int NumberOfSensors = 3;

int LED1         = 13;
int LCDTxPin     = 11;            //  LCD transfer pin

SoftwareSerial lcdSerial  = SoftwareSerial(255, LCDTxPin);  // RX, TX  for the LCD display


void setup()
{
  pinMode(LED1,OUTPUT);
  Serial.begin(9600);
  Serial.println("Emartee.Com");
  Serial.println("Voltage: ");
  
  lcdSerial.begin(9600);                     // activate the Serial connection, sets baud rate
  SplashScreen();
  StartUp();

}

void loop()
{

    SensorBankCheck();

    VoltageCharge   = VoltagesBank[0];
    VoltageSource   = VoltagesBank[1];
    VoltageCAP      = VoltagesBank[2];
    VoltageDelta  = VoltageCharge - VoltagesStart[0];
    
    DisplayInfo();

}

void SensorBankCheck(){
    int temp;
    
    for (int i=0; i <= (NumberOfSensors-1); i++ ) {
        temp=analogRead(i);  // read the voltage for SOURCE
        VoltagesBank [i] = temp/40.92;
      }
 
}

void StartUp() {
  
  int temp;
  for (int i=0; i <= NumberOfSensors; i++ ) {
        temp=analogRead(i);  // read the voltage for SOURCE
        VoltagesStart [i] = temp/40.92;
      }
}


void DisplayInfo() { 

  float TempTime = (millis()/1000);
  if (TempTime/60 > 60) {TempTime = TempTime/60/60;} else {TempTime = TempTime/60;}
  
  // update the LCD display with relavant data
           lcdSerial.write(128);
           lcdSerial.print(VoltageCharge);                  
           lcdSerial.write(134);
           lcdSerial.print(VoltageDelta);                   
           lcdSerial.write(148);
           lcdSerial.print(VoltageSource);                   
           lcdSerial.write(154);
           lcdSerial.print(VoltageCAP);                   
           lcdSerial.write(160);
           lcdSerial.print(TempTime);            // display time since start
          delay(15);                                // Required delay
  }

void SplashScreen(){                                // ... Splash screen & user startup instructions ...
       // Serial.println(".SplashScreen.");
 //      lcdSerial.write(128);
        lcdSerial.write(12);                        // Clear             
        lcdSerial.write(17);                        // Turn backlight on
        delay(250);                                 // Required delay
        lcdSerial.write(12);                        // Clear             
        lcdSerial.print("Voltage Sensor ");         // print title
        lcdSerial.write(13);                        // Form feed
        lcdSerial.print("v1.0");                    // print version
        delay(2000);                                // vanity pause
         lcdSerial.write(12);                       // Clear             
        delay(250);                                 // Required delay
 }


