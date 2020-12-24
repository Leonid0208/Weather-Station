#define _pkPin 8
#define _stPin 9
#define PHOTORESISTOR A5

#include <DHT.h>
#include <SoftwareSerial.h>
#include <SFE_BMP180.h>
#include <Wire.h>
/*#include <TimeLib.h>
#include <DS1307RTC.h>*/

#define DHTPIN 7 // Digital PIN of DHT22
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
SFE_BMP180 pressure;

// power Up GPRS Shield
void powerOn() {
  Serial.print("Powering on...");
  delay(1000);
  pinMode(_pkPin, OUTPUT);
  pinMode(_stPin, INPUT);
  if (!digitalRead(_stPin)) {
      digitalWrite(_pkPin, HIGH);
      delay(3000);
  }
  digitalWrite(_pkPin, LOW);
  if (Serial1) Serial1.begin(9600);
  delay(1000);
  Serial.println("Done");
}

// power Off GPRS Shield
void powerOff() {
  Serial.print("Powering off...");
  pinMode(_pkPin, OUTPUT);
  pinMode(_stPin, INPUT);
  if (digitalRead(_stPin)) {
      digitalWrite(_pkPin, HIGH);
      delay(3000);
  }
  digitalWrite(_pkPin, LOW);
  if (Serial1) Serial1.end();
  delay(1000);
  Serial.println("Done");
}


void readSerial1() {
  if (Serial1.available()) {
    while (Serial1.available()) {
      Serial.write(Serial1.read());
    }
  
    // Serial.println();
  }
}

String readSerial1String() {
  String s = "";
  while (Serial1.available() && s.length() < 100) {
    char c = Serial1.read();
    s += c;
  }

  return s;
}

float h_DHT, t_DHT;

void getTemperature() {
  h_DHT = dht.readHumidity();
  t_DHT = dht.readTemperature();

  if (isnan(t_DHT) || isnan(h_DHT)) { // Check the data from DHT22
    Serial.println("Failed to read from DHT");
  }
  else {
    Serial.println("Humidity: " + (String)h_DHT);
    Serial.println("Temperature: " + (String)t_DHT + " *C");
  }
}

double t_BMP, p_BMP;
void getPressure(){
    char status;

    status = pressure.startTemperature();
    if (status != 0){
        delay(status);
        status = pressure.getTemperature(t_BMP);
        if (status != 0){
            status = pressure.startPressure(3);
            if (status != 0){
                // ожидание замера давления
                delay(status);
                status = pressure.getPressure(p_BMP, t_BMP);
                if (status != 0){
                    Serial.println("Pressure: " + (String)p_BMP);
                    Serial.println(t_BMP);
                }
            }
        }
    }
}

String print2digits(int number) { // Help function for print time
  String result_number = "";
  if (number >= 0 && number < 10) {
    result_number += "0";
  }
  result_number += (String)number;
  return result_number;
}

void setup() {
  // SIM900 (перевод в текстовый)
  Serial1.print("AT+CMGF=1\r");
  delay(200);
  
  // "Убиваем" все ненужные выводы
  //for (int pinN = 4; pinN <= 13; pinN++) pinMode(pinN, INPUT_PULLUP);
  //for (int pinN = A0; pinN <= A5; pinN++) pinMode(pinN, INPUT_PULLUP);
  //pinMode(SDA, INPUT_PULLUP); pinMode(SCL, INPUT_PULLUP);

  pinMode(PHOTORESISTOR, INPUT);
  
  Serial.begin(9600);
  Serial.flush();
  Serial1.begin(9600);
  Serial1.flush();
  Serial1.setTimeout(10);
  while (!Serial) {};

  Serial.println("Initializing sensors...");

  // Датчики
  if (pressure.begin())
    Serial.println("BMP180 initialized");
  else
    Serial.println("BMP180 failed");
  dht.begin();
  Serial.println("DHT initialized");
  
  Serial.println("GSM Tester loaded!");
  //Serial.println("Doing power cycle");
  //powerOff();
  //powerOn();
  int triesNum = 0;
  while (1) {
    Serial1.print("AT\r");
    delay(500);
    String s = readSerial1String();
    if (s.indexOf("OK") != -1) {
      Serial.println("Init successful");
      break;
    }
    else {
      Serial.println("Init failed! (" + s + ")");
      if (triesNum++ == 10) {
        Serial.println("Init failed, restarting...");
//        break;

        powerOff();
        delay(1000);
        powerOn();
        delay(5000);
        
        // SIM900 (перевод в текстовый)
        Serial1.print("AT+CMGF=1\r");
        delay(200);

        triesNum = 0;
      }
      delay(1000);
    }
  }

  // Разрешаем загрузку времени с интернета
  Serial1.print("AT+CLTS=1\r");
  delay(200);
}

void sendCommand(String command, int del = 200, bool readSerial = true) {
  Serial1.print(command + "\r");
  delay(del);

  if (readSerial)
    readSerial1();
}

int lightLevel = 0;
void getLightLevel() {
  lightLevel = analogRead(PHOTORESISTOR);
  Serial.println("Light level: " + (String)lightLevel);
}

void getSensorData() {
  getTemperature();
  getPressure();
  getLightLevel();
}

struct Time {
  int year;
  int month;
  int day; 
  int hour;
  int min;
  int sec;

  Time(int y, int M, int d, int h, int m, int s) {
    year = y; month = M; day = d, hour = h, min = m, sec = s;
  }
};

// Получить информацию о времени
struct Time getTimeStamp() {
  String timeStamp;
  int tries = 20;
  while (--tries) {
    sendCommand("AT+CCLK?", 1000, false);
    
    timeStamp = readSerial1String();
    Serial.println("Got: '" + timeStamp + "'");
    if (timeStamp.indexOf("OK") != -1) break;
  }

  if (tries <= 0) return Time(0, 0, 0, 0, 0, 0);
  
  int year = (((timeStamp[19])-'0')*10)+((timeStamp[20])-48);
  int month = (((timeStamp[22])-'0')*10)+((timeStamp[23])-48);
  int day  = (((timeStamp[25])-'0')*10)+((timeStamp[26])-48);
  int hour = (((timeStamp[28])-'0')*10)+((timeStamp[29])-48);
  int min = (((timeStamp[31])-'0')*10)+((timeStamp[32])-48);
  int sec = (((timeStamp[34])-'0')*10)+((timeStamp[35])-48);

  return Time(year, month, day, hour, min, sec);
}

void sendDataToServer() {
  // Получить актуальную информацию с датчиков
  getSensorData();
  
  sendCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  sendCommand("AT+SAPBR=3,1,\"APN\",\"internet\"");
  sendCommand("AT+SAPBR=3,1,\"USER\",\"internet\"");
  sendCommand("AT+SAPBR=3,1,\"PWD\",\"internet\"");
  sendCommand("AT+SAPBR=1,1", 1000);
  sendCommand("AT+SAPBR=2,1", 1000);
  sendCommand("AT+SAPBR=4,1", 1000);
  
  sendCommand("AT+HTTPINIT", 1000);
  // sendCommand("AT+HTTPSSL=1");
  sendCommand("AT+HTTPPARA=\"CID\",1", 1000);
  sendCommand("AT+HTTPPARA=\"URL\",\"http://ptsv2.com/t/dv4f2-1607713594/post\"", 2000);

  // Получить информацию о локальном времени с вышки
  Time t = getTimeStamp();
 
  String s;
  s += "{";
  s += "\"thisMeteoID\":" + String(12345) + ",";
  s += "\"year\":" + String(t.year) + ",";
  s += "\"month\":" + String(t.month) + ",";
  s += "\"day\":" + String(t.day) + ",";
  s += "\"hour\":" + String(t.hour) + ",";
  s += "\"minute\":" + String(t.min) + ",";
  s += "\"second\":" + String(t.sec) + ",";
  s += "\"photolight\":" + String(lightLevel) + ",";
  s += "\"humair\":" + String(h_DHT) + ",";
  s += "\"tair\":" + String(t_BMP) + ",";
  s += "\"airpressure\":" + String(p_BMP) + ",";
  s += "\"wingspeed\":" + String(0) + ",";
  s += "\"wingdir\":" + String(0) + ",";
  s += "\"error\":" + String(0) + ",";
  s += "\"errormsg\":\"" + String("no error") + "\"";
  s += "}";
  
  sendCommand("AT+HTTPDATA=" + String(s.length()) + ",25000", 3000);
  Serial1.write(s.c_str());
  delay(5000);
  readSerial1();

  sendCommand("AT+HTTPACTION=1", 5000);
  sendCommand("AT+HTTPREAD", 3000);
  sendCommand("AT+HTTPTERM");

  sendCommand("AT+SAPBR=0,1");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '$') {
        sendDataToServer();
      }
      else if (c == '~') {
        // Serial.print("Powering on...");
        powerOn();
        // Serial.println("Done");
      }
      else if (c == '`') {
        // Serial.print("Powering off...");
        powerOff();
        // Serial.println("Done");
      }
      else if (c == '&') {
        Serial1.println("AT+CMGS=\"+79788418050\""); 
        delay(200);
      
        Serial1.println("Test SMS");
        delay(200);
      
        // End AT command with a ^Z, ASCII code 26
        Serial1.println((char)26); 
        delay(200);
        Serial1.println();
      }
      else if (c == '%') {
        getSensorData();
      }
      else if (c == '^') {
        Time t = getTimeStamp();
        Serial.println(t.year);
        Serial.println(t.month);
        Serial.println(t.day);
        Serial.println(t.hour);
        Serial.println(t.min);
        Serial.println(t.sec);
      }
      else {
        Serial1.write(c);
      }
    }
  }

  // Каждую минуту отправлять данные на сервер
  if (millis() % 60000 == 0)
    sendDataToServer();

  // DEBUG
  readSerial1();
}
