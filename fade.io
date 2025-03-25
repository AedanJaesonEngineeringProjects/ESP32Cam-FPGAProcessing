#define LED_PIN 4  // Built-in LED on ESP32-CAM (or change to 2 for external LED)
#define LEDC_CHANNEL 0
#define LEDC_TIMER_BIT 8
#define LEDC_BASE_FREQ 5000

int brightness = 0;  // Current brightness
int fadeAmount = 5;  // Step size for fading

void setup() {
  // Setup LED PWM Channel
  ledcSetup(LEDC_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
  ledcAttachPin(LED_PIN, LEDC_CHANNEL);
}

void loop() {
  // Set LED brightness
  ledcWrite(LEDC_CHANNEL, brightness);

  // Change brightness
  brightness += fadeAmount;

  // Reverse direction if at limits
  if (brightness <= 0 || brightness >= 255) {
    fadeAmount = -fadeAmount;
  }

  delay(30);
}
