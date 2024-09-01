#include "mbed.h"
#include "Servo.hpp"
#include <cstdint>

CAN can1(PA_11, PA_12, (int)1e6); //CAN初期化
BufferedSerial serial(USBTX, USBRX, 115200); //シリアル初期化
int16_t can_output_1[4] = {0}; //CAN送信データ アーム展開 0:掬い 1:妨害 2:支え 3:掬い上下
int16_t can_output_2[4] = {0}; //CAN送信データ その他 0:アーム左右 1:LT 2:ベルト前後 3:ベルト左右
CANMessage msg; //CANメッセージ定義
DigitalOut led(LED1); //LED初期化
DigitalIn sw(BUTTON1,PullUp); //スイッチ初期化

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

uint8_t seto_catched[6][3]={0};

float CdS_base[7] = {0.0};

int goal[2] = {0,2}; //目標位置情報 x,y
int pos[2] = {0,2}; //位置情報 x,y

bool auto_sort = 0;
bool big_belt_move = 0;
bool small_belt_move = 0;
bool detect_seto = 0;
bool belt_orientation_vertical = 0;
bool belt_orientation_horizontal = 0;

int sort(int seto_kind)
{
    for (int i = 0; i < 6; i++)
    {
        if (seto_catched[i][seto_kind]<3)
        {
            seto_catched[i][seto_kind]++;
            return i;
        }
    }
    return -1;
}

void set_goal(int case_num){
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
}

void calibrate(){
    CdS_base[0] = CdS_left.read() -1.0;
    CdS_base[1] = CdS_right.read() +0.5;
    CdS_base[2] = CdS_forward.read() -1.0;
    CdS_base[3] = CdS_center.read() -1.0;
    CdS_base[4] = CdS_back.read() -1.0;
    CdS_base[5] = CdS_first.read() +0.5;
    CdS_base[6] = CdS_belt.read() -1.0;
}
int main()
{
    LED_belt = 1;
    LED_horizon = 1;
    LED_vertic = 1;
    ThisThread::sleep_for(1s);
    calibrate();
    

    while (1)
    {

        int8_t CAN_Send; //CAN送信データ保存
        int sort_res=-1; //ソート結果保存

        if (serial.readable())
        {
            int i = 0; //繰り返し変数
            char buff='0'; //シリアル受信
            char data[5]; //受信データ保存

            while (buff != '\0' and i < 5)
            {
                serial.read(&buff, sizeof(buff)); //シリアル受信
                data[i] = buff; //受信データ保存
                i++;

            }

            if (strcmp(data, "w\0") == 0) //掬い上昇
            {
                CAN_Send = 1;
            }
            else if (strcmp(data, "s\0") == 0) //掬い下降
            {
                CAN_Send = -1;
            }
            else if (strcmp(data, "w0\0") == 0 or strcmp(data, "s0\0") == 0) //掬い停止
            {
                CAN_Send = 10;
            }else if (strcmp(data, "a\0") == 0) //アーム左
            {
                CAN_Send = 2;
            }
            else if (strcmp(data, "d\0") == 0) //アーム右
            {
                CAN_Send = -2;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) //アーム停止
            {
                CAN_Send = 20;
            }
            else if (strcmp(data, "a\0") == 0) //妨害アーム展開
            {
                CAN_Send = 3;
            }
            else if (strcmp(data, "d\0") == 0) //妨害アーム収納
            {
                CAN_Send = -3;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) //妨害アーム停止
            {
                CAN_Send = 30;
            }
            else if (strcmp(data, "a\0") == 0) //支えアーム展開
            {
                CAN_Send = 4;
            }
            else if (strcmp(data, "d\0") == 0) //支えアーム収納
            {
                CAN_Send = -4;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) //支えアーム停止
            {
                CAN_Send = 40;
            }
            else if (strcmp(data, "a\0") == 0) //掬いアーム展開
            {
                CAN_Send = 5;
            }
            else if (strcmp(data, "d\0") == 0) //掬いアーム収納
            {
                CAN_Send = -5;
            }
            else if (strcmp(data, "a0\0") == 0 or strcmp(data, "d0\0") == 0) //掬いアーム停止
            {
                CAN_Send = 50;
            }
            else if (strcmp(data, "0\0") == 0){ //えび検知

                sort_res=sort(0) + 1;
                detect_seto = 1;
            }
            else if (strcmp(data, "1\0") == 0){ //のり検知

                sort_res=sort(1) + 1;
                detect_seto = 1;

            }
            else if (strcmp(data, "2\0") == 0){ //ゆず検知

                sort_res=sort(2) + 1;
                detect_seto = 1;

            }

            printf("%d\n",sort_res);
        }//受信データを送信データに整理
        
        if (auto_sort){ //自動仕分け
            if (CdS_belt.read() < CdS_base[6]){
                big_belt = 0.0;
            }
            if (detect_seto){
                small_belt = 0.0;
                big_belt = 0.0;
                set_goal(sort_res);
                belt_orientation_horizontal = (goal[0] > pos[0]);
                belt_orientation_vertical = (goal[1] > pos[1]);
            }
            if (goal[0] != pos[0]){
                if (belt_orientation_horizontal){
                    can_output_2[3] = 8000;
                }else{
                    can_output_2[3] = -8000;
                }
            }else{
                can_output_2[3] = 0;
            }

            if (goal[1] != pos[1]){
                if (belt_orientation_vertical){
                    can_output_2[2] = 8000;
                }else{
                    can_output_2[2] = -8000;
                }
            }else{
                can_output_2[2] = 0;
            }
        }

        CANMessage msg(4, (const uint8_t *)can_output_1, 8); //メッセージ構築
        CANMessage msg(1, (const uint8_t *)can_output_2, 8); //メッセージ構築

        can1.write(msg); //CAN送信
        
    }
}