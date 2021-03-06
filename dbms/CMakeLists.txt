include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/find_vectorclass.cmake)

set (CONFIG_VERSION ${CMAKE_CURRENT_BINARY_DIR}/include/DB/Common/config_version.h)
set (CONFIG_COMMON ${CMAKE_CURRENT_BINARY_DIR}/include/DB/Common/config.h)
set (CONFIG_BUILD ${CMAKE_CURRENT_BINARY_DIR}/src/Common/config_build.cpp)

include (cmake/version.cmake)
message (STATUS "Will build ${VERSION_FULL}")
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/DB/Common/config.h.in ${CONFIG_COMMON})
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/include/DB/Common/config_version.h.in ${CONFIG_VERSION})

get_property (BUILD_COMPILE_DEFINITIONS DIRECTORY ${ClickHouse_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
get_property (BUILD_INCLUDE_DIRECTORIES DIRECTORY ${ClickHouse_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
string (TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/src/Common/config_build.cpp.in ${CONFIG_BUILD})


include(${ClickHouse_SOURCE_DIR}/cmake/dbms_include.cmake)

include_directories (BEFORE ${ClickHouse_SOURCE_DIR}/contrib/libdivide)
include_directories (BEFORE ${ClickHouse_SOURCE_DIR}/contrib/libcpuid/include)
include_directories (BEFORE ${ClickHouse_SOURCE_DIR}/contrib/libfarmhash)
include_directories (BEFORE ${ClickHouse_SOURCE_DIR}/contrib/libmetrohash/src)
include_directories (${ClickHouse_SOURCE_DIR}/libs/libdaemon/include)
include_directories (${ODBC_INCLUDE_DIRECTORIES})
include_directories (${ClickHouse_BINARY_DIR}/dbms/include)

if (NOT NO_WERROR)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
endif ()

find_package (Threads)

add_subdirectory (src)

add_library(string_utils
	include/DB/Common/StringUtils.h
	src/Common/StringUtils.cpp)

set(dbms_headers)
set(dbms_sources)

include(${ClickHouse_SOURCE_DIR}/cmake/dbms_glob_sources.cmake)
add_headers_and_sources(dbms Functions)
add_headers_and_sources(dbms Functions/Conditional)
add_headers_and_sources(dbms TableFunctions)
add_headers_and_sources(dbms Parsers)
add_headers_and_sources(dbms Analyzers)
add_headers_and_sources(dbms AggregateFunctions)
add_headers_and_sources(dbms Core)
add_headers_and_sources(dbms DataStreams)
add_headers_and_sources(dbms DataTypes)
add_headers_and_sources(dbms Databases)
add_headers_and_sources(dbms DataBases/Distributed)
add_headers_and_sources(dbms Dictionaries)
add_headers_and_sources(dbms Dictionaries/Embedded)
add_headers_and_sources(dbms Interpreters)
add_headers_and_sources(dbms Interpreters/ClusterProxy)
add_headers_and_sources(dbms Common)
add_headers_and_sources(dbms Common/HashTable)
add_headers_and_sources(dbms IO)
add_headers_and_sources(dbms Columns)
add_headers_and_sources(dbms Storages)
add_headers_and_sources(dbms Storages/Distributed)
add_headers_and_sources(dbms Storages/MergeTree)
add_headers_and_sources(dbms Core)
add_headers_and_sources(dbms Client)
add_headers_only(dbms Server)

list (APPEND dbms_sources ${CONFIG_BUILD})
list (APPEND dbms_headers ${CONFIG_VERSION} ${CONFIG_COMMON})

list(REMOVE_ITEM dbms_sources
	src/Client/Client.cpp
	src/Client/Benchmark.cpp
	src/Storages/StorageCloud.cpp
	src/Databases/DatabaseCloud.cpp
	src/Common/StringUtils.cpp)

if (APPLE OR CMAKE_SYSTEM MATCHES "FreeBSD")
	list(REMOVE_ITEM dbms_headers
		include/DB/Common/AIO.h
		include/DB/IO/WriteBufferAIO.h
		include/DB/IO/ReadBufferAIO.h)
	list(REMOVE_ITEM dbms_sources
		src/IO/ReadBufferAIO.cpp
		src/IO/WriteBufferAIO.cpp)
endif()

if (MAKE_STATIC_LIBRARIES)
	add_library(dbms ${dbms_headers} ${dbms_sources})
else ()
	add_library(dbms SHARED ${dbms_headers} ${dbms_sources})
	set_target_properties (dbms PROPERTIES SOVERSION ${VERSION_MAJOR} VERSION ${VERSION_SO} OUTPUT_NAME clickhouse)
	install (TARGETS dbms LIBRARY DESTINATION lib COMPONENT clickhouse)
endif ()


if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
	# Won't generate debug info for files with heavy template instantiation to achieve faster linking and lower size.
	set_source_files_properties(
		src/Functions/FunctionsArithmetic.cpp
		src/Functions/FunctionsArray.cpp
		src/Functions/FunctionsCoding.cpp
		src/Functions/FunctionsComparison.cpp
		src/Functions/FunctionsConditional.cpp
		src/Functions/FunctionsConversion.cpp
		src/Functions/FunctionsDateTime.cpp
		src/Functions/FunctionsDictionaries.cpp
		src/Functions/FunctionsFormatting.cpp
		src/Functions/FunctionsHashing.cpp
		src/Functions/FunctionsHigherOrder.cpp
		src/Functions/FunctionsLogical.cpp
		src/Functions/FunctionsRandom.cpp
		src/Functions/FunctionsReinterpret.cpp
		src/Functions/FunctionsRound.cpp
		src/Functions/FunctionsString.cpp
		src/Functions/FunctionsStringArray.cpp
		src/Functions/FunctionsStringSearch.cpp
		src/Functions/FunctionsURL.cpp
		src/Functions/FunctionsVisitParam.cpp
		src/Functions/FunctionsMath.cpp
		src/Functions/FunctionsGeo.cpp
		src/Functions/FunctionsMiscellaneous.cpp
		src/Functions/FunctionsTransform.cpp
		src/Dictionaries/FlatDictionary.cpp
		src/Dictionaries/HashedDictionary.cpp
		src/Dictionaries/CacheDictionary.cpp
		src/Dictionaries/RangeHashedDictionary.cpp
		src/Dictionaries/ComplexKeyHashedDictionary.cpp
		src/Dictionaries/ComplexKeyCacheDictionary.cpp
		PROPERTIES COMPILE_FLAGS -g0)
endif ()

if (NOT AARCH64)
	set (LINK_LIBRARIES_ONLY_ON_X86_64 cpuid)
endif()

if (CMAKE_SYSTEM MATCHES "FreeBSD")
    set(PLATFORM_LIBS "execinfo")
else()
    set(PLATFORM_LIBS "")
endif()

target_link_libraries (dbms
	common
	zkutil
	mysqlxx
	cityhash farmhash metrohash
	${LZ4_LIBRARY}
	${ZSTD_LIBRARY}
	string_utils
	${DOUBLE_CONVERSION_LIBRARY}
	${ZLIB_LIBRARIES}
	${LINK_LIBRARIES_ONLY_ON_X86_64}
	${RE2_LIBRARY}
	${RE2_ST_LIBRARY}
	${OPENSSL_CRYPTO_LIBRARY}
	${Boost_SYSTEM_LIBRARY}
	${Poco_Data_LIBRARY}
)

if (Poco_DataODBC_FOUND)
	target_link_libraries (dbms ${Poco_DataODBC_LIBRARY})
endif()

if (Poco_MongoDB_FOUND)
	target_link_libraries (dbms ${Poco_MongoDB_LIBRARY})
endif()

if (Poco_NetSSL_FOUND)
	target_link_libraries (dbms ${Poco_NetSSL_LIBRARY})
endif()

target_link_libraries (dbms
	${Poco_Foundation_LIBRARY}
	${ICU_LIBS}
	${PLATFORM_LIBS}
	${CMAKE_DL_LIBS}
	${LTDL_LIB}
	${CMAKE_THREAD_LIBS_INIT}
)

target_include_directories (dbms
	INTERFACE include
)

if (ENABLE_TESTS)
	# attach all dbms gtest sources
	grep_gtest_sources(${ClickHouse_SOURCE_DIR}/dbms dbms_gtest_sources)
	add_executable(unit_tests_dbms ${dbms_gtest_sources})
	target_link_libraries(unit_tests_dbms gtest_main dbms)
	add_check(unit_tests_dbms)
endif()
