
cmake_minimum_required(VERSION 3.16)
option (ARM_build "ARM_build" OFF)
if (${ARM_build}) 
set(CMAKE_C_COMPILER "/usr/bin/aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/aarch64-linux-gnu-g++")
set(CMAKE_FIND_ROOT_PATH "/usr/aarch64-linux-gnu/")
endif()

project(zenoh_c LANGUAGES C )
   

include_directories(
  include
)



link_directories(include)
find_package(PkgConfig REQUIRED)




file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" )

foreach(file ${files})
	get_filename_component(filename ${file} NAME_WE)
	set(target ${filename})
	add_executable(${target} ${file})
	set_target_properties(${target} PROPERTIES 
		OUTPUT_NAME ${filename}
		RUNTIME_OUTPUT_DIRECTORY "runtime")
	install(TARGETS ${target}
	  DESTINATION lib/${PROJECT_NAME})
	  set_property(TARGET ${target} PROPERTY LANGUAGE C)
	  set_property(TARGET ${target} PROPERTY C_STANDARD 11)
	if (${ARM_build}) 
	target_link_libraries(${target}  ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm/libzenohc.so)
	target_link_libraries(${target}  pthread)

	else()
	target_link_libraries(${target}  ${CMAKE_CURRENT_SOURCE_DIR}/lib/x86/zenohc.dll)
	target_link_libraries(${target}  ${GLIB_LIBRARIES})
	target_link_libraries(${target}  pthread)
	target_include_directories(${target} PUBLIC ${GLIB_INCLUDE_DIRS})

	endif()

	
endforeach()
