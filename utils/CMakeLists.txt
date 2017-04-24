file(GLOB src "*.cc")
FILE(GLOB head "*.h")
add_library(bpvo_utils ${LIBRARY_TYPE} ${src} ${head})
set_target_properties(bpvo_utils PROPERTIES LINKER_LANGUAGE CXX)

if(TRUE OR IS_OSX)
  target_link_libraries(bpvo_utils ${MY_LIBRARIES_CORE} bpvo)
endif()

install(TARGETS bpvo_utils DESTINATION lib)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/utils"
  DESTINATION include/bpvo FILES_MATCHING PATTERN "*.h")


