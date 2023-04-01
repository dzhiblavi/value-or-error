set(DOXYGEN_CONF_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxygen.conf.in)
set(DOXYGEN_CONF_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxygen.conf)

configure_file(${DOXYGEN_CONF_IN} ${DOXYGEN_CONF_OUT} @ONLY)
message(STATUS "Configured Doxygen.conf in ${DOXYGEN_CONF_OUT}")

add_custom_target(
  doxygen_docs
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONF_OUT}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating documentation"
  VERBATIM
)
