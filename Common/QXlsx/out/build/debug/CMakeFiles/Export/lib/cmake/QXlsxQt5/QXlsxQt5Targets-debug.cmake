#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "QXlsx::QXlsx" for configuration "Debug"
set_property(TARGET QXlsx::QXlsx APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(QXlsx::QXlsx PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/QXlsxQt5.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS QXlsx::QXlsx )
list(APPEND _IMPORT_CHECK_FILES_FOR_QXlsx::QXlsx "${_IMPORT_PREFIX}/lib/QXlsxQt5.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
