from tkinter import *
from tkVideoPlayer import TkinterVideo

def start(root):
    root.title("Application")
    
    root.geometry("1080x720")
    root.configure(bg='white')
    global videoplayer
    videoplayer = TkinterVideo(master= root, scaled = False)
    videoplayer.load("buffer.mp4")

    videoplayer.pack(anchor=S, expand= True, fill= "both")
    # videoplayer.place(x = 100, y= 200)
    
    videoplayer.play()
    global temp
    temp = 3
    videoplayer.bind('<<Ended>>', loopVideo)
    
    root.mainloop()

def loopVideo(event):
    global temp
    temp -= 1
    # print(temp)
    if temp >=0:
        videoplayer.play()
    # if temp < 0:
    #     videoplayer.destroy()
    
        
    


def call():
    root = Tk()
    start(root)
    

call()