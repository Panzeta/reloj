#include <SoftwareSerial.h>


// Header file includes
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

#include "Font_Data.h"

#define USE_DS1307 1

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8

#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10


#include <DHT.h> 
#define DHTPIN 2 //Conectamos el Sensor al pin digital 2
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);


#if USE_DS1307
#include <MD_DS1307.h>
#include <Wire.h>
#endif

// Hardware SPI connection
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

#define SPEED_TIME  75
#define PAUSE_TIME  0

#define MAX_MESG  30

 
// Turn on debug statements to the serial output
#define DEBUG 0

// Global variables
float temp;
float Humi;
char szTime[9];    // mm:ss\0
char szMesg[MAX_MESG+1] = "";

uint8_t degC[] = { 6, 3, 3, 56, 68, 68, 68 }; // Deg C
uint8_t degF[] = { 6, 3, 3, 124, 20, 20, 4 }; // Deg F

char *mon2str(uint8_t mon, char *psz, uint8_t len)
// Get a label from PROGMEM into a char array
{
  static const char str[][4] PROGMEM =
  {
    "Ene", "Feb", "Mar", "Abr", "May", "Jun",
    "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"
  };

  *psz = '\0';
  mon--;
  if (mon < 12)
  {
    strncpy_P(psz, str[mon], len);
    psz[len] = '\0';
  }

  return(psz);
}

char *dow2str(uint8_t code, char *psz, uint8_t len)
{
  static const char str[][10] PROGMEM =
  {
    "Lunes", "Martes",
    "Miercoles", "Jueves", "Viernes", "Sabado", "Domingo", 
         
  };

  *psz = '\0';
  code--;
  if (code < 7)
  {
    strncpy_P(psz, str[code], len);
    psz[len] = '\0';
  }

  return(psz);
}

void getTime(char *psz, bool f = true)
// Code for reading clock time
{
#if	USE_DS1307
  RTC.readTime();
  sprintf(psz, "%02d%c%02d", RTC.h, (f ? ':' : ' '), RTC.m);
#else
  uint16_t  h, m, s;

  s = millis()/1000;
  m = s/60;
  h = m/60;
  m %= 60;
  s %= 60;
  sprintf(psz, "%02d%c%02d", h, (f ? ':' : ' '), m);
#endif
}

void getDate(char *psz)
// Code for reading clock date
{
#if	USE_DS1307
  char  szBuf[10];

  RTC.readTime();
  sprintf(psz, "%d %s %04d", RTC.dd, mon2str(RTC.mm, szBuf, sizeof(szBuf)-1), RTC.yyyy);
#else
  strcpy(szMesg, "2 Nov 2019");
#endif
}

void setup(void)
{
  
  P.begin(2);
  P.setInvert(false);

  P.setZone(0, 0, MAX_DEVICES-5);
  P.setZone(1, MAX_DEVICES-4, MAX_DEVICES-1);
  P.setFont(1, numeric7Seg);

  P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);
  P.displayZoneText(0, szMesg, PA_CENTER, SPEED_TIME, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  P.addChar('$', degC);
  P.addChar('&', degF);

#if USE_DS1307
  RTC.control(DS1307_CLOCK_HALT, DS1307_OFF);
  RTC.control(DS1307_12H, DS1307_OFF);
#endif

  getTime(szTime);
}

void loop(void)
{ 

  static uint32_t lastTime = 0; // millis() memory
  static uint8_t  display = 0;  // current display mode
  static bool flasher = false;  // seconds passing flasher
  temp = dht.readTemperature();
  Humi = dht.readHumidity();
  P.displayAnimate();
  
  if (P.getZoneStatus(0))
  {
    switch (display)
    {
      case 0: // Temperature deg C
        P.setTextEffect(0, PA_OPENING, PA_SCROLL_UP_LEFT);
        display++;
        

        
        {
          dtostrf(temp, 3, 0, szMesg);
          strcat(szMesg, "$");
          
        }

        break;

      case 1: // Temperature deg F
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;

        {
          dtostrf(temp, 3, 1, szMesg);
          strcat(szMesg, " $");
        }

        
        break;

      case 2: // Relative Humidity
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;

        {
          dtostrf(Humi, 3, 0, szMesg);
          strcat(szMesg, "% RH");
        }

        break;

      case 3: // day of week
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display++;
#if	USE_DS1307
        dow2str(RTC.dow, szMesg, MAX_MESG);
#else
        dow2str(4, szMesg, MAX_MESG);
#endif
        break;

      default:  // Calendar
        P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        display = 0;
        getDate(szMesg);
        break;
    }

    P.displayReset(0);
  }

  // Finally, adjust the time string if we have to
  if (millis() - lastTime >= 1000)
  {
    lastTime = millis();
    getTime(szTime, flasher);
    flasher = !flasher;

    P.displayReset(1);
  }
}
