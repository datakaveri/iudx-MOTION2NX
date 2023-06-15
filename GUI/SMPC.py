
from tkinter import *
from tkinter import ttk
import paint 
import File_location
import time
import NN
import NN_helper
import two_layer
import five_layer

def call():

    global xVelocity_yellow_1,yVelocity_yellow_1, xVelocity_yellow_2, yVelocity_yellow_2, xVelocity_red_1,yVelocity_red_1,  xVelocity_red_2,yVelocity_red_2, xVelocity_blue_1,yVelocity_blue_1,  xVelocity_blue_2,yVelocity_blue_2
    WIDTH = 800
    HEIGHT = 600



    #yellow velocity
    xVelocity_yellow_1 = 1.7*0.5
    yVelocity_yellow_1 = -0.3*0.5
    xVelocity_yellow_2 = 1.7*0.5
    yVelocity_yellow_2 = 0.4*0.5

    #red velocity
    xVelocity_red_1 = 1.5*0.5
    yVelocity_red_1 = -0.3*0.5
    xVelocity_red_2 = 1.5*0.5
    yVelocity_red_2 = 0.5*0.5

    #blue velocity
    xVelocity_blue_1 = 1.6*0.5
    yVelocity_blue_1 = -0.4*0.5
    xVelocity_blue_2 = 1.6*0.5
    yVelocity_blue_2 = 0.35*0.5

    window = Tk()
    window.title("SMPC")
    window.geometry("1550x700") 

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
        if choice == "Neural Network Inferencing":
            window.destroy()
            NN.call()
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
    my_combo.set( "Secure Multiparty Computation" )
    my_combo.bind("<<ComboboxSelected>>", display_selected)
    
    # datatype of menu text
    # clicked = StringVar()
    
    # # initial menu text
    # clicked.set( "Secure Multiparty Computation" )

    # def display_selected(choice):
    #     choice = clicked.get()
    #     if choice == "Neural Network Inferencing":
    #         window.destroy()
    #         NN.call()

    # drop = OptionMenu( window , clicked , *options,command=display_selected )
    # drop.config(font=('Times 20 bold'))
    # drop.grid(row = 0 , column=0, columnspan=4, ipadx=10,ipady=10)





    # 
    # application_title = "Secure Multi-Party Computation"
    # canvas_2.create_text(850, 35,anchor= CENTER, text=application_title, fill="black", font=('Roboto 50 normal'))
    # my_image2 = canvas_2.create_image(427.5,308.5,anchor=CENTER,image = back_image2)


    # canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT)
    canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT, highlightthickness=2, highlightbackground="black")
    canvas_1.grid(row = 1 , column=0,padx=20, rowspan=2)

    back_image2 = PhotoImage(file="./Images_Video/SMPC.png") 
    my_image2 = canvas_1.create_image(WIDTH/2,HEIGHT/2,anchor=CENTER,image = back_image2)


    canvas = Canvas(window, width= WIDTH -100, height=HEIGHT-100)
    canvas.grid(row = 1 , column=1,columnspan=3)


    details_smpc = "Secure Multiparty Computation\n\n   • Compute servers to jointly compute a function\n      over the inputs of data providers\n\n   • Data providers provide secret shares to compute\n      servers to preserve privacy.\n\n   • Compute servers have no knowledge of inputs\n      and intermediate values and the final solution.\n"
    canvas.create_text(700/2, 512/2,anchor= CENTER, text=details_smpc, fill="black", font=('sans 18 normal'))

    def client():
        yellow_file_1 = PhotoImage(file="./Images_Video/yellow.png") 
        my_yellow_image_1 = canvas_1.create_image(150,100,anchor=CENTER,image = yellow_file_1)

        yellow_file_2 = PhotoImage(file="./Images_Video/yellow.png") 
        my_yellow_image_2 = canvas_1.create_image(150,100,anchor=CENTER,image = yellow_file_2)


        red_file_1 = PhotoImage(file="./Images_Video/red.png") 
        my_red_image_1 = canvas_1.create_image(170,310,anchor=CENTER,image = red_file_1)

        red_file_2 = PhotoImage(file="./Images_Video/red.png") 
        my_red_image_2 = canvas_1.create_image(170,310,anchor=CENTER,image = red_file_2)

        blue_file_1 = PhotoImage(file="./Images_Video/blue.png") 
        my_blue_image_1 = canvas_1.create_image(160,520,anchor=CENTER,image = blue_file_1)

        blue_file_2 = PhotoImage(file="./Images_Video/blue.png") 
        my_blue_image_2 = canvas_1.create_image(160,520,anchor=CENTER,image = blue_file_2)


        while True:
            global xVelocity_yellow_1,yVelocity_yellow_1, xVelocity_yellow_2, yVelocity_yellow_2, xVelocity_red_1,yVelocity_red_1,  xVelocity_red_2,yVelocity_red_2, xVelocity_blue_1,yVelocity_blue_1,  xVelocity_blue_2,yVelocity_blue_2

            #yellow coordinates
            coordinate_y1 = canvas_1.coords(my_yellow_image_1)
            coordinate_y2 = canvas_1.coords(my_yellow_image_2)

            #yellow coordinates
            coordinate_r1 = canvas_1.coords(my_red_image_1)
            coordinate_r2 = canvas_1.coords(my_red_image_2)

            #yellow coordinates
            coordinate_b1 = canvas_1.coords(my_blue_image_1)
            coordinate_b2 = canvas_1.coords(my_blue_image_2)
            
            # yellow 1 velocity
            if(coordinate_y1[0] >= 320):
                xVelocity_yellow_1 = 2.8*0.5
            if(coordinate_y1[1] < 70 ):
                yVelocity_yellow_1 = 0.85*0.5
            
            if(coordinate_y1[0] >= 500):
                xVelocity_yellow_1 = 0
            if(coordinate_y1[1] >= 125):
                yVelocity_yellow_1 = 0
            
            # yellow 2 velocity
            if(coordinate_y2[0] >= 320):
                xVelocity_yellow_2 = 2.8*0.5
            if(coordinate_y2[1] >= 140 ):
                yVelocity_yellow_2 = 5.15*0.5

            if(coordinate_y2[0] >= 500):
                xVelocity_yellow_2 = 0
            if(coordinate_y2[1] >= 465):
                yVelocity_yellow_2 = 0

            # red 1 velocity
            if(coordinate_r1[0] >= 320):
                xVelocity_red_1 = 1.8*0.8
            if(coordinate_r1[1] < 280 ):
                yVelocity_red_1 = -1.55*0.8
            
            if(coordinate_r1[0] >= 500):
                xVelocity_red_1 = 0
            if(coordinate_r1[1] < 125):
                yVelocity_red_1 = 0
            
            # red 2 velocity
            if(coordinate_r2[0] >= 320):
                xVelocity_red_2 = 1.8*0.8
            if(coordinate_r2[1] >= 360 ):
                yVelocity_red_2 = 1.30*0.8
            
            if(coordinate_r2[0] >= 500):
                xVelocity_red_2 = 0
            if(coordinate_r2[1] >= 465):
                yVelocity_red_2 = 0
            
            
            # blue 1 velocity
            if(coordinate_b1[0] >= 320):
                xVelocity_blue_1 = 2.8*0.5
            if(coordinate_b1[1] < 480 ):
                yVelocity_blue_1 = -5.55*0.5
            
            if(coordinate_b1[0] >= 500):
                xVelocity_blue_1 = 0
            if(coordinate_b1[1] < 125):
                yVelocity_blue_1 = 0
            
            # blue 2 velocity
            if(coordinate_b2[0] >= 320):
                xVelocity_blue_2 = 2.8*0.5
            if(coordinate_b2[1] >= 555 ):
                yVelocity_blue_2 = -1.4*0.5
            
            if(coordinate_b2[0] >= 500):
                xVelocity_blue_2 = 0
            if(coordinate_b2[1] < 465):
                yVelocity_blue_2 = 0
            

            
            if(xVelocity_yellow_1 == 0 and yVelocity_yellow_1 == 0 and xVelocity_yellow_2 == 0 and yVelocity_yellow_2 == 0 and xVelocity_red_1 == 0 and yVelocity_red_1 == 0 and xVelocity_red_2 == 0 and yVelocity_red_2 == 0 and xVelocity_blue_1 == 0 and yVelocity_blue_1 == 0 and xVelocity_blue_2 == 0 and yVelocity_blue_2 == 0):
                time.sleep(1)
                canvas_1.delete(my_yellow_image_1)
                canvas_1.delete(my_yellow_image_2)
                canvas_1.delete(my_red_image_1)
                canvas_1.delete(my_red_image_2)
                canvas_1.delete(my_blue_image_1)
                canvas_1.delete(my_red_image_2)
                break
            
            canvas_1.move(my_yellow_image_1,xVelocity_yellow_1,yVelocity_yellow_1)
            canvas_1.move(my_yellow_image_2,xVelocity_yellow_2,yVelocity_yellow_2)
            canvas_1.move(my_red_image_1,xVelocity_red_1,yVelocity_red_1)
            canvas_1.move(my_red_image_2,xVelocity_red_2,yVelocity_red_2)
            canvas_1.move(my_blue_image_1,xVelocity_blue_1,yVelocity_blue_1)
            canvas_1.move(my_blue_image_2,xVelocity_blue_2,yVelocity_blue_2)
            
            window.update()
            
            time.sleep(0.01)

    def server():

        yellow_file_4 = PhotoImage(file="./Images_Video/grey.png") 
        my_yellow_image_4 = canvas_1.create_image(500,125,anchor=CENTER,image = yellow_file_4)
        xVelocity_yellow_4 = -0.28*0.5
        yVelocity_yellow_4 = 1.65*0.5
        canvas_1.move(my_yellow_image_4,xVelocity_yellow_4,yVelocity_yellow_4)
        window.update()
        
        while True:
            

            #yellow coordinates
            coordinate_y3 = canvas_1.coords(my_yellow_image_4)
            
            # print(coordinate_y3)
            
            
            # yellow 1 velocity
            if(coordinate_y3[0] < 500 and coordinate_y3[1] < 125 ):
                time.sleep(1)

                break
            

            if(471 <= coordinate_y3[0] < 500 and 465 >coordinate_y3[1] >= 290 ):
                xVelocity_yellow_4 = 0.28*0.5
                yVelocity_yellow_4 = 1.75*0.5

            if(499 < coordinate_y3[0] < 530 and 466 >= coordinate_y3[1] > 290 ):
                xVelocity_yellow_4 = 0.28*0.5
                yVelocity_yellow_4 = -1.75*0.5

            if(500 < coordinate_y3[0] < 530 and 291 >= coordinate_y3[1] > 124 ):
                xVelocity_yellow_4 = -0.28*0.5
                yVelocity_yellow_4 = -1.65*0.5
            
            
            

            # if(coordinate_y3[0] < 472):
            #     xVelocity_yellow_4 = 0.28*0.5
            # if(coordinate_y3[1] > 290 ):
            #     yVelocity_yellow_4 = 1.75*0.5

            # if(coordinate_y3[0] > 500):
            #     xVelocity_yellow_4 = 0.28*0.5
            # if(coordinate_y3[1] > 465 ):
            #     yVelocity_yellow_4 = -1.75*0.5

            # if(coordinate_y3[0] > 530):
            #     xVelocity_yellow_4 = -0.28*0.5
            # if(coordinate_y3[1] < 290 ):
            #     yVelocity_yellow_4 = -1.65*0.5
            
            

            canvas_1.move(my_yellow_image_4,xVelocity_yellow_4,yVelocity_yellow_4)
            window.update()
            
            time.sleep(0.01)

    # grey_file_1 = PhotoImage(file="grey.png") 
    # my_grey_image_1 = canvas_1.create_image(770,295,anchor=CENTER,image = grey_file_1)




    def result():
        grey_file_1 = PhotoImage(file="./Images_Video/grey.png") 
        my_grey_image_1 = canvas_1.create_image(500,125,anchor=CENTER,image = grey_file_1)
        xVelocity_grey_1 = 2.7*0.5
        yVelocity_grey_1 = 1.7*0.5
        grey_file_2 = PhotoImage(file="./Images_Video/grey.png") 
        my_grey_image_2 = canvas_1.create_image(500,465,anchor=CENTER,image = grey_file_2)
        xVelocity_grey_2 = 2.7*0.5
        yVelocity_grey_2 = -1.7*0.5
        
        while True:
            

            #yellow coordinates
            coordinate_g1 = canvas_1.coords(my_grey_image_1)
            coordinate_g2 = canvas_1.coords(my_grey_image_2)

            if(coordinate_g1[0] > 770 and coordinate_g1[1] > 295 ):
                xVelocity_grey_1 = 0
                yVelocity_grey_1 = 0
            
            if(coordinate_g2[0] > 770 and coordinate_g2[1] <= 295 ):
                xVelocity_grey_2 = 0
                yVelocity_grey_2 = 0

            if(xVelocity_grey_1 == 0 and yVelocity_grey_1 == 0 and xVelocity_grey_2 == 0 and yVelocity_grey_2 == 0 ):
                result2(my_grey_image_1,my_grey_image_2)
                



            canvas_1.move(my_grey_image_1,xVelocity_grey_1,yVelocity_grey_1)
            canvas_1.move(my_grey_image_2,xVelocity_grey_2,yVelocity_grey_2)
            window.update()
            
            time.sleep(0.01)

    # grey_file_3 = PhotoImage(file="grey.png") 
    # my_grey_image_3 = canvas_1.create_image(25,100,anchor=CENTER,image = grey_file_3)

    def result2(my_grey_image_1,my_grey_image_2):
        grey_file_3 = PhotoImage(file="./Images_Video/grey.png") 
        my_grey_image_3 = canvas_1.create_image(770,295,anchor=CENTER,image = grey_file_3)
        

        xVelocity_grey_1 = xVelocity_grey_2 = xVelocity_grey_3 = 0
        yVelocity_grey_1 = yVelocity_grey_2 = yVelocity_grey_3 = 1.5
        
        
        while True:
            coordinate_g1 = canvas_1.coords(my_grey_image_1)
            coordinate_g2 = canvas_1.coords(my_grey_image_2)
            coordinate_g3 = canvas_1.coords(my_grey_image_3)
            # print(coordinate_g1)
            if(coordinate_g1[1] >= 590 or coordinate_g2[1] >= 590 or coordinate_g3[1] >= 590):
                xVelocity_grey_1 = xVelocity_grey_2 = xVelocity_grey_3 = -1.5
                yVelocity_grey_1 = yVelocity_grey_2 = yVelocity_grey_3 = 0
            
            if(coordinate_g1[0] < 25 or coordinate_g2[0] < 25 or coordinate_g3[0] < 25):
                
                result3(my_grey_image_1,my_grey_image_2,my_grey_image_3)


            
            

            canvas_1.move(my_grey_image_1,xVelocity_grey_1,yVelocity_grey_1)
            canvas_1.move(my_grey_image_2,xVelocity_grey_2,yVelocity_grey_2)
            canvas_1.move(my_grey_image_3,xVelocity_grey_3,yVelocity_grey_3)
            window.update()
            
            time.sleep(0.01)


    def result3(my_grey_image_1,my_grey_image_2,my_grey_image_3):
        
        xVelocity_grey_1 = xVelocity_grey_2 = xVelocity_grey_3 = 0
        yVelocity_grey_1 = yVelocity_grey_2 = yVelocity_grey_3 = -1.5
        
        while True:
            coordinate_g1 = canvas_1.coords(my_grey_image_1)
            coordinate_g2 = canvas_1.coords(my_grey_image_2)
            coordinate_g3 = canvas_1.coords(my_grey_image_3)

            if(coordinate_g1[1] <= 510 and coordinate_g1[0] <27):
                xVelocity_grey_1 = 1.5
                yVelocity_grey_1 = 0
            
            if(coordinate_g2[1] <= 305 and coordinate_g2[0] <27):
                xVelocity_grey_2 = 1.5
                yVelocity_grey_2 = 0
            
            if(coordinate_g3[1] <= 90 and coordinate_g3[0] <27):
                xVelocity_grey_3 = 1.5
                yVelocity_grey_3 = 0
            
            if(coordinate_g1[1] <= 510 and coordinate_g1[0] >160):
                xVelocity_grey_1 = 0
                yVelocity_grey_1 = 0
            
            if(coordinate_g2[1] <= 305 and coordinate_g2[0] >170):
                xVelocity_grey_2 = 0
                yVelocity_grey_2 = 0
            
            if(coordinate_g3[1] <= 90 and coordinate_g3[0] >150):
                xVelocity_grey_3 = 0
                yVelocity_grey_3 = 0
            
            if(xVelocity_grey_1 == 0 and yVelocity_grey_1 == 0 and xVelocity_grey_2 == 0 and yVelocity_grey_2 == 0 and xVelocity_grey_3 == 0 and yVelocity_grey_3 == 0 ):
                time.sleep(0.3)
                canvas_1.delete(my_grey_image_1)
                canvas_1.delete(my_grey_image_2)
                canvas_1.delete(my_grey_image_3)

            canvas_1.move(my_grey_image_1,xVelocity_grey_1,yVelocity_grey_1)
            canvas_1.move(my_grey_image_2,xVelocity_grey_2,yVelocity_grey_2)
            canvas_1.move(my_grey_image_3,xVelocity_grey_3,yVelocity_grey_3)
            window.update()
            
            time.sleep(0.01)

    # yellow_file_4 = PhotoImage(file="grey.png") 
    # my_yellow_image_4 = canvas_1.create_image(530,290,anchor=CENTER,image = yellow_file_4)



    client_button = Button(window, text="Client", padx=40,pady=10, command=client,highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
    server_button = Button(window, text="Server", padx=40,pady=10, command=server,highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
    output_button = Button(window, text="Output", padx=40,pady=10, command=result,highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))


    client_button.grid(row=2,column=1,ipadx=5)
    server_button.grid(row=2,column=2,ipadx=5)
    output_button.grid(row=2,column=3,ipadx=5)

    # def painter(root):
    #     root.destroy()
    #     paint.call()

    # def Uploader(root):
    #     root.destroy()
    #     File_location.call()

    # paint_button = Button(window, text="Draw your Number", padx=80,pady=10, command=lambda: painter(window),highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))
    # upload_button = Button(window, text="Upload your Image", padx=80,pady=10, command=lambda: Uploader(window),highlightthickness=3, highlightbackground="black", font=('Times 20 bold'))




    # paint_button.grid(row=3,column=0,ipadx=5, pady=20)
    # upload_button.grid(row=3,column=1,ipadx=5, columnspan=3, pady=20)





    window.resizable(False , False)

    window.mainloop()


