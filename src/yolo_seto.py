from ultralytics import YOLO #type:ignore  画像認識
import cv2 #type:ignore  importできてるのにエラー吐くため  カメラ画像取得
import serial  #シリアル通信
import tkinter as tk  #GUI表示
import threading as th  #並行処理

cam=cv2.VideoCapture(0)  #カメラ初期化
ser=serial.Serial("COM8",115200,timeout=2)  #シリアル初期化
model = YOLO('src\\best.pt')  #学習済モデル

class Serials:  #GUI

    def __init__(self,master,ser):  #initialize
        self.ser=ser  #シリアルのやつ
        self.master=master  #tkのマスターウィンドウ
        self.Thread_stop=False  #プログラム停止フラグ
        self.yolo_res=-1  #画像認識結果

        send1=tk.Button(master,text="1を送信",command=lambda: self.ser_send("1\0"))  #ボタン作成
        send2=tk.Button(master,text="2を送信",command=lambda: self.ser_send("2\0"))
        send3=tk.Button(master,text="3を送信",command=lambda: self.ser_send("3\0"))
        send1.pack()
        send2.pack()
        send3.pack()
        
        self.read=tk.Button(master,text="受信")
        self.read.pack()

        self.keys=[]  #押されているキーを格納

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
            self.read.config(text=text)

    def yolo(self):  #画像認識

        while not self.Thread_stop:

            ret, frame = cam.read()  #カメラ情報の取得
            if not ret:  #読み込み失敗時
                print("failed")
                break

            results = model.predict(frame,conf=0.8)  #画像認識本体

            for r in results:  #結果整理
                boxes = r.boxes  #結果取得
                self.cls =[-1]
                for box in boxes:
                    
                    self.cls.insert(0,box.cls.item()) #0 ebi 1 nori 2 yuzu
                
                if len(self.cls)==2:  #結果エコー
                    self.yolo_res=int(self.cls[0])
                else:
                    self.yolo_res=-1

    def ser_read(self):

        while not self.Thread_stop:
            reads=self.ser.readline().strip().decode()  #シリアル受信
            self.read_show(reads)  #GUI上に反映
            if reads=="quit":
                self.Thread_stop=True
                break
            if reads=="read":
                print(self.cls[0])
                self.ser.write((str(self.yolo_res)+"\0").encode())


root=tk.Tk()
gui=Serials(root,ser)
yolo_th=th.Thread(target=gui.yolo)
yolo_th.start()
read_th=th.Thread(target=gui.ser_read)
read_th.start()
root.mainloop()
gui.Thread_stop=True