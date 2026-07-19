file(GLOB _tarox_driver_cmake_files
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "*/CMakeLists.txt")
list(SORT _tarox_driver_cmake_files)

foreach(_tarox_driver_cmake_file IN LISTS _tarox_driver_cmake_files)
  get_filename_component(_tarox_driver_dir "${_tarox_driver_cmake_file}" DIRECTORY)
  string(TOUPPER "${_tarox_driver_dir}" _tarox_driver_config_suffix)
  string(REGEX REPLACE "[^A-Z0-9]" "_" _tarox_driver_config_suffix
    "${_tarox_driver_config_suffix}")

  set(_tarox_driver_config_var "CONFIG_DRIVERS_${_tarox_driver_config_suffix}")
  if(${_tarox_driver_config_var})
    add_subdirectory(${_tarox_driver_dir})
  endif()
endforeach()
