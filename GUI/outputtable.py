from tkinter import *
from tkinter import ttk
import paint 
import File_location
import time
import SMPC
import NN
import NN_helper


# window = Tk()


def call():
    window = Tk()
    

    WIDTH = 800
    HEIGHT = 575

    

    window.geometry("930x675") 

    canvas_2 = Canvas(window, width= WIDTH, height=70)
    canvas_2.grid(row = 0 , column=0)
    canvas_2.option_add("*TCombobox*Listbox.font", "Times 20 bold")
    canvas_2.option_add("*TCombobox*Listbox.justify", "center")


    style = ttk.Style()
    style.theme_use("clam")
    style.configure('W.TCombobox', arrowsize = 25)

    options = [
        "Secure Multiparty Computation",
        "Neural Network Inferencing",
        "Neural Network Inferencing with helper node",
        "Output Result Table"
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
        

    clicked = StringVar()
    my_combo = ttk.Combobox(canvas_2, values=options,font=('Times 20 bold'), justify=CENTER, textvariable=clicked, style='W.TCombobox', state = "readonly")
    my_combo.grid(row = 0 , column=0, ipadx=300,ipady=10, padx=10, pady=10)
    # my_combo.config(dropdown_font = ('Times 20 bold'))
    my_combo.set( "Output Result Table" )
    my_combo.bind("<<ComboboxSelected>>", display_selected)
    

    # canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT)
    canvas_1 = Canvas(window, width= WIDTH, height=HEIGHT, highlightthickness=2, highlightbackground="black")
    canvas_1.grid(row = 1 , column=0,padx=20)

    back_image2 = PhotoImage(file="./Images_Video/NN.png") 
    my_image2 = canvas_1.create_image(WIDTH/2,HEIGHT/2,anchor=CENTER,image = back_image2)


    # canvas = Canvas(window, width= WIDTH -100, height=HEIGHT-100)
    # canvas.grid(row = 1 , column=1,columnspan=3)


    # details_smpc = "Secure Multiparty Computation\n   - point 1\n   - point 2\n   - point 3\n     continue point 3\n   - point 4"
    # canvas.create_text(750/2 + 50, 512/2,anchor= E, text=details_smpc, fill="black", font=('Times 20 bold'))


    
    window.resizable(False , False)
    window.mainloop()




