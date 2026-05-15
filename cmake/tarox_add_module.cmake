include(CMakeParseArguments)

function(tarox_add_module)
  set(options)
  set(oneValueArgs MODULE MAIN STACK PRIORITY)
  set(multiValueArgs COMPILE_FLAGS INCLUDES DEPENDS SRCS)
  cmake_parse_arguments(TAROX_MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT TAROX_MODULE_MODULE OR NOT TAROX_MODULE_MAIN)
    message(FATAL_ERROR "tarox_add_module requires MODULE and MAIN")
  endif()

  if(NOT TAROX_MODULE_SRCS)
    message(FATAL_ERROR "tarox_add_module(${TAROX_MODULE_MODULE}) requires SRCS")
  endif()

  if(NOT TAROX_MODULE_PRIORITY)
    set(TAROX_MODULE_PRIORITY 100)
  endif()

  if(NOT TAROX_MODULE_STACK)
    set(TAROX_MODULE_STACK 2048)
  endif()

  add_library(${TAROX_MODULE_MODULE} STATIC EXCLUDE_FROM_ALL ${TAROX_MODULE_SRCS})

  if(TAROX_MODULE_COMPILE_FLAGS)
    target_compile_options(${TAROX_MODULE_MODULE} PRIVATE ${TAROX_MODULE_COMPILE_FLAGS})
  endif()

  if(TAROX_MODULE_INCLUDES)
    target_include_directories(${TAROX_MODULE_MODULE} PRIVATE ${TAROX_MODULE_INCLUDES})
  endif()

  if(TAROX_MODULE_OS_INCLUDES)
    target_include_directories(${TAROX_MODULE_MODULE} PRIVATE ${TAROX_MODULE_OS_INCLUDES})
  endif()

  if(TAROX_MODULE_DEPENDS)
    foreach(dep ${TAROX_MODULE_DEPENDS})
      if(TARGET ${dep})
        get_target_property(dep_type ${dep} TYPE)
        if((dep_type STREQUAL "STATIC_LIBRARY") OR (dep_type STREQUAL "INTERFACE_LIBRARY"))
          target_link_libraries(${TAROX_MODULE_MODULE} PRIVATE ${dep})
        else()
          add_dependencies(${TAROX_MODULE_MODULE} ${dep})
        endif()
      else()
        message(STATUS "tarox_add_module(${TAROX_MODULE_MODULE}): dependency target '${dep}' is not available at configure time, skip direct link")
      endif()
    endforeach()
  endif()

  set_property(GLOBAL APPEND PROPERTY TAROX_MODULE_LIBRARIES ${TAROX_MODULE_MODULE})
  set_target_properties(
    ${TAROX_MODULE_MODULE}
    PROPERTIES
      MAIN ${TAROX_MODULE_MAIN}
      PRIORITY ${TAROX_MODULE_PRIORITY}
      STACK_MAIN ${TAROX_MODULE_STACK}
  )

  if(CONFIG_BUILTIN AND CONFIG_NSH_BUILTIN_APPS)
    set_property(
      GLOBAL APPEND_STRING PROPERTY TAROX_BUILTIN_APPS_STRING
      "{ \"${TAROX_MODULE_MODULE}\", ${TAROX_MODULE_PRIORITY}, ${TAROX_MODULE_STACK}, ${TAROX_MODULE_MAIN} },\n"
    )
    set_property(
      GLOBAL APPEND_STRING PROPERTY TAROX_BUILTIN_APPS_DECL_STRING
      "int ${TAROX_MODULE_MAIN}(int argc, char *argv[]);\n"
    )
  endif()
endfunction()
