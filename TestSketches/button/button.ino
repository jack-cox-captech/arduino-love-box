#include <Bounce2.h>

#define BUTTON_1 27
#define BUTTON_2 25
#define RESET_PIN 32

  Bounce button1 = Bounce();
  Bounce button2 = Bounce(); 
  
void setup() {
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  // put your setup code here, to run once:


  Serial.begin(115200);
  while(!Serial); // wait for serial to init de-comment if you want prints to work during setup
  Serial.println("\n");
  Serial.println("Starting setup");



  button1.attach(BUTTON_1,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT mode
  button1.interval(25); 
  button2.attach(BUTTON_2,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT mode
  button2.interval(25); 
}

void loop() {

  button1.update(); // Update the Bounce instance
  button2.update();
  if ( button1.rose() ) {  // Call code if button transitions from HIGH to LOW
     Serial.println("Button 1 pressed");
  }
  if (button2.rose()) {
    Serial.println("Button 2 pressed");
  }

  if (button1.read() && button2.read()) {
    ESP.restart();
  }
}
