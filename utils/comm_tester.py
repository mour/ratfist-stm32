#!/usr/bin/python

import serial
import sys
import threading
import readline
import re

import time



def calc_checksum(msg_string):
    csum = 0
    for ch in list(msg_string):
        csum ^= ord(ch)

    return csum


def process_msg(msg):
    msg_groups = re.search("^\$(?P<payload_str>.*)*(?P<checksum_str>[0-9a-fA-F]{2})\r\n$", msg)
    if msg_groups == None:
        print("invalid <-- ratfist ({})".format(msg[:-2]))


    csum = calc_checksum(msg[1:-5])

    if csum != int(msg_groups.group('checksum_str'), 16):
        print("{} <-- ratfist ({}) - INVALID checksum".format(msg_groups.group('payload_str'), msg[:-2]))
    else:
        print("{} <-- ratfist ({})".format(msg_groups.group('payload_str'), msg[:-2]))


def listener_thread_func():
    msg = ""

    while not done_event.is_set():
        ch = serial.read().decode('ascii')

        if ch != '':
            try:
                if ch == '$':
                    msg = ch

                elif len(msg) > 0:
                    msg += ch

                    if msg[-2] == '\r' and msg[-1] == '\n':
                        process_msg(msg)

            except IndexError:
                pass



serial = serial.Serial(sys.argv[1], 115200, timeout=0.1)


done_event = threading.Event()

listener_thread = threading.Thread(name='listener', target=listener_thread_func)
listener_thread.start()


while True:
    command = input("> ")
    if command == "":
        continue

    if command == "exit":
        done_event.set()
        break


    csum = calc_checksum(command)

    msg = "${}*{:02X}\r\n".format(command, csum)

    print("{} --> ratfist ({})".format(command, msg[:-2]))

    serial.write(bytearray(msg, 'ascii'))



print("Exiting")
