set (BASELIB_SRC_LIST
        base/snowflake_id_util.cc
        base/snowflake_id_util.h
        base/config/rendezvous_hash.cc
        base/config/rendezvous_hash.h
        )

add_library (baselib ${BASELIB_SRC_LIST})

if(APPLE)
target_link_libraries (baselib folly boost_context-mt  boost_filesystem-mt boost_system-mt double-conversion glog gflags event ssl crypto pthread z)
elseif(UNIX)
target_link_libraries (baselib folly boost_context boost_filesystem boost_system double-conversion glog gflags event ssl crypto pthread z rt)
endif()

add_subdirectory(base/test)