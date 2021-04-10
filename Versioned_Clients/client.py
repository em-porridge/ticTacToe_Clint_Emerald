# Adapted from: https://realpython.com/python-sockets/

"""
Copyright: Clinton Fernandes, February 2021
e-mail: clintonf@gmail.com
"""

import argparse
import socket
from Versioned_Clients.game_data import GameData
from Versioned_Clients.game_data import GameData_v2

CODES = {
    "WELCOME": 1,
    "INVITE": 4,
    "INVALID": 5,
    "ACCEPTED": 6,
    "WIN": 7,
    "LOSE": 8,
    "TIE": 9,
    "DISCONNECT": 10,
    "VERSION": 11
}

MESSAGES = {
    1: "Welcome",
    4: "Your turn! What is your move?",
    5: "Invalid move. Try again",
    6: "Move accepted",
    7: "You win!",
    8: "You lose!",
    9: "Tie game",
    88: "You are X",
    79: "You are O",
    11: "Version number",
    10: "Opponent has disconnected. Please stand by."
}

IDENTITIES = {
    "X": 88,
    "O": 79
}

hosts = {
    'emerald': '24.85.240.252',
    'emerald-home': '192.168.0.10',
    'macair': '192.168.1.76',
    'clint': '74.157.196.143'
}

HOST = hosts['macair']  # The server's hostname or IP address
PORT = 65432  # The port used by the server
MAX_VERSION = 2


def get_game_object(protocol_version: int) -> GameData:
    """
    Gets the correct game_data object.

    :param protocol_version: protocol version of game
    :return: GameData
    """
    if protocol_version == 1:
        return GameData()
    if protocol_version == 2:
        return GameData_v2()


def play_game(host: str, port: int, protocol_version: int = 1):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))

        proposed_play = None
        game_data = get_game_object(protocol_version)

        while True:
            server_message = s.recv(game_data.get_bytes_to_expect())
            message = int.from_bytes(server_message, 'big')

            if message == CODES["VERSION"]:
                pass

            if message == CODES["WELCOME"]:
                game_data.process_welcome(s, MESSAGES[CODES["WELCOME"]])
                print(game_data)

                if game_data.get_identity() == IDENTITIES["X"]:
                    proposed_play = game_data.make_play(s, MESSAGES[CODES["INVITE"]])

            if message == CODES["INVITE"]:
                game_data.set_game_board(s)
                game_data.print_board()

                proposed_play = game_data.make_play(s, MESSAGES[CODES["INVITE"]])

            if message == CODES["INVALID"]:
                print(MESSAGES[CODES["INVALID"]])
                proposed_play = game_data.make_play(s, MESSAGES[CODES["INVITE"]])

            if message == CODES["ACCEPTED"]:
                print(MESSAGES[CODES["ACCEPTED"]])
                try:
                    game_data.set_play(proposed_play)
                except IndexError:
                    # I have not idea what's happening
                    pass

                proposed_play = None

                game_data.print_board()

            if message == CODES["WIN"]:
                print(MESSAGES[CODES["WIN"]])

            if message == CODES["LOSE"]:
                print(MESSAGES[CODES["LOSE"]])

            if message == CODES["TIE"]:
                print(MESSAGES[CODES["TIE"]])

            if message == CODES["DISCONNECT"]:
                print(MESSAGES[CODES["DISCONNECT"]])


def get_version_options() -> str:
    string = "["

    for i in range(1, MAX_VERSION):
        string = string + str(i) + " | "

    string = string + str(MAX_VERSION) + "]"

    return string


def create_arguments() -> argparse:
    parser = argparse.ArgumentParser()

    protocol_help = "protocol version " + get_version_options() + ", default = 1"

    parser.add_argument("host", help="server IP address")
    parser.add_argument("--version", help=protocol_help, type=int)
    parser.add_argument("--port", help="server port #. Default = " + str(PORT))

    return parser


def main():
    args = create_arguments().parse_args()
    try:
        version = int(args.version)
    except TypeError:
        version = 1

    try:
        port = int(args.port)
    except TypeError:
        port = PORT

    play_game(args.host, port, version)


if __name__ == "__main__":
    main()
