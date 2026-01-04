if(tarox_message_included)
	  return()
endif(tarox_message_included)
set(tarox_message_included true)

function(tarox_message)
    if (ARGC LESS 2)
        message(WARNING "tarox message must specify which message mode")
        return()
    endif()

    set(mode "${ARGV0}")
    list(REMOVE_AT ARGV 0)
    string(REPLACE ";" " " msg "${ARGV}")

    file(RELATIVE_PATH REL_PATH
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_CURRENT_LIST_FILE}")

    set(line "")
    if(ARGC GREATER 1)
      # If last argument is a number, treat as line
        list(GET ARGV -1 last_arg)
        if(last_arg MATCHES "^[0-9]+$")
            set(line ":${last_arg}")
            list(REMOVE_AT ARGV -1)
            string(REPLACE ";" " " msg "${ARGV}")
        endif()
    endif()

    message(${mode} "[${REL_PATH}${line}] ${msg}")
endfunction()