include_directories("../masktopoly")
if(BOUNDARY_EDIT) 
	set(Libraries u24_m2p u24_pe ${Libraries})
	add_executable(processMaskEdit processMaskEdit.cpp)
	target_link_libraries(processMaskEdit ${Libraries})
else()
	set(Libraries u24_m2p ${Libraries})
endif()

add_executable(processMask processMask.cpp)
target_link_libraries(processMask ${Libraries})

install(TARGETS processMask RUNTIME DESTINATION bin)
