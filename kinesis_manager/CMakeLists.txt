cmake_minimum_required(VERSION 2.8.3)
project(kinesis_manager)
set(KINESIS_MANAGER_VERSION 0.1.0)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++14 ")

find_package(aws_common REQUIRED)
if(AWSSDK_FOUND)
  set(SERVICE kinesis)
  AWSSDK_DETERMINE_LIBS_TO_LINK(SERVICE AWSSDK_LIBS_TO_LINK)
  link_directories("${AWSSDK_LIB_DIR}")
endif()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/") # Required for FindKVSSDK

find_package(CURL REQUIRED)
find_package(KVSSDK REQUIRED)

#############
## Library ##
#############
## Declare a C++ library
add_library(${PROJECT_NAME} SHARED src/stream_definition_provider.cpp src/kinesis_stream_manager.cpp src/default_callbacks.cpp)
set(${PROJECT_NAME}_IMPORTED_LIBRARIES producer log4cplus)

## Add KVS SDK libraries
if(NOT KVSSDK_FOUND)
  add_subdirectory(kvssdk)
  add_dependencies(${PROJECT_NAME} KVS_SDK_IMPORT)
endif()

foreach(_imported_lib ${${PROJECT_NAME}_IMPORTED_LIBRARIES})
  if(NOT TARGET ${_imported_lib})
    add_library(${_imported_lib} UNKNOWN IMPORTED)
    set_target_properties(${_imported_lib} PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${AWSSDK_EXTERNAL_INCLUDE_DIR}")
    set_target_properties(${_imported_lib} PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
      IMPORTED_LOCATION "${KVSSDK_EXTERNAL_LIB_DIR}/lib${_imported_lib}.so")
    set_property(TARGET producer APPEND PROPERTY
      IMPORTED_CONFIGURATIONS RELEASE)
  endif()
endforeach()

target_include_directories(${PROJECT_NAME} PRIVATE ${aws_common_INCLUDE_DIRS} ${AWSSDK_INCLUDE_DIR} ${KVSSDK_EXTERNAL_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(${PROJECT_NAME} ${AWSSDK_LIBS_TO_LINK} ${${PROJECT_NAME}_IMPORTED_LIBRARIES})

#############
## Tests ##
#############

enable_testing()

# Build tests which perform API calls to AWS and require user configuration set up
if(BUILD_AWS_TESTING)
    add_definitions(-DBUILD_AWS_TESTING)
endif()

find_package(GTest QUIET)
if(NOT GTEST_FOUND)
  message(WARNING "Could not find GTest. Not building unit tests.")
else()
  add_executable(test_kinesis_manager test/kinesis_manager_test.cpp)
  target_include_directories(test_kinesis_manager PRIVATE ${aws_common_INCLUDE_DIRS} ${KVSSDK_EXTERNAL_INCLUDE_DIR})

  target_link_libraries(test_kinesis_manager
        ${GTEST_LIBRARIES}
        pthread
        ${aws_common_LIBRARIES}
        ${PROJECT_NAME}
        ${rclcpp_LIBRARIES}
        ${PRODUCER_LIBRARY}
        ${LOG_LIBRARY}
        ${CURL_LIBRARIES}
  )
  add_test(NAME test_kinesis_manager COMMAND test_kinesis_manager --gtest_output=xml:test_results/)
endif()

#############
## Install ##
#############

## Mark executables and/or libraries for installation
install(TARGETS ${PROJECT_NAME}
  EXPORT "${PROJECT_NAME}-targets"
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

## Mark cpp header files for installation
install(
  DIRECTORY include/
  DESTINATION include
)

## Export KVS SDK artifacts
install(
  DIRECTORY ${KVSSDK_EXTERNAL_LIB_DIR}/
  DESTINATION lib/
)

install(
  DIRECTORY ${KVSSDK_EXTERNAL_INCLUDE_DIR}/
  DESTINATION include
)

export(TARGETS ${PROJECT_NAME}
  FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake")
export(PACKAGE ${PROJECT_NAME})

CONFIGURE_FILE(${PROJECT_NAME}Config.cmake.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake @ONLY)
CONFIGURE_FILE(${PROJECT_NAME}ConfigVersion.cmake.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake @ONLY)

INSTALL(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION share/${PROJECT_NAME}/cmake)
INSTALL(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake DESTINATION share/${PROJECT_NAME}/cmake)
INSTALL(EXPORT ${PROJECT_NAME}-targets DESTINATION share/${PROJECT_NAME}/cmake)
INSTALL(FILES FindKVSSDK.cmake DESTINATION share/${PROJECT_NAME}/cmake)
