include_directories(/usr/local/include ../..)

set (CONFIG_TEST_LIST
        io_thread_pool_manager_test.cc
        )

set (CONFIG_HTTP_TEST_LIST
        http_client_test.cc
)

add_executable (http_client_test ${CONFIG_HTTP_TEST_LIST})
add_executable (io_thread_pool_manager_test ${CONFIG_TEST_LIST})
target_link_libraries (http_client_test baselib gflags wangle folly proxygenlib glog crypto ssl)
target_link_libraries (io_thread_pool_manager_test baselib gflags wangle folly glog crypto ssl)