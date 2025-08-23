# Copies a file if it exists, otherwise does nothing.
# Usage: cmake -DSRC="<source path>" -DDST="<destination dir>" -P CopyIfExists.cmake

if(NOT DEFINED SRC)
  message(FATAL_ERROR "SRC not defined for CopyIfExists.cmake")
endif()

if(NOT DEFINED DST)
  message(FATAL_ERROR "DST not defined for CopyIfExists.cmake")
endif()

if(EXISTS "${SRC}")
  message(STATUS "CopyIfExists: copying ${SRC} -> ${DST}")
  file(COPY "${SRC}" DESTINATION "${DST}")
else()
  message(STATUS "CopyIfExists: source not found, skipping: ${SRC}")
endif()
