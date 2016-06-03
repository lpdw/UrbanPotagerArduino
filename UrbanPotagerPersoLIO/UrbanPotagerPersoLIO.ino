/*
  Hello World.ino
  2013 Copyright (c) Seeed Technology Inc.  All right reserved.
  Author:Loovee

*/
/***** Own Libs *****/
#include "chardef.h"   
#include <Wire.h>
#include "rgb_lcd.h"

#include "DHT_grove.h"
#define DHTPIN 2     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#define FLOATPIN 8     // what pin we're connected to


#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#define WIFI_AP "LPDW-IOT"
#define WIFI_PASSWORD "LPDWIOTROUTER2015"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.
#define SITE_URL "urbanpotager.labesse.me"
LWiFiClient c;
long webServerDelay = 60000; // 1 Mn
long lastServerUpdate = 0; 
boolean disconnectedMsg = false;

rgb_lcd lcd;
const int colorR = 0;
const int colorG = 0;
const int colorB = 255;
byte celcius[8]		= CELCIUS_ARRAY; 
byte light[8]		= LIGHT_ARRAY;
byte humidity[8]	= HUMIDITY_ARRAY;
byte temp[8]		= TEMP_ARRAY;
byte water1[8]		= WATER_ARRAY;
byte water2[8]		= WATER2_ARRAY;

const int pinLight = A0;

String textTemp;
String textHumid;
String textLight;
String textWater;
 

void setup() 
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);
  lcd.setCursor(0, 0);
  // Print a message to the LCD.
  lcd.print("UrbanPotager LIO");
  Serial.println("UrbanPotager LIO");
  delay(1000);
  
// keep retrying until connected to website
  LWiFi.begin();
  
  // keep retrying until connected to AP
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    delay(1000);
  }
  pinMode(FLOATPIN,INPUT_PULLUP);  
  // set up the LCD's number of columns and rows:

  lcd.createChar(0, celcius);
  lcd.createChar(1, light);
  lcd.createChar(2, humidity);
  lcd.createChar(3, temp);
  lcd.createChar(4, water1);
  lcd.createChar(5, water2);

  dht.begin();
  lcd.clear();
}

void loop() 
{
 String waterLevel;
 float t = 0.0;
 float h = 0.0;
 int value = map(analogRead(pinLight),0,1023,0,100);
 if(dht.readHT(&t, &h))
  {
    int temp = t;
    int humid = h;
    int waterSwitch = digitalRead(FLOATPIN);
    switch (digitalRead(FLOATPIN)) {
      case HIGH:
        waterLevel = "HIGH";
        lcd.setRGB(100, 100, 255);
        break;
      case LOW:
        waterLevel = "LOW";
        lcd.setRGB(200, 0, 0);
        break;
      default: 
        waterLevel = "LOW";
        break;
        
    } 
    
    textTemp = doubleToString(temp,0);
    textHumid = doubleToString(humid,0);
    textLight = doubleToString(value,0);
    textWater = doubleToString(waterSwitch,0);
        
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(byte(3));  // Temp char    
    lcd.setCursor(1, 0);
    lcd.print(":");
    lcd.setCursor(2, 0);
    lcd.print(textTemp);
    lcd.setCursor(4, 0);
    lcd.write(byte(0));  // Celcius char
    lcd.setCursor(10, 0);    
    lcd.write(byte(2));  // Celcius char
    lcd.setCursor(11, 0);    
    lcd.print(":");
    lcd.setCursor(12, 0);
    lcd.print(textHumid);   
    lcd.setCursor(14, 0); 
    lcd.print("%");
    
    lcd.setCursor(0, 1);
    lcd.write(byte(1));  // Light char    
    lcd.setCursor(1, 1);
    lcd.print(":");
    lcd.setCursor(2, 1);
    lcd.print(textLight);
    lcd.setCursor(4, 1);
    lcd.print("%");
    lcd.setCursor(10, 1);
    lcd.write(byte(4));  // Water char    
    lcd.setCursor(11, 1);
    lcd.print(":");
    lcd.setCursor(12, 1);
    lcd.print(waterLevel);  
   
    
  }
  delay(2000);
  
  SendDataWebServer();

}

/*********************************************************************************************************
  END LOOP
*********************************************************************************************************/

/****************************************************************
 DISPLAY Initialization
****************************************************************/
void SendDataWebServer() {
 
  if (((millis() - lastServerUpdate) >  webServerDelay)  || (millis() < lastServerUpdate) ) {
    Serial.print("Millis=");
    Serial.print(millis());
    Serial.print(" - lastServerUpdate=");
    Serial.print(lastServerUpdate);
    Serial.print(" - webServerDelay=");
    Serial.println(webServerDelay);    
    Serial.println("");
    // keep retrying until connected to website
    Serial.println("Connecting to WebSite");
    while (0 == c.connect(SITE_URL, 80))
    {
      Serial.println("Re-Connecting to WebSite");
      delay(1000);
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  Connecting to ");
    lcd.setCursor(0, 1);    
    lcd.print("  WebSite");
    while (0 == c.connect(SITE_URL, 80))
    {
      Serial.println("Re-Connecting to WebSite");
      delay(1000);
    }
  
    // send HTTP request, ends with 2 CR/LF
    Serial.println("send HTTP GET request");
    /*c.println("GET / HTTP/1.1");
    c.println("Host: " SITE_URL);
    c.println("Connection: close");
    c.println();*/
    c.print("GET /UrbanaBot/InsertData.php?airTemp=");
    c.print(textTemp);
    c.print("&airHumidity=");
    c.print(textHumid); 
    c.print("&airQuality=");
    c.print(0); 
    c.print("&lightValue=");
    c.print(textLight); 
    c.print("&waterLevel=");
    c.print(0); 
    c.print("&waterTemp=");
    c.print(0); // temporaire
    c.print("&nutriValue=");
    c.print(0); 
    c.println(" HTTP/1.1");
    c.println("Host: nesko-no-ip.org");
    c.println("Connection: close");                             
    c.println();
    // waiting for server response
    Serial.println("waiting HTTP response:");
    while (!c.available())
    {
      delay(100);
    }
    // Make sure we are connected, and dump the response content to Serial
    while (c)
    {
      int v = c.read();
      if (v != -1)
      {
        Serial.print((char)v);
      }
      else
      {
        Serial.println("no more content, disconnect");
        c.stop();
        while (1)
        {
          delay(1);
        }
      }
    }
  
    if (!disconnectedMsg)
    {
      Serial.println("disconnected by server");
      disconnectedMsg = true;
    }

    lastServerUpdate = millis();
  }
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

