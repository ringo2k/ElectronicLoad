#!/usr/bin/env python
# -*- coding: utf-8 -*-

import serial
import matplotlib.pyplot as plt

def main(args, help):
    '''
    Simple line numbering program to demonstrate CLI interface
    '''

    plt.ylabel('some numbers')
    plt.ion()
   # plt.show(block=False)

    heatsinkTemperature = list()
    fanspeed = list()
    try:
        while(True):
            ser = serial.Serial(args.device, args.baud)
            lineOfData = ser.readline()
            arrayOfData = lineOfData.replace("\r\n","").split("\t")
            serialData = dict()
            serialData['heatsink'] = float(arrayOfData[0])
            serialData['fanspeed'] = float(arrayOfData[1])
            serialData['current'] = float(arrayOfData[2])
            serialData['voltage'] = float(arrayOfData[3])
            heatsinkTemperature.append(serialData['heatsink'])
            fanspeed.append(serialData['fanspeed'])
            plt.plot(heatsinkTemperature, c="#0feef0")
            plt.plot(fanspeed, c="#0f33f0")
            #plt.scatter(range(0,len(heatsinkTemperature)),heatsinkTemperature, c="#ff00ff")
            print(serialData)
            plt.pause(0.001)
    except EnvironmentError as e:
        sys.stderr.write("%s\n" % e)
    except KeyboardInterrupt:
        print("finishing")
        ser.close()
        pass

if __name__ == "__main__":
    import os, sys, select, argparse, fileinput
    parser = argparse.ArgumentParser(formatter_class=lambda prog: argparse.HelpFormatter(prog,max_help_position=30),
                                     description="serial data reader for constant current load")
    parser.add_argument('-d', '--device', help='serial device filename')
    parser.add_argument('-b', '--baud', help='serial baudrate', type=int, default=115200)
    main(parser.parse_args(), parser.print_help)
