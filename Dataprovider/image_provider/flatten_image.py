#sudo apt install python3-pip
#pip3 install numpy pandas pillow
#python3 flatten_image.py --input_image_path [path to image]  --output_image_ID [image id]

import numpy as np
import pandas as pd
import os
import argparse
from PIL import Image
def main():
    parser = argparse.ArgumentParser(description='Flatten MNIST Image')
    parser.add_argument('--input_image_path', type=str,help='path to the input image file')
    parser.add_argument('--output_image_ID', type=int,help='Image ID to be saved')
    args = parser.parse_args()
    
    base_dir = os.getenv("BASE_DIR")
    if(base_dir==None):
        print("Run the setup.sh file to set the environment variables.")
        return -1
    try:
        test_image = pd.read_csv(args.input_image_path, header=None)
    except:
        if(not args.input_image_path):
            print("No input image given")
        elif(args.input_image_path.split(".")[-1] != ".csv"):
            image = Image.open(args.input_image_path).convert("L")
            np_image = np.array(image.getdata())
            test_image = pd.DataFrame(np_image)
        else:
            print("Unable to find the image")
            return -1
    scale = 255
    flattened_img = test_image.to_numpy().flatten()
    flattened_img = flattened_img/255.0
    new_number = 12

    # Insert the new number at the top of the array
    final_image = np.insert(flattened_img, 0, new_number, axis=0)
    np.savetxt(os.path.join(base_dir,f"Dataprovider/image_provider/images/X{args.output_image_ID}.csv"),final_image,delimiter=",")
    return 1
if __name__=="__main__":
    main()
