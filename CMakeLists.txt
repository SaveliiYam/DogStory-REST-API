cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CONAN_DISABLE_CHECK_COMPILER TRUE CACHE BOOL "" FORCE )
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
conan_basic_setup(TARGETS)

find_package(Boost 1.81.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавляем библиотеку, указывая, что она статическая.
add_library(GameLib STATIC 
	src/model.h
	src/model.cpp
	src/tagged.h
	src/sdk.h
	src/boost_json.cpp
	
	src/json_loader.h
	src/json_loader.cpp
	src/json_serializer.h
	src/json_serializer.cpp
	
	src/dog.cpp
	src/dog.h
	src/game_session.cpp
	src/game_session.h
	
	src/event_logger.cpp
	src/event_logger.h
	
	src/player_tokens.cpp
	src/player_tokens.h
	src/server_exceptions.h
	
	src/ticker.h
	src/loot_generator.cpp
	src/loot_generator.h
	src/utils.h
	src/geom.h
	src/collision_detector.h
	src/collision_detector.cpp
	src/model_serialization.h
	src/postgres.h
	src/postgres.cpp
	src/utility_functions.h
	src/connection_engine.h
	src/tagged_uuid.h
	src/tagged_uuid.cpp
)

# они должны быть видны и в библиотеке GameLib и в зависимостях.
target_link_libraries(GameLib PUBLIC Threads::Threads CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_include_directories(GameLib PUBLIC CONAN_PKG::boost)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h

	src/request_handler.cpp
	src/request_handler.h

	src/api_handler.cpp
	src/api_handler.h
)

add_executable(collision_tests
	tests/test_utils.h
	tests/collision_detector_tests.cpp
)

target_link_libraries(game_server PRIVATE GameLib)

target_link_libraries(collision_tests PRIVATE CONAN_PKG::catch2)
target_link_libraries(collision_tests PRIVATE GameLib)