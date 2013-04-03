PROJECT(async_net_test_server)
SET(async_net_test_server_src_files
    async_net_test_server.cpp
)
ADD_EXECUTABLE(async_net_server ${async_net_test_server_src_files})
TARGET_LINK_LIBRARIES(async_net_server async_net)

PROJECT(async_net_test_client)
SET(async_net_test_client_src_files
    async_net_test_client.cpp
)
ADD_EXECUTABLE(async_net_client ${async_net_test_client_src_files})
TARGET_LINK_LIBRARIES(async_net_client async_net)