#include "mbed.h"
#include "Servo.hpp"
#include <cstdint>

CAN can1(PA_11, PA_12, (int)1e6);            // CAN初期化
BufferedSerial serial(USBTX, USBRX, 115200); // シリアル初期化
int16_t can_output_1[4] = {0};               // CAN送信データ 0:掬い展開 1:支え展開 2:回収系上下 3:アーム左右
int16_t can_output_2[4] = {0};               // CAN送信データ  0:妨害展開 1:LT移動 2:ベルト前後 3:ベルト左右
int16_t can_output3[4] = {0};                // CAN送信データ  0:移動ベルト
CANMessage msg1;
CANMessage msg2;                              // CANメッセージ定義
CANMessage msg3;                              // CANメッセージ定義
DigitalOut EB_led(PA_0);                        // 動力表示灯初期化
DigitalIn sw(BUTTON1, PullUp);               // スイッチ初期化

AnalogIn CdS_left(PA_4);
AnalogIn CdS_right(PB_0);
AnalogIn CdS_forward(PC_1);
AnalogIn CdS_center(PC_0);
AnalogIn CdS_back(PC_2);
//AnalogIn CdS_first(PC_3);
//AnalogIn CdS_belt(PC_4);

DigitalOut LED_vertic(PB_2);
//DigitalOut LED_belt(PB_1);
DigitalOut LED_horizon(PB_15);

uint8_t seto_catched[6][3] = {0};

float CdS_base[5] = {0.0}; // CdS基準値 0:左 1:右 2:前 3:初期 4:中央 5:後 6:ベルト
float CdS_now[5] = {0.0};  // CdS現在値 0:左 1:右 2:前 3:初期 4:中央 5:後 6:ベルト

int goal[2] = {1, 1}; // 目標位置情報 x,y
int pos[2] = {1, 1};  // 位置情報 x,y

bool auto_sort = 0;
bool big_belt_move = 0;
bool detect_seto = 0;
bool belt_orientation_vertical = 0;
bool belt_orientation_horizontal = 0;
bool wait_belt_move = 0;
bool at_CdS[2] = {0}; // 0:horizontal 1:vertical
bool stop_belt[2] = {1}; // 0:horizontal 1:vertical


void set_goal(int case_num)
{
    goal[0] = ((case_num - 1) / 3) * 2;

    switch (case_num % 3)
    {
    case 1:
        goal[1] = 0;
        break;
    case 2:
        goal[1] = 2;
        break;
    case 0:
        goal[1] = 4;
        break;
    }
    printf("goal,%d, %d\n", goal[0], goal[1]);
}

void CdS_calibrate()
{
    CdS_base[0] = CdS_left.read() +0.5;
    CdS_base[1] = CdS_right.read() + 0.5;
    CdS_base[2] = CdS_forward.read() +0.5;
    CdS_base[3] = CdS_center.read() +0.5;
    CdS_base[4] = CdS_back.read() + 0.1;
}

void CdS_read()
{
    CdS_now[0] = CdS_left.read();
    CdS_now[1] = CdS_right.read();
    CdS_now[2] = CdS_forward.read();
    CdS_now[3] = CdS_center.read();
    CdS_now[4] = CdS_back.read();
}

void CdS_pos_read(bool prev_at_CdS_horizontal, bool prev_at_CdS_vertical)
{
    at_CdS[0] = 0;
    at_CdS[1] = 0;

    for (int i = 0; i < 2; i++) // x座標取得
    {
        if (CdS_now[i] > CdS_base[i])
        {
            pos[0] = i * 2;
            at_CdS[0] = 1;
        }
    }

    for (int i = 2; i < 3; i++) // y座標取得
    {
        if (CdS_now[i] > CdS_base[i])
        {   
            pos[1] = (i - 2) * 2;
            at_CdS[1] = 1;
        }
    }
    if (prev_at_CdS_horizontal && !at_CdS[0])
    {
        if (belt_orientation_horizontal)
        {
            pos[0]++;
        }
        else
        {
            pos[0]--;
        }
    }

    if (prev_at_CdS_vertical && !at_CdS[1])
    {
        if (belt_orientation_vertical)
        {
            pos[1]++;
        }
        else
        {
            pos[1]--;
        }
    }
    printf("pos,%d, %d\n", pos[0], pos[1]);
}

void send_case_data()
{
    printf("send_case\n");
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            printf("%d\n", seto_catched[i][j]);
        }
    }
}

int sort(int seto_kind)
{
    for (int i = 0; i < 6; i++)
    {
        if (seto_catched[i][seto_kind] < 3)
        {
            return i;
        }
    }
    return -1;
}


int main()
{
    LED_horizon = 1;
    LED_vertic = 1;
    printf("auto,0\n");
    EB_led = 1;

    ThisThread::sleep_for(1s);
    CdS_calibrate();
    int sort_res = -1; // ソート結果保存
    int detect_seto_kind = -1;

    while (1)
    {

        if (serial.readable())
        {
            int i = 0;       // 繰り返し変数
            char buff = '0'; // シリアル受信
            char data[8];    // 受信データ保存

            while (buff != '\0' and i < 8)
            {
                serial.read(&buff, sizeof(buff)); // シリアル受信
                data[i] = buff;                   // 受信データ保存
                i++;
            }

            if (strcmp(data, "u\0") == 0) // 掬い上昇
            {
                can_output_1[2] = 12000;
            }
            else if (strcmp(data, "p\0") == 0) // 掬い下降
            {
                can_output_1[2] = -12000;
            }
            else if (strcmp(data, "u0\0") == 0 or strcmp(data, "p0\0") == 0) // 掬い停止
            {
                can_output_1[2] = 0;
            }

            else if (strcmp(data, "b\0") == 0) // アーム左
            {
                can_output_1[3] = 12000;
            }
            else if (strcmp(data, "i\0") == 0) // アーム右
            {
                can_output_1[3] = -12000;
            }
            else if (strcmp(data, "b0\0") == 0 or strcmp(data, "i0\0") == 0) // アーム停止
            {
                can_output_1[3] = 0;
            }

            else if (strcmp(data, "e\0") == 0) // 妨害アーム展開
            {
                can_output_2[0] = 20000;
            }
            else if (strcmp(data, "c\0") == 0) // 妨害アーム収納
            {
                can_output_2[0] = -20000;
            }
            else if (strcmp(data, "e0\0") == 0 or strcmp(data, "c0\0") == 0) // 妨害アーム停止
            {
                can_output_2[0] = 0;
            }

            else if (strcmp(data, "l\0") == 0) // 支えアーム展開
            {
                can_output_1[1] = 24000;
            }
            else if (strcmp(data, "k\0") == 0) // 支えアーム収納
            {
                can_output_1[1] = -24000;
            }
            else if (strcmp(data, "l0\0") == 0 or strcmp(data, "k0\0") == 0) // 支えアーム停止
            {
                can_output_1[1] = 0;
            }

            else if (strcmp(data, "m\0") == 0) // 掬いアーム展開
            {
                can_output_1[0] = 24000;
            }
            else if (strcmp(data, "n\0") == 0) // 掬いアーム収納
            {
                can_output_1[0] = -24000;
            }
            else if (strcmp(data, "m0\0") == 0 or strcmp(data, "n0\0") == 0) // 掬いアーム停止
            {
                can_output_1[0] = 0;
            }

            else if (strcmp(data, "r\0") == 0) // LTステージ側
            {
                can_output_2[1] = 8000;
            }
            else if (strcmp(data, "v\0") == 0) // LTトレー側
            {
                can_output_2[1] = -8000;
            }
            else if (strcmp(data, "r0\0") == 0 or strcmp(data, "v0\0") == 0) // LT停止
            {
                can_output_2[1] = 0;
            }

            else if (strcmp(data, "auto\0") == 0)
                {
                    auto_sort = !auto_sort;
                    printf("auto,%d\n", auto_sort);
                    if (auto_sort)
                    {
                        can_output3[0] = 8000;
                    }
                    else
                    {
                        can_output3[0] = 0;
                        stop_belt[0] = 1;
                        stop_belt[1] = 1;
                        detect_seto = 0;
                        wait_belt_move = 0;

                    }
                }

            if (auto_sort) // 自動仕分けモード時
            {
                if (strcmp(data, "shoot\0") == 0)
                {
                    printf("shot\n");
                    can_output3[0] = 8000;
                    wait_belt_move = 0;
                    ++seto_catched[sort_res][detect_seto_kind];
                    send_case_data();
                }
                if (!wait_belt_move){

                    if (strcmp(data, "0\0") == 0)
                    { // えび検知

                        sort_res = sort(0);
                        detect_seto = 1;
                        detect_seto_kind = 0;
                    }
                    else if (strcmp(data, "1\0") == 0)
                    { // のり検知

                        sort_res = sort(1);
                        detect_seto = 1;
                        detect_seto_kind = 1;
                    }
                    else if (strcmp(data, "2\0") == 0)
                    { // ゆず検知

                        sort_res = sort(2);
                        detect_seto = 1;
                        detect_seto_kind = 2;

                    }
                }
            }
            else // 手動仕分けモード時
            {
                if (strcmp(data, "a\0") == 0) // コンベア左
                {
                    can_output_2[3] = 8000;
                }
                else if (strcmp(data, "d\0") == 0) // コンベア右
                {
                    can_output_2[3] = -8000;
                }
                else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) // コンベア左右停止
                {
                    can_output_2[3] = 0;
                }

                else if (strcmp(data, "w\0") == 0) // コンベア前
                {
                    can_output_2[2] = 8000;
                }
                else if (strcmp(data, "s\0") == 0) // コンベア後
                {
                    can_output_2[2] = -8000;
                }
                else if (strcmp(data, "w0\0") == 0 or strcmp(data, "s0\0") == 0) // コンベア前後停止
                {
                    can_output_2[2] = 0;
                }

                if (strcmp(data, "q\0")) // ベルト停止切り替え
                {
                    if (can_output3[0])
                    {
                        can_output3[0] = 0;
                    }
                    else
                    {
                        can_output3[0] = 8000;
                    }
                }

                
                if (strcmp(data, "case\0") == 0)
                {
                    char upload[3] = {0};
                    serial.read(&upload, sizeof(upload));
                    printf("case,%d,%d,%d\n", upload[0], upload[1], upload[2]);

                    upload[0] -= '0';
                    upload[1] -= '0';
                    upload[2] -= '0';
                    printf("case,%d,%d,%d\n", upload[0], upload[1], upload[2]);

                    seto_catched[upload[0]][upload[1]] = upload[2];
                    send_case_data();
                }
            }

            //printf("%d\n", sort_res);
        } // 受信データを送信データに整理

        if (auto_sort)
        { // 自動仕分け
            CdS_read();
            CdS_pos_read(at_CdS[0], at_CdS[1]);
            
            if (detect_seto)
            {
                can_output3[0] = 0;
                set_goal(sort_res +1);
                wait_belt_move = 1;
                detect_seto = 0;
                printf("stop\n");
                stop_belt[0] = 0;
                stop_belt[1] = 0;
            }

            belt_orientation_horizontal = (goal[0] > pos[0]);
            belt_orientation_vertical = (goal[1] > pos[1]);

            if (goal[0] != pos[0] && !stop_belt[0])
            {
                if (belt_orientation_horizontal)
                {
                    can_output_2[3] = 15000;
                }
                else
                {
                    can_output_2[3] = -15000;
                }
            }
            else
            { 
                can_output_2[3] = 0;
            }

            if (goal[1] != pos[1] && !stop_belt[1])
            {
                if (belt_orientation_vertical)
                {
                    can_output_2[2] = 20000;
                }
                else
                {
                    can_output_2[2] = -20000;
                }
            }
            else
            {
                can_output_2[2] = 0;
            }

            if (goal[0] == pos[0])
            {
                stop_belt[0] = 1;
            }
            if (goal[1] == pos[1])
            {
                stop_belt[1] = 1;
            }
            
            if (stop_belt[0] && stop_belt[1] && wait_belt_move) 
            {   
                printf("moved\n");
                can_output3[0] = 8000;
                wait_belt_move = 0;
            }

        }

        CANMessage msg1(2, (const uint8_t *)can_output_1, 8); // メッセージ構築
        CANMessage msg2(4, (const uint8_t *)can_output_2, 8); // メッセージ構築
        CANMessage msg3(1, (const uint8_t *)can_output3, 8); // メッセージ構築

        can1.write(msg1); // CAN送信
        can1.write(msg2); // CAN送信
        can1.write(msg3); // CAN送信
    }
}