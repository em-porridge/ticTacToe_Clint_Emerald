cmake_minimum_required(VERSION 3.17)
project(A01_Clint_Emerald C)
set(CMAKE_C_STANDARD 99)

cmake_minimum_required(VERSION 3.15)
project(A01_Clint_Emerald C)

add_executable(server v4_server.c fsm.h fsm.c  GameCollection_LinkedList.c GameCollection_LinkedList.h v3_TTT_FSM.c v3_TTT_FSM.c RPS_FSM.c RPS_FSM.h)
add_executable(client dc_udp_client.c)
