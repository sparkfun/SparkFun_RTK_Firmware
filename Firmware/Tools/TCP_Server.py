# TCP Server:
# https://realpython.com/python-sockets/#echo-server
# https://stackoverflow.com/a/61539628

import socket

HOST = "192.168.0.41" # Replace this with your local IP address
PORT = 2947

print("Listening on {}:{}".format(HOST,PORT))

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((HOST, PORT))
s.listen()
s.settimeout(2)

try:
    while True:
        try:
            conn, addr = s.accept()
            with conn:
                print("Connection from {}:".format(addr[0]))
                try:
                    while True:
                        data = conn.recv(1024)
                        if data:
                            print("{}".format(data.decode()))
                except KeyboardInterrupt:
                    break
        except KeyboardInterrupt:
            break
        except socket.timeout:
            pass

except KeyboardInterrupt:
    pass

finally:
    s.close()