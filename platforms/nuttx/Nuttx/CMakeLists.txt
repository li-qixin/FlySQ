set(NUTTX_CONFIG_DIR ${TAROX_CONFIG_DIR}/nuttx-config)
set(APPS_DIR ${NUTTX_SRC_DIR}/apps)
set(NUTTX_INCLUDE_DIR ${NUTTX_KERNEL_DIR}/include)
###############################################################################
# Nuttx build
###############################################################################

configure_file(${TAROX_SOURCE_DIR}/platforms/nuttx/Nuttx/Make.defs.in ${CMAKE_CURRENT_BINARY_DIR}/nuttx/Make.defs)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/nuttx/Make.defs ${NUTTX_KERNEL_DIR}/Make.defs)

# create .config and Make.defs in nuttx
add_custom_command(
	OUTPUT ${NUTTX_KERNEL_DIR}/.config
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NUTTX_DEFCONFIG} ${NUTTX_KERNEL_DIR}/.config
	COMMAND ${NUTTX_SRC_DIR}/tools/nuttx_make_olddefconfig.sh > ${CMAKE_CURRENT_BINARY_DIR}/nuttx_olddefconfig.log
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NUTTX_KERNEL_DIR}/.config ${CMAKE_CURRENT_BINARY_DIR}/nuttx/.config
	DEPENDS ${NUTTX_DEFCONFIG}
	WORKING_DIRECTORY ${NUTTX_KERNEL_DIR}
)

add_custom_command(
	OUTPUT ${NUTTX_KERNEL_DIR}/include/arch/chip ${NUTTX_KERNEL_DIR}/include/nuttx/config.h
	COMMAND make --no-print-directory --silent clean_context
	COMMAND make --no-print-directory --silent pass1dep > ${CMAKE_CURRENT_BINARY_DIR}/nuttx_context.log
	DEPENDS ${NUTTX_KERNEL_DIR}/.config
	WORKING_DIRECTORY ${NUTTX_KERNEL_DIR}
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nuttx/config.h
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NUTTX_KERNEL_DIR}/include/nuttx/config.h ${CMAKE_CURRENT_BINARY_DIR}/nuttx/config.h
	DEPENDS ${NUTTX_KERNEL_DIR}/include/nuttx/config.h
)

add_custom_target(nuttx_context
	DEPENDS 
	${CMAKE_CURRENT_BINARY_DIR}/nuttx/config.h
)

if(NOT DEFINED builtin_apps_string)
	set(builtin_apps_string "")
endif()

if(NOT DEFINED builtin_apps_decl_string)
	set(builtin_apps_decl_string "")
endif()

if(NOT DEFINED kernel_builtin_apps_proxy_string)
	set(kernel_builtin_apps_proxy_string "")
endif()

get_property(TAROX_BUILTIN_APPS_STRING GLOBAL PROPERTY TAROX_BUILTIN_APPS_STRING)
if(TAROX_BUILTIN_APPS_STRING)
	string(APPEND builtin_apps_string "${TAROX_BUILTIN_APPS_STRING}")
endif()

get_property(TAROX_BUILTIN_APPS_DECL_STRING GLOBAL PROPERTY TAROX_BUILTIN_APPS_DECL_STRING)
if(TAROX_BUILTIN_APPS_DECL_STRING)
	string(APPEND builtin_apps_decl_string "${TAROX_BUILTIN_APPS_DECL_STRING}")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tarox.bdat.in ${CMAKE_CURRENT_BINARY_DIR}/tarox.bdat)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tarox.pdat.in ${CMAKE_CURRENT_BINARY_DIR}/tarox.pdat)
# APPS

# libapps.a
file(GLOB_RECURSE nuttx_apps_files LIST_DIRECTORIES false
	${APPS_DIR}/*.c
	${APPS_DIR}/*.h
)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/apps/libapps.a
	COMMAND ${CMAKE_COMMAND} -E remove -f ${APPS_DIR}/libapps.a ${APPS_DIR}/builtin/builtin_list.h ${APPS_DIR}/builtin/builtin_proto.h
	COMMAND find ${APPS_DIR} -type f -name \*.o -delete
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/tarox.bdat ${APPS_DIR}/builtin/registry/tarox.bdat
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/tarox.pdat ${APPS_DIR}/builtin/registry/tarox.pdat
	COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${APPS_DIR}/builtin/registry/.updated
	COMMAND make --no-print-directory --silent TOPDIR="${NUTTX_KERNEL_DIR}" > ${CMAKE_CURRENT_BINARY_DIR}/nuttx_apps.log
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${APPS_DIR}/libapps.a ${CMAKE_CURRENT_BINARY_DIR}/apps/libapps.a
	DEPENDS
		${nuttx_apps_files}
		nuttx_context
		${CMAKE_CURRENT_BINARY_DIR}/nuttx/config.h
		${CMAKE_CURRENT_BINARY_DIR}/tarox.bdat
		${CMAKE_CURRENT_BINARY_DIR}/tarox.pdat
	WORKING_DIRECTORY ${APPS_DIR}
	#USES_TERMINAL
)
add_custom_target(nuttx_apps_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/apps/libapps.a)
add_library(nuttx_apps STATIC IMPORTED GLOBAL)
set_property(TARGET nuttx_apps PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/apps/libapps.a)
add_dependencies(nuttx_apps nuttx_apps_build)

# helper for all targets
function(add_nuttx_dir nuttx_lib nuttx_lib_dir kernel extra target)
	if (${target} STREQUAL all)
		set(nuttx_lib_target all)
	else()
		set(nuttx_lib_target lib${target}.a)
	endif()

	file(GLOB_RECURSE nuttx_lib_files LIST_DIRECTORIES false
		${CMAKE_CURRENT_SOURCE_DIR}/nuttx/${nuttx_lib_dir}/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/nuttx/${nuttx_lib_dir}/*.h
	)

	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nuttx/${nuttx_lib_dir}/lib${nuttx_lib}.a
		COMMAND ${CMAKE_COMMAND} -E remove -f ${NUTTX_KERNEL_DIR}/${nuttx_lib_dir}/lib${nuttx_lib}.a
		COMMAND find ${nuttx_lib_dir} -type f -name \*.o -delete
		COMMAND make -C ${nuttx_lib_dir} --no-print-directory --silent ${nuttx_lib_target} TOPDIR="${NUTTX_KERNEL_DIR}" KERNEL=${kernel} EXTRAFLAGS=${extra}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NUTTX_KERNEL_DIR}/${nuttx_lib_dir}/lib${nuttx_lib}.a ${CMAKE_CURRENT_BINARY_DIR}/nuttx/${nuttx_lib_dir}/lib${nuttx_lib}.a
		DEPENDS
			${nuttx_lib_files}
			nuttx_context ${CMAKE_CURRENT_BINARY_DIR}/nuttx/config.h
		WORKING_DIRECTORY ${NUTTX_KERNEL_DIR}
		#USES_TERMINAL
	)
	add_custom_target(nuttx_${nuttx_lib}_build DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/nuttx/${nuttx_lib_dir}/lib${nuttx_lib}.a)

	add_library(nuttx_${nuttx_lib} STATIC IMPORTED GLOBAL)
	target_include_directories(nuttx_${nuttx_lib} INTERFACE ${NUTTX_INCLUDE_DIR})
	set_property(TARGET nuttx_${nuttx_lib} PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/nuttx/${nuttx_lib_dir}/lib${nuttx_lib}.a)
	add_dependencies(nuttx_${nuttx_lib} nuttx_${nuttx_lib}_build)
endfunction()

# add_nuttx_dir(NAME DIRECTORY KERNEL EXTRA)
add_nuttx_dir(binfmt binfmt y -D__KERNEL__ all)
add_nuttx_dir(boards boards y -D__KERNEL__ all)
add_nuttx_dir(drivers drivers y -D__KERNEL__ all)
add_nuttx_dir(fs fs y -D__KERNEL__ all)
add_nuttx_dir(sched sched y -D__KERNEL__ all)
add_nuttx_dir(xx libs/libxx n "" all)
add_nuttx_dir(crypto crypto y -D__KERNEL__ all)
add_nuttx_dir(arch arch/${CONFIG_ARCH}/src y -D__KERNEL__ all)
add_nuttx_dir(c libs/libc n "" all)
add_nuttx_dir(mm mm n "" mm)