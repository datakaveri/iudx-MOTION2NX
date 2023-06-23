from tkinter import *
from tkinter import ttk
from utility import paint 
from utility import File_location
import time
from utility import SMPC
from utility import NN_helper
from utility import two_layer
from utility import five_layer


# window = Tk()


def call():
    window = Tk()
    window.title("SMPC")
    # global xVelocity_yellow_1,yVelocity_yellow_1, xVelocity_yellow_2, yVelocity_yellow_2, xVelocity_red_1,yVelocity_red_1,  xVelocity_red_2,yVelocity_red_2, xVelocity_blue_1,yVelocity_blue_1,  xVelocity_blue_2,yVelocity_blue_2

    WIDTH = 800
    HEIGHT = 600

   

   

    window.geometry("1550x775") 

    canvas_2 = Canvas(window, width= WIDTH*2, height=70)
    canvas_2.grid(row = 0 , column=0, columnspan=4)
    canvas_2.option_add("*TCombobox*Listbox.font", "sans 20 bold")
    canvas_2.option_add("*TCombobox*Listbox.justify", "center")


    style = ttk.Style()
    style.theme_use("clam")
    style.configure('W.TCombobox', arrowsize = 25)

    options = [
        "Secure Multiparty Computation",
        "Neural Network Inferencing",
        "Neural Network Inferencing with helper node",
        "Setup: Two Layer Neural Network",
        "Setup: Five Layer Neural Network"
    ]


    def display_selected(event):
        choice = my_combo.get()
        if choice == "Secure Multiparty Computation":
            window.destroy()
            SMPC.call()
        elif choice == "Neural Network Inferencing with helper node":
            window.destroy()
            NN_helper.call()
        elif choice == "Setup: Two Layer Neural Network":
            window.destroy()
            two_layer.call()
        elif choice == "Setup: Five Layer Neural Network":
            window.destroy()
            five_layer.call()

    clicked = StringVar()
    my_combo = ttk.Combobox(canvas_2, values=options,font=('sans 20 bold'), justify=CENTER, textvariable=clicked, style='W.TCombobox', state = "readonly")
    my_combo.grid(row = 0 , column=0, columnspan=4, ipadx=300,ipady=10, padx=10, pady=10)
    # my_combo.config(dropdown_font = ('Times 20 bold'))
    my_combo.set( "Neural Network Inferencing" )
    my_combo.bind("<<ComboboxSelected>>", display_selected)
    

    # canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT)
    canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT, highlightthickness=2, highlightbackground="black")
    canvas_1.grid(row = 1 , column=0,padx=20, rowspan=2)

    back_image2 = PhotoImage(file="./utility/Images_Video/NN.png") 
    my_image2 = canvas_1.create_image(WIDTH/2,HEIGHT/2,anchor=CENTER,image = back_image2)


    canvas = Canvas(window, width= WIDTH -100, height=HEIGHT-100)
    canvas.grid(row = 1 , column=1,columnspan=3)


    details_smpc = "Neural Network Inferencing\n\n   • Compute servers to jointly compute a function\n      over the inputs of data providers\n\n   • Data providers provide secret shares to compute\n      servers to preserve privacy.\n\n   • Compute servers have no knowledge of inputs\n      and intermediate values and the final solution.\n\nIn this context there are only 2 data providers, \nimage provider and model provider. \n\nImage provider is the output owner, I.e., output \nshares are received only by the image provider"
    canvas.create_text(700/2, 512/2,anchor= CENTER, text=details_smpc, fill="black", font=('sans 18 normal'))

    

    def client():
        # global xVelocity_yellow_1,yVelocity_yellow_1, xVelocity_yellow_2, yVelocity_yellow_2, xVelocity_red_1,yVelocity_red_1,  xVelocity_red_2,yVelocity_red_2
         #yellow velocity
        xVelocity_yellow_1 = 1.1*0.5
        yVelocity_yellow_1 = -0.4*0.5
        xVelocity_yellow_2 = 1.1*0.5
        yVelocity_yellow_2 = 0.2*0.5

        #red velocity
        xVelocity_red_1 = 1.0*0.5
        yVelocity_red_1 = -0.5*0.5
        xVelocity_red_2 = 1.0*0.5
        yVelocity_red_2 = 0.2*0.5
        
        red_file_1 = PhotoImage(file="./utility/Images_Video/red.png") 
        my_red_image_1 = canvas_1.create_image(290,155,anchor=CENTER,image = red_file_1)

        red_file_2 = PhotoImage(file="./utility/Images_Video/red.png") 
        my_red_image_2 = canvas_1.create_image(290,155,anchor=CENTER,image = red_file_2)
        
        yellow_file_1 = PhotoImage(file="./utility/Images_Video/yellow.png") 
        my_yellow_image_1 = canvas_1.create_image(290,440,anchor=CENTER,image = yellow_file_1)

        yellow_file_2 = PhotoImage(file="./utility/Images_Video/yellow.png") 
        my_yellow_image_2 = canvas_1.create_image(290,440,anchor=CENTER,image = yellow_file_2)

        while True:
            # global xVelocity_yellow_1,yVelocity_yellow_1, xVelocity_yellow_2, yVelocity_yellow_2, xVelocity_red_1,yVelocity_red_1,  xVelocity_red_2,yVelocity_red_2

            #yellow coordinates
            coordinate_y1 = canvas_1.coords(my_yellow_image_1)
            coordinate_y2 = canvas_1.coords(my_yellow_image_2)

            #red coordinates
            coordinate_r1 = canvas_1.coords(my_red_image_1)
            coordinate_r2 = canvas_1.coords(my_red_image_2)

            # red 1 velocity
            if(coordinate_r1[0] >= 390):
                xVelocity_red_1 = 1.3*0.5
            if(coordinate_r1[1] < 110 ):
                yVelocity_red_1 = 0.4*0.5
            
            if(coordinate_r1[0] >= 520 and coordinate_r1[1] >= 150):
                xVelocity_red_1 = 0
                yVelocity_red_1 = 0
            
            # red 2 velocity
            if(coordinate_r2[0] >= 390):
                xVelocity_red_2 = 1.3*0.5
            if(coordinate_r2[1] >= 175 ):
                yVelocity_red_2 = 2.55*0.5
            
            if(coordinate_r2[0] >= 520):
                xVelocity_red_2 = 0
            if(coordinate_r2[1] >= 430):
                yVelocity_red_2 = 0
            
            
            # yellow 1 velocity
            if(coordinate_y1[0] >= 400):
                xVelocity_yellow_1 = 1.2*0.5
            if(coordinate_y1[1] < 400 ):
                yVelocity_yellow_1 = -2.5*0.5
            
            if(coordinate_y1[0] >= 520):
                xVelocity_yellow_1 = 0
            if(coordinate_y1[1] < 150):
                yVelocity_yellow_1 = 0
            
            # yellow 2 velocity
            if(coordinate_y2[0] >= 400):
                xVelocity_yellow_2 = 1.2*0.5
            if(coordinate_y2[1] >= 460):
                yVelocity_yellow_2 = -0.3*0.5

            if(coordinate_y2[0] >= 520):
                xVelocity_yellow_2 = 0
            if(coordinate_y2[1] < 430):
                yVelocity_yellow_2 = 0

            
        

            
            if(xVelocity_yellow_1 == 0 and yVelocity_yellow_1 == 0 and xVelocity_yellow_2 == 0 and yVelocity_yellow_2 == 0 and xVelocity_red_1 == 0 and yVelocity_red_1 == 0 and xVelocity_red_2 == 0 and yVelocity_red_2 == 0):
                time.sleep(1)
                canvas_1.delete(my_yellow_image_1)
                canvas_1.delete(my_yellow_image_2)
                canvas_1.delete(my_red_image_1)
                canvas_1.delete(my_red_image_2)
                break
            
            canvas_1.move(my_yellow_image_1,xVelocity_yellow_1,yVelocity_yellow_1)
            canvas_1.move(my_yellow_image_2,xVelocity_yellow_2,yVelocity_yellow_2)
            canvas_1.move(my_red_image_1,xVelocity_red_1,yVelocity_red_1)
            canvas_1.move(my_red_image_2,xVelocity_red_2,yVelocity_red_2)
            
            window.update()
            
            time.sleep(0.01)

    # yellow_file_1 = PhotoImage(file="./utility/Images_Video/yellow.png") 
    # my_yellow_image_1 = canvas_1.create_image(540,280,anchor=CENTER,image = yellow_file_1)

    def server():

        yellow_file_4 = PhotoImage(file="./utility/Images_Video/grey.png") 
        my_yellow_image_4 = canvas_1.create_image(520,150,anchor=CENTER,image = yellow_file_4)
        xVelocity_yellow_4 = -0.2*0.5
        yVelocity_yellow_4 = 1.3*0.5
        canvas_1.move(my_yellow_image_4,xVelocity_yellow_4,yVelocity_yellow_4)
        window.update()
        
        while True:
            #yellow coordinates
            coordinate_y3 = canvas_1.coords(my_yellow_image_4)
            
            # print(coordinate_y3)
            
            
            # yellow 1 velocity
            if(coordinate_y3[0] < 520 and coordinate_y3[1] < 150 ):
                time.sleep(1)
                break
            

            if(500 <= coordinate_y3[0] < 522 and 430 >= coordinate_y3[1] > 278 ):
                xVelocity_yellow_4 = 0.2*0.5
                yVelocity_yellow_4 = 1.5*0.5

            if(520 < coordinate_y3[0] < 540 and 430 >= coordinate_y3[1] > 280 ):
                xVelocity_yellow_4 = 0.2*0.5
                yVelocity_yellow_4 = -1.5*0.5

            if(520 < coordinate_y3[0] < 540 and 280 >= coordinate_y3[1] > 150 ):
                xVelocity_yellow_4 = -0.2*0.5
                yVelocity_yellow_4 = -1.3*0.5

            

            canvas_1.move(my_yellow_image_4,xVelocity_yellow_4,yVelocity_yellow_4)
            window.update()
            
            time.sleep(0.01)
    


    # yellow_file_1 = PhotoImage(file="./utility/Images_Video/yellow.png") 
    # my_yellow_image_1 = canvas_1.create_image(220,170,anchor=CENTER,image = yellow_file_1)

    def result():
        grey_file_1 = PhotoImage(file="./utility/Images_Video/grey.png") 
        my_grey_image_1 = canvas_1.create_image(520,150,anchor=CENTER,image = grey_file_1)
        xVelocity_grey_1 = 1.6*0.5
        yVelocity_grey_1 = 0
        grey_file_2 = PhotoImage(file="./utility/Images_Video/grey.png") 
        my_grey_image_2 = canvas_1.create_image(520,430,anchor=CENTER,image = grey_file_2)
        xVelocity_grey_2 = 1.4*0.5
        yVelocity_grey_2 = 0
        
        while True:
            

            #yellow coordinates
            coordinate_g1 = canvas_1.coords(my_grey_image_1)
            coordinate_g2 = canvas_1.coords(my_grey_image_2)

            if(coordinate_g1[0] > 680 and coordinate_g1[1] == 150 ):
                xVelocity_grey_1 = 0
                yVelocity_grey_1 = 4.0*0.5
            
            if(coordinate_g2[0] > 660 and coordinate_g2[1] == 430 ):
                xVelocity_grey_2 = 0
                yVelocity_grey_2 = 0.9*0.5

            if(coordinate_g1[0] > 680 and coordinate_g1[1] > 550 ):
                xVelocity_grey_1 = -5.6*0.5
                yVelocity_grey_1 = 0
            
            if(coordinate_g2[0] > 660 and coordinate_g2[1] > 520 ):
                xVelocity_grey_2 = -5.15*0.5
                yVelocity_grey_2 = 0
            
            if(coordinate_g1[0] < 120 and coordinate_g1[1] > 550 ):
                xVelocity_grey_1 = 0
                yVelocity_grey_1 = -3.8*0.5
            
            if(coordinate_g2[0] < 145 and coordinate_g2[1] > 520 ):
                xVelocity_grey_2 = 0
                yVelocity_grey_2 = -3.4*0.5
                
            if(coordinate_g1[0] < 120 and coordinate_g1[1] < 170 ):
                xVelocity_grey_1 = 1.0*0.5
                yVelocity_grey_1 = 0
            
            if(coordinate_g2[0] < 145 and coordinate_g2[1] < 180 ):
                xVelocity_grey_2 = 0.75*0.5
                yVelocity_grey_2 = 0
                
            if(300>coordinate_g1[0] > 220 and coordinate_g1[1] < 170 ):
                xVelocity_grey_1 = 0
                yVelocity_grey_1 = 0
            
            if(300>coordinate_g2[0] > 220 and coordinate_g2[1] < 180 ):
                xVelocity_grey_2 = 0
                yVelocity_grey_2 = 0
            



            if(xVelocity_grey_1 == 0 and yVelocity_grey_1 == 0 and xVelocity_grey_2 == 0 and yVelocity_grey_2 == 0 ):
                canvas_1.delete(my_grey_image_1)
                canvas_1.delete(my_grey_image_2)
                break
                



            canvas_1.move(my_grey_image_1,xVelocity_grey_1,yVelocity_grey_1)
            canvas_1.move(my_grey_image_2,xVelocity_grey_2,yVelocity_grey_2)
            window.update()
            
            time.sleep(0.01)
    
    client_button = Button(window, text="Client", padx=40,pady=10, command=client,highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
    server_button = Button(window, text="Server", padx=40,pady=10, command=server,highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
    output_button = Button(window, text="Output", padx=40,pady=10, command=result,highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))


    client_button.grid(row=2,column=1,ipadx=5)
    server_button.grid(row=2,column=2,ipadx=5)
    output_button.grid(row=2,column=3,ipadx=5)

    def painter(root):
        root.destroy()
        paint.call()

    def Uploader(root):
        root.destroy()
        File_location.call("")

    paint_button = Button(window, text="Draw your Number", padx=80,pady=10, command=lambda: painter(window),highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
    upload_button = Button(window, text="Upload your Image", padx=80,pady=10, command=lambda: Uploader(window),highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
    




    paint_button.grid(row=3,column=0,ipadx=5, pady=20)
    upload_button.grid(row=3,column=1,ipadx=5, columnspan=3, pady=20)





    window.resizable(False , False)
    window.mainloop()




