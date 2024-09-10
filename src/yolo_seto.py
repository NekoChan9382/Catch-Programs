from ultralytics import YOLO #type:ignore  画像認識
import cv2 #type:ignore  importできてるのにエラー吐くため  カメラ画像取得
import serial  #シリアル通信
import tkinter as tk  #GUI表示
import threading as th  #並行処理
import numpy as np  #配列処理
import subprocess  #シェルコマンド実行

cam=cv2.VideoCapture(0)  #カメラ初期化
ser=serial.Serial("/dev/ttyACM0",115200,timeout=0.5)  #シリアル初期化
model = YOLO("src/new_seto.pt")  #学習済モデル

class Serials:  #GUI

    def __init__(self,master,ser):  #initialize
        self.ser=ser  #シリアルのやつ
        self.master=master  #tkのマスターウィンドウ
        self.Thread_stop=False  #プログラム停止フラグ
        self.yolo_res=-1  #画像認識結果
        self.yolo_conf=0  #画像認識信頼度
        font = "Yu Gothic UI"  #フォント設定
        self.encode="ASCII"  #文字コード設定

        self.frame_status=tk.Frame(master,width=700,height=200,bg=bg)
        self.frame_read=tk.Frame(master,width=700,height=30,bg=bg)
        self.frame_buttons=tk.Frame(master,width=700,height=200,bg=bg)
        self.frame_tray=tk.Frame(master,width=300,height=570,bg=bg)

        self.frame_status.place(x=30,y=0)
        self.frame_read.place(x=0,y=570)
        self.frame_buttons.place(x=0,y=200)
        self.frame_tray.place(x=700,y=0)

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

        self.auto_button=tk.Button(self.frame_buttons,text="自動制御開始",font=(font,15),bg=bg,fg="white",command=lambda: self.ser_send("auto\0"))
        self.auto_button.place(x=0,y=0)

        self.tray_canvas=tk.Canvas(self.frame_tray,width=300,height=570,bg=bg)
        self.tray_canvas.place(x=0,y=0)

        self.tray_canvas.create_rectangle(30,30,130,180,fill=bg,outline="white",width=2)
        self.tray_canvas.create_rectangle(30,200,130,350,fill=bg,outline="white",width=2)
        self.tray_canvas.create_rectangle(30,370,130,520,fill=bg,outline="white",width=2)
        self.tray_canvas.create_rectangle(160,30,260,180,fill=bg,outline="white",width=2)
        self.tray_canvas.create_rectangle(160,200,260,350,fill=bg,outline="white",width=2)
        self.tray_canvas.create_rectangle(160,370,260,520,fill=bg,outline="white",width=2)

        self.tray_button=[[tk.Button(self.frame_tray,text="ebi: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(0,0)),tk.Button(self.frame_tray,text="nori: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(0,1)),tk.Button(self.frame_tray,text="yuzu: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(0,2))],
                            [tk.Button(self.frame_tray,text="ebi: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(1,0)),tk.Button(self.frame_tray,text="nori: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(1,1)),tk.Button(self.frame_tray,text="yuzu: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(1,2))],
                            [tk.Button(self.frame_tray,text="ebi: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(2,0)),tk.Button(self.frame_tray,text="nori: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(2,1)),tk.Button(self.frame_tray,text="yuzu: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(2,2))],
                            [tk.Button(self.frame_tray,text="ebi: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(3,0)),tk.Button(self.frame_tray,text="nori: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(3,1)),tk.Button(self.frame_tray,text="yuzu: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(3,2))],
                            [tk.Button(self.frame_tray,text="ebi: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(4,0)),tk.Button(self.frame_tray,text="nori: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(4,1)),tk.Button(self.frame_tray,text="yuzu: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(4,2))],
                            [tk.Button(self.frame_tray,text="ebi: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(5,0)),tk.Button(self.frame_tray,text="nori: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(5,1)),tk.Button(self.frame_tray,text="yuzu: 0",font=(font,10),bg=bg,fg="white",command=lambda: self.update_tray(5,2))]]

        for i in range(3):
            for j in range(3):
                self.tray_button[i][j].place(x=40, y=40 + i*170 + j*50)

        for i in range(3):
            for j in range(3):
                self.tray_button[i+3][j].place(x=170, y=40 + i*170 + j*50)

        self.keys=[]  #押されているキーを格納

        self.case_seto_data = np.zeros((6,3),dtype=int)  #ケースのセット状況
        self.belt_pos = [0,2]
        self.stop_predict=False
        self.auto_move=False
        self.between_goal_to_shoot = False
        self.toray_update_select = [0,0,0]

        master.bind("<KeyPress>",self.key_press)  #キー認識の設定
        master.bind("<KeyRelease>",self.key_release)

    def ser_send(self,send):  #シリアル送信
        self.ser.write(send.encode(self.encode))  

    def key_press(self,event):  #キー押下受信

        if event.keysym not in self.keys:  #新たに押されたやつだったら

            if "0" <= event.keysym and event.keysym <= "9":
                if self.toray_update_select[0] == 1:
                    self.ser.write(("case\0" + str(self.toray_update_select[1])+str(self.toray_update_select[2])+event.keysym).encode(self.encode))
                    print("case\0" + str(self.toray_update_select[1])+str(self.toray_update_select[2])+event.keysym)
                    self.toray_update_select = [0,0,0]
                    for i in range(6):
                        for j in range(3):
                            self.tray_button[i][j].config(fg="white")
                
            else:
                self.keys.append(event.keysym)
                send=event.keysym+"\0"
                self.ser.write(send.encode(self.encode))

    def key_release(self,event):  #キー離脱受信

        if not ("0" <= event.keysym and event.keysym <= "9"):

            send=event.keysym+"0\0"
            self.keys.remove(event.keysym)
            self.ser.write(send.encode(self.encode))

    def read_show(self,text):  #シリアル受信を反映
        
        if text!="":
            self.raw_read.config(text="Serial Data: "+text)

    def yolo(self):  #画像認識

        while not self.Thread_stop:

            ret, frame = cam.read()  #カメラ情報の取得
            if not ret:  #読み込み失敗時
                print("failed")
                break

            results = model.predict(frame,conf=0.7)  #画像認識本体
            
            if ((not self.stop_predict) & self.auto_move) | self.between_goal_to_shoot:  #自動制御時
                
                for r in results:  #結果整理
                    boxes = r.boxes  #結果取得
                    self.cls =[-1]
                    for box in boxes:
                        
                        self.cls.insert(0,box.cls.item()) #0 ebi 1 nori 2 yuzu
                    
                    if self.cls[0] == self.yolo_res:
                            self.yolo_conf += 1

                    else:
                        self.yolo_conf = 0
                    
                    if len(self.cls)==2:  #結果エコー
                        
                        self.yolo_res=int(self.cls[0])
                    
                    elif len(self.cls)==1: #結果なし
                        self.yolo_res=-1
                        if self.between_goal_to_shoot and self.yolo_conf >= 5:
                            self.ser.write("shoot\0".encode(self.encode))
                            print("shoot")
                            self.between_goal_to_shoot = False
                            self.stop_predict = False
                            self.yolo_conf = 0
                            self.status_predict.config(text="Predict: ")
                    

                if self.yolo_res != -1 and self.yolo_conf >= 5:
                    self.ser.write((str(self.yolo_res)+"\0").encode(self.encode))
                    self.status_predict.config(text="Predict: "+str(self.yolo_res).translate(str.maketrans({'0':'ebi','1':'nori','2':'yuzu'})))


    def ser_read(self):

        while not self.Thread_stop:
                
                reads=self.ser.readline().strip().decode(self.encode)  #シリアル受信
                
                self.receive_serial_analysis(reads)
                self.read_show(reads)  #GUI上に反映

    def receive_serial_analysis(self,received):

        received_split=received.split(",")
        if received_split[0]=="auto":
            if (received_split[1]=="1"):
                self.auto_button.config(text="自動制御停止")
                self.auto_move=True
            else:
                self.auto_button.config(text="自動制御開始")
                self.auto_move=False

        if received_split[0]=="send_case":
            i=0
            j=0

            for i in range(6):
                for j in range(3):
                    self.case_seto_data[i][j] = int(self.ser.readline().strip().decode(self.encode))
                    self.tray_button[i][j].config(text=self.case_seto_data[i][j])
            print("end\n")

        if received_split[0]=="pos":
            self.belt_pos[0] = int(received_split[1])
            self.belt_pos[1] = int(received_split[2])
            self.test.config(text="pos: "+str(self.belt_pos[0])+","+str(self.belt_pos[1]))
        else:
            print(received)

        if received_split[0]=="goal":
            self.goal.config(text="goal: "+str(received_split[1])+","+str(received_split[2]))

        if received_split[0]=="stop":
            self.stop_predict = True
            
        if received_split[0]=="moved":
            self.between_goal_to_shoot = True

    def update_tray(self,toray,kind):
        if not self.auto_move:
            for i in range(6):
                for j in range(3):
                    self.tray_button[i][j].config(fg="white")
            if self.tray_button[toray][kind]["fg"] == "white":
                self.toray_update_select = [1,toray,kind]
                self.tray_button[toray][kind].config(fg="#ffaa30")
            else:
                self.toray_update_select = [0,0,0]
                self.tray_button[toray][kind].config(fg="white")

bg="#202028"

subprocess.run(["xset","r","off"])  #キーリピート設定
subprocess.run(["xmodmap","/home/bit/.Xmodmap"])  #スクリーンセーバー設定
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