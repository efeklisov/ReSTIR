project(ReSTIR)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER clang++)

add_definitions(-DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 -DVK_ENABLE_BETA_EXTENSIONS)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness -pthread -O0 -g")
else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness -pthread -O3 -DNDEBUG")
    message("Building in release mode")
endif()

if(MSVC)
    set(CONAN_DISABLE_CHECK_COMPILER ON)
endif(MSVC)

if(UNIX)
    set(CONAN_SYSTEM_INCLUDES ON)
endif(UNIX)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories(src src/hdvw src/external src/engine)

add_executable(neo 
    src/main.cpp 
    src/hdvw/window.cpp
    src/hdvw/instance.cpp
    src/hdvw/surface.cpp
    src/hdvw/device.cpp
    src/hdvw/queue.cpp
    src/hdvw/commandpool.cpp
    src/hdvw/commandbuffer.cpp
    src/hdvw/allocator.cpp
    src/hdvw/swapchain.cpp
    src/hdvw/renderpass.cpp
    src/hdvw/attachment.cpp
    src/hdvw/framebuffer.cpp
    src/hdvw/shader.cpp
    src/hdvw/pipelinelayout.cpp
    src/hdvw/pipeline.cpp
    src/hdvw/semaphore.cpp
    src/hdvw/fence.cpp
    src/hdvw/buffer.cpp
    src/hdvw/image.cpp
    src/hdvw/texture.cpp
    src/hdvw/descriptorlayout.cpp
    src/hdvw/descriptorpool.cpp
    src/hdvw/descriptorset.cpp
    src/engine/blas.cpp
    src/engine/tlas.cpp
    src/engine/sbt.cpp
    src/engine/model.cpp
    src/engine/saveimg.cpp
    src/external/vk_mem_alloc.cpp
    src/external/stb_image.cpp
)

target_link_libraries(neo ${CONAN_LIBS} -ldl)

file(COPY models/ DESTINATION models/)
file(COPY textures/ DESTINATION textures/)

if(UNIX)
    execute_process(
        COMMAND sh "../recompile_shaders.sh" 
    )
endif(UNIX)

if(MSVC)
    execute_process(
        COMMAND sh "../slow_recompile_shaders.sh" 
    )
endif(MSVC)