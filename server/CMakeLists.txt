aux_source_directory(. CPP_LIST) 

add_subdirectory(../connection)
add_subdirectory(../threadPool)

add_library(libserver ${CPP_LIST})   

target_link_libraries(HTTPserver libconnection)
target_link_libraries(HTTPserver libthreadpool)
