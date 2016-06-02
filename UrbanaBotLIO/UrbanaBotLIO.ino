/***************************************************************************
                UrbanPotager Project 
Components :
- Arduino Mega + Breadboard (or PCB)
- TFT Display 1'8 (see #define TFTDISPLAY)
- Tiny RTC 
- DHT22 Temperature / Humidity sensor (on D8)
- DS18S20 Water Temperature sensor
- Basic Photo Resistor (on A1)
- MOQ-7 Gas Sensor (on D12/D13)
*/

/****** DISPLAY Definition **************************/
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
// TFT display and SD card will share the hardware SPI interface. Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno, Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK)
#define SPI_SCK 52
#define SPI_DI  50
#define SPI_DO  51
#define TFT_CS  10  // Chip select line for TFT display
#define TFT_DC   9  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)
#define SD_CS    4  // Chip select line for SD card
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, SPI_DO, SPI_SCK, TFT_RST)
  
/****** ETHERNET Definition **************************/
#include <SPI.h>
#include <Ethernet.h>
EthernetClient client;
byte mac[] = {0x90, 0xAD, 0xDA, 0x0D, 0x96, 0xFE };
byte myIP[] = { 192, 168, 0, 51 };   
byte myDNS[] = { 192, 168, 0, 254 };   
byte myGATEWAY[] = { 192, 168, 0, 254 };
byte mySUBNET[] = { 255, 255, 255, 0 };
byte webServerIP[] = { 82,245,102,86 };      // Static Public IP
//byte webServerIP[] = { 192, 168, 0, 37 };  // Local IP
char webServerName[] = "nesko.no-ip.org";    // Web Server URL
unsigned long webserverInterval = 900;    // interval at which to refresh (seconds) --> 15mn
unsigned long webserverLastUpdate = 0;    // last refresh timestamp (seconds) 


/****** RTC Libraries & Variables ******************/
#include <RTClib.h>          // RTC library
#include <Wire.h>            // linked to RTC
RTC_DS1307 RTC;
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif


/****** DHT22 & LIGHT Variables ******************/
#include <DHT22.h>           // DHT22 Library

int tempPin=7;      // Digital D8  
int lightPin = 1;   // Analog A1 
DHT22 myDHT22(tempPin);

long longLightReading=0;
long longTempVal=0;
long longHumiVal=0;
int intGasVal=0;
float waterLevel = 0; // in %

String lightVal=doubleToString(0,0);
String lightEst="";
String tempVal=doubleToString(0,0);
String humiVal=doubleToString(0,0);
String gasVal="N/A";
String waterVal=doubleToString(0,0);

String myTimeOld="";
String lightValOld=doubleToString(0,0);
String lightEstOld=doubleToString(0,0);
String tempValOld=doubleToString(0,0);
String humiValOld=doubleToString(0,0);
String gasValOld=doubleToString(0,0);
String waterValOld=doubleToString(0,0);

unsigned long tempSensorInterval = 5;  // interval for data refresh (seconds) --> 5s
unsigned long DHT22LastUpdate = 0;

/********** Utilisation du capteur Ultrason HC-SR04 *******************/
int trig = 13; 
int echo = 6; 
long lecture_echo; 
long cm;

/********** Utilisation du Flotteur à Niveau **************************/
int levelPin = 3; // un fil sur A3 et un fil sur -

/********** Transistor NPN for LED LIGHTS / PUMP **********************/
int pinLight = 5;
int pinPump = 2;

/********** MOQ-7 Library & Variables **********************/

#include <CS_MQ7.h>          // MQ7 Gas Sensor Library
CS_MQ7 MQ7(12, 13);  // 12 = digital Pin connected to "tog" from sensor board / 13 = digital Pin connected to LED Power Indicator
int CoSensorOutput = 0; //analog Pin connected to "out" from sensor board
int sensorValue;
unsigned long gasSensorInterval= 10000; // interval for data refresh
unsigned long gasLastUpdate=0;


/********* LED STATUS Variables ******************/
const int RED_PIN = 44;
const int GREEN_PIN = 45;
const int BLUE_PIN = 46;
unsigned long time; 
int brightness = 0 ;
int fadeAmount = 5;
unsigned long ledFadeDelay = 30;   // 20 milis
unsigned long ledBlinkDelay = 500; // milis
int ledBlinkState = LOW;
unsigned long ledLastUpdate = 0;
int generalStatus = 1; // 1 = Ok (green) / 2 = Watering (blue) / 3 = Info (orange) / 4 = Warning (red)

/********* GENERAL STATUS Variables **************/
boolean bIsWatering = false;
boolean bIsLightsOn = false;
boolean bIsWaterLow = false;
String detailledStatus;

/****** REFRESH SCREEN Variables *****************/
unsigned long screenInterval = 10;    // interval at which to refresh (seconds) --> 10 s
unsigned long screenLastUpdate = 0;    // last refresh timestamp (seconds) 
int currentScreenID = 0; 
boolean bTime = false;

/****** TESTING Variables *******************/
unsigned long testInterval = 30;    // interval at which to refresh (seconds) --> 30 s
unsigned long testLastUpdate = 0;  
int testNewScreen = 1;

int startLight = 8;       // Heure de DEBUT lumière (8 = allumage à 8:00)
int endLight = 20;        // Heure de FIN lumière (22 = extinction à 22:00)
int minTempValue = 16;    // Température MINIMUM avant alerte
int maxTempValue = 28;    // Température MAXIMUM avant alerte
int minHumiValue = 20;    // Taux d'humidité MINIMUM avant alerte
int maxHumiValue = 70;    // Taux d'humidité MAXIMUM avant alerte
int minWaterLevel = 20;   // Niveau d'eau MINIMUM avant alerte
int minLightValue = 200;  // Valeur MINIMUM de la luminosité avant alerte
int baseLightValue = 850;  // Valeur de BASE pour la limonisté (si < alors on allume les lumières)
long waterTankMax = 14;   // 14 cm Hauteur MAXIMUM niveau eau
long waterTankMin = 4;    // 4 cm Hauteur MINIMUM niveau eau
long waterTankZ = 16;     // 16 cm hauteur du réservoir
long waterTankX = 20;     // 20 cm longeur du réservoir
long waterTankY = 20;     // 20 cm largeur du réservoir
  
int pumpPowerValue = 200;               // Puissance de la pompe 0 --> 255
unsigned long wateringInterval = 120;    // interval between 2 watering in seconds (defaut = 12h = 43200 seconds)
unsigned long wateringDuration = 20;    // watering duration in seconds (defaut = 30 seconds)
unsigned long wateringLastUpdate = 0;  
unsigned long wateringLastStart = 0;  


int mode = 0;  // 0 = Prod / 1 = Demo (Screens)
boolean bNetwork = false; // 


/****************************************************************
  SETUP 
****************************************************************/
void setup(void) {
  Serial.begin(9600);
  // Pump Relay
  //pinMode(pinPump, OUTPUT);  
  // Water Level
  pinMode(trig, OUTPUT); 
  digitalWrite(trig, LOW); 
  pinMode(echo, INPUT); 
  // RGB LED
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  // RELAY 
  pinMode(pinLight, OUTPUT); 
  Wire.begin();
  RTC.begin();
  RTC.adjust(DateTime(__DATE__, __TIME__));
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  displayInit();
  if (bNetwork==true) {connectToWebServer();}
  DateTime now = RTC.now();
  long time = now.unixtime(); // seconds
  wateringLastUpdate = time;  
  wateringLastStart = time;  
  delay(3000);
}

/*****************************************************************************
  ! LOOP !
  Rafraichissement de l'écran : 10s (10000)
  Rafraichissement des sensors : 20s (20000)
*****************************************************************************/
void loop() {
  int value;
  // CHANGE THE MAX TEMP / MAX HUMID / LOW WATER 
  if (Serial.available() > 0) {
    value = Serial.read() - '0';
    Serial.print("I received value; : ");
    Serial.println(value, DEC);
    switch (value) {
      case 1:
        pumpPowerValue = 255;
        Serial.println("Pump to Max (255)");
        break;
      case 2:
        pumpPowerValue = 200;
        Serial.println("Pump to 200");
        break;
      case 3:
        pumpPowerValue = 150;
        Serial.println("Pump to 150");
        break;
      case 4:
        pumpPowerValue = 100;
        Serial.println("Pump to 100");
        break;
      case 5:
        pumpPowerValue = 50;
        Serial.println("Pump to 50");
        break;
    }
      
  }
  
  getSensorsValues();            // UPDATE Light / Temp / Humidity / Water Level values
  getGasSensor();                // UPDATE Gas value

  if (mode == 0) {
    getGeneralStatus();          // Based on all sensors values, determine the General Status
    manageDayLight();            // Based on Settings & RTC, swith High / Low lights 
    manageWatering();            // Based on Settings & RTC, swith on / off water pump
  }

  //displayScreen(testNewScreen);  
  displayScreen(generalStatus);  // Based on GeneralStatus, display the appropriate screen + infos
  displayLedStatus();            // Based on GeneralStatus, display the appropriare RGB animation
  if (bNetwork==true) {sendDataToWebServer();}         // Send data to WebServer (every X seconds)
  
  // ***** TEST SCREENS for DEMO Mode ******
  if (mode == 1) {
    //unsigned long time;            // Variable pour Millis()
    //time = millis();
    DateTime now = RTC.now();
    long time = now.unixtime(); // seconds
    
    if ((time - testLastUpdate) <=  testInterval) {
        //Serial.println("screenLastUpdate: No changes");
     } 
     else {
        generalStatus++;
        detailledStatus = "Sample Text Demo!";
        if (generalStatus == 5) {
          generalStatus=1;
        }  
        Serial.println("generalStatus = "+String(generalStatus));
        testLastUpdate = time;
     }
  }
  // ***** End TEST SCREENS ******
}


/*****************************************************************************
 UPDATE GENERAL STATUS based on Sensors Values
*****************************************************************************/
void getGeneralStatus() {
  generalStatus = 1;
  
  if (bIsWaterLow == true) {
    generalStatus = 4;
    detailledStatus = " ! Refill Now ! ";    
    return;
  }
  
  if (longTempVal>maxTempValue) {
    generalStatus = 3;
    detailledStatus = " Temp too High! ";
    return;
  }
  
  if (longTempVal < minTempValue) {
    generalStatus = 3;
    detailledStatus = " Temp too Low!  ";
    return;
  }
  if (longHumiVal>maxHumiValue) {
    generalStatus = 3;
    detailledStatus = "Humidty too High!";
    return;
  }
  
  if (longHumiVal < minHumiValue) {
    generalStatus = 3;
    detailledStatus = "Humidity too Low!";
    return;
  }
  
  if (bIsWatering == true) {
    generalStatus = 2;
    detailledStatus = "Watering in Progress!";
    return;
  } 
  //bIsLightsOn = false;
  //bIsWaterLow = false; 
}


/****************************************************************
       DISPLAY TFT Screens 
****************************************************************/
void displayScreen(int screenID) {
  boolean updateNow = false;
  boolean bCleanAll=false;
  //unsigned long time;            // Variable pour Millis()
  //time = millis();
  DateTime now = RTC.now();
  long time = now.unixtime(); // seconds
  if (screenID == currentScreenID)  {
    //Serial.println("Same screen !");
    if ((time - screenLastUpdate) <=  screenInterval) {
      //Serial.println("screenLastUpdate: No changes");
      updateNow=false;
      return;
    } 
    else {
      updateNow=true;
    }
  }
  else
  {
    updateNow = true;
    bCleanAll = true;
  }

  if (updateNow=true) {
    displayScreenTFT(screenID);
  }
} 


/****************************************************************
       DISPLAY TFT Screens 
****************************************************************/
void displayScreenTFT(int screenID) {
  DateTime now = RTC.now();
  String myDate;
  String myTime;
  long time = now.unixtime(); // seconds
  
  if (now.day() < 10) { myDate ="0"+ String(now.day())+"/";} else {myDate = String(now.day())+"/";}             // correct date if necessary
  if (now.month() < 10) { myDate =myDate+"0"+ String(now.month())+"/";} else {myDate = myDate+String(now.month())+"/";}             // correct date if necessary
  myDate = myDate+String(now.year());
  
  if (now.hour() < 10) { myTime ="0"+ String(now.hour())+":";} else {myTime = String(now.hour())+":";}             // correct date if necessary
  if (now.minute() < 10) { myTime =myTime+"0"+ String(now.minute());} else {myTime = myTime+String(now.minute());}             // correct date if necessary

  switch (screenID) {
      case 1:
        // SENSORS Screen (TEST)
        //generalStatus = 1;
        if (screenID == currentScreenID) {
          //currentScreenID = screenID;
          // Meme ecran : On efface les datas en écrivant la meme chose en blanc / puis on recharge
          tft.setTextColor(tft.Color565( 0x45, 0xBF, 0x55));
          tft.setCursor(25, 5);
          tft.setTextSize(2);
          tft.println(tempValOld); //tempVal
          tft.setTextSize(1);
          tft.println("c       ");
          tft.print(myTimeOld);
          tft.setCursor(25, 38);
          tft.setTextSize(2);
          tft.println(humiValOld);//humiVal
          tft.setTextSize(1);
          tft.print("%");
          tft.setCursor(25, 70);
          tft.setTextSize(2);
          tft.println(lightValOld);   
          tft.setCursor(25, 104);
          tft.setTextSize(2);
          tft.print(gasValOld);  
          tft.setTextSize(1);
          tft.println("ppm");
          // On ré-écrit les nouvelles valeurs 
          tft.setTextColor(ST7735_WHITE);
          tft.setCursor(25, 5);
          tft.setTextSize(2);
          tft.print(tempVal);
          tft.setTextSize(1);
          tft.print("c       ");
          tft.print(myTime);
          tft.setCursor(25, 38);
          tft.setTextSize(2);
          tft.print(humiVal);
          tft.setTextSize(1);
          tft.print("%");
          tft.setCursor(25, 70);
          tft.setTextSize(2);
          tft.print(lightVal);  
          tft.setCursor(25, 104);
          tft.setTextSize(2);
          tft.print(gasVal);  
          tft.setTextSize(1);
          tft.println("ppm"); 
          
          myTimeOld = myTime;
          tempValOld = tempVal;
          humiValOld = humiVal;
          lightValOld =lightVal;
          lightEstOld = lightEst;
          gasValOld = gasVal;
          currentScreenID = screenID;
        }
        else
        {  
          //Serial.println("case 2: _ if NOT (screenID == currentScreenID) {");
          currentScreenID = screenID;
          tft.fillScreen(tft.Color565( 0x45, 0xBF, 0x55));
          bmpDraw("ICONS1G.BMP", 0, 0);
          delay(150);
          tft.setTextColor(ST7735_WHITE);
          tft.setCursor(25, 5);
          tft.setTextSize(2);
          tft.print(tempVal);
          tft.setTextSize(1);
          tft.print("c       ");
          tft.print(myTime);
          tft.setCursor(25, 38);
          tft.setTextSize(2);
          tft.print(humiVal);
          tft.setTextSize(1);
          tft.print("%");
          tft.setCursor(25, 70);
          tft.setTextSize(2);
          tft.print(lightVal);  
          tft.setCursor(25, 104);
          tft.setTextSize(2);
          tft.print(gasVal);  
          tft.setTextSize(1);
          tft.println("ppm"); 
          tempValOld = tempVal;
          humiValOld = humiVal;
          lightValOld =lightVal; 
          lightEstOld = lightEst;
          gasValOld = gasVal;
        } 
        screenLastUpdate = time;
        break;
      
      case 2:
        // TIME & WATERING Screen (TEST)
        tft.fillScreen(tft.Color565( 0x00, 0x00, 0xFF)); // Bleu 0000FF
        tft.fillRoundRect( 14,8, 132, 20, 4, tft.Color565( 0x00, 0x00, 0xFF) ); // Bleu 0000FF
        tft.drawRoundRect( 14,8, 132, 20, 4, ST7735_WHITE); // Gris Foncé
        tft.setTextColor(ST7735_WHITE);
        tft.setCursor(29, 14);
        tft.setTextColor(ST7735_WHITE);
        tft.print("    WATERING");
        
        tft.setCursor(50, 50);
        tft.setTextSize(2);
        tft.println(myTime); 
        tft.setCursor(50, 75);
        tft.setTextSize(1);
        tft.println(myDate); 
        currentScreenID = screenID;
        screenLastUpdate = time;
        break;
      
      case 3:
        // INFO Screen (TEST)
        //generalStatus = 3;
        tft.fillScreen(tft.Color565(0xF7, 0x93, 0x1E)); // Orange F7931E
        tft.fillRoundRect( 14,8, 132, 20, 4, tft.Color565(0xF7, 0x93, 0x1E)); // Orange F7931E
        tft.drawRoundRect( 14,8, 132, 20, 4, ST7735_WHITE); 
        tft.setTextColor(ST7735_WHITE);
        tft.setCursor(29, 14);
        tft.print("   NEXT ACTIONS");
        tft.setCursor(50, 65);
        tft.setTextSize(1);
        tft.setCursor(14, 58);
        tft.print(detailledStatus);
        /*tft.setCursor(14, 78);
        tft.print("- Nutriments (5d)");
        tft.setCursor(14, 98);
        tft.print("- Harvest (10d)"); */ 
        currentScreenID = screenID;
        screenLastUpdate = time;
        break;
        
      case 4:
        // WARNING Screen (TEST)
        //generalStatus = 4;
        tft.fillScreen(tft.Color565( 0xFF, 0, 0) ); // Rouge
        tft.fillRoundRect( 14,8, 132, 20, 4, tft.Color565( 0xFF, 0, 0) ); // Rouge
        tft.drawRoundRect( 14,8, 132, 20, 4, ST7735_WHITE); 
        tft.setTextColor(ST7735_WHITE);
        tft.setCursor(29, 14);
        tft.print("    WARNING");
        tft.setCursor(50, 65);
        tft.setTextSize(1);
        tft.setCursor(14, 58);
        tft.print("Please proceed a water");
        tft.setCursor(14, 78);
        tft.print("refill NOW ! ");
        currentScreenID = screenID;
        screenLastUpdate = time;
        break;
    }
}



/**********************************************************************
             Light / Temp / Humidity
**********************************************************************/
void getSensorsValues() { 
  int err;
  //unsigned long time;            // Variable pour Millis()
  //time = millis();
  DateTime now = RTC.now();
  long time = now.unixtime(); // seconds
  
  if ((tempVal==0)&&(humiVal==0)) {
    DHT22LastUpdate = time;
    Serial.print("FIRST DHT22LastUpdate: ");
    Serial.println(DHT22LastUpdate);       
  }  
  else { 
    if ((time - DHT22LastUpdate) <=  tempSensorInterval)  {
      //Serial.println("DHT22LastUpdate: No changes");
      return;
    } 
    else {
        DHT22LastUpdate = time;
    }
  }  
  
  // Display Temperature in C
  longLightReading = analogRead(lightPin);
 
  DHT22_ERROR_t errorCode;
  errorCode = myDHT22.readData();

  switch(errorCode)
  {
    case DHT_ERROR_NONE:
      longTempVal = myDHT22.getTemperatureC();
      longHumiVal = myDHT22.getHumidity();
      break;
      
    case DHT_ERROR_CHECKSUM:
      Serial.print("check sum error ");
      Serial.print(myDHT22.getTemperatureC());
      Serial.print("C ");
      Serial.print(myDHT22.getHumidity());
      Serial.println("%");
      break;
      
    case DHT_BUS_HUNG:
      Serial.println("BUS Hung ");
      break;
      
    case DHT_ERROR_NOT_PRESENT:
      Serial.println("Not Present ");
      break;
      
    case DHT_ERROR_ACK_TOO_LONG:
      Serial.println("ACK time out ");
      break;
      
    case DHT_ERROR_SYNC_TIMEOUT:
      Serial.println("Sync Timeout ");
      break;
      
    case DHT_ERROR_DATA_TIMEOUT:
      Serial.println("Data Timeout ");
      break;
      
    case DHT_ERROR_TOOQUICK:
      Serial.println("Polled to quick ");
      break;
  }
  
 
  // WATER LEVEL
  int levelReading = analogRead(levelPin);
  //Serial.print("levelReading=");
  //Serial.println(levelReading); 
  if (levelReading==LOW) 
  {
    Serial.println("MAX LEVEL");        // TO DO : Managee MAX Water Level
  }
  else
  {
    Serial.print("WATER LEVEL OK");
  }    
  
  long cm = getWaterLevel();
  Serial.print("waterTank cm=");
  Serial.println(cm);
  if (cm >= waterTankZ-waterTankMin) {
    bIsWaterLow=true;
  }
  waterLevel = (waterTankMax-cm);
  //Serial.print("(waterTankMax-cm)=");
  //Serial.println(waterLevel);
  waterLevel = (waterLevel/waterTankMax);
  //Serial.print("(waterLevel/waterTankMax)=");
  //Serial.println(waterLevel);
  waterLevel = (waterLevel * 100);
  //Serial.print("waterLevel * 100");
  //Serial.println(waterLevel);
  
  if (waterLevel > 100) {
    waterLevel = 100;
  } 
  if (waterLevel < 20) {
    bIsWaterLow = true;
  } else {
    bIsWaterLow = false;
  }
  Serial.print(" / waterLevel =");
  Serial.print(waterLevel);
  
  /*waterTankMax = 14; // 14 cm Hauteur max niveau eau
  waterTankMin = 4; // 4 cm Hauteur min niveau eau
  waterTankZ = 16;    // 16 cm hauteur 
  */
  lightVal = doubleToString(longLightReading, 0);
  tempVal = doubleToString(longTempVal, 0);
  humiVal = doubleToString(longHumiVal, 0);
  waterVal = doubleToString(waterLevel, 0);
  
  Serial.println("lightVal = "+lightVal);
  Serial.println("tempVal = "+tempVal);
  Serial.println("humiVal = "+humiVal);
  Serial.println("waterlevel = "+waterVal);
  // We'll have a few threshholds, qualitatively determined
  if (longLightReading < 10) {
    lightEst = "Night";
  } else if (longLightReading < 200) {
    lightEst = "Dim";
  } else if (longLightReading < 500) {
    lightEst = "Light";
  } else if (longLightReading < 800) {
    lightEst = "Bright";
  } else {
    lightEst = "Very Bright";
  }
}



/**********************************************************************
             GAS Sensor / CO2
**********************************************************************/
void getGasSensor() { 
  //unsigned long time;            // Variable pour Millis()
  //time = millis();
  DateTime now = RTC.now();
  long time = now.unixtime(); // seconds
  if (gasVal==0) {
    gasLastUpdate = time;
    //Serial.print("FIRST gasLastUpdate: ");
    //Serial.println(gasLastUpdate);       
  }  
  else { 
    if ((time - gasLastUpdate) <=  gasSensorInterval)  {
      return;
    } 
    else {
      gasLastUpdate = time;
    }
  }  
    MQ7.CoPwrCycler();  
    
  if(MQ7.CurrentState() == LOW){   //we are at 1.4v, read sensor data!
    intGasVal = analogRead(CoSensorOutput);
    gasVal = doubleToString(intGasVal, 0);
  }
  else{  
    //Serial.println("CO-Gas Concentration = N/A");
  }
  
}

/*****************************************************************************
 Display Status via a RGB LED
 1 = Ok (green) - 2 = Watering (blue) / 3 = Info (orange) - 4 = Warning (red) 
*****************************************************************************/
void displayLedStatus () {
  time = millis();
  if (generalStatus==4) {
    if ((time - ledLastUpdate) <=  ledBlinkDelay)  {  
      return;
    } 
      else {
        ledLastUpdate = millis();
      }
  }
  else {
    if ((time - ledLastUpdate) <=  ledFadeDelay)  {  
      return;
    } 
    else {
      ledLastUpdate = millis();
    }
  }
  
  switch (generalStatus) {
    case 1: // Green (Everything OK)
    
      analogWrite(RED_PIN, LOW);
      analogWrite(GREEN_PIN, brightness);
      analogWrite(BLUE_PIN, LOW);
      //fadeAmount = 5;
      brightness = brightness + fadeAmount;
      if (brightness <= 0 || (brightness+ fadeAmount) >= 200) {
        fadeAmount = - fadeAmount ; // roll over the brightness between the highest and lowest
      }
      break;
      
    case 2: // Blue (Watering)
      analogWrite(RED_PIN, LOW);
      analogWrite(GREEN_PIN, LOW);
      analogWrite(BLUE_PIN, brightness);
      brightness = brightness + fadeAmount;
      if (brightness <= 0 || (brightness+ fadeAmount) >= 200) {
        fadeAmount = - fadeAmount ; // roll over the brightness between the highest and lowest
      }
      break;
    
    case 3: // Orange (To do in 2 days)
      analogWrite(RED_PIN, 200);
      analogWrite(GREEN_PIN, brightness);
      analogWrite(BLUE_PIN, 0);
 
      brightness = brightness + fadeAmount;
      if (brightness <= 0 || (brightness+ fadeAmount) >= 200) {
        fadeAmount = - fadeAmount ; // roll over the brightness between the highest and lowest
      }
      break;
      
    case 4: // Red  (To do Now !)
      // if the LED is off turn it on and vice-versa:
      if (ledBlinkState == LOW)
        ledBlinkState = HIGH;
      else
        ledBlinkState = LOW;
      // set the LED with the ledState of the variable:
      digitalWrite(RED_PIN, ledBlinkState);
      analogWrite(GREEN_PIN, LOW);
      analogWrite(BLUE_PIN, LOW);
      break;
      
    default: // Red 
      // if the LED is off turn it on and vice-versa:
      if (ledBlinkState == LOW)
        ledBlinkState = HIGH;
      else
        ledBlinkState = LOW;
      // set the LED with the ledState of the variable:
      digitalWrite(RED_PIN, ledBlinkState);
      analogWrite(GREEN_PIN, LOW);
      analogWrite(BLUE_PIN, LOW);
      break;
      
   }
}
// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.
#define BUFFPIXEL 20

/*****************************************************************************
 Draw BMP file from SD Card to TFT 
*****************************************************************************/
void bmpDraw(char *filename, uint8_t x, uint8_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
      bmpFile.close();
    Serial.print("File not found");
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print("Image size: ");
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.Color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print("Loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println("BMP format not recognized.");
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}


/****************************************************************
 Rounds down (via intermediary integer conversion truncation)
****************************************************************/
String doubleToString(double input,int decimalPlaces)
{
  if(decimalPlaces!=0){
  String string = String((int)(input*pow(10,decimalPlaces)));
  if(abs(input)<1)
    {
    if(input>0)
      string = "0"+string;
    else if(input<0)
      string = string.substring(0,1)+"0"+string.substring(1);
    }
    return string.substring(0,string.length()-decimalPlaces)+"."+string.substring(string.length()-decimalPlaces);
  }
  else 
  {
    return String((int)input);
  }
}

/****************************************************************
 DISPLAY Initialization
****************************************************************/
void displayInit() {
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }
  Serial.println("OK!");
  bmpDraw("LOGOSM~1.BMP", 29, 23);
  delay(3000);
}
  

/****************************************************************
 MANAGE WATERING (based on Intervalls)
****************************************************************/
void manageWatering() {
  //unsigned long time;            // Variable pour Millis()
  //time = millis();
  DateTime now = RTC.now();
  long time = now.unixtime(); // seconds
  
  if (bIsWatering==true) {
    if ((time - wateringLastStart) <=  wateringDuration) {
      //Serial.println("Watering in progress, not finished");
      return; // Watering in progress, not finished
    } else {
      // STOP WATERING  
      Serial.println("STOP WATERING");
      //digitalWrite(pinPump, LOW);
      analogWrite(pinPump, 0);
      bIsWatering = false;
      wateringLastUpdate = time;
    }
      
  } else {
    if ((time - wateringLastUpdate) <=  wateringInterval) {
      //Serial.println("NOT Time for WATERING: No changes");
    } 
    else {
      bIsWatering = true;
      Serial.println("START WATERING");
      wateringLastStart = time;
      analogWrite(pinPump, pumpPowerValue);
     }
  }
}
  

/****************************************************************
 GET WATER LEVEL (based on distance)
****************************************************************/
long getWaterLevel(){
  digitalWrite(trig, HIGH); 
  delayMicroseconds(10); 
  digitalWrite(trig, LOW); 
  lecture_echo = pulseIn(echo, HIGH); 
  //Serial.print("lecture_echo = "); 
  //Serial.print(lecture_echo); 
  long cm = lecture_echo / 58.00; 
  Serial.print("Distance en cm : "); 
  Serial.println(cm);
  return cm; 
  /*delay(1000);
  float value = (cm/16)*100;
  Serial.print(" / Level : "); 
  Serial.println(value);  
  return value;*/
}  

/****************************************************************
 MANAGE Day Light (based on RTC)
****************************************************************/
void manageDayLight() {
  DateTime now = RTC.now();
  int myHour = now.hour();
  //Serial.print("myHour = ");
  //Serial.println(myHour);
  if ((myHour >= startLight ) && (myHour < endLight)) { 
    if (longLightReading < baseLightValue) {
      digitalWrite(pinLight,LOW); // Turn OFF
    } else {
      digitalWrite(pinLight,HIGH); // Turn OFF
    }
  } else {
    digitalWrite(pinLight,HIGH);  // Turn OFF
  }  
}

/****************************************************************
 CONNECT to Web Server
****************************************************************/
void connectToWebServer() {
  Serial.println("Attempting to get an IP address using DHCP:");
  if (!Ethernet.begin(mac)) {
  //if DHCP fails, start with a hard-coded address:
    Serial.println("failed to get an IP address using DHCP, trying manually --> Ethernet.begin(mac) = NOK");
    Ethernet.begin(mac, myIP, myDNS, myGATEWAY, mySUBNET);
    Serial.println("IP address using fixed IP, DNS, GateWay, Subnet  --> Ethernet.begin(mac, myIP, myDNS, myGATEWAY, mySUBNET) ");
    Serial.print("My IP address:");
    Serial.println(Ethernet.localIP());     
    
  }
  else
  {
    Serial.println("IP address using DHCP received successfuly --> Ethernet.begin(mac) = Ok ");
    Serial.print("My IP address:");
    Serial.println(Ethernet.localIP()); 
  }
  // connect to Server
  delay(1000);
}

/****************************************************************
 SEND DATA to Web Server
****************************************************************/
void sendDataToWebServer() {
  DateTime now = RTC.now();
  long time = now.unixtime(); // seconds
  if ((time - webserverLastUpdate) <=  webserverInterval) {
    return;
  } else {
    // if you get a connection, report back via serial:
    if (client.connect(webServerIP, 80)) 
    {
      /***********************************************
              Sending Data to Web Server
      ************************************************/
      Serial.println("Connected to WebServer ! Making HTTP request...");
      // UrbanaBot/InsertData.php?airTemp=23.54&airHumidity=34&airQuality=6&lightValue=917&waterLevel=8&waterTemp=22.16&nutriValue=8
      client.print("GET /UrbanaBot/InsertData.php?airTemp=");
      client.print(tempVal);
      client.print("&airHumidity=");
      client.print(humiVal); 
      client.print("&airQuality=");
      client.print(7); 
      client.print("&lightValue=");
      client.print(lightVal); 
      client.print("&waterLevel=");
      client.print(waterVal); 
      client.print("&waterTemp=");
      client.print(tempVal); // temporaire
      client.print("&nutriValue=");
      client.print(8); 
      client.println(" HTTP/1.1");
      client.println("Host: nesko-no-ip.org");
      client.println("Connection: close");                             
      client.println();
      client.stop();
      Serial.println("HTTP request passed ! Connection closed");
    } 
    else 
    {
      // kf you didn't get a connection to the server:
      Serial.println("Connection to Web Server failed");
    }
    // note the time of this connect attempt:
    webserverLastUpdate = time;
  }
}  
