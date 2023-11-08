#include <Adafruit_GFX.h>        
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <TridentTD_LineNotify.h>

#define SSID        "NAME_WIFI"
#define PASSWORD    "PASSWORD_WIFI"
#define LINE_TOKEN  "IDTOKEN_LINE"

MAX30105 particleSensor;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)

static const unsigned char PROGMEM logo2_bmp[] =
{ 0x03, 0xC0, 0xF0, 0x06, 0x71, 0x8C, 0x0C, 0x1B, 0x06, 0x18, 0x0E, 0x02, 0x10, 0x0C, 0x03, 0x10,              //Logo2 and Logo3 are two bmp pictures that display on the OLED if called
0x04, 0x01, 0x10, 0x04, 0x01, 0x10, 0x40, 0x01, 0x10, 0x40, 0x01, 0x10, 0xC0, 0x03, 0x08, 0x88,
0x02, 0x08, 0xB8, 0x04, 0xFF, 0x37, 0x08, 0x01, 0x30, 0x18, 0x01, 0x90, 0x30, 0x00, 0xC0, 0x60,
0x00, 0x60, 0xC0, 0x00, 0x31, 0x80, 0x00, 0x1B, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x04, 0x00,  };

static const unsigned char PROGMEM logo3_bmp[] =
{ 0x01, 0xF0, 0x0F, 0x80, 0x06, 0x1C, 0x38, 0x60, 0x18, 0x06, 0x60, 0x18, 0x10, 0x01, 0x80, 0x08,
0x20, 0x01, 0x80, 0x04, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x02, 0xC0, 0x00, 0x08, 0x03,
0x80, 0x00, 0x08, 0x01, 0x80, 0x00, 0x18, 0x01, 0x80, 0x00, 0x1C, 0x01, 0x80, 0x00, 0x14, 0x00,
0x80, 0x00, 0x14, 0x00, 0x80, 0x00, 0x14, 0x00, 0x40, 0x10, 0x12, 0x00, 0x40, 0x10, 0x12, 0x00,
0x7E, 0x1F, 0x23, 0xFE, 0x03, 0x31, 0xA0, 0x04, 0x01, 0xA0, 0xA0, 0x0C, 0x00, 0xA0, 0xA0, 0x08,
0x00, 0x60, 0xE0, 0x10, 0x00, 0x20, 0x60, 0x20, 0x06, 0x00, 0x40, 0x60, 0x03, 0x00, 0x40, 0xC0,
0x01, 0x80, 0x01, 0x80, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x30, 0x0C, 0x00,
0x00, 0x08, 0x10, 0x00, 0x00, 0x06, 0x60, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x01, 0x80, 0x00  };

const unsigned char Passive_buzzer = D5;

const char* host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------

WiFiClientSecure client; //--> Create a WiFiClientSecure object.

String GAS_ID = "AKfycbzyRZXladBnUhoDjM5e8frolg1jWswkyQ6yQi_2eCqLP6AN2Cqg7TdMl6f7TUi44uwK";
void setup()
{
  Serial.begin(115200);
  pinMode (Passive_buzzer,OUTPUT);
  Serial.println(LINE.getVersion());
  WiFi.begin(SSID, PASSWORD);
  Serial.printf("WiFi connecting to %s\n",  SSID);
  while(WiFi.status() != WL_CONNECTED) { 
   Serial.print(".");
   delay(400); 
  }
  Serial.print("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());  

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  display.display();
  delay(3000);

  // กำหนด Line Token
  LINE.setToken(LINE_TOKEN);

  // ตัวอย่างส่งข้อความ
  LINE.notify("สวัสดีครับ ขอต้อนรับสู่ระบบตรวจวัดออกซิเจนในเลือด");
  Serial.println("Initializing...");
  Serial.print("Successfully connected to : ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //----------------------------------------
  client.setInsecure();
  tone(Passive_buzzer, 880) ; //SOL note ...
  delay (500); 
  tone(Passive_buzzer, 523) ; //DO note 523 Hz
  delay (500); 
  tone(Passive_buzzer, 987) ; //LA note ...
  delay (500); 
  noTone(Passive_buzzer); 

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}


void loop(){
  long irValue = particleSensor.getIR();
  bool select = false;
  if(irValue > 5000){
  display.clearDisplay();                                   //Clear the display
  display.drawBitmap(5, 5, logo2_bmp, 24, 21, WHITE);       //Draw the first bmp picture (little heart)
  display.setTextSize(2);                                   //Near it display the average BPM you can display the BPM if you want
  display.setTextColor(WHITE); 
  display.setCursor(60,0);                
  display.println("BPM");             
  display.setCursor(60,18);                
  display.println(beatsPerMinute); 
  display.display();

  if (checkForBeat(irValue) == true){
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);
    if (beatsPerMinute < 59 && beatsPerMinute > 50){
      display.clearDisplay();                                //Clear the display
      display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE);    //Draw the second picture (bigger heart)
      display.setTextSize(2);                                //And still displays the average BPM
      display.setTextColor(WHITE);             
      display.setCursor(60,0);                
      display.println("BPM");             
      display.setCursor(60,18);                
      display.println(beatsPerMinute); 
      display.display();
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++){
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      }
      tone(Passive_buzzer, 587);
      delay (400);
      tone(Passive_buzzer, 880);
      delay (200);
      tone(Passive_buzzer, 783);
      delay (300);
      noTone(Passive_buzzer);
      select = true;
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute);
      Serial.print(", Avg BPM=");
      Serial.print(beatAvg);
      delay(2000);
      if(select == true){
      LINE.notify("ค่าออกซิเจนในเลือดของคุณตอนนี้คือ "+String(beatsPerMinute)+" BPM ซึ่งอยู่ในเกณฑ์ที่ ต่ำกว่าปกติ อาจจะมีความผิดพลาด กรุณาลองใหม่ด้วยครับ");
      LINE.notifySticker("ขอบคุณครับ",1070,17839);
      delay(1000);
      }
      sendData(beatsPerMinute,beatAvg);
      beatsPerMinute = 0;
      delay(1000);
    }
    if (beatsPerMinute <= 100 && beatsPerMinute > 59){
      display.clearDisplay();                                //Clear the display
      display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE);    //Draw the second picture (bigger heart)
      display.setTextSize(2);                                //And still displays the average BPM
      display.setTextColor(WHITE);             
      display.setCursor(60,0);                
      display.println("BPM");             
      display.setCursor(60,18);                
      display.println(beatsPerMinute); 
      display.display();
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++){
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      }
      tone(Passive_buzzer, 1046);
      delay (200);
      tone(Passive_buzzer, 587);
      delay (200);
      tone(Passive_buzzer, 783);
      delay (200);
      tone(Passive_buzzer, 880);
      delay (200);
      noTone(Passive_buzzer);
      select = true;
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute);
      Serial.print(", Avg BPM=");
      Serial.print(beatAvg);
      delay(2000);
      if(select == true){
      LINE.notify("ค่าออกซิเจนในเลือดของคุณตอนนี้คือ "+String(beatsPerMinute)+" BPM ซึ่งอยู่ในเกณฑ์ที่ ปกติ");
      LINE.notify("ข้อมูลจะถูกนำไปเก็บที่ https://docs.google.com/spreadsheets/d/1qp2ydx1gM-kvkBIpZef-VsCo23HKlCUn13eg5sEDKUM/edit?usp=sharing ซึ่งสามารถเช็คย้อนหลังได้");
      LINE.notifySticker("ขอบคุณครับ",1070,17839);
      }
      sendData(beatsPerMinute,beatAvg);
      beatsPerMinute = 0;
      delay(1000);
    }
    if (beatsPerMinute >= 101){
      display.clearDisplay();                                //Clear the display
      display.drawBitmap(0, 0, logo3_bmp, 32, 32, WHITE);    //Draw the second picture (bigger heart)
      display.setTextSize(2);                                //And still displays the average BPM
      display.setTextColor(WHITE);             
      display.setCursor(60,0);                
      display.println("BPM");             
      display.setCursor(60,18);                
      display.println(beatsPerMinute); 
      display.display();
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++){
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      }
      tone(Passive_buzzer, 783) ; 
      delay (250); 
      tone(Passive_buzzer, 987) ; 
      delay (500); 
      tone(Passive_buzzer, 659) ; 
      delay (250); 
      noTone(Passive_buzzer);
      select = true;
      Serial.print(", BPM=");
      Serial.print(beatsPerMinute);
      Serial.print(", Avg BPM=");
      Serial.print(beatAvg);
      delay(2000);
      if(select == true){
      LINE.notify("ค่าออกซิเจนในเลือดของคุณตอนนี้คือ "+String(beatsPerMinute)+" BPM ซึ่งอยู่ในเกณฑ์ที่ อันตราย หรืออาจจะเกิดมีความผิดพลาด กรุณาลองใหม่ด้วยครับ");
      LINE.notify("ข้อมูลจะถูกนำไปเก็บที่ https://docs.google.com/spreadsheets/d/1qp2ydx1gM-kvkBIpZef-VsCo23HKlCUn13eg5sEDKUM/edit?usp=sharing ซึ่งสามารถเช็คย้อนหลังได้");
      LINE.notifySticker("ขอบคุณครับ",1070,17839);
      }
      sendData(beatsPerMinute,beatAvg);
      beatsPerMinute = 0;
      delay(1000);
    }
  }
}

  Serial.print("IR=");
  Serial.print(irValue);

  if (irValue < 5000){
    Serial.print(" No finger?");
    beatAvg=0;
    display.clearDisplay();
    display.setTextSize(1);                    
    display.setTextColor(WHITE);             
    display.setCursor(30,5);                
    display.println("Please Place "); 
    display.setCursor(30,15);
    display.println("your finger ");  
    display.display();
    noTone(3);
  }
  if (irValue = 0){
    return ;
  }
  Serial.println();
delay(1);
}

void sendData(float BPM,float Avg){
  Serial.println("===========");
  Serial.print("Connecting to ");
  Serial.println(host);
  
  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
 
  float string_BPM = BPM; 
  float string_Avg = Avg; 
  String url = "/macros/s/" + GAS_ID + "/exec?BPM=" + string_BPM + "&AvgBPM=" + string_Avg; //  2 variables 
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266 successfull!");
  } else {
    Serial.println("esp8266 has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
}


