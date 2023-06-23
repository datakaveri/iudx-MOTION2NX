from tkinter import *
from tkinter import colorchooser
import pyscreenshot as ImageGrab
from tkinter import filedialog
from tkinter import messagebox
from utility import File_location
from utility import SMPC
from utility import NN




    

def call():
    root = Tk()
    global prevPoint
    global currentPoint
    root.title("Paint App")
    root.geometry("880x600")
    

    # -------------- variables --------------------

    # stroke size options 


    stroke_size = IntVar()
    stroke_size.set(56)

    stroke_color = StringVar()
    stroke_color.set("black")

    previousColor = StringVar()
    previousColor.set("white")

    previousColor2 = StringVar()
    previousColor2.set("white")

    # variables for pencil 
    prevPoint = [0,0]
    currentPoint = [0,0] 

    # variable for text
    

    # --------------------- functions -------------------------

    def usePencil():
        stroke_color.set("black")
        canvas["cursor"] = "arrow"

    def useEraser():
        stroke_color.set("white")
        canvas["cursor"] = DOTBOX



    def paint(event):
        global prevPoint
        global currentPoint
        x = event.x
        y = event.y
        currentPoint = [x,y]
        # canvas.create_oval(x , y , x +5 , y + 5 , fill="black")

        if prevPoint != [0,0] : 
            canvas.create_polygon(prevPoint[0] , prevPoint[1] , currentPoint[0] , currentPoint[1],fill=stroke_color.get() , outline=stroke_color.get() , width=stroke_size.get())        

        prevPoint = currentPoint

        if event.type == "5" :
            prevPoint = [0,0]

    def paintRight(event):
        x = event.x
        y = event.y
        canvas.create_arc(x,y,x+stroke_size.get() , y+stroke_size.get() , fill=stroke_color.get() , outline=stroke_color.get() , width=stroke_size.get())
        

    def saveImage():
        try:
            
            # print(fileLocation)
            x = root.winfo_rootx() + frame2.winfo_x()
            y = root.winfo_rooty()+ canvas.winfo_y()
            # print(x,y)
            # print(canvas.winfo_x(), canvas.winfo_y())

            x1 = x + canvas.winfo_width()
            y1 = y + canvas.winfo_height()

            
            img = ImageGrab.grab().crop((x,y,x1,y1))
            
            try:
                fileLocation = filedialog.asksaveasfilename(defaultextension=".png")
                img.save(fileLocation)
                # showImage = messagebox.askyesno("Paint App" , "Do you want to open image?")
                # if showImage:
                #     img.show()
                showImage1 = messagebox.askyesno("Paint App" , "Do you want to Upload Image?")
                if showImage1:
                    Uploader(root, fileLocation)
                else:
                    root.destroy()
                    call()

            except Exception as e:
                showImage1 = messagebox.askyesno("Paint App" , "Do you want to Try Saving Again Image?")
                if showImage1:
                    save_image()
                else:
                    showImage1 = messagebox.askyesno("Paint App" , "Do you want to Upload Image Directly?")
                    if showImage1:
                        Uploader(root, "")
                    else:
                        root.destroy()
                        call()

            
            
            
            

        except Exception as e:
            messagebox.showinfo("Paint app: " , "Error occured")


    def clear():
        if messagebox.askokcancel("Paint app" , "Do you want to clear everything?"):
            canvas.delete('all')

    def createNew():
        if messagebox.askyesno("Paint app" , "Do you want to save before you clear everything?"):
            saveImage()
        clear()


    def Uploader(root,fileLocation):
        root.destroy()
        File_location.call(fileLocation)
    
    def Uploader_direct(root):
        if messagebox.askyesno("Paint app" , "Do you want to save before go to uploading"):
            saveImage()
        root.destroy()
        File_location.call("")

    
    
    def back(root):
        root.destroy()
        NN.call()

    # ------------------- User Interface -------------------

    # Frame - 1 : Tools 

    frame1 = Frame(root , height=300 , width=50 , highlightthickness=5, highlightbackground="black")
    frame1.grid(row=0 , column=0, sticky=NW)

    # toolsFrame 

    

    pen_image = PhotoImage(file="./utility/Images_Video/pen.png") 
    eraser_image = PhotoImage(file="./utility/Images_Video/eraser.png") 
    save_image = PhotoImage(file="./utility/Images_Video/save.png") 
    new_image = PhotoImage(file="./utility/Images_Video/new.png") 
    clear_image = PhotoImage(file="./utility/Images_Video/clear.png") 
    back_image = PhotoImage(file="./utility/Images_Video/back.png") 
    upload_image = PhotoImage(file="./utility/Images_Video/upload.png")

    
    



    pencilButton = Button(frame1 ,image=pen_image, command=usePencil)
    pencilButton.grid(row=1 , column=0)
    eraserButton = Button(frame1 , image=eraser_image, command=useEraser)
    eraserButton.grid(row=2 , column=0)


    # saveImageFrame

    

    saveImageButton = Button(frame1 , image=save_image , bg="white"  , command=saveImage)
    saveImageButton.grid(row=3 , column=0)
    newImageButton = Button(frame1 , image=new_image , bg="white"  , command=createNew)
    newImageButton.grid(row=4 , column=0)
    clearImageButton = Button(frame1 , image=clear_image , bg="white" , command=clear)
    clearImageButton.grid(row=5 , column=0)
    uploadImageButton = Button(frame1 , image=upload_image , bg="white" , command=lambda: Uploader_direct(root))
    uploadImageButton.grid(row=6 , column=0)
    backButton = Button(frame1 , image=back_image , bg="white" , command=lambda: back(root))
    backButton.grid(row=0 , column=0)

    # Frame - 2 - Canvas

    frame2 = Frame(root , height=500 , width=800 , bg="yellow", highlightthickness=5, highlightbackground="black")
    frame2.grid(row=0, column=1)

    canvas = Canvas(frame2 , height=500 , width=800 , bg="white" )
    canvas.grid(row=0 , column=1)
    canvas.bind("<B1-Motion>", paint)
    canvas.bind("<ButtonRelease-1>", paint)
    canvas.bind("<B3-Motion>" , paintRight)


    root.resizable(False , False)


    

    
    root.mainloop()




