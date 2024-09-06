from ultralytics import YOLO #type:ignore  画像認識
import cv2 #type:ignore  importできてるのにエラー吐くため  カメラ画像取得
import serial  #シリアル通信
import tkinter as tk  #GUI表示
import threading as th  #並行処理
import numpy as np  #配列処理

cam=cv2.VideoCapture(0)  #カメラ初期化
ser=serial.Serial("/dev/ttyACM0",115200,timeout=2)  #シリアル初期化
model = YOLO("src/best.pt")  #学習済モデル

class Serials:  #GUI

    def __init__(self,master,ser):  #initialize
        self.ser=ser  #シリアルのやつ
        self.master=master  #tkのマスターウィンドウ
        self.Thread_stop=False  #プログラム停止フラグ
        self.yolo_res=-1  #画像認識結果
        font = "Yu Gothic UI"  #フォント設定

        self.frame_status=tk.Frame(master,width=1000,height=200,bg=bg)
        self.frame_read=tk.Frame(master,width=1000,height=30,bg=bg)
        self.frame_buttons=tk.Frame(master,width=1000,height=200,bg=bg)

        self.frame_status.place(x=30,y=0)
        self.frame_read.place(x=0,y=570)
        self.frame_buttons.place(x=0,y=200)

        self.status_title=tk.Label(self.frame_status,text="Status",font=(font,15),bg=bg,fg="white")
        self.status_predict=tk.Label(self.frame_status,text="Predict: ",font=(font,15),bg=bg,fg="white")
        self.status_title.place(x=30,y=0)
        self.status_predict.place(x=30,y=30)

        self.raw_read=tk.Label(self.frame_read,text="Serial Data:",font=(font,15),bg=bg,fg="white")
        self.raw_read.place(x=0,y=0)

        self.test=tk.Label(self.frame_status,text="pos: ",font=(font,15),bg=bg,fg="white")
        self.test.place(x=30,y=60)

        self.goal=tk.Label(self.frame_status,text="goal: ",font=(font,15),bg=bg,fg="white")
        self.goal.place(x=30,y=90)

        self.auto_button=tk.Button(self.frame_buttons,text="自動制御切り替え: OFF",font=(font,15),bg=bg,fg="white",command=lambda: self.ser_send("auto\0"))
        self.auto_button.place(x=0,y=0)

        self.keys=[]  #押されているキーを格納

        self.case_seto_data = np.zeros((6,3),dtype=int)  #ケースのセット状況
        self.belt_pos = [0,2]
        self.stop_predict=False
        self.auto_move=False

        master.bind("<KeyPress>",self.key_press)  #キー認識の設定
        master.bind("<KeyRelease>",self.key_release)

    def ser_send(self,send):  #シリアル送信
        self.ser.write(send.encode())  

    def key_press(self,event):  #キー押下受信

        if event.keysym not in self.keys:  #新たに押されたやつだったら

            self.keys.append(event.keysym)
            send=event.keysym+"\0"
            self.ser.write(send.encode())

    def key_release(self,event):  #キー離脱受信

        send=event.keysym+"0\0"
        self.keys.remove(event.keysym)
        self.ser.write(send.encode())

    def read_show(self,text):  #シリアル受信を反映
        
        if text!="":
            self.raw_read.config(text="Serial Data: "+text)

    def yolo(self):  #画像認識

        while not self.Thread_stop:

            ret, frame = cam.read()  #カメラ情報の取得
            if not ret:  #読み込み失敗時
                print("failed")
                break

            results = model.predict(frame,conf=0.8)  #画像認識本体
            
            if (not self.stop_predict) & self.auto_move:  #自動制御時
                
                for r in results:  #結果整理
                    boxes = r.boxes  #結果取得
                    self.cls =[-1]
                    for box in boxes:
                        
                        self.cls.insert(0,box.cls.item()) #0 ebi 1 nori 2 yuzu
                    
                    if len(self.cls)==2:  #結果エコー
                        self.yolo_res=int(self.cls[0])
                        self.status_predict.config(text="Predict: "+str(self.yolo_res).translate(str.maketrans({'0':'ebi','1':'nori','2':'yuzu'})))
                    else:
                        self.yolo_res=-1

                if self.yolo_res != -1:
                    self.ser.write((str(self.yolo_res)+"\0").encode())

                print(self.case_seto_data)

    def ser_read(self):

        while not self.Thread_stop:
                
                reads=self.ser.readline().strip().decode()  #シリアル受信
                self.receive_serial_analysis(reads)
                self.read_show(reads)  #GUI上に反映

    def receive_serial_analysis(self,received):

        received_split=received.split(",")
        if received_split[0]=="auto":
            if (received_split[1]=="1"):
                self.auto_button.config(text="自動制御切り替え: ON")
                self.auto_move=True
            else:
                self.auto_button.config(text="自動制御切り替え: OFF")
                self.auto_move=False

        if received_split[0]=="send_case":
            i=0
            j=0

            for i in range(6):
                for j in range(3):
                    self.case_seto_data[i][j] = int(self.ser.readline().strip().decode())

        if received_split[0]=="pos":
            self.belt_pos[0] = int(received_split[1])
            self.belt_pos[1] = int(received_split[2])
            self.test.config(text="pos: "+str(self.belt_pos[0])+","+str(self.belt_pos[1]))

        if received_split[0]=="goal":
            self.goal.config(text="goal: "+str(received_split[1])+","+str(received_split[2]))

        if received_split[0]=="stop":
            self.stop_predict=int(received_split[1])
            if not (int(received_split[1])):
                self.status_predict.config(text="Predict:")

bg="#202028"

root=tk.Tk()
root.geometry("1000x600")
root.configure(bg=bg)
gui=Serials(root,ser)
yolo_th=th.Thread(target=gui.yolo)
yolo_th.start()
read_th=th.Thread(target=gui.ser_read)
read_th.start()
root.mainloop()
gui.Thread_stop=True