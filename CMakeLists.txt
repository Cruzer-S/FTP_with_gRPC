project(FTPServer)

cmake_minimum_required(VERSION 3.18)

add_subdirectory(Protocol)
add_subdirectory(Client)
add_subdirectory(Server)

add_subdirectory(Library)