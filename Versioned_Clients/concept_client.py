#source: https://realpython.com/python-sockets/

#!/usr/bin/env python3

import socket

hosts = {
    'emerald': '24.85.240.252',
    'emerald-home': '192.168.0.10',
    'macair':  '192.168.1.76',
    'clint':   '74.157.196.143'
}

HOST = hosts['emerald-home']  # The server's hostname or IP address
PORT = 65432        # The port used by the server

def print_data(data):
    # print('Received (repr())', repr(data))
    # print('Received (decode())', data.decode())
    print('Received int.from_bytes()', int.from_bytes(data, 'big'))

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))

    while (True):
        message = input("What message do you want to send?\n")
        s.sendall(message.encode())
        #s.sendall(b'Hello world')
        data = s.recv(1)
        print_data(data)

        for i in range(0, 9):
            boo = s.recv(1)
            print_data(boo)
