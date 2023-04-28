import os
import cv2
import numpy as np
import math
from scipy import ndimage
from PIL import Image

def getBestShift(img):
    cy,cx = ndimage.center_of_mass(img)

    rows,cols = img.shape
    shiftx = np.round(cols/2.0-cx).astype(int)
    shifty = np.round(rows/2.0-cy).astype(int)

    return shiftx,shifty

def shift(img,sx,sy):
    rows,cols = img.shape
    M = np.float32([[1,0,sx],[0,1,sy]])
    shifted = cv2.warpAffine(img,M,(cols,rows))
    return shifted

def main():
    base_dir = os.getenv("BASE_DIR")

    img_dir = os.path.join(base_dir,"example-data/data/ImageProvider")

    # print(img_dir)

    j = str(9)

    # read the image
    for i in range (0,200):
        # old_img = Image.open(os.path.join(img_dir,"raw_images/9/"+str(i)+".png"))

        # img = Image.new("RGBA", old_img.size, "WHITE")
        # img.paste(old_img, (0,0), old_img)
        # img.convert("RGB").save(os.path.join(img_dir,"raw_images/9/"+str(i)+".png"))

        # img.show()
        
        img = cv2.imread(os.path.join(img_dir,"raw_images/"+j+"/"+str(i)+".png"), cv2.IMREAD_GRAYSCALE)

        # img = cv2.imread(no, 0)
        
        # rescale it
        img = cv2.resize(255-img, (28, 28))

        # better black and white version
        (thresh, img) = cv2.threshold(img, 128, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)

        while np.sum(img[0]) == 0:
            img = img[1:]

        while np.sum(img[:,0]) == 0:
            img = np.delete(img,0,1)

        while np.sum(img[-1]) == 0:
            img = img[:-1]
        
        while np.sum(img[:,-1]) == 0:
            img = np.delete(img,-1,1)

        rows, cols = img.shape

        if rows > cols:
            factor = 20.0/rows
            rows = 20
            cols = int(round(cols*factor))
	        # first cols than rows
            img = cv2.resize(img, (cols,rows))
    
        else:
            factor = 20.0/cols
            cols = 20
            rows = int(round(rows*factor))
    	    # first cols than rows
            img = cv2.resize(img, (cols, rows))
    
        colsPadding = (int(math.ceil((28-cols)/2.0)),int(math.floor((28-cols)/2.0)))
        rowsPadding = (int(math.ceil((28-rows)/2.0)),int(math.floor((28-rows)/2.0)))
        img = np.lib.pad(img,(rowsPadding,colsPadding),'constant')

        # shifitng the image to center it at the centre of gravity of the number
        shiftx,shifty = getBestShift(img)
        shifted = shift(img,shiftx,shifty)
        img = shifted

        # save the processed images
        if not(os.path.exists(os.path.join(img_dir,"processed_images/"+j+"/"))):
            os.mkdir(os.path.join(img_dir,"processed_images/"+j+"/"))
        
        cv2.imwrite(os.path.join(img_dir,"processed_images/"+j+"/"+str(i)+".png"), img)

        """
        all images in the training set have an range from 0-1
        and not from 0-255 so we divide our flattened_img images
        (a one dimensional vector with our 784 pixels)
        to use the same 0-1 based range
        """
        flattened_img = img.flatten() / 255.0
        new_number = int(j)

        # Insert the new number at the top of the array
        final_image = np.insert(flattened_img, 0, new_number, axis=0)
        np.savetxt(os.path.join(img_dir,"images/"+str(19000+i)+".csv"),final_image,delimiter=",")

    return 1

if __name__ == "__main__":
    main()