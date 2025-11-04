# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\QXlsx_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\QXlsx_autogen.dir\\ParseCache.txt"
  "QXlsx_autogen"
  )
endif()
