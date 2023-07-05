from tkinter import *
from tkinter import ttk
from utility import paint 
from utility import File_location
import time
from utility import SMPC
from utility import NN
from utility import NN_helper
from utility import five_layer


# window = Tk()


def call():
    window = Tk()
    window.title("SMPC")

    WIDTH = 800
    HEIGHT = 575

    

    window.geometry("970x850") 

    canvas_2 = Canvas(window, width= WIDTH, height=70)
    canvas_2.grid(row = 0 , column=0)
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
        elif choice == "Neural Network Inferencing":
            window.destroy()
            NN.call()
        elif choice == "Neural Network Inferencing with helper node":
            window.destroy()
            NN_helper.call()
        elif choice == "Setup: Five Layer Neural Network":
            window.destroy()
            five_layer.call()
        

    clicked = StringVar()
    my_combo = ttk.Combobox(canvas_2, values=options,font=('sans 20 bold'), justify=CENTER, textvariable=clicked, style='W.TCombobox', state = "readonly")
    my_combo.grid(row = 0 , column=0, ipadx=270,ipady=10, padx=10, pady=10)
    # my_combo.config(dropdown_font = ('Times 20 bold'))
    my_combo.set( "Setup: Two Layer Neural Network" )
    my_combo.bind("<<ComboboxSelected>>", display_selected)
    

    # canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT)
    canvas_1 = Canvas(window, width= WIDTH, height=260)
    canvas_1.grid(row = 1 , column=0,padx=20)

    # back_image2 = PhotoImage(file="./utility/Images_Video/NN.png") 
    # my_image2 = canvas_1.create_image(WIDTH/2,HEIGHT/2,anchor=CENTER,image = back_image2)

    text_1 = "Cloud VM specification"
    canvas_1.create_text(20, 25,anchor= W, text=text_1, fill="black", font=('sans 20 bold'))
    text_2 = "   • Server 0 : Azure: bs1 1vcpu, RAM: 1GB, SSD: 30GB"
    canvas_1.create_text(20, 60,anchor= W, text=text_2, fill="black", font=('sans 18 normal'))
    text_3 = "   • Server 1 : AWS: t2.micro 1vcpu, RAM: 1GB, SSD: 30GB"
    canvas_1.create_text(20, 90,anchor= W, text=text_3, fill="black", font=('sans 18 normal'))
    text_6 = "   • Helper Node : AWS: t2.nano 1vcpu, RAM: 0.5GB, SSD: 30GB"
    canvas_1.create_text(20, 120,anchor= W, text=text_6, fill="black", font=('sans 18 normal'))
    text_7 = "   • Image Provider: Laptop"
    canvas_1.create_text(20, 150,anchor= W, text=text_7, fill="black", font=('sans 18 normal'))
    text_8 = "   • Model Provider: Laptop"
    canvas_1.create_text(20, 180,anchor= W, text=text_8, fill="black", font=('sans 18 normal'))
    text_4 = "Layer 1 : weights (256 X 784), bias (256 X 1)"
    canvas_1.create_text(20, 210,anchor= W, text=text_4, fill="black", font=('sans 18 bold'))
    text_5 = "Layer 2 : weights (10 X 256), bias (10 X 1)"
    canvas_1.create_text(20, 240,anchor= W, text=text_5, fill="black", font=('sans 18 bold'))


    canvas_2 = Canvas(window, width= WIDTH, height=170, highlightthickness=2, highlightbackground="black")
    canvas_2.grid(row = 2 , column=0,padx=20, pady=20)

    text_00 = "2 Layers"
    text_01 = "No intra-layer \nsplit"
    text_02 = "Layer 1 : 16 Splits\nLayer 2 : No Split"
    text_03 = "Helper Node (no\nintra-layer split)"
    text_10 = "Memory Requirement"
    text_11 = "3.2 GB"
    text_12 = "0.253 GB"
    text_13 = "0.1 GB"
    text_20 = "Execution Time"
    text_21 = "*"
    text_22 = "200 seconds"
    text_23 = "12 seconds"


    Label_00 = Label(canvas_2, text = text_00,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_00.grid(row = 0, column=0)

    Label_10 = Label(canvas_2, text = text_10,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_10.grid(row = 0, column=1)

    Label_20 = Label(canvas_2, text = text_20,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_20.grid(row = 0, column=2)
    
    Label_01 = Label(canvas_2, text = text_01,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_01.grid(row = 1, column=0)

    Label_11 = Label(canvas_2, text = text_11,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_11.grid(row = 1, column=1)

    Label_21 = Label(canvas_2, text = text_21,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_21.grid(row = 1, column=2)
    
    Label_02 = Label(canvas_2, text = text_02,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20, justify=LEFT)
    Label_02.grid(row = 2, column=0)

    Label_12 = Label(canvas_2, text = text_12,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_12.grid(row = 2, column=1)

    Label_22 = Label(canvas_2, text = text_22,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_22.grid(row = 2, column=2)
    
    Label_03 = Label(canvas_2, text = text_03,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_03.grid(row = 3, column=0)

    Label_13 = Label(canvas_2, text = text_13,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_13.grid(row = 3, column=1)

    Label_23 = Label(canvas_2, text = text_23,highlightthickness=2, highlightbackground="black",font=('sans 16 normal'), height=4, width=20)
    Label_23.grid(row = 3, column=2)


    # canvas = Canvas(window, width= WIDTH -100, height=HEIGHT-100)
    # canvas.grid(row = 1 , column=1,columnspan=3)


    # details_smpc = "Secure Multiparty Computation\n   - point 1\n   - point 2\n   - point 3\n     continue point 3\n   - point 4"
    # canvas.create_text(750/2 + 50, 512/2,anchor= E, text=details_smpc, fill="black", font=('Times 20 bold'))


    
    window.resizable(False , False)
    window.mainloop()




