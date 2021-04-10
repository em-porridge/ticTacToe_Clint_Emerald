import socket
import sys

MESSAGES = {1: "Welcome",
            2: "What is your name",
            3: "Your turn! What is your move?",
            4: "Invalid move. Try again",
            5: "Move accepted",
            6: "You win!",
            7: "You lose!",
            8: "Bye",
            9: "WEST VIRGINNIAAAAAAAAAAAAAAA",
            10: "Mountain Mammmmaaaaaaaaaa",
            88: "You are X",
            79: "You are O"
            }

INVITE_TO_PLAY = (3, 4, 88)

def to_binary(i: int) -> "byte":
    return i.to_bytes(1, 'big')


def test_connect():
    print("You are connected")


def pseudo_server(server_message: str) -> "bytes":
    """
    Converts a string into a "stream" of bytes.
    :param server_message: the servers byte stream, but as a string
    :return: byte(s)
    """

    encoded_message = server_message.encode()

    return encoded_message


def decode_message(encoded_message: "bytes") -> int:
    """
    Decodes a the first byte in a binary message into an integer.

    :param encoded_message: binary stream
    :return: int
    """
    message_as_byte = encoded_message[0]
    return message_as_byte


def process_message(message_in_bytes: int) -> str:
    """
    Processes a transmitted byte into a text server_message.
    :param message_in_bytes: byte, the code for the server_message
    :return: str
    """

    return MESSAGES[message_in_bytes]


def print_board(board_bytes: "bytes"):
    """
    Prints the current board state.

    :param board_bytes: the board state
    :return: void
    """

    print("Game board:")
    decoded_message = board_bytes.decode()

    board_list = [decoded_message[i:i + 1] for i in range(0, len(decoded_message), 1)]

    output_board(board_list)


def output_board(board: list):
    """
    Prints the board in a formatted way.

    :param board: the board. Values are ints
    :return:  void
    """
    count = 0

    for c in board:
        print(c, end='')

        printSeparator(count)

        count += 1


def create_string_list(bytes_as_string: str):
    """
    Splits up a string representation of binary into an array of those digits.

    :param bytes_as_string: string
    :return: list
    """
    board = []

    for i in range(0, 72, 8):
        s = bytes_as_string[i: i + 8]
        board.append(s)

    return board


def printSeparator(count: int):
    """
    Prints out formatted characters for the board.

    :param count: int
    :return: void
    """
    if count == 0 or count == 3 or count == 6:
        print("|", end='')
        return

    if count == 1 or count == 4 or count == 7:
        print("|", end='')
        return

    if count == 2 or count == 5:
        print("\n-+-+-")
        return

    if count == 8:
        print('')
        return


def play_game():
    """
    Main game driver
    :return: void
    """

    connection_args = get_connection_args()

    if len(connection_args) != 2:
        print("Usage: python client_game.py <host> <port>")
        exit(1)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(connection_args)

        while True:
            server_message_code = s.recv(1) # receive 1 byte
            board_message =       s.recv(9) # receive 10 bytes

            message_as_int = decode_message(server_message_code)

            print(MESSAGES[message_as_int])

            print_board(board_message)

            process_server_response(s, message_as_int)


def process_server_response(sock: socket, message_code: int):
    if message_code in INVITE_TO_PLAY:
        if message_code == 88:
            print(MESSAGES[3])

        play = input(MESSAGES[message_code])
        play_ord = ord(play)
        print("Sending", play_ord)
        sock.sendall(play_ord.to_bytes(1, 'big'))


def concept_play_game():
    while True:
        proto_server_message = input("What message (in binary) does the server send?")
        server_message = pseudo_server(proto_server_message)

        message_as_int = decode_message(server_message)

        print(MESSAGES[message_as_int])

        print_board(server_message)


def game_loop():
    pass


def get_connection_args() -> tuple:
    host = sys.argv[1]
    port = int( sys.argv[2] )

    return (host, port)


def main():
    play_game()
    # concept_play_game()


if __name__ == "__main__":
    main()
