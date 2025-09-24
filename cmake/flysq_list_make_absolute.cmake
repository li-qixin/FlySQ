# cmake include guard
if(flysq_list_make_absolute_included)
	return()
endif(flysq_list_make_absolute_included)
set(flysq_list_make_absolute_included true)

#=============================================================================
#
#	flysq_list_make_absolute
#
#	prepend a prefix to each element in a list, if the element is not an abolute
#	path
#
function(flysq_list_make_absolute var prefix)
	set(list_var "")
	foreach(f ${ARGN})
		if(IS_ABSOLUTE ${f})
			list(APPEND list_var "${f}")
		else()
			list(APPEND list_var "${prefix}/${f}")
		endif()
	endforeach(f)
	set(${var} "${list_var}" PARENT_SCOPE)
endfunction()