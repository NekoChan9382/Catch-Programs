from ultralytics import YOLO
import cv2
import serial
import tkinter as tk
import threading as th

cam=cv2.VideoCapture(0)
ser=serial.Serial("COM3",115200,timeout=2)
model = YOLO('src\\best.pt')

Thread_stop=False

def yolo():

    while not Thread_stop:

        ret, frame = cam.read()
        if not ret:
            break

        results = model.predict(frame,conf=0.8)

        for r in results:
            boxes = r.boxes
            cls =[]
            for box in boxes:
                
                cls.append(box.cls.item()) #0 ebi 1 nori 2 yuzu
            
            if len(cls)==1:
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
        reads=ser.readline()
        gui.ser_read(reads.decode())
        print(reads.decode())

class Serials:



    def __init__(self,master,ser):
        self.ser=ser
        self.master=master

        send1=tk.Button(master,text="1を送信",command=lambda: self.ser_send("1\0"))
        send2=tk.Button(master,text="2を送信",command=lambda: self.ser_send("2\0"))
        send3=tk.Button(master,text="3を送信",command=lambda: self.ser_send("3\0"))
        send1.pack()
        send2.pack()
        send3.pack()
        
        self.read=tk.Button(master,text="受信")
        self.read.pack()

        self.keys=[]

        master.bind("<KeyPress>",self.key_press)
        master.bind("<KeyRelease>",self.key_release)

    def ser_send(self,send):
        self.ser.write(send.encode())

    def key_press(self,event):

        if event.keysym not in self.keys:

            self.keys.append(event.keysym)
            send=event.keysym+"\0"
            self.ser.write(send.encode())

    def key_release(self,event):

        send=event.keysym+"0\0"
        self.keys.remove(event.keysym)
        self.ser.write(send.encode())

    def ser_read(self,text):
        
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