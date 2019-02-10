#include <ChainableLED.h>

#define CLK_LED_PIN D0
#define DATA_LED_PIN D1

ChainableLED led(CLK_LED_PIN, DATA_LED_PIN, 1);

void setup() {
  led.init();
  led.setColorRGB(0, random(255), random(255), random(255));
}

void loop() {
}
