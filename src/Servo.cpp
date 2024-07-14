#include "Servo.hpp"
#include <cstdint>

// サーボ制御用データの宣言
extern uint8_t servo1_data[8];
uint32_t servo_can_id = 141;

// ServoController クラスのコンストラクタ
ServoController::ServoController(CAN& can) : can(can)
{
  // コンストラクタの実装
  // servo1_data 配列を初期化
  memset(servo1_data, 0, sizeof(servo1_data));
}

// サーボ制御の実行
void ServoController::run(uint8_t deg[8], uint8_t motor)
{

      // サーボの制御データを設定
      memcpy(servo1_data, deg, sizeof(deg));

      // CAN メッセージを作成し送信
      auto msg4 = CANMessage{servo_can_id, servo1_data, sizeof(servo1_data)};
      can.write(msg4);

      // 20 ミリ秒待機
      ThisThread::sleep_for(20ms);
    
  
}
