PROJECT(async_net)
AUX_SOURCE_DIRECTORY(. DIR_SRC)
AUX_SOURCE_DIRECTORY(win32 DIR_SRC_win32)
SET(header_files
    async_net.h
    async_service.h
    error_code.h
    sock_addr.h
    sock_buff.h
    socket.h    
    socket_pool.h
    buff_pool.h
    read_bufff_pool.h
    write_buff_pool.h
)
SET(win32_header_files
    win32/base_socket_win32.h
    win32/iocp_impl.h
    win32/Overlapped.h
    win32/winhdef.h
)
SET(SRC_LIST 
    ${DIR_SRC}
    ${DIR_SRC_win32}	
)
ADD_LIBRARY(async_net ${SRC_LIST} ${header_files} ${win32_header_files})