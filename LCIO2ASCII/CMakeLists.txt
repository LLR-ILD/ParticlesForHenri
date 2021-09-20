# project name
PROJECT( lcio2ascii )


# project version
SET( LCIO2ASCII_VERSION_MAJOR 1 )
SET( LCIO2ASCII_VERSION_MINOR 0 )
SET( LCIO2ASCII_VERSION_PATCH 0 )

cmake_minimum_required(VERSION 3.10)

# install binary here:
SET(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}" CACHE PATH "Where to install ${PROJECT_NAME}" FORCE)

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(LCIO)
# find_package(ROOT)

# build the executable and put it into bin/
add_executable(lcio2ascii ./src/lcio2ascii.cc ./src/tools.cc)
target_include_directories(lcio2ascii PRIVATE ${LCIO_INCLUDE_DIRS} ./src)
target_link_libraries(lcio2ascii ${LCIO_LIBRARIES})
install(TARGETS lcio2ascii DESTINATION . )