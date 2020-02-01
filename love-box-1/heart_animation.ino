#include <Adafruit_LEDBackpack.h>

#define AS_STOPPED  0
#define AS_RUNNING  1
int animation_state = AS_STOPPED;
int animation_step = 0;
unsigned long next_step_time = 0;
int animation_step_count = 36;
unsigned long animation_duration = 4000; // 3 second duration
unsigned long millis_per_step = animation_duration / animation_step_count;

static const uint8_t PROGMEM
  animation_steps[] =
  { 0, 0, 0, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15, 15, 14, 13, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,0,0};

void start_heart_animation() {
  animation_state = AS_RUNNING;
  animation_step = 0;
  
  frontMatrix.clear();
  frontMatrix.drawBitmap(0, 0, heart1_bmp, 8, 8, LED_ON);
  frontMatrix.setBrightness(animation_steps[animation_step % animation_step_count]);
  frontMatrix.writeDisplay();
  next_step_time = get_next_step_millis();
  
}

unsigned long get_next_step_millis() {
  return millis() + millis_per_step;
}

void continue_heart_animation() {
  if (animation_state == AS_RUNNING) {
    unsigned long current_millis = millis();
    if (next_step_time <= current_millis) { // wait millis_per_step before changing the brightness
      animation_step++;
      frontMatrix.setBrightness(animation_steps[animation_step % animation_step_count]);
      frontMatrix.writeDisplay();
      next_step_time = get_next_step_millis();
    }
  }
  
}

void stop_heart_animation() {
  animation_state = AS_STOPPED;
  animation_step = 0;
  
  frontMatrix.clear();
  frontMatrix.writeDisplay();
  
}
