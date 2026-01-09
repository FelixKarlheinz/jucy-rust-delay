cmake_minimum_required(VERSION 3.22)

project(rust_delay)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Rust Delay: Debug Build")
  set(CARGO_BUILD_ARG "")
  set(TARGET_DIR "debug")
else()
  message(STATUS "Rust Delay: Release Build")
  set(CARGO_BUILD_ARG "--release")
  set(TARGET_DIR "release")
endif()

if(DEFINED ENV{CARGO_BUILD_TARGET})
  set(TARGET_DIR "$ENV{CARGO_BUILD_TARGET}/${TARGET_DIR}")
endif()

if(WIN32)
  set(RUST_LIB_EXTENSION ".lib")
  set(RUST_LIB_PREFIX "")
elseif(APPLE)
  set(RUST_LIB_EXTENSION ".a")
  set(RUST_LIB_PREFIX "lib")
else()
  set(RUST_LIB_EXTENSION ".a")
  set(RUST_LIB_PREFIX "lib")
endif()

set(CARGO_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(RUST_STATIC_LIB "${CARGO_TARGET_DIR}/${TARGET_DIR}/${RUST_LIB_PREFIX}${PROJECT_NAME}${RUST_LIB_EXTENSION}")
set(RUST_CXX_SOURCE "${CARGO_TARGET_DIR}/cxxbridge/${PROJECT_NAME}/src/lib.rs.cc")
set(RUST_CXX_HEADER "${CARGO_TARGET_DIR}/cxxbridge/${PROJECT_NAME}/src/lib.rs.h")
set(RUST_CXX_WRAPPER "${CARGO_TARGET_DIR}/${PROJECT_NAME}.cpp")
set(RUST_HEADER_WRAPPER "${CARGO_TARGET_DIR}/${PROJECT_NAME}.h")

if(WIN32)
  add_custom_command(
        OUTPUT ${RUST_STATIC_LIB} ${RUST_CXX_WRAPPER} ${RUST_HEADER_WRAPPER}
        COMMAND ${CMAKE_COMMAND} -E env CARGO_TARGET_DIR=${CARGO_TARGET_DIR}
                cargo build ${CARGO_BUILD_ARG}
        COMMAND ${CMAKE_COMMAND} -E copy ${RUST_CXX_SOURCE} ${RUST_CXX_WRAPPER}
        COMMAND ${CMAKE_COMMAND} -E copy ${RUST_CXX_HEADER} ${RUST_HEADER_WRAPPER}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs
                ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
                ${CMAKE_CURRENT_SOURCE_DIR}/build.rs
        COMMENT "Building Rust delay library and generating CXX bindings"
        VERBATIM
    )
else()
  add_custom_command(
        OUTPUT ${RUST_STATIC_LIB} ${RUST_CXX_WRAPPER} ${RUST_HEADER_WRAPPER}
        COMMAND CARGO_TARGET_DIR=${CARGO_TARGET_DIR} cargo build ${CARGO_BUILD_ARG}
        COMMAND ${CMAKE_COMMAND} -E copy ${RUST_CXX_SOURCE} ${RUST_CXX_WRAPPER}
        COMMAND ${CMAKE_COMMAND} -E copy ${RUST_CXX_HEADER} ${RUST_HEADER_WRAPPER}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lib.rs
                ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
                ${CMAKE_CURRENT_SOURCE_DIR}/build.rs
        COMMENT "Building Rust delay library and generating CXX bindings"
        VERBATIM
    )
endif()

add_custom_target(${PROJECT_NAME}_build
    DEPENDS ${RUST_STATIC_LIB} ${RUST_CXX_WRAPPER} ${RUST_HEADER_WRAPPER}
)

add_library(${PROJECT_NAME} STATIC ${RUST_CXX_WRAPPER})

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_build)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC ${RUST_STATIC_LIB})

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32 userenv bcrypt ntdll)
elseif(UNIX AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME} PUBLIC pthread dl m)
elseif(APPLE)
  target_link_libraries(${PROJECT_NAME} PUBLIC "-framework CoreFoundation" "-framework Security")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
