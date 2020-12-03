#include <Arduino.h>
#include <Ticker.h>

#define LED_PIN LED_BUILTIN

Ticker blinker(TIMERID0);
Ticker toggler(TIMERID1);
Ticker changer(TIMERID2);
float blinkerPace = 0.1;  //seconds
const float togglePeriod = 5; //seconds

void change() {
  blinkerPace = 0.5;
}

void blink() {
  if (digitalRead(LED_PIN) == HIGH)
    digitalWrite(LED_PIN, LOW);
  else
    digitalWrite(LED_PIN, HIGH);
}

void toggle() {
  static bool isBlinking = false;
  if (isBlinking) {
    blinker.detach();
    isBlinking = false;
  }
  else {
    blinker.attach(blinkerPace, blink);
    isBlinking = true;
  }
  digitalWrite(LED_PIN, LOW);  //make sure LED on on after toggling (pin LOW = led ON)
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  toggler.attach(togglePeriod, toggle);
  changer.once(30, change);
}

void loop() {
  
}
