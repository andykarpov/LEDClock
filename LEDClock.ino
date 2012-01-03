/**
 * LED panel clock
 * Based on 
 * - Atmega 328p controller with Arduino boot,
 * - Sure Electronics 24x16 green LED panel 
 * - DS1307 RTC
 *
 * Written by Andy Karpov <andy.karpov@gmail.com>
 * Copyright 2011 Andy Karpov
 */

#include <Wire.h>
#include <digitalWriteFast.h>
#include <ht1632c.h>
#include <RealTimeClockDS1307.h>
#include <Button.h>

const float pi = 3.141592653;
ht1632c panel; // LED panel (DATA - D10, WRCLK - D11, CS1 - D4)
Button btnMode(2, PULLUP); // button (D2)
Button btnPlus(3, PULLUP); // button (D3)
#define buzzerPin 6 // buzzer (D6)
enum modes {APP_CLOCK1, APP_CLOCK2, APP_CLOCK3, APP_SETUP, SET_HOURS, SET_MINUTES, SET_YEAR, SET_MONTH, SET_DAY, SET_DOW};
char* month_names[12] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
char* dow_names[7] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

int mode = 0; // default mode to APP_CLOCK1
int prevMode = 0; // previous mode
int seconds; // current seconds
int minutes; // current minutes
int hours; // current hours
int dayofweek; // current day of week
int day; // current day
int month; // current month
int year; // current year

long curTime = 0; // current time (timestamp from atmega)
long lastPush = 0; // last pushed settings button (timestamp)
long lastRead = 0; // last read from RTC (timestamp)
int lastSec = 0; // last second value
boolean dotsOn = false; // current dots on / off flag
boolean enableTone = false; // enable / disable tone

/**
 * Setup routine
 */
void setup() {
  // start clock if stopped
  if (RTC.isStopped()) {
    RTC.start();
  }
    RTC.readClock();
}

/**
 * Main loop routine
 */
void loop() {
  curTime = millis();

  // read clock values into buffers
  if (curTime - lastRead >= 1000) {
    RTC.readClock();
    lastRead = curTime;
  }
  
  // update seconds / minutes / hours variables
  seconds = RTC.getSeconds();
  minutes = RTC.getMinutes();
  hours = RTC.getHours(); 
  
  year = RTC.getYear();
  month = RTC.getMonth();
  dayofweek = RTC.getDayOfWeek();
  day = RTC.getDate();

  switch (mode) {
    case APP_CLOCK1:
      ApplicationClock1();
    break;
    case APP_CLOCK2:
      ApplicationClock2();
    break;
    case APP_CLOCK3:
      ApplicationClock3();
    break;
    case APP_SETUP:
      ApplicationSetup();
    break;
    case SET_HOURS:
      ApplicationSetHours();
    break;
    case SET_MINUTES:
      ApplicationSetMinutes();
    break;
    case SET_YEAR:
      ApplicationSetYear();
    break;
    case SET_MONTH:
      ApplicationSetMonth();
    break;
    case SET_DAY:
      ApplicationSetDay();
    break;
    case SET_DOW:
      ApplicationSetDow();
    break;
  }   
}

/**
 * Common callback to switch mode
 */
void OnModeChanged() {
  if (prevMode != mode) {
     panel.clear();
     prevMode = mode;
  }
  
  if (btnMode.isPressed()) {
    if (mode == APP_SETUP || mode == SET_DOW) {
      mode = APP_CLOCK1;
    } else {
      mode++;
    }
    if (enableTone) {
      tone(buzzerPin, 5000, 100);
    } else {
      delay(100);
    }
    delay(100);
    lastPush = curTime;
  }
}

/**
 * Small clock with date 
 */
void ApplicationClock1() {
  OnModeChanged();
  
  char* month_name = month_names[month-1];
  panel.set_font(3,5);
  panel.putstring(1,1, month_name);
  char date[2];
  sprintf(date,"%d%d", (day>9) ? (day/10) : 0, day%10);
  panel.putstring(16,1, date);
  
  char time[4];
  sprintf(time, "%d%d%d%d", (hours>9) ? (hours/10) : 0, hours%10, (minutes>9) ? (minutes/10) : 0, minutes%10);
  
  panel.set_font(5,7);
  panel.put_char(0,8, time[0]);
  panel.put_char(6,8, time[1]);    
  panel.put_char(13,8, time[2]);
  panel.put_char(19,8, time[3]);  
}

/**
 * Big digits
 */
void ApplicationClock2() {
  OnModeChanged();

  char time[4];
  sprintf(time, "%d%d%d%d", (hours>9) ? (hours/10) : 0, hours%10, (minutes>9) ? (minutes/10) : 0, minutes%10);
  
  panel.set_font(6,12);
  panel.put_char(-1, 2, time[0]);
  panel.put_char( 5, 2, time[1]);
  panel.put_char(12, 2, time[2]);
  panel.put_char(18, 2, time[3]);
}

/**
 * print a binary clock column
 */
void printBinaryColumn(int value, int column) {
  int x=(column-1)*4;
  int y=0;
  int r=2;
  for(int i=3; i>=0; i--) {
    if (bitRead(value, i)) {
      panel.rect(x,y,x+r,y+r,1);
      panel.plot(x+1, y+1, 1);
    } else {
      panel.rect(x,y,x+r,y+r,0);
      panel.plot(x+1, y+1, 1);
    }
    y=y+4;
  }
}

/**
 * Binary clock
 */
void ApplicationClock3() {
  OnModeChanged();

  printBinaryColumn((hours>9) ? (hours/10) : 0, 1);
  printBinaryColumn(hours%10, 2);
  printBinaryColumn((minutes>9) ? (minutes/10) : 0, 3);
  printBinaryColumn(minutes%10, 4);
  printBinaryColumn((seconds>9) ? (seconds/10) : 0, 5);
  printBinaryColumn(seconds%10, 6);
}

/**
 * Setup main screen
 */
void ApplicationSetup() {
  OnModeChanged();

  if (btnPlus.isPressed()) {
    mode = SET_HOURS;
    tone(buzzerPin, 4500, 100);
    tone(buzzerPin, 5000, 100);
    panel.clear();
    delay(300);
  }
  
  panel.set_font(3,5);
  panel.putstring(1,1,"SETUP");
  panel.putstring(1,8,"CLOCK");
  
  if (curTime - lastPush > 5000) {
    panel.fade_down();
    delay(1000);
    mode = APP_CLOCK1;
  }
}

/**
 * Set hours
 */
void ApplicationSetHours() {
  OnModeChanged();

  if (btnPlus.isPressed()) {
    hours++;
    if (hours > 23) hours = 0;
    RTC.setHours(hours);
    RTC.setClock();
    lastPush = curTime;
    delay(100);
  }
  panel.set_font(3,5);
  panel.putstring(1,1,"HOURS"); 
  char time[2];
  sprintf(time, "%d%d", (hours>9) ? (hours/10) : 0, hours%10);
  
  panel.set_font(5,7);
  panel.put_char(13,8, time[0]);
  panel.put_char(19,8, time[1]);  
}

/**
 * Set minutes
 */
void ApplicationSetMinutes() {
  OnModeChanged();

  if (btnPlus.isPressed()) {
    minutes++;
    if (minutes > 59) minutes = 0;
    RTC.setMinutes(minutes);
    RTC.setClock();
    lastPush = curTime;
    delay(100);
  }
  panel.set_font(3,5);
  panel.putstring(1,1,"MINUTES"); 
  char time[2];
  sprintf(time, "%d%d", (minutes>9) ? (minutes/10) : 0, minutes%10);
  
  panel.set_font(5,7);
  panel.put_char(13,8, time[0]);
  panel.put_char(19,8, time[1]);
}

/**
 * Set year
 */
void ApplicationSetYear() {
  OnModeChanged();

  if (btnPlus.isPressed()) {
    year++;
    if (year > 99) year = 11;
    RTC.setYear(year);
    RTC.setClock();
    lastPush = curTime;
    delay(100);
  }
  panel.set_font(3,5);
  panel.putstring(1,1,"YEAR"); 
  char time[2];
  itoa(year, time,10);

  panel.set_font(5,7);
  panel.put_char(0,8, '2');
  panel.put_char(6,8, '0');  
  panel.put_char(12,8, time[0]);
  panel.put_char(18,8, time[1]);
}

/**
 * Set month
 */
void ApplicationSetMonth() {
  OnModeChanged();

  if (btnPlus.isPressed()) {
    month++;
    if (month > 12) month = 1;
    RTC.setMonth(month);
    RTC.setClock();
    lastPush = curTime;
    delay(100);
  }
  panel.set_font(3,5);
  panel.putstring(1,1,"MONTH"); 
  char time[2];
  itoa(year, time,10);

  char* month_name = month_names[month-1];
  panel.set_font(5,7);
  panel.putstring(5,8, month_name);
}

/**
 * Set day
 */
void ApplicationSetDay() {
  OnModeChanged();

  if (btnPlus.isPressed()) {
    day++;
    if (day > 31) day = 1;
    RTC.setDate(day);
    RTC.setClock();
    lastPush = curTime;
    delay(100);
  }
  panel.set_font(3,5);
  panel.putstring(1,1,"DAY"); 
  char time[2];
  sprintf(time, "%d%d", (day>9) ? (day/10) : 0, day%10);
  
  panel.set_font(5,7);
  panel.put_char(13,8, time[0]);
  panel.put_char(19,8, time[1]);
}

/**
 * Set day of week
 */
void ApplicationSetDow() {
  OnModeChanged();

  if (btnPlus.isPressed()) {
    dayofweek++;
    if (dayofweek > 7) dayofweek = 1;
    RTC.setDayOfWeek(dayofweek);
    RTC.setClock();
    lastPush = curTime;
    delay(100);
  }
  panel.set_font(3,5);
  panel.putstring(1,1,"DOW"); 
  char* dow_name = dow_names[dayofweek-1];
  panel.set_font(5,7);
  panel.putstring(5,8, dow_name);
}
