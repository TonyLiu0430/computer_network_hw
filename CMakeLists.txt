cmake_minimum_required(VERSION 3.11)

# Include vcpkg toolchain
file(TO_CMAKE_PATH $ENV{VCPKG_ROOT} VCPKG_ROOT)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")


project(computer_network_hw)


# avoid windows SDK minmax conflict             
add_compile_definitions(NOMINMAX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")
file(GLOB_RECURSE SOURCES "src/*.cpp")

if (MSVC)
    add_compile_options(/DEBUG /utf-8)
endif()


add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})


include_directories(include)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE 
                    Ws2_32
                    nlohmann_json::nlohmann_json)