from tkinter import *
from PIL import ImageTk, Image
import cv2


def call(image, answer):

    WIDTH = 800
    HEIGHT = 600

    window = Tk()
    window.geometry("1600x760") 

    canvas_2 = Canvas(window, width= WIDTH*2, height=70)
    canvas_2.grid(row = 0 , column=0, columnspan=2)
    application_title = "Secure Multi-Party Computation"
    canvas_2.create_text(850, 35,anchor= CENTER, text=application_title, fill="black", font=('Roboto 50 normal'))

    canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT)
    canvas_1.grid(row = 1 , column=0)

    back_image1 = PhotoImage(file=("/home/daksh1115/iudx-MOTION2NX-public/data/ImageProvider/raw_images/" + image)) 
    my_image2 = canvas_1.create_image(WIDTH/2,HEIGHT/2,anchor=CENTER,image = back_image1)


    canvas = Canvas(window, width= WIDTH, height=HEIGHT)
    canvas.grid(row = 1 , column=1)
    img = cv2.imread("/home/daksh1115/iudx-MOTION2NX-public/data/ImageProvider/processed_images/" +image, cv2.IMREAD_UNCHANGED)
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



    canvas_3 = Canvas(window, width= WIDTH*2, height=70)
    canvas_3.grid(row = 2 , column=0, columnspan=2)
    Result_print = "Number detected is: " + str(answer)
    canvas_3.create_text(850, 35,anchor= CENTER, text=Result_print, fill="black", font=('Roboto 50 normal'))
    window.resizable(False , False)
    window.mainloop()

# call("1.png", 1)






