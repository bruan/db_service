# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)
# 设置头文件
include_directories(../../3rd/protobuf/include)
include_directories(../../3rd/mysql/include)
include_directories(../../db_common)
include_directories(../../db_service/inc)

# 设置输出目录
SET(LIBRARY_OUTPUT_PATH ${BIN_OUTPUT_PATH})
# libDbService.a
add_library(DbService ${DIR_SRCS})