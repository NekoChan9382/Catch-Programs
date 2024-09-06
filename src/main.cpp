#include "mbed.h"
#include "Servo.hpp"
#include <cstdint>

//CAN can1(PA_11, PA_12, (int)1e6);            // CAN初期化
BufferedSerial serial(USBTX, USBRX, 115200); // シリアル初期化
int16_t can_output_1[4] = {0};               // CAN送信データ アーム展開 0:掬い 1:妨害 2:支え 3:掬い上下
int16_t can_output_2[4] = {0};               // CAN送信データ その他 0:アーム左右 1:LT 2:ベルト前後 3:ベルト左右
CANMessage msg1;
CANMessage msg2;                              // CANメッセージ定義
DigitalOut led(LED1);                        // LED初期化
DigitalIn sw(BUTTON1, PullUp);               // スイッチ初期化

AnalogIn CdS_left(PA_4);
AnalogIn CdS_right(PB_0);
AnalogIn CdS_forward(PC_1);
AnalogIn CdS_center(PC_0);
AnalogIn CdS_back(PC_2);
AnalogIn CdS_first(PC_3);
AnalogIn CdS_belt(PC_4);

DigitalOut LED_vertic(PB_2);
DigitalOut LED_belt(PB_1);
DigitalOut LED_horizon(PB_15);

PwmOut big_belt(PA_0);
PwmOut small_belt(PA_1);

uint8_t seto_catched[6][3] = {0};

float CdS_base[7] = {0.0}; // CdS基準値 0:左 1:右 2:前 3:初期 4:中央 5:後 6:ベルト
float CdS_now[7] = {0.0};  // CdS現在値 0:左 1:右 2:前 3:初期 4:中央 5:後 6:ベルト

int goal[2] = {0, 1}; // 目標位置情報 x,y
int pos[2] = {0, 1};  // 位置情報 x,y

bool auto_sort = 0;
bool big_belt_move = 0;
bool small_belt_move = 0;
bool detect_seto = 0;
bool belt_orientation_vertical = 0;
bool belt_orientation_horizontal = 0;
bool wait_belt_move = 0;
bool at_CdS[2] = {0}; // 0:horizontal 1:vertical


void set_goal(int case_num)
{
    goal[0] = case_num / 3;

    switch (case_num % 3)
    {
    case 1:
        goal[1] = 0;
        break;
    case 2:
        goal[1] = 4;
        break;
    case 0:
        goal[1] = 6;
        break;
    }
    printf("goal,%d, %d\n", goal[0], goal[1]);
}

void CdS_calibrate()
{
    CdS_base[0] = CdS_left.read() +0.5;
    CdS_base[1] = CdS_right.read() + 0.5;
    CdS_base[2] = CdS_forward.read() +0.5;
    CdS_base[3] = CdS_first.read() + 0.5;
    CdS_base[4] = CdS_center.read() +0.5;
    CdS_base[5] = CdS_back.read() + 0.1;
    CdS_base[6] = CdS_belt.read() - 0.1;
}

void CdS_read()
{
    CdS_now[0] = CdS_left.read();
    CdS_now[1] = CdS_right.read();
    CdS_now[2] = CdS_forward.read();
    CdS_now[3] = CdS_first.read();
    CdS_now[4] = CdS_center.read();
    CdS_now[5] = CdS_back.read();
    CdS_now[6] = CdS_belt.read();
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

    for (int i = 2; i < 6; i++) // y座標取得
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
    LED_belt = 1;
    LED_horizon = 1;
    LED_vertic = 1;
    ThisThread::sleep_for(1s);
    CdS_calibrate();
    small_belt = 0.1;
    big_belt = 0.1;
    printf("auto,0\n");
    int sort_res = -1; // ソート結果保存
    int detect_seto_kind = -1;

    while (1)
    {

        int8_t CAN_Send;   // CAN送信データ保存

        if (serial.readable())
        {
            int i = 0;       // 繰り返し変数
            char buff = '0'; // シリアル受信
            char data[5];    // 受信データ保存

            while (buff != '\0' and i < 5)
            {
                serial.read(&buff, sizeof(buff)); // シリアル受信
                data[i] = buff;                   // 受信データ保存
                i++;
            }

            if (strcmp(data, "w\0") == 0) // 掬い上昇
            {
                can_output_1[3] = 8000;
            }
            else if (strcmp(data, "s\0") == 0) // 掬い下降
            {
                can_output_1[3] = -8000;
            }
            else if (strcmp(data, "w0\0") == 0 or strcmp(data, "s0\0") == 0) // 掬い停止
            {
                can_output_1[3] = 0;
            }

            else if (strcmp(data, "a\0") == 0) // アーム左
            {
                can_output_2[0] = 8000;
            }
            else if (strcmp(data, "d\0") == 0) // アーム右
            {
                can_output_2[0] = -8000;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) // アーム停止
            {
                can_output_2[0] = 0;
            }

            else if (strcmp(data, "a\0") == 0) // 妨害アーム展開
            {
                can_output_1[1] = 8000;
            }
            else if (strcmp(data, "d\0") == 0) // 妨害アーム収納
            {
                can_output_1[1] = -8000;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) // 妨害アーム停止
            {
                can_output_1[1] = 0;
            }

            else if (strcmp(data, "a\0") == 0) // 支えアーム展開
            {
                can_output_1[2] = 8000;
            }
            else if (strcmp(data, "d\0") == 0) // 支えアーム収納
            {
                can_output_1[2] = -8000;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) // 支えアーム停止
            {
                can_output_1[2] = 0;
            }

            else if (strcmp(data, "a\0") == 0) // 掬いアーム展開
            {
                can_output_1[0] = 8000;
            }
            else if (strcmp(data, "d\0") == 0) // 掬いアーム収納
            {
                can_output_1[0] = -8000;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) // 掬いアーム停止
            {
                can_output_1[0] = 0;
            }

            else if (strcmp(data, "a\0") == 0) // LTステージ側
            {
                can_output_2[1] = 8000;
            }
            else if (strcmp(data, "d\0") == 0) // LTトレー側
            {
                can_output_1[1] = -8000;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) // LT停止
            {
                can_output_1[1] = 0;
            }

            else if (strcmp(data, "auto\0") == 0)
                {
                    auto_sort = !auto_sort;
                    printf("auto,%d\n", auto_sort);
                }

            if (auto_sort) // 自動仕分けモード時
            {
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
                if (strcmp(data, "w\0") == 0) // コンベア左
                {
                    can_output_2[3] = 8000;
                }
                else if (strcmp(data, "s\0") == 0) // コンベア右
                {
                    can_output_1[3] = -8000;
                }
                else if (strcmp(data, "w0\0") == 0 or strcmp(data, "s0\0") == 0) // コンベア左右停止
                {
                    can_output_1[3] = 0;
                }

                else if (strcmp(data, "w\0") == 0) // コンベア前
                {
                    can_output_2[2] = 8000;
                }
                else if (strcmp(data, "s\0") == 0) // コンベア後
                {
                    can_output_1[2] = -8000;
                }
                else if (strcmp(data, "w0\0") == 0 or strcmp(data, "s0\0") == 0) // コンベア前後停止
                {
                    can_output_1[2] = 0;
                }

                
                if (strcmp(data, "case\0") == 0)
                {
                    int upload[3] = {0};
                    serial.read(&upload[0], sizeof(upload[0]));
                    serial.read(&upload[1], sizeof(upload[1]));
                    serial.read(&upload[2], sizeof(upload[2]));

                    upload[0] -= '0' - 1;
                    upload[1] -= '0';
                    upload[2] -= '0';

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
            
            if (CdS_now[6] < CdS_base[6])
            {
                big_belt = 0.0;
            }
            if (detect_seto)
            {
                small_belt = 0.0;
                big_belt = 0.0;
                set_goal(sort_res +1);
                wait_belt_move = 1;
                detect_seto = 0;
                printf("stop,1\n");
            }

            belt_orientation_horizontal = (goal[0] > pos[0]);
            belt_orientation_vertical = (goal[1] > pos[1]);

            if (goal[0] != pos[0])
            {
                if (belt_orientation_horizontal)
                {
                    can_output_2[3] = 8000;
                }
                else
                {
                    can_output_2[3] = -8000;
                }
            }
            else
            { 
                can_output_2[3] = 0;
            }

            if (goal[1] != pos[1])
            {
                if (belt_orientation_vertical)
                {
                    can_output_2[2] = 8000;
                }
                else
                {
                    can_output_2[2] = -8000;
                }
            }
            else
            {
                can_output_2[2] = 0;
            }
            if (goal[0] == pos[0] && goal[1] == pos[1] && wait_belt_move)
            {
                small_belt = 0.1;
                big_belt = 0.1;
                wait_belt_move = 0;
                ++seto_catched[sort_res][detect_seto_kind];
                send_case_data();
                printf("stop,0\n");
            }

        }

        CANMessage msg1(4, (const uint8_t *)can_output_1, 8); // メッセージ構築
        CANMessage msg2(1, (const uint8_t *)can_output_2, 8); // メッセージ構築

        //can1.write(msg1); // CAN送信
    }
}