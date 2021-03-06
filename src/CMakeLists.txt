cmake_minimum_required(VERSION 3.5.1)

project(access_control_system)

add_compile_options(-std=c++11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "../../../out")
set(CMAKE_CXX_FLAGS_DEBUG "-fPIC -O3 -g -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-fPIC -O3 -Wall")

add_definitions(-DENABLE_DVPP_INTERFACE)
set(INC_PATH $ENV{DDK_PATH})

if (NOT DEFINED ENV{DDK_PATH})
    set(INC_PATH "/usr/local/Ascend")
    message(STATUS "set default INC_PATH: ${INC_PATH}")
else ()
    message(STATUS "env INC_PATH: ${INC_PATH}")
endif()

set(LIB_PATH $ENV{NPU_HOST_LIB})

if (NOT DEFINED ENV{NPU_HOST_LIB})
    set(LIB_PATH "/usr/local/Ascend/acllib/lib64/stub/")
    message(STATUS "set default LIB_PATH: ${LIB_PATH}")
else ()
    message(STATUS "env LIB_PATH: ${LIB_PATH}")
endif()

include_directories(
    ../inc/
    $ENV{HOME}/ascend_ddk/arm/include/opencv4/
    ${INC_PATH}/acllib/include/
    $ENV{HOME}/ascend_ddk/arm/include/
    $ENV{HOME}/ascend_ddk/arm/include/ascenddk
    $ENV{HOME}/Ascend/
    $ENV{HOME}/Ascend/ascenddk/
    /usr/local/include/
)

if(target STREQUAL "Simulator_Function")
    add_compile_options(-DFUNC_SIM)
endif()

link_directories(
    /usr/local/lib
    ${LIB_PATH}
    $ENV{HOME}/ascend_ddk/arm/lib/
    $ENV{HOME}/Ascend/driver/
    $ENV{HOME}/Ascend/
    ${INC_PATH}/atc/lib64/
)

add_executable(main
    i2c.cpp
    gpio.cpp
    utils.cpp
    model_process.cpp
    resource_load.cpp
    peripheral_ctrl.cpp
    mind_camera.cpp
    face_register.cpp
    face_detection.cpp
    face_anti_spoofing.cpp
    face_key_point_detection.cpp
    face_feature_extraction.cpp
    facial_recognition_message.pb.cc
    face_post_process.cpp
    dvpp_process.cpp
    dvpp_resize.cpp
    dvpp_jpege.cpp
    dvpp_jpegd.cpp
    dvpp_cropandpaste.cpp
    main.cpp
)

if(target STREQUAL "Simulator_Function")
    target_link_libraries(main funcsim)
else()
    target_link_libraries(main ascendcl acl_dvpp stdc++  c_sec slog media_mini opencv_highgui opencv_core opencv_imgproc opencv_imgcodecs opencv_calib3d opencv_flann opencv_features2d opencv_videoio  protobuf presenteragent pthread)
endif()

install(TARGETS main DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
execute_process(COMMAND ${CMAKE_COMMAND} -E copy ../../../scripts/param.conf ../../../out/param.conf)
