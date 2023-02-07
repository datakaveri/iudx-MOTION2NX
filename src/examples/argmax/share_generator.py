import os
import random

def main():
    compute_server0 = open("S1.txt","a")
    compute_server1 = open("S2.txt","a")

    delta0 = random.getrandbits(63)
    delta1 = random.getrandbits(63)


    value = int(input("Enter the input variable for multiplication:- "))

    Delta = delta0 + delta1
    compute_server0.write(str(Delta) + " " + str(delta0 - value) + "\n")
    compute_server1.write(str(Delta) + " " + str(delta1) + "\n")



if __name__ == '__main__':
    main()
