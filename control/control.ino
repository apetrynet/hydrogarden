/*
 * control.ino
 *
 * Control code for hydrogarden watering, health meassurements and light 
 *
 */
 
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Time.h>
#include <TimeAlarms.h>
//#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <DS3232RTC.h>  //http://github.com/JChristensen/DS3232RTC
#include "tx433_Nexa.h" // Nexa headers
#include <buttons.h>  // Button sensing

// Temperature Pin
const int TEMP_PIN = A1;
float tempC = 0;
  
// Setup software serial
SoftwareSerial com(11, 10); // RX, TX

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Nexa ID
String tx_nexa = "1010101010101010101010101010101010101010101010101010";
String ch_nexa="1010";

// RF transmit Pin 3
Tx433_Nexa Nexa(3, tx_nexa, ch_nexa);

// Set timer intervals and water pin

bool WATERON = false;
const int PUMP_PIN = 5;
int CHECK_INTERVAL_MINUTES = 1;
unsigned int FILL_INTERVAL = 5 * 60; // seconds between fills
unsigned long FILLTIME_HALF_MINUTES = 4; // in x30 second bulks
unsigned long FILLTIME_MILLIS = FILLTIME_HALF_MINUTES * 30 * 1000; //  in milliseconds  
unsigned long last_fill_start;

const int OVERFLOW_PIN = 2;

// Light timers
int light_time_on[3] = {8, 0, 0};
int light_time_off[3] = {0, 0, 2};

// Push button to force cylce
const int FORCE_CYCLE_PIN = 13;
Button ForceCycleBtn;

void setup()
{
  // Set aRef to INTERNAL 1.1v
  analogReference(INTERNAL);

  pinMode(TEMP_PIN, OUTPUT);
  
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);
  
  pinMode(OVERFLOW_PIN, INPUT);
  
  pinMode(FORCE_CYCLE_PIN, INPUT);
  digitalWrite(FORCE_CYCLE_PIN, LOW);
  
  ForceCycleBtn.assign(FORCE_CYCLE_PIN);
  ForceCycleBtn.setMode(OneShotTimer);
  ForceCycleBtn.turnOffPullUp();
  
  // LCD Setup
  lcd.begin();
  lcd.backlight();
  
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  
  // Set control time
  last_fill_start = millis();
  
  //Serial.begin(9600);
  
  Alarm.timerRepeat(0, CHECK_INTERVAL_MINUTES, 0, CheckWaterOn);            // timer for every fill time    
  //Alarm.timerRepeat(FILL_INTERVAL, WaterOn);  //For testing with higher frequencey

  // Water alarms
  Alarm.alarmRepeat(0,00,1, WaterOn);  
  Alarm.alarmRepeat(6,00,0, WaterOn);
  Alarm.alarmRepeat(12,00,0, WaterOn);
  Alarm.alarmRepeat(18,00,0, WaterOn);
  
  // Light alarms
  Alarm.alarmRepeat(light_time_on[0], light_time_on[1], light_time_on[2], LightOn);
  Alarm.alarmRepeat(light_time_off[0], light_time_off[1], light_time_off[2], LightOff);

  // Make sure light is on/off as planned. Accurate within the hour
  checkLight();
  
  // Set data rate for communication with espXXX
  com.begin(9600);
  
}

void  loop(){  
  CheckWaterOn();
  CheckButtons();
  sendTemp();
  digitalClockDisplay();
  Alarm.delay(0); // o == check all the time
}

// Light On
void LightOn(){
  //Serial.println("Light on, Good morning!");
  Nexa.Device_On(0);
  sendData("id=light value=on");
}

// Light timer
void LightOff(){
  //Serial.println("Light Off, Good nights!");
  Nexa.Device_Off(0);
  sendData("id=light value=off");
}

void ForcedTimer(){
  //Serial.println("ForcedTimer: - turn water on");    
  WaterOn();
}

void WaterOn(){
  if (digitalRead(PUMP_PIN)){
    lcd.setCursor(0, 1);
    digitalWrite(PUMP_PIN, LOW);
    last_fill_start = millis();
    Alarm.timerOnce(0, FILLTIME_HALF_MINUTES, 0, WaterOff);
    WATERON = true;
    lcd.print("Water ON        ");
    sendData("id=water value=on");
  }
}

void WaterOff(){
  //Serial.println("Water OFF");
  digitalWrite(PUMP_PIN, HIGH);
  if (WATERON)
    sendData("id=water value=off");
  WATERON = false;
}

void CheckButtons(){
 switch (ForceCycleBtn.check()) {
   case ON:
     //Serial.println("BUTTON PRESSED");
     if (digitalRead(PUMP_PIN)) {
       //Alarm.timerOnce(0, 0, 1, WaterOn);
       WaterOn();
     }
     else {
       //Serial.println("BUTTON IGNORED");
     }
     break;
   case Hold:
     //Serial.println("BUTTON HELD");
     if (!digitalRead(PUMP_PIN)) {
       //Alarm.timerOnce(0, 0, 1, WaterOn);
       WaterOff();
     }
     else {
       //Serial.println("BUTTON IGNORED");
     }
     break;
   default:
     break;
 }
}
void CheckWaterOn(){
  unsigned long diff = millis() - last_fill_start;
  lcd.setCursor(0, 1);
  bool pumping = !digitalRead(PUMP_PIN);
  bool overflowing = !digitalRead(OVERFLOW_PIN);
  
  if (overflowing) {
    lcd.print("WATER OVERFLOW!!");
    digitalWrite(PUMP_PIN, HIGH);
  }
  else if (pumping && diff > FILLTIME_MILLIS){
    lcd.print("Water too long  ");
    //Serial.print("Water on passed fill time ");
    //Serial.println(diff);
    //Serial.println(FILLTIME_MILLIS);
    WaterOff();
  }
  else if (pumping && diff < FILLTIME_MILLIS) {
    lcd.print("Within fill, OK ");
    //Serial.println("Within fill cycle, all OK");
  }
  else if (!pumping && WATERON && diff < FILLTIME_MILLIS) {
    lcd.print("Resume fill, OK ");
    //Serial.println("Within fill cycle, resume OK");
    digitalWrite(PUMP_PIN, LOW);
  }
  else {
    WaterOff();
    lcd.print("Water off, OK   ");
    //Serial.println("Water off, all OK");
  }
}

void sendData(String value)
{
  com.print("set-property " + value + "\r\n");
  /*static long last_transmit = millis();
  if ((millis() - last_transmit) >= 100)
   { 
    com.print("set-property " + value + "\r\n");
    last_transmit = millis();
   }*/
}

void checkLight()
{
  if (light_time_off[0] < light_time_on[0])
    {
      if (hour() >= light_time_on[0])
        LightOn();
      else
        LightOff();
    }
  else
    {
      if (hour() >= light_time_on[0] && hour() < light_time_off[0])
        LightOn();
      else
        LightOff();
    }
}

void sendTemp()
{
  static long last_temp_transmit = millis();
  if ((millis() - last_temp_transmit) >= 60000)
   { 
    readTemp();
    String data = "id=temp value=";
    data += tempC;
    sendData(data);
    //Serial.print("Temp in room: ");
    //Serial.println(data);
    last_temp_transmit = millis();
   }
  }
  
void readTemp()
{
  int temp = RTC.temperature();
  tempC = temp / 4.0;
  /*
  int tmp = 0;
  for (int i=0; i < 10; i++)
    tmp += analogRead(TEMP_PIN);

  tempC = (tmp / 10) / 9.31;
  */
  }

void digitalClockDisplay()
{
  // digital clock display of the time
  lcd.home();
  printDigits(hour());
  lcd.print(":");
  printDigits(minute());
  lcd.print(":");
  printDigits(second());
}

void printDigits(int digits)
{
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

