# TCP Client:
# https://realpython.com/python-sockets/#echo-server
# https://stackoverflow.com/a/61539628

import socket
import sys

HOST = ""
PORT = 2947
defaultHOST = "192.168.222.123"

if HOST == "":
    # Check if the host was passed in argv
    if len(sys.argv) > 1: HOST = sys.argv[1]

# Ask user for filename offering defaultHOST as the default
if HOST == "":
    HOST = input('Enter the host IP address (default: ' + defaultHOST + '): ')
    print()
if HOST == "": HOST = defaultHOST

print("Connecting to {}:{}".format(HOST,PORT))

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:
        s.connect((HOST, PORT))
        while True:
            try:
                data = s.recv(1024)
                if not data:
                    break
                print("{}".format(data.decode()))
            except KeyboardInterrupt:
                break
    except KeyboardInterrupt:
        pass
    except:
        print("Could not connect!")
