#ifndef SERVO_HP
#define SERVO_HP

#include "mbed.h"

extern uint8_t servo1_data[8];

class ServoController {
public:
  ServoController(CAN& can);
  uint32_t servo_can_id;

  void run(uint8_t deg[8], uint8_t motor);

private:
  CAN& can;

  uint8_t servo1_data[8] = {0}; // サーボ制御用データの初期化
};

#endif
