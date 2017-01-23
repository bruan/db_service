# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)
aux_source_directory(./proto_src DIR_SRCS)
# 设置头文件
include_directories(../3rd/protobuf/include)

# 设置输出目录
SET(LIBRARY_OUTPUT_PATH ${BIN_OUTPUT_PATH})
# libDbCommon.a
add_library(DbCommon ${DIR_SRCS})

add_custom_command(TARGET DbCommon
	POST_BUILD
	COMMAND cp -rf proto/ ${BIN_OUTPUT_PATH}
)