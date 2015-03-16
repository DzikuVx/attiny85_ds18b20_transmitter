#include <VirtualWire.h>
#include <OneWire.h> 

#include <avr/sleep.h>
#include <avr/wdt.h>

#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC

#define ONEWIRE_BUSS 0
#define LED_PIN 3
#define TX_PIN 1

OneWire TemperatureSensor(ONEWIRE_BUSS);

int messageId = 0;

int iCounter = 0;

void setup() {
  adc_disable();
  
  pinMode(LED_PIN, OUTPUT);
  
  vw_set_tx_pin(TX_PIN);
  vw_setup(1000);
  
  setup_watchdog(8);
  enterSleep(); 
}

void loop() {
  
  byte i;
  byte data[12];
  int16_t raw;
  float celsius;
  int iTemp;
  
  messageId++;
  
  if (messageId > 255) {
    messageId = 0;
  }
  
  digitalWrite(LED_PIN, LOW);
  
  TemperatureSensor.reset();       // reset one wire buss
  TemperatureSensor.skip();        // select only device
  TemperatureSensor.write(0x44);   // start conversion
   
  delay(800);
  
  TemperatureSensor.reset();
  TemperatureSensor.skip();
  TemperatureSensor.write(0xBE);   // Read Scratchpad
  for ( i = 0; i < 9; i++) {       // 9 bytes
    data[i] = TemperatureSensor.read();
  }
   
  // Convert the data to actual temperature
  raw = (data[1] << 8) | data[0];
  celsius = (float) raw / 16.0;
  
  iTemp = (int) (celsius * 10);
  
  digitalWrite(LED_PIN, HIGH);
  char output[7];
  
  
  output[0] = 0x00; //Broadcast
  output[1] = 0x64; //Sensor / Temperature in decy Celcius
  output[2] = 0x77; //Sensor 77
  output[3] = 0x04; //Data type int32
  output[4] = messageId;
  
  output[5] = (char) (iTemp >> 8) & 0x00FF;
  output[6] = (char) iTemp & 0x00ff;
  
  for (i = 0; i < 3; i++) {
    vw_send((uint8_t *)output, 7);
    vw_wait_tx();
    delay(100);
  }
  
  digitalWrite(LED_PIN, LOW);
  
  enterSleep();
}

void enterSleep(void) {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();
  sleep_mode();                        // System sleeps here
  sleep_disable(); 
}

//****************************************************************
// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {

  byte bb;
  int ww;
  if (ii > 9 ) ii=9;
  bb=ii & 7;
  if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);
  ww=bb;

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}
//****************************************************************  
// Watchdog Interrupt Service / is executed when  watchdog timed out
ISR(WDT_vect) {
  iCounter++;
}
