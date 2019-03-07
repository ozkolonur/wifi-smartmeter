#include <stdlib.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include "SoftwareSerial.h"
#include "config.h"

//Pin definition for Attiny85 and regular arduino
#if defined(__AVR_ATtiny85__)
#define LED_BUILTIN     1
#define ESP_ENABLE_PIN  2
#define SOFT_RX_PIN     4
#define SOFT_TX_PIN     3
#define INTR_PIN        0
#else
//ATMEGA328
#define ESP_ENABLE_PIN  8
#define SOFT_RX_PIN     7
#define SOFT_TX_PIN     6
#define INTR_PIN        2
#endif

//Remove debug if using attiny85
#if defined(__AVR_ATtiny85__)
#undef DEBUG
#endif

#ifdef DEBUG
#define DEBUG_CONNECT(x)  Serial.begin(x)
#define DEBUG_PRINT(x)    Serial.println(x)
#define DEBUG_FLUSH()     Serial.flush()
#else
#define DEBUG_CONNECT(x)
#define DEBUG_PRINT(x)
#define DEBUG_FLUSH()
#endif

//How many times to retry talking to module before giving up
#define RETRY_COUNT   2
uint8_t retry_attempt = 0;
bool connected = false;   //TRUE when a connection is made with the target server

//PROGMEM answer strings from ESP
const char STR_OK[] PROGMEM =     "OK";
const char STR_CONNECT[] PROGMEM =     "CONNECT";
const char STR_BOOT[] PROGMEM =   "ready";  //End of bootup message
const char STR_APNAME[] PROGMEM = "+CWJAP:\"";
const char STR_SENDMODE[] PROGMEM = ">";
const char STR_HTTP_OK[] PROGMEM = "HTTP/1.1 202";

char cmd[64];
char str_buffer[64];

//counter_committed is counter synced with server
unsigned int counter = 0;
unsigned int counter_committed = 0;

//Software serial connected to ESP8266
SoftwareSerial SoftSerial(SOFT_RX_PIN, SOFT_TX_PIN); // RX, TX

//Disabling ADC saves ~230uAF. Needs to be re-enable for the internal voltage check
#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC

void setup() {
  //Disable ADC
  adc_disable();

  //Setup pins
  pinMode(ESP_ENABLE_PIN, OUTPUT);
  digitalWrite(ESP_ENABLE_PIN, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //Set pin d2 to input using the buildin pullup resistor
  pinMode(INTR_PIN, INPUT);
  digitalWrite(INTR_PIN, HIGH);

  //Debug serial print only works if DEBUG is defined
  DEBUG_CONNECT(115200);
  DEBUG_PRINT(F("Arduino started"));

  //Make sure ESP8266 is set to 9600
  SoftSerial.begin(9600);
}

void loop() {
  DEBUG_PRINT(F("Going to sleep..."));
  DEBUG_FLUSH();
  
  delay(180);

  sleepTillChg();

  counter++;
  DEBUG_PRINT("+");

  digitalWrite(LED_BUILTIN, HIGH);
  delay(20);
  digitalWrite(LED_BUILTIN, LOW);

  //it is time to update server?
  if (counter - counter_committed > SERVER_UPDATE_THRESHOLD) {
    boolean updateResult = updateServer();
    DEBUG_PRINT("updateResult=");
    DEBUG_PRINT(updateResult);
    if (updateResult) {
      counter_committed = counter;
      DEBUG_PRINT(counter);
    }
    cleanUp();
  }
}

boolean updateServer() {
  //Try to enable ESP8266 Module. Return if can't
  if (!enableESP()) {
    DEBUG_PRINT(F("Can't talk to the module"));
    return false;
  }

  //Set transmission mode
  SoftSerial.println(F("AT+CIPMUX=0"));
  SoftSerial.flush();

  if (!waitForString(getString(STR_OK), 2, 1000)) {
    DEBUG_PRINT(F("Error setting CIPMUX"));
    return false;
  }
  //Set transmission mode
  SoftSerial.println(F("AT+CIPMODE=1"));
  SoftSerial.flush();

  if (!waitForString(getString(STR_OK), 2, 1000)) {
    DEBUG_PRINT(F("Error setting CIPMODE"));
    return false;
  }

  //Connect to TCP server
  sprintf_P(cmd, PSTR("AT+CIPSTART=\"TCP\",\"%S\",%S"), PSTR(HOSTNAME), PSTR(HTTP_PORT));
  DEBUG_PRINT(cmd);
  SoftSerial.println(cmd);
  SoftSerial.flush();

  //Check for connection errors
  if (!waitForString(getString(STR_CONNECT), 7, 5000)) {
    DEBUG_PRINT(F("Error connecting to server"));
    DEBUG_PRINT(str_buffer);
    return false;
  }

  //Connected fine to server
  DEBUG_PRINT(F("Connected to server!"));
  connected = true;

  SoftSerial.println(F("AT+CIPSEND"));
  SoftSerial.flush();

  //We are in transmit mode
  if (waitForString(getString(STR_SENDMODE), 1, 1000)) {

    DEBUG_PRINT(F("Ready to send..."));

    char str_content[24];
    memset(str_content, 0, sizeof(str_content));
    sprintf(str_content, "counter %u\n\n", counter);

    uint8_t content_length = 0;
    content_length = strlen(str_content); //we are adding \n to the end

    /*
       POST %s HTTP/1.1\r\n
       Host: %s:%s\r\n
       Content-Length: %d\r\n\r\n
       %s\n\n
    */
    sprintf_P(cmd, PSTR("POST %S HTTP/1.1\r\n") , PSTR(HTTP_CONTEXT));
    DEBUG_PRINT(cmd);
    SoftSerial.print(cmd);
    SoftSerial.flush();

    sprintf_P(cmd, PSTR("Host: %S:%S\r\n") , PSTR(HOSTNAME), PSTR(HTTP_PORT));
    DEBUG_PRINT(cmd);
    SoftSerial.print(cmd);
    SoftSerial.flush();

    sprintf_P(cmd, PSTR("Content-Length: %d\r\n\r\n") , content_length);
    DEBUG_PRINT(cmd);
    SoftSerial.print(cmd);
    SoftSerial.flush();

    DEBUG_PRINT(str_content);
    SoftSerial.print(str_content);
    SoftSerial.flush();

    DEBUG_PRINT(F("All messages sent!"));

    if (waitForString(getString(STR_HTTP_OK), 12, 1000)) {
      DEBUG_PRINT("HTTP OK 202");
      return true;
    } else {
      DEBUG_PRINT("ERROR HTTP POST");
      ledblink(3);
      return false;
    }
  }
}

void wakeUp() {
  //ISR
}

//Bring Enable pin up, wait for module to wake-up/connect and turn off echo.
bool enableESP() {
  //Enable ESP8266 Module

  digitalWrite(ESP_ENABLE_PIN, HIGH);

  //digitalWrite(LED_BUILTIN, HIGH);
  DEBUG_PRINT(F("ESP8266 Enabled. Waiting to spin up..."));

  //Wait for module to boot up and connect to AP
  waitForString(getString(STR_BOOT), 5, 7000);  //End of bootup message
  clearBuffer(); //Clear local buffer

  //Local echo off
  SoftSerial.println(F("ATE0"));
  SoftSerial.flush();

  if (waitForString(getString(STR_OK), 2, 1000)) {
    DEBUG_PRINT(F("Local echo OFF"));
    DEBUG_PRINT(F("ESP8266 UART link okay"));

    //Connect to AP if not already
    return connectWiFi();
  }

  //Couldn't connect to the module
  DEBUG_PRINT(F("Error initializing the module"));

  //Try again until retry counts expire
  if (retry_attempt < RETRY_COUNT) {
    retry_attempt++;
    return enableESP();
  } else {
    retry_attempt = 0;
    return false;
  }
}

//Only connect to AP if it's not already connected.
//It should always reconnect automatically.
boolean connectWiFi() {

  //Check if already connected
  SoftSerial.println(F("AT+CWJAP?"));
  SoftSerial.flush();

  //Check if connected to AP
  if (waitForString(getString(STR_APNAME), 8, 500)) {
    DEBUG_PRINT(F("Connected to AP"));
    return true;
  }

  //Not connected, connect now
  DEBUG_PRINT(F("Not connected to AP yet"));

  SoftSerial.println(F("AT+CWMODE=1"));
  DEBUG_PRINT(F("STA Mode set"));
  waitForString(getString(STR_OK), 2, 500);

  //Create connection string
  sprintf_P(cmd, PSTR("AT+CWJAP=\"%S\",\"%S\""), PSTR(SSID), PSTR(PASS));
  SoftSerial.println(cmd);
  SoftSerial.flush();
  //DEBUG_PRINT(cmd);

  if (waitForString(getString(STR_OK), 2, 10000)) {
    return true;
  } else {
    return false;
  }
}


void cleanUp() {

  //Close connection if needed
  if (connected) {
    SoftSerial.println(F("AT+CIPCLOSE"));
    SoftSerial.flush();
    DEBUG_PRINT(F("Closed connection"));
  }

  connected = false;

  //Clear 64byte receive buffer
  clearBuffer();
  DEBUG_PRINT(F("Buffer Empty"));

  //Bring chip enable pin low
  disableESP();
}


//Remove all bytes from the buffer
void clearBuffer() {
  while (SoftSerial.available())
    SoftSerial.read();
}

//Bring Enable pin down
bool disableESP() {

  //Disable ESP8266 Module
  digitalWrite(ESP_ENABLE_PIN, LOW);
  //digitalWrite(LED_BUILTIN, LOW);
  DEBUG_PRINT(F("ESP8266 Disabled"));

  return true;
}


//Return char string from PROGMEN
char* getString(const char* str) {
  //memset(str_buffer, 0, sizeof(str_buffer));
  strcpy_P(str_buffer, (char*)str);
  return str_buffer;
}

//Wait for specific input string until timeout runs out
bool waitForString(char* input, uint8_t length, unsigned int timeout) {

  unsigned long end_time = millis() + timeout;
  int current_byte = 0;
  uint8_t index = 0;

  while (end_time >= millis()) {

    if (SoftSerial.available()) {

      //Read one byte from serial port
      current_byte = SoftSerial.read();

      if (current_byte != -1) {
        //Search one character at a time
        if (current_byte == input[index]) {
          index++;

          //Found the string
          if (index == length) {
            return true;
          }
          //Restart position of character to look for
        } else {
          index = 0;
        }
      }
    }
  }
  //Timed out
  return false;
}

#if defined(__AVR_ATtiny85__)
void sleepTillChg() {  // 0.02ma drain while sleeping here
  GIMSK |= _BV(PCIE);                     // Enable Pin Change Interrupts
  PCMSK |= _BV(PCINT0);                   // Use PB0 (was PB3) as interrupt pin
  // Turn off unnecessary peripherals
  ADCSRA &= ~_BV(ADEN);                   // ADC off
  ACSR |= _BV(ACD); // Disable analog comparator

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement
  sleep_enable();                         // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();                                  // Enable interrupts
  sleep_cpu();                            // sleep ... Zzzz

  cli();                                  // Disable interrupts
  PCMSK &= ~_BV(PCINT0);                  // Turn off PB0 (was PB3) as interrupt pin
  sleep_disable();                        // Clear SE bit
  //    ADCSRA |= _BV(ADEN);                    // ADC on
  sei();                                  // Enable interrupts
}
#else
void sleepTillChg() {
  sleep_enable();//Enabling sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
  attachInterrupt(digitalPinToInterrupt(INTR_PIN), wakeUp, LOW);//attaching a interrupt to pin d2
  sleep_cpu();//activating sleep mode

  //first thinkg to removes the interrupt;
  detachInterrupt(digitalPinToInterrupt(INTR_PIN));
  //Disable sleep mode
  sleep_disable();
}

#endif

void ledblink(int num) {
  delay(100);
  for (int i = 0; i < num; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
  delay(500);
}
