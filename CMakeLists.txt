cmake_minimum_required(VERSION 3.12)

option(CONVERTIBLE_BUILD_TESTS "Build tests" ON)

project(convertible CXX)

add_library( ${PROJECT_NAME} INTERFACE)
add_library(helmesjo::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_include_directories( ${PROJECT_NAME} 
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    INTERFACE $<INSTALL_INTERFACE:include>
)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)

# ----------- INSTALL & EXPORT -----------
include(GNUInstallDirs)
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
)
install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-config.cmake
    NAMESPACE ${CMAKE_PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)
export(
    TARGETS ${PROJECT_NAME} 
    FILE ${PROJECT_NAME}-config.cmake
)

if(CONVERTIBLE_BUILD_TESTS)

    enable_testing()

    add_subdirectory(catch2)

    add_executable( convertible_tests
        "tests/convertible_tests.cpp"
    )
    target_link_libraries( convertible_tests
        convertible
        catch2
    )
    add_test(
        NAME convertible_tests
        COMMAND convertible_tests
    )
endif()