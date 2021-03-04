cmake_minimum_required(VERSION 3.17)
project(A01_Clint_Emerald C)
set(CMAKE_C_STANDARD 99)

cmake_minimum_required(VERSION 3.15)
project(A01_Clint_Emerald C)

add_executable(server v3_server.c fsm.h fsm.c game_collection_linked_list.h game_collection_linked_list.c v2_FSM.c v2_FSM.h)
