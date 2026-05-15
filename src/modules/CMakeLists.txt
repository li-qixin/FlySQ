# Each option is declared in the root Kconfig; board *.taroxconfig sets
# CONFIG_MODULES_*=y. Add a matching tarox_module_if_enabled() line per module.

macro(tarox_module_if_enabled config_var subdir)
  if(${config_var})
    set(_mod_dir "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
    if(NOT IS_DIRECTORY "${_mod_dir}")
      message(FATAL_ERROR "${config_var}: not a directory: ${_mod_dir}")
    endif()
    if(NOT EXISTS "${_mod_dir}/CMakeLists.txt")
      message(
        FATAL_ERROR
        "${config_var}: missing CMakeLists.txt in src/modules/${subdir}")
    endif()
    add_subdirectory(${subdir})
  endif()
endmacro()

tarox_module_if_enabled(CONFIG_MODULES_LED_DEMO led_demo)
tarox_module_if_enabled(CONFIG_MODULES_PWM_DEMO pwm_demo)
