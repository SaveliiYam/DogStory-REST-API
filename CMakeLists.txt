cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CONAN_DISABLE_CHECK_COMPILER TRUE CACHE BOOL "" FORCE )
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/json_serializer.h
	src/json_serializer.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/event_logger.cpp
	src/event_logger.h
	src/dog.cpp
	src/dog.h
	src/game_session.cpp
	src/game_session.h
	src/player_tokens.cpp
	src/player_tokens.h
	src/server_exceptions.h
	src/api_handler.cpp
	src/api_handler.h
	src/ticker.h
)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
