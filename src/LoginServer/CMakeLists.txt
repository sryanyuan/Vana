file(GLOB LOGIN_SRC *.cpp)
file(GLOB LOGIN_HEADERS *.hpp)
source_group("Login Sources" FILES ${LOGIN_SRC})
source_group("Login Headers" FILES ${LOGIN_HEADERS})

add_executable(LoginServer ${LOGIN_SRC} ${LOGIN_HEADERS})


target_link_libraries(LoginServer
	Common
	${MYSQL_LIBRARIES}
	${SOCI_LIBRARIES}
	${LUA_LIBRARIES}
	${BOTAN_LIBRARIES}
	${Boost_FILESYSTEM_LIBRARY}
	${Boost_SYSTEM_LIBRARY}
	${Boost_THREAD_LIBRARY}
	-ldl
	-lpthread
)
