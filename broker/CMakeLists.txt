include(FetchContent)
include(GNUInstallDirs)

file(GLOB_RECURSE AILOY_BROKER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/broker.cpp)
add_library(ailoy_broker STATIC ${AILOY_BROKER_SRCS})
target_include_directories(ailoy_broker PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>    
)
target_link_libraries(ailoy_broker PUBLIC ailoy_core)
target_link_libraries(ailoy_broker PRIVATE $<BUILD_INTERFACE:magic_enum>)

file(GLOB_RECURSE AILOY_BROKER_CLIENT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/broker_client.cpp)
add_library(ailoy_broker_client STATIC ${AILOY_BROKER_CLIENT_SRCS})
target_include_directories(ailoy_broker_client PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>    
)
target_link_libraries(ailoy_broker_client PUBLIC ailoy_core)

if(AILOY_WITH_TEST)
    add_executable(test_broker ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_broker.cpp)
    add_test(NAME TestBroker COMMAND test_broker)
    target_link_libraries(test_broker PRIVATE ailoy_core ailoy_broker_client ailoy_broker GTest::gtest)
    target_link_options(test_broker PRIVATE -fsanitize=undefined)
endif()

# install ailoy_broker and ailoy_broker_client
install(TARGETS ailoy_broker ailoy_broker_client
    EXPORT ailoyTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)
