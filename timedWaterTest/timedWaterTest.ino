/*
 * TimeAlarmExample.pde
 *
 * This example calls alarm functions at 8:30 am and at 5:45 pm (17:45)
 * and simulates turning lights on at night and off in the morning
 * A weekly timer is set for Saturdays at 8:30:30
 *
 * A timer is called every 15 seconds
 * Another timer is called once only after 10 seconds
 *
 * At startup the time is set to Jan 1 2011  8:29 am
 */
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <Time.h>
#include <TimeAlarms.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Set timer intervals and water pin

bool WATERON = false;
int PUMP_PIN = 5;
int CHECK_INTERVAL_MINUTES = 1;
unsigned int FILL_INTERVAL = 5 * 60; // seconds between fills
unsigned long FILLTIME_HALF_MINUTES = 3; // in x30 second bulks
unsigned long FILLTIME_MILLIS = FILLTIME_HALF_MINUTES * 30 * 1000; //  in milliseconds  
unsigned long last_fill_start;

int OVERFLOW_PIN = 2;
bool shitscreek = false;

void setup()
{
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(OVERFLOW_PIN, INPUT);
  
  // LCD Setup
  lcd.begin();
  lcd.backlight();
  
  while (!Serial) ; // wait until Arduino Serial Monitor opens
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  
  // Set control time
  last_fill_start = millis();
  
  Serial.begin(9600);
  Serial.println(FILLTIME_MILLIS);
  
  //setTime(8,29,50,1,1,11); // set time to Saturday 8:29:00am Jan 1 2011
  
  // create the alarms 
  //Alarm.alarmRepeat(8,30,0, MorningAlarm);  // 8:30am every day
  //Alarm.alarmRepeat(8,35,0,EveningAlarm);  // 8:45pm every day 
  
  Alarm.timerRepeat(0, CHECK_INTERVAL_MINUTES, 0, CheckWaterOn);            // timer for every fill time    
  Alarm.timerRepeat(FILL_INTERVAL, MorningAlarm);
}

void  loop(){  
  checkOverflow();
  digitalClockDisplay();
  Alarm.delay(1000); // wait one second between clock display
}

// functions to be called when an alarm triggers:
void MorningAlarm(){
  Serial.println("MorningAlarm: - turn water on");    
  WaterOn();
}

void EveningAlarm(){
  Serial.println("EveningAlarm: - turn water on");           
  WaterOn();
}

void WaterOn(){
  if (!WATERON){
    lcd.setCursor(0, 1);
    digitalWrite(PUMP_PIN, HIGH);
    last_fill_start = millis();
    Alarm.timerOnce(0, FILLTIME_HALF_MINUTES, 0, WaterOff);
    WATERON = true;
    lcd.print("Water ON        ");
  }
}

void WaterOff(){
  if (WATERON){
    Serial.println("Water OFF");
    digitalWrite(PUMP_PIN, LOW);
    WATERON = false;
  }
  lcd.setCursor(0, 1);
  lcd.print("Water OFF       ");
}

void CheckWaterOn(){
  unsigned long diff = millis() - last_fill_start;
  lcd.setCursor(0, 1);
  if (WATERON && diff > FILLTIME_MILLIS){
    lcd.print("Water too long  ");
    Serial.print("Water on passed fill time ");
    Serial.println(diff);
    Serial.println(FILLTIME_MILLIS);
    WaterOff();
  }
  else if (WATERON && diff < FILLTIME_MILLIS) {
    lcd.print("Within fill, OK ");
    Serial.println("Within fill cycle, all OK");
  }
  else {
    lcd.print("Water off, OK   ");
    Serial.println("Water off, all OK");
  }
}

void checkOverflow()
{
  Serial.println(digitalRead(OVERFLOW_PIN));
  lcd.setCursor(0, 1); 
  if (digitalRead(OVERFLOW_PIN) == LOW) {
    shitscreek = true;
    lcd.print("WATER OVERFLOW!!");
    WaterOff();
  }
  else {
    if (shitscreek == true) {
      shitscreek = false;
      WaterOn();
      CheckWaterOn();
//      lcd.print("                ");
    }
  }
}

void digitalClockDisplay()
{
  // digital clock display of the time
  lcd.home();
  lcd.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" Millis: ");
  Serial.print(millis());
  Serial.print(" Last: ");
  Serial.print(last_fill_start);
  
  Serial.println(); 
}

void printDigits(int digits)
{
  lcd.print(":");
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

