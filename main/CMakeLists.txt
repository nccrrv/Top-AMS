# file(GLOB LIB_SRCS
#     "../managed_components/s00500_ESPUI/src/*.cpp"
# )


idf_component_register(
    SRCS 
    # ${LIB_SRCS}
    "main.cpp"
    INCLUDE_DIRS ""
    # "../managed_components/s00500_ESPUI/src"
)

# 根据 MOTOR_CONFIG 变量定义相应的宏
if(DEFINED MOTOR_CONFIG)
    if(MOTOR_CONFIG STREQUAL "MOTORS_4")
        target_compile_definitions(${COMPONENT_LIB} PRIVATE MOTORS_4)
        message(STATUS "Building with MOTORS_4 configuration")
    elseif(MOTOR_CONFIG STREQUAL "MOTORS_6")
        target_compile_definitions(${COMPONENT_LIB} PRIVATE MOTORS_6)
        message(STATUS "Building with MOTORS_6 configuration")
    elseif(MOTOR_CONFIG STREQUAL "MOTORS_8")
        # MOTORS_8 不定义任何宏，使用默认配置
        message(STATUS "Building with MOTORS_8 configuration (default)")
    else()
        message(WARNING "Unknown MOTOR_CONFIG value: ${MOTOR_CONFIG}, using default (MOTORS_8)")
    endif()
else()
    # 如果没有指定 MOTOR_CONFIG，使用默认的 MOTORS_8 配置
    message(STATUS "No MOTOR_CONFIG specified, using default (MOTORS_8)")
endif()

# 确定通道数量
if(DEFINED MOTOR_CONFIG)
    if(MOTOR_CONFIG STREQUAL "MOTORS_4")
        set(CHANNEL_COUNT 4)
    elseif(MOTOR_CONFIG STREQUAL "MOTORS_6")
        set(CHANNEL_COUNT 6)
    else()
        set(CHANNEL_COUNT 8)  # MOTORS_8 或默认
    endif()
else()
    set(CHANNEL_COUNT 8)  # 默认8通道
endif()

# 添加自定义目标执行Python脚本
add_custom_target(
    run_helloworld ALL  # ALL表示包含在默认构建目标中
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH= MOTOR_CONFIG=${MOTOR_CONFIG} CHANNEL_COUNT=${CHANNEL_COUNT} python ${CMAKE_CURRENT_SOURCE_DIR}/convert_html_to_hpp.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}  # 设置工作目录为脚本所在位置
    COMMENT "RUN convert_html_to_hpp.py with CHANNEL_COUNT=${CHANNEL_COUNT}..."  # 可选：添加构建时的提示信息
)