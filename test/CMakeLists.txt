# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)
# 设置头文件
include_directories(../3rd/protobuf/include)
include_directories(../db_common)
include_directories(../db_common/proto_src)
include_directories(../db_service/inc)

# 设置输出目录
set(EXECUTABLE_OUTPUT_PATH ${BIN_OUTPUT_PATH})
set(LINK_LIBS ${BIN_OUTPUT_PATH}/libDbCommon.a ${BIN_OUTPUT_PATH}/libDbService.a ${PROJECT_SOURCE_DIR}/3rd/protobuf/lib/linux/libprotobuf.a ${PROJECT_SOURCE_DIR}/3rd/mysql/lib/linux/libmysqlclient_r.a pthread)
# 设置链接选项
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-rpath=./ ")
# 生成test
add_executable(TestDb ${DIR_SRCS})
# 链接库
target_link_libraries(TestDb ${LINK_LIBS})

add_custom_command(TARGET TestDb
	POST_BUILD
	COMMAND cp -rf proto/ ${BIN_OUTPUT_PATH}
)