set( target_name veeam-signature2 )
add_executable( ${target_name} )

target_include_directories( ${target_name} PUBLIC . )

set( build_flags ${CMAKE_CXX_FLAGS} )
if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" )
	set( build_flags "-march=native -Wall -Wextra ${build_flags}" )
elseif( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
	set( build_flags "-march=native -Wall -Wextra ${build_flags}" )
else()
	message( SEND_ERROR "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}" )
endif()
set_target_properties( ${target_name} PROPERTIES COMPILE_FLAGS ${build_flags} )
set_target_properties( ${target_name} PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON )

target_link_libraries(
		${target_name} PRIVATE

		Threads::Threads

		Boost::system
		Boost::thread
		Boost::program_options
		Boost::filesystem

		Boost::headers

		spdlog::spdlog_header_only
)

target_sources(
		${target_name} PRIVATE

		"${CMAKE_CURRENT_SOURCE_DIR}/signature.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/signature.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/arguments.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/arguments.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/block_file.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/block_file.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/block_reader.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/block_reader.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/hasher.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/hasher.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/writer.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/writer.cpp"

		"${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

