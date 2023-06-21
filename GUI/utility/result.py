import os
from tkinter import *
from PIL import ImageTk, Image
import cv2
from utility import SMPC
from utility import File_location


def call(image):

    WIDTH = 800
    HEIGHT = 600
    try:
        window = Tk()
        window.title("Neural Network Inferencing Result")
        window.geometry("1600x900") 

        base_dir = os.getenv("BASE_DIR")

        canvas_2 = Canvas(window, width= WIDTH*2, height=70)
        canvas_2.grid(row = 0 , column=0, columnspan=2)
        application_title = "Secure Multi-Party Computation"
        canvas_2.create_text(850, 35,anchor= CENTER, text=application_title, fill="black", font=('Roboto 50 normal'))

        canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT)
        canvas_1.grid(row = 1 , column=0)

        back_image1 = PhotoImage(file=(base_dir+"/data/ImageProvider/raw_images/" + image)) 
        my_image2 = canvas_1.create_image(WIDTH/2,HEIGHT/2,anchor=CENTER,image = back_image1)


        canvas = Canvas(window, width= WIDTH, height=HEIGHT)
        canvas.grid(row = 1 , column=1)
        img = cv2.imread(base_dir+"/data/ImageProvider/processed_images/" +image, cv2.IMREAD_UNCHANGED)
        width = int(back_image1.width())
        height = int(back_image1.height())
        dim = (width, height)
        
        # resize image
        resized = cv2.resize(img, dim, interpolation = cv2.INTER_AREA)
        im = Image.fromarray(resized)
        # imgtk = ImageTk.PhotoImage(image=im)

        back_image2 = ImageTk.PhotoImage(image=im) 
        # back_image2.zoom(1000,1000)
        my_image2 = canvas.create_image(WIDTH/2,HEIGHT/2,anchor=CENTER,image = back_image2)
        window.resizable(False , False)

        with open('output.txt', 'r') as f:
            lines = f.read().split('\n')
        result = int(lines[5][-1])
        # print(result)

        canvas_3 = Canvas(window, width= WIDTH*2, height=70)
        canvas_3.grid(row = 2 , column=0, columnspan=2)
        Result_print = "Number detected is: " + str(result)
        canvas_3.create_text(850, 35,anchor= CENTER, text=Result_print, fill="black", font=('Roboto 50 normal'))
        
        canvas_4 = Canvas(window, width= WIDTH*2, height=70)
        canvas_4.grid(row = 3 , column=0, columnspan=2)

        def retry(window):
            window.destroy()
            SMPC.call()

        def exit_app(window):
            window.destroy()

        retry_button = Button(canvas_4, text="Run Again", padx=10,pady=10, command=lambda: retry(window),highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
        exit_button = Button(canvas_4, text="Exit", padx=10,pady=10, command=lambda: exit_app(window),highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))

        retry_button.grid(row=0,column=0,ipadx=20, pady=20)
        exit_button.grid(row=0,column=1,ipadx=20, pady=20)

        window.resizable(False , False)
        window.mainloop()
    except EXCEPTION as e:
        print(e)
        File_location.call("")


# call("1111.png")






