PROJECT(angelica)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 

set(boost_path "C:\\Users\\qianqians\\Documents\\boost" CACHE PATH "boost path")
INCLUDE_DIRECTORIES(${boost_path})

set(boost_lib_path "C:\\Users\\qianqians\\Documents\\boost\\stage\\lib" CACHE PATH "boost lib path")
link_directories(${boost_lib_path})

INCLUDE_DIRECTORIES(..)

set(angelica_lib_path "C:\\Users\\qianqians\\Documents\\angelica\\build\\bin\\Debug" CACHE PATH "angelica lib path")
link_directories(${angelica_lib_path})

set(angelica_bin_path "C:\\Users\\qianqians\\Documents\\angelica\\build\\bin" CACHE PATH "angelica bin path")
ADD_SUBDIRECTORY(async_net ${angelica_bin_path})
ADD_SUBDIRECTORY(test ${angelica_bin_path})