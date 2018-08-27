include_directories(/usr/local/include . .. ../..)

set (SRC_LIST
        IotHubServer.cc
        IotHubServer.h
  )

add_executable (IotHubServer ${SRC_LIST})
target_link_libraries (IotHubServer baselib jemalloc)

