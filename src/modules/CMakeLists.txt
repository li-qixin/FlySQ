file(GLOB _tarox_module_cmake_files
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*/CMakeLists.txt")
list(SORT _tarox_module_cmake_files)

foreach(_tarox_module_cmake_file IN LISTS _tarox_module_cmake_files)
  get_filename_component(_tarox_module_dir "${_tarox_module_cmake_file}" DIRECTORY)
  string(TOUPPER "${_tarox_module_dir}" _tarox_module_config_suffix)
  string(REGEX REPLACE "[^A-Z0-9]" "_" _tarox_module_config_suffix
    "${_tarox_module_config_suffix}")

  set(_tarox_module_config_var "CONFIG_MODULES_${_tarox_module_config_suffix}")
  if(${_tarox_module_config_var})
    add_subdirectory(${_tarox_module_dir})
  endif()
endforeach()
