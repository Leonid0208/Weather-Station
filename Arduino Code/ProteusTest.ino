#include <DHT.h>
#include <SoftwareSerial.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>


#define DHTPIN 2 // Digital PIN of DHT22
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHT22);
SFE_BMP180 pressure;


void setup(void) {
  Serial.begin(9600);
  pressure.begin();
  dht.begin();
  float humidity, temperature;
}

void loop(void) {
  getTime();
  getTemperature();
  getPressure();
  Serial.println();
  delay(1000);
}

void getTime(){
  tmElements_t tm;  // Initialization DS1307
  if (RTC.read(tm)){
    Serial.println("Time: " + print2digits(tm.Hour) + ":" + print2digits(tm.Minute) + ":" + print2digits(tm.Second));
    Serial.println("Date (D/M/Y): " + (String)tm.Day + "/" + (String)tm.Month + "/" + (String)tmYearToCalendar(tm.Year));
    
  }
  else{
    if (RTC.chipPresent()){ // DS1307 working, but stopped
      Serial.println("The DS1307 is stopped.");
    }
    else{ // If DS1307 is not connected
      Serial.println("DS1307 read error!");
    }
  }
}

void getTemperature(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(t) || isnan(h)) { // Check the data from DHT22
    Serial.println("Failed to read from DHT");
  }
  else {
    Serial.println("Humidity: " + (String)h);
    Serial.println("Temperature: " + (String)t + " *C");
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


void getPressure(){
    char status;
    double T,P;

    status = pressure.startTemperature();
    if (status != 0){
        delay(status);
        status = pressure.getTemperature(T);
        if (status != 0){
            status = pressure.startPressure(3);
            if (status != 0){
                // ожидание замера давления
                delay(status);
                status = pressure.getPressure(P,T);
                if (status != 0){
                    Serial.println("Pressure: " + (String)P);
//                    Serial.println(T);
// Temperature from BMP180
                }
            }
        }
    }
}
