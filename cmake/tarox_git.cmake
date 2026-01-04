if(tarox_git_included)
	return()
endif(tarox_git_included)
set(tarox_git_included true)
include(tarox_parse_function_args)
#=============================================================================
#
#	tarox_add_git_submodule
#
#	This function add a git submodule target.
#
#	Usage:
#		tarox_add_git_submodule(TARGET <target> PATH <path>)
#
#	Input:
#		PATH		: git submodule path
#
#	Output:
#		TARGET		: git target
#
#	Example:
#		tarox_add_git_submodule(TARGET git_nuttx PATH "Nuttx")
#
function(tarox_add_git_submodule)
	tarox_parse_function_args(
		NAME tarox_add_git_submodule
		ONE_VALUE TARGET PATH
		REQUIRED TARGET PATH
		ARGN ${ARGN})

	set(REL_PATH)

	if(IS_ABSOLUTE ${PATH})
		file(RELATIVE_PATH REL_PATH ${TAROX_SOURCE_DIR} ${PATH})
	else()
		file(RELATIVE_PATH REL_PATH ${TAROX_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${PATH})
	endif()

	execute_process(
		COMMAND tools/check_submodules.sh ${REL_PATH}
		WORKING_DIRECTORY ${TAROX_SOURCE_DIR}
		)

	string(REPLACE "/" "_" NAME ${PATH})
	string(REPLACE "." "_" NAME ${NAME})

	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/git_init_${NAME}.stamp
		COMMAND tools/check_submodules.sh ${REL_PATH}
		COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/git_init_${NAME}.stamp
		DEPENDS ${TAROX_SOURCE_DIR}/.gitmodules ${PATH}/.git
		COMMENT "git submodule ${REL_PATH}"
		WORKING_DIRECTORY ${TAROX_SOURCE_DIR}
		USES_TERMINAL
		)

	add_custom_target(${TARGET} DEPENDS git_init_${NAME}.stamp)
endfunction()