# TCP Server:
# https://realpython.com/python-sockets/#echo-server
# https://stackoverflow.com/a/61539628

import socket

HOST = socket.gethostbyname(socket.gethostname())
PORT = 2947

print("Listening {}:{}".format(HOST,PORT))

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
                print(f"Connection from {addr}:")
                while True:
                    data = conn.recv(1024)
                    if data:
                        print("{}".format(data.decode()))
        except socket.timeout:
            pass
        except KeyboardInterrupt:
            break

except KeyboardInterrupt:
    pass

finally:
    s.close()