project (New_Shallow_Water)
cmake_minimum_required (VERSION 3.10)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_COMPILER clang++)

add_definitions (-DVULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 -DVK_ENABLE_BETA_EXTENSIONS)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")

if (UNIX AND NOT APPLE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g" )
    find_package (OpenMP REQUIRED)
else ()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g" )
endif ()

find_package (glfw3 3.3 REQUIRED)
find_package (glm REQUIRED)
find_package (Vulkan REQUIRED)
find_package (Assimp REQUIRED)

include_directories (src src/hdvw src/external src/engine)

add_executable (neo 
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
    src/external/vk_mem_alloc.cpp
    src/external/stb_image.cpp
)

if (UNIX AND NOT APPLE)
    target_link_libraries (neo glfw glm assimp -ldl)
else ()
    target_link_libraries (neo glfw glm assimp)
endif ()
