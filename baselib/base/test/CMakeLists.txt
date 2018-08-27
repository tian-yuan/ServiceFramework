include_directories(/usr/local/include ../..)

set (CONFIG_TEST_LIST
  rendezvous_hash_test.cc
  )

set (NLOHMAN_JSON_FILE_LIST
        nlohmann_json_test.cpp
)

add_executable (rendezvous_hash_test ${CONFIG_TEST_LIST})
add_executable (nlohmann_json_test ${NLOHMAN_JSON_FILE_LIST})
target_link_libraries (rendezvous_hash_test baselib gflags)
target_link_libraries (nlohmann_json_test baselib gflags)
