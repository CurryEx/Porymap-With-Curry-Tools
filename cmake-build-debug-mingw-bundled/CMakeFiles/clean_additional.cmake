# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\porymap_master_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\porymap_master_autogen.dir\\ParseCache.txt"
  "porymap_master_autogen"
  )
endif()
