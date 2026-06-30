#include <Arduino.h>

constexpr uint8_t LED_PIN = 19;

void setup()
{
    pinMode(LED_PIN, OUTPUT);
}

void loop()
{
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(500);
}