# SuperFastPython.com
# example of a long-running daemon thread
from time import sleep
from random import random
from threading import Thread
 

from tkinter import *
from tkinter import ttk
import time
from tkinter import *
from tkVideoPlayer import TkinterVideo
from utility import result
import subprocess
import os

def call(image):
    # image = "Daksh.jpg"

    global my_progress_bar,videoplayer,root, temp

    root = Tk()
    root.title("Output share Receiver")
    root.geometry("300x300")

    # root.title("Application")

    # root.geometry("300x300")
    root.configure(bg='white')

    temp = True

    # long-running background task
    def background_task():
        global videoplayer, temp
        while True:
            if (not temp):
                break

            else:
                base_dir = os.getenv("BASE_DIR")
                f = open("output.txt", "w")
                iter = subprocess.call("python3 " + base_dir + "/Dataprovider/image_provider/preprocess_image.py -f "+image, shell=True)
                subprocess.call(base_dir + "/scripts/ServerModel_Architecture/HelperNode/ImageProvider_genr.sh", stdout=f)
                temp = False
                time.sleep(1)


    # create and start the daemon thread
    print('Starting background task...')
    daemon = Thread(target=background_task, daemon=True, name='Monitor')
    daemon.start()
    # main thread is carrying on...
    print('Main thread is carrying on...')

    videoplayer = TkinterVideo(master= root, scaled = False)
    videoplayer.load("./utility/Images_Video/buffer1.mp4")

    videoplayer.pack(anchor=S, expand= True, fill= "both")
    
    # videoplayer.place(x = 100, y= 200)


    def loopVideo(event):
        global temp,my_progress_bar,videoplayer,root
        
        # print(temp)
        if temp:
            videoplayer.play()


        if not temp:
            root.destroy()
            result.call(image)


    videoplayer.play()

    temp = 3
    videoplayer.bind('<<Ended>>', loopVideo)


    root.mainloop()
    print('Main thread done.')

# call("Daksh.jpg")