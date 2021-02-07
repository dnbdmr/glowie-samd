#!/usr/bin/env python
# source: https://github.com/sudar/Arduino-Makefile/blob/master/bin/ard-reset-arduino
# 

from __future__ import print_function
import serial
import sys
import os.path
import argparse
from time import sleep

parser = argparse.ArgumentParser(description='Reset an Arduino')
parser.add_argument('--caterina', action='store_true', help='Reset a Leonardo, Micro, Robot or LilyPadUSB.')
parser.add_argument('--uf2', action='store_true', help='Reset an Atmel SAMD board with UF2 bootloader')
parser.add_argument('--verbose', action='store_true', help="Watch what's going on on STDERR.")
parser.add_argument('--period', type=float, default=0.1, help='Specify the DTR pulse width in seconds.')
parser.add_argument('port', nargs=1, help='Serial device e.g. /dev/ttyACM0')
args = parser.parse_args()

if args.caterina:
        if args.verbose: print('Forcing reset using 1200bps open/close on port %s' % args.port[0])
        ser = serial.Serial(args.port[0], 57600)
        ser.close()
        ser.baudrate = 1200
        ser.open()
        ser.setRTS(True)  # RTS line needs to be held high and DTR low
        ser.setDTR(False) # (see Arduino IDE source code)
        ser.close()
        sleep(1)

        while not os.path.exists(args.port[0]):
            if args.verbose: print('Waiting for %s to come back' % args.port[0])
            sleep(1)

        if args.verbose: print('%s has come back after reset' % args.port[0])

elif args.uf2:
        if args.verbose: print('Forcing reset using 1200bps open/close on port %s' % args.port[0])
        ser = serial.Serial(args.port[0], 57600)
        ser.close()
        ser.baudrate = 1200
        ser.open()
        ser.setRTS(True)  # RTS line needs to be held high and DTR low
        ser.setDTR(False) # (see Arduino IDE source code)
        ser.close()
        sleep(1)

        while not os.path.exists('/media/daniel/ITSYBOOT'):
            if args.verbose: print('Waiting for uf2 to mount')
            sleep(1)

        if args.verbose: print('Board has mounted after reset')

else:
        if args.verbose: print('Setting DTR high on %s for %ss' % (args.port[0],args.period))
        ser = serial.Serial(args.port[0], 1200)
        ser.dtr = False
        sleep(args.period)
        ser.close()
