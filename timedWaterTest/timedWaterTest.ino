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

#include <buttons.h>

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Set timer intervals and water pin

bool WATERON = false;
int PUMP_PIN = 5;
int CHECK_INTERVAL_MINUTES = 1;
unsigned int FILL_INTERVAL = 5 * 60; // seconds between fills
unsigned long FILLTIME_HALF_MINUTES = 4; // in x30 second bulks
unsigned long FILLTIME_MILLIS = FILLTIME_HALF_MINUTES * 30 * 1000; //  in milliseconds  
unsigned long last_fill_start;

int OVERFLOW_PIN = 2;

// Push button to force cylce
int FORCE_CYCLE_PIN = 7;
Button ForceCycleBtn;

void setup()
{
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);
  pinMode(OVERFLOW_PIN, INPUT);
  pinMode(FORCE_CYCLE_PIN, INPUT);
  digitalWrite(FORCE_CYCLE_PIN, LOW);
  
  ForceCycleBtn.assign(7);
  ForceCycleBtn.setMode(OneShotTimer);
  ForceCycleBtn.turnOffPullUp();
  
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
  //Alarm.alarmRepeat(8,00,0, MorningAlarm);  // 8:00am every day
  //Alarm.alarmRepeat(8,35,0,EveningAlarm);  // 8:45pm every day 
  
  Alarm.timerRepeat(0, CHECK_INTERVAL_MINUTES, 0, CheckWaterOn);            // timer for every fill time    
  //Alarm.timerRepeat(FILL_INTERVAL, FillTimer);  //For testing with higher frequencey

  Alarm.alarmRepeat(0,00,1, FillTimer);  
  Alarm.alarmRepeat(8,00,0, FillTimer);
  Alarm.alarmRepeat(12,00,0, FillTimer);
  Alarm.alarmRepeat(20,00,0, FillTimer);
}

void  loop(){  
  CheckWaterOn();
  CheckButtons();
  digitalClockDisplay();
  Alarm.delay(0); // wait one second between clock display
}

// functions to be called when an alarm triggers:
void FillTimer(){
  Serial.println("FillTimer: - turn water on");    
  WaterOn();
}

void ForcedTimer(){
  Serial.println("ForcedTimer: - turn water on");    
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
  }
}

void WaterOff(){
  //Serial.println("Water OFF");
  digitalWrite(PUMP_PIN, HIGH);
  WATERON = false;
}

void CheckButtons(){
 switch (ForceCycleBtn.check()) {
   case ON:
     Serial.println("BUTTON PRESSED");
     if (digitalRead(PUMP_PIN)) {
       //Alarm.timerOnce(0, 0, 1, WaterOn);
       WaterOn();
     }
     else {
       Serial.println("BUTTON IGNORED");
     }
     break;
   case Hold:
     Serial.println("BUTTON HELD");
     if (!digitalRead(PUMP_PIN)) {
       //Alarm.timerOnce(0, 0, 1, WaterOn);
       WaterOff();
     }
     else {
       Serial.println("BUTTON IGNORED");
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

void digitalClockDisplay()
{
  // digital clock display of the time
  lcd.home();
  lcd.print(hour());
  printDigits(minute());
  printDigits(second());
  //Serial.print(" Millis: ");
  //Serial.print(millis());
  //Serial.print(" Last: ");
  //Serial.print(last_fill_start);
  
  //Serial.println(); 
}

void printDigits(int digits)
{
  lcd.print(":");
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

