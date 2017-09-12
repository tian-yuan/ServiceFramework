set (BASELIB_SRC_LIST
        base/snowflake_id_util.cc
        base/snowflake_id_util.h

        base/config/rendezvous_hash.cc
        base/config/rendezvous_hash.h
        base/config_util.cc
        base/config_util.h
        base/configurable.h
        base/configuration.cc
        base/configuration.h

        base/glog_util.cc
        base/glog_util.h
        base/time_util.cc
        base/time_util.h

        message_util/call_chain_util.cc
        message_util/call_chain_util.h

        net/base/client_bootstrap2.h
        net/base/default_pipeline_factory.h
        net/base/service_base.h
        net/base/service_factory_manager.cc
        net/base/service_factory_manager.h

        net/server/http_client.cc
        net/server/http_client.h
        net/server/http_server.cc
        net/server/http_server.h
        net/server/http_server_lib.cc
        net/server/http_server_lib.h
        net/server/http_server_lib_acceptor.cc
        net/server/http_server_lib_acceptor.h
        net/server/service_plugin.h
        net/server/simple_tcp_client.cc
        net/server/simple_tcp_client.h
        net/server/tcp_client.cc
        net/server/tcp_client.h
        net/server/tcp_client_group.cc
        net/server/tcp_client_group.h
        net/server/tcp_server.cc
        net/server/tcp_server.h
        net/server/tcp_server_t.h
        net/server/tcp_service_base.cc
        net/server/tcp_service_base.h
        net/server/udp_client.cc
        net/server/udp_client.h
        net/server/udp_server.cc
        net/server/udp_server.h

        net/rabbit_http_client.cc
        net/rabbit_http_client.h
        net/fiber_data_manager.cc
        net/fiber_data_manager.h
        net/io_thread_pool_manager.cc
        net/io_thread_pool_manager.h
        net/rabbit_http_request_handler.cc
        net/rabbit_http_request_handler.h
        net/simple_conn_handler.cc
        net/simple_conn_handler.h
        net/simple_conn_pipeline_factory.cc
        net/simple_conn_pipeline_factory.h
        net/thread_local_conn_manager.cc
        net/thread_local_conn_manager.h
        net/conn_pipeline_factory.cc
        net/conn_pipeline_factory.h
        net/conn_pipeline_factory_t.h
        net/conn_handler.cc
        net/conn_handler.h
        )

add_library (baselib ${BASELIB_SRC_LIST})

if(APPLE)
target_link_libraries (baselib folly boost_context-mt  boost_filesystem-mt boost_system-mt double-conversion glog gflags event ssl crypto pthread z)
elseif(UNIX)
target_link_libraries (baselib folly boost_context boost_filesystem boost_system double-conversion glog gflags event ssl crypto pthread z rt)
endif()

add_subdirectory(base/test)
add_subdirectory(net/test)