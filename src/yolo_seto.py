from ultralytics import YOLO #type:ignore  画像認識
import cv2 #type:ignore  importできてるのにエラー吐くため  カメラ画像取得
import serial  #シリアル通信
import tkinter as tk  #GUI表示
import threading as th  #並行処理

cam=cv2.VideoCapture(0)  #カメラ初期化
ser=serial.Serial("COM8",115200)  #シリアル初期化
model = YOLO('src\\best.pt')  #学習済モデル

Thread_stop=False  #プログラム停止フラグ

def yolo():  #画像認識

    while not Thread_stop:

        ret, frame = cam.read()  #カメラ情報の取得
        if not ret:  #読み込み失敗時
            break

        results = model.predict(frame,conf=0.8)  #画像認識本体

        for r in results:  #結果整理
            boxes = r.boxes  #結果取得
            cls =[]
            for box in boxes:
                
                cls.append(box.cls.item()) #0 ebi 1 nori 2 yuzu
            
            if len(cls)==1:  #結果エコー
                if cls[0]==0:
                    print("ebi")
                    ser.write(b"1\0")
                elif cls[0]==1:
                    print("nori")
                    ser.write(b"2\0")
                elif cls[0]==2:
                    print("yuzu")
                    ser.write(b"3\0")

def ser_read():
    while not Thread_stop:
        reads=ser.readline()  #シリアル受信
        gui.ser_read(reads.decode())  #GUI上に反映
        print(reads.decode())

class Serials:  #GUI



    def __init__(self,master,ser):  #initialize
        self.ser=ser  #シリアルのやつ
        self.master=master  #tkのマスターウィンドウ

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

    def ser_read(self,text):  #シリアル受信を反映
        
        self.read.config(text=text)
        print(text)


root=tk.Tk()
gui=Serials(root,ser)
yolo_th=th.Thread(target=yolo)
yolo_th.start()
read_th=th.Thread(target=ser_read)
read_th.start()
root.mainloop()
Thread_stop=True