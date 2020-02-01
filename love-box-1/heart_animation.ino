#include <Adafruit_LEDBackpack.h>

#define AS_STOPPED  0
#define AS_RUNNING  1
int animation_state = AS_STOPPED;
int animation_step = 0;
unsigned long last_step_time = 0;


static const uint8_t PROGMEM
  animation_steps[] =
  { 0, 0, 0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 15, 14, 13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,0,0};

void start_heart_animation() {

  animation_state = AS_RUNNING;
  animation_step = 0;
  
  frontMatrix.clear();
  frontMatrix.drawBitmap(0, 0, heart1_bmp, 8, 8, LED_ON);
  frontMatrix.setBrightness(animation_steps[animation_step % 32]);
  frontMatrix.writeDisplay();
  
}

void continue_heart_animation() {
  if (animation_state == AS_RUNNING) {
    animation_step++;
    frontMatrix.setBrightness(animation_steps[animation_step % 36]);
    frontMatrix.writeDisplay();
  }
  
}

void stop_heart_animation() {
  animation_state = AS_STOPPED;
  animation_step = 0;
  
  frontMatrix.clear();
  frontMatrix.writeDisplay();
  
}
