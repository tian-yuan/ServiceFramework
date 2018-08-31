include_directories(/usr/local/include . .. ../..)

set (SRC_LIST
        IotHubServer.cc
        IotHubServer.h

        IotHubUtil.cc
        IotHubUtil.h
  )

add_executable (IotHubServer ${SRC_LIST})
target_link_libraries (IotHubServer baselib gflags folly wangle proxygenlib proxygenhttpserver glog crypto ssl)

