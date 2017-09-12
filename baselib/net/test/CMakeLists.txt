include_directories(/usr/local/include ../..)

set (CONFIG_TEST_LIST
        io_thread_pool_manager_test.cc
        )

add_executable (io_thread_pool_manager_test ${CONFIG_TEST_LIST})
target_link_libraries (io_thread_pool_manager_test baselib gflags wangle folly glog crypto ssl)