include_directories(/usr/local/include ../..)

set (CONFIG_TEST_LIST
  rendezvous_hash_test.cc
  )

add_executable (rendezvous_hash_test ${CONFIG_TEST_LIST})
target_link_libraries (rendezvous_hash_test baselib gflags)