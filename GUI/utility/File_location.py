from tkinter import *
from tkinter import filedialog, PhotoImage
from PIL import ImageTk, Image
from tkinter.filedialog import askopenfile
from utility import SMPC
from utility import NN
from utility import loading
import subprocess
import os


def call(fileLocation):

    try:
        root = Tk()
        global myLabel, filepath
        filepath = fileLocation 
        root.title("Upload File")
        # root.attributes('-fullscreen', True)


        root.geometry("1080x800")

        myLabel = Label(root)

        # my_img = ImageTk.PhotoImage(Image.open("Daksh.jpg"))
        # myLabel = Label(image=my_img)
        # myLabel.pack()
        

        def FilePath():
            global filepath
            filepath = filedialog.askopenfilename(filetypes=[("images",".jpeg"),("images",".jpg"),("images",".png")])
            e.delete(0,END)
            e.insert(0,str(filepath))    
            # print(filepath)


        def openFile(path):
            # my_img_1 = ImageTk.PhotoImage(Image.open("download.png"))
            # print(my_img_1)
            # # myLabel = Label(image=my_img)
            # myLabel.config(image=my_img_1)
            # canvas = Canvas(root, width = 500, height = 1100)  
            # canvas.grid(row=2,column=0, columnspan=2, padx=5,pady=5)
            # img = ImageTk.PhotoImage(Image.open("/home/daksh1115/Desktop/IUDX/Tkinter/Learning/Project 1/Daksh.jpg"))  
            # canvas.create_image(20, 20, anchor=NW, image=img) 
            try:
                global myLabel
                img = ImageTk.PhotoImage(file=path)
                b2 =Button(root) # using Button 
                
                myLabel.destroy()
                myLabel = Label()
                myLabel.grid(row=2,column=0,columnspan=2)
                
                myLabel.image = img
                myLabel['image'] = img
            except:
                # print(e)
                root.destroy()
                call(fileLocation)

            

        # e = Entry(root, width=75,borderwidth=5,font=('calibre',18,'normal'))
        # e.grid(row=1,column=0,  padx=10,pady=10)
        # e.pack()

        def uplaod():
            try:
                global filepath
                global myLabel
                root.destroy()
                file_path_list = filepath.split('/')
                base_dir = os.getenv("BASE_DIR")
                # subprocess.call("chmod u+r+x " + file_path_list[-1], shell=True)
                # print("cp " + filepath +" /home/daksh1115/iudx-MOTION2NX-public/data/ImageProvider/raw_images")
                subprocess.call("cp -r " + filepath +" " +base_dir+"/data/ImageProvider/raw_images/2.png" , shell=True)

                # subprocess.call("")
                # print(file_path_list[-1])
                loading.call("2.png")
            except:
                call(fileLocation)


            

        e = Entry(root, width=58,borderwidth=5,font=('calibre',18,'normal'))
        e.grid(row=1,column=0,  padx=10,pady=10)
        # e.pack()
        e.insert(0,str(fileLocation))

        def back(root):
            root.destroy()
            NN.call()



        myButton = Button(root, text= "Upload File", command= FilePath, width=15)
        myButton.grid(row=1,column=1, padx=5,pady=10)
        # myButton.pack()

        showButton = Button(root, text= "Show Image", command= lambda: openFile(filepath), width=15)
        showButton.grid(row=3,column=0, columnspan=2, padx=75,pady=10)
        # showButton.pack()


        nextButton = Button(root, text= "Send Seceret Shares", command= uplaod, width=30)
        nextButton.grid(row=4,column=0, columnspan=2, padx=75,pady=10)

        App_title = Label(root, text= "Upload your Image", command= None,font=('Times 20 bold'))
        App_title.grid(row=0,column=0,columnspan=2, padx=10,pady=10)

        back_Image = PhotoImage(file="./utility/Images_Video/back.png")
        backButton = Button(root, image = back_Image, command=lambda: back(root), width=50, height=50)
        backButton.grid(row=0,column=0,columnspan=2, padx=10,pady=10, sticky=W)

        root.resizable(False , False)

        root.mainloop()
    except EXCEPTION as e:
        # print(e)
        call(fileLocation)