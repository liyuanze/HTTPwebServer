aux_source_directory(. CPP_LIST) 
 
add_library(libthreadpool ${CPP_LIST})   

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(libthreadpool Threads::Threads)