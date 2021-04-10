import socket


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
        print('')
        print("-+-+-")
        return

    if count == 8:
        print('')
        return


def update_board(s) -> list:
    """
    Updates the local game board.

    :param s: socket
    :return: list
    """
    play = s.recv(9)

    board_bytes = play.decode()

    new_board = [ord(board_bytes[i:i + 1]) for i in range(0, len(board_bytes), 1)]

    return new_board


class GameData:

    def __init__(self):
        self.__identity = None
        self.__game_board = [45, 45, 45, 45, 45, 45, 45, 45, 45]
        self.__bytes_to_expect = 1
        self.__version = 1

    def get_identity(self):
        return self.__identity

    def get_game_board(self):
        return self.__game_board

    def get_bytes_to_expect(self):
        return self.__bytes_to_expect

    def set_identity(self, identity):
        self.__identity = identity

    def set_game_board(self, s: socket):
        self.__game_board = update_board(s)

    def set_bytes_to_expect(self, bytes_to_expect):
        self.__bytes_to_expect = bytes_to_expect

    def set_play(self, position):
        self.__game_board[position] = self.__identity

    def print_board(self):
        """
        Prints the board in a formatted way.

        :return:  void
        """

        print("Board update:")

        count = 0

        for c in self.__game_board:
            i = int(c)

            char_to_print = chr(i)

            if char_to_print in ('X', 'O'):
                print(chr(i), end='')
            else:
                print(" ", end='')

            printSeparator(count)

            count += 1

    def make_play(self, s: socket, invitation: str):
        proposed_play = input(invitation)

        play_ord = ord(proposed_play)
        s.sendall(play_ord.to_bytes(1, 'big'))

        return int(proposed_play)

    def get_version(self):
        return self.__version

    def set_version(self, new_version):
        self.__version = new_version

    def process_welcome(self, s: socket, message: str):
        print(message)

        self.set_identity(int.from_bytes(s.recv(self.get_bytes_to_expect()), 'big'))
        print("You are player", chr(self.get_identity()))

    def __str__(self):
        me = "Protocol version: " + str(self.get_version()) + " Player identity: " + chr(self.get_identity())
        return me


class GameData_v2(GameData):
    def __init__(self):
        super().__init__()
        self.__game_id = None
        super().set_version(2)

    def get_game_id(self):
        return self.__game_id

    def set_game_id(self, new_id):
        self.__game_id = new_id

    def make_play(self, s: socket, invitation: str):
        proposed_play = input(invitation)

        play_ord = ord(proposed_play)
        s.sendall(self.__game_id.to_bytes(1, 'big'))
        s.sendall(play_ord.to_bytes(1, 'big'))

        return int(proposed_play)

    def process_welcome(self, s: socket, message: str):
        super().process_welcome(s, message)
        sent_id = int.from_bytes(s.recv(self.get_bytes_to_expect()), 'big')
        self.set_game_id(sent_id)

    def __str__(self):
        me = "Game ID: " + str(self.__game_id) + " Protocol version: " + str(self.get_version()) +  " Player identity: " + chr(self.get_identity())
        return me
