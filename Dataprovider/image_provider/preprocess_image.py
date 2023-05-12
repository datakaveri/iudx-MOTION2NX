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

def convert_to_white_bg(path, img_dir):
    old_img = Image.open(path)

    img = Image.new("RGBA", old_img.size, "WHITE")
    img.paste(old_img, (0,0), old_img)
    img.convert("RGB").save(path)


def main():
    base_dir = os.getenv("BASE_DIR")

    img_dir = os.path.join(base_dir,"data/ImageProvider")

    j = str(56)

    # read the image
    for i in range (2222, 2223):

        path = os.path.join(img_dir,"raw_images/"+str(i)+".png")

        transparent_bg = False

        if(transparent_bg):
            convert_to_white_bg(path, img_dir)       
        
        img_ = cv2.imread(path, cv2.IMREAD_GRAYSCALE)

        # img_ = cv2.imread(os.path.join(img_dir,"raw_images/2222.png"), cv2.IMREAD_GRAYSCALE)
        
        # rescale it
        img_ = cv2.resize(255-img_, (28, 28))

        # better black and white version
        (thresh, img_) = cv2.threshold(img_, 128, 255, cv2.THRESH_BINARY | cv2.THRESH_OTSU)

        # this is used to remove unnecessary black edges in the image to better refactor and resize it
        while np.sum(img_[0]) == 0:
            img_ = img_[1:]

        while np.sum(img_[:,0]) == 0:
            img_ = np.delete(img_,0,1)

        while np.sum(img_[-1]) == 0:
            img_ = img_[:-1]
        
        while np.sum(img_[:,-1]) == 0:
            img_ = np.delete(img_,-1,1)

        rows, cols = img_.shape

        if rows > cols:
            factor = 20.0/rows
            rows = 20
            cols = int(round(cols*factor))
	        # first cols than rows
            img_ = cv2.resize(img_, (cols,rows))
    
        else:
            factor = 20.0/cols
            cols = 20
            rows = int(round(rows*factor))
    	    # first cols than rows
            img_ = cv2.resize(img_, (cols, rows))
    
        colsPadding = (int(math.ceil((28-cols)/2.0)),int(math.floor((28-cols)/2.0)))
        rowsPadding = (int(math.ceil((28-rows)/2.0)),int(math.floor((28-rows)/2.0)))
        img_ = np.lib.pad(img_,(rowsPadding,colsPadding),'constant')

        # shifitng the image to center it at the centre of gravity of the number
        shiftx,shifty = getBestShift(img_)
        shifted = shift(img_,shiftx,shifty)
        img_ = shifted

        # save the processed images
        if not(os.path.exists(os.path.join(img_dir,"processed_images/"))):
            os.mkdir(os.path.join(img_dir,"processed_images/"))
        
        # cv2.imwrite(os.path.join(img_dir,"processed_images/"+str(i)+".png"), img_)
        cv2.imwrite(os.path.join(img_dir,"processed_images/2222.png"), img_)


        """
        all images in the training set have an range from 0-1
        and not from 0-255 so we divide our flattened_img images
        (a one dimensional vector with our 784 pixels)
        to use the same 0-1 based range
        """
        flattened_img = img_.flatten() / 255.0
        new_number = int(j)

        # Insert the new number at the top of the array
        final_image = np.insert(flattened_img, 0, new_number, axis=0)
        np.savetxt(os.path.join(img_dir,"images/X2222.csv"),final_image,delimiter=",")

    return 1

if __name__ == "__main__":
    main()