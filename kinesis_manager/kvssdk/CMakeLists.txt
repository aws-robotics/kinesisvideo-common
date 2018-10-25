cmake_minimum_required(VERSION 3.0.2)
project(KVSSDK)
set(KVSSDK_VERSION 1.5.1)
set(KVSSDK_MD5 360d0abd477b176663d6ae045493976b)

if(NOT EXTERNAL_INSTALL_LOCATION)
  set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
endif()

#############
## Library ##
#############
file(MAKE_DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/include)
file(MAKE_DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/lib)

include(ExternalProject)
ExternalProject_Add(KVS_SDK_IMPORT
  SOURCE_DIR ${EXTERNAL_INSTALL_LOCATION}
  URL https://github.com/awslabs/amazon-kinesis-video-streams-producer-sdk-cpp/archive/${KVSSDK_VERSION}.tar.gz
  URL_MD5 ${KVSSDK_MD5}
  BINARY_DIR ${EXTERNAL_INSTALL_LOCATION}/kinesis-video-native-build
  PATCH_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/apply-kvs-patch ${EXTERNAL_INSTALL_LOCATION}/kinesis-video-native-build
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ${EXTERNAL_INSTALL_LOCATION}/kinesis-video-native-build/min-install-script
  INSTALL_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/install-kvs ${EXTERNAL_INSTALL_LOCATION}
)

#############
## Install ##
#############
install(
  DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/lib/
  DESTINATION lib/
)

install(
  DIRECTORY ${EXTERNAL_INSTALL_LOCATION}/include/
  DESTINATION include
)

## Mark executables and/or libraries for installation
export(PACKAGE ${PROJECT_NAME})
