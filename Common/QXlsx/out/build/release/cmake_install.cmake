# Install script for directory: E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/QXlsx")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/out/build/release/QXlsxQt5.lib")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/QXlsxQt5" TYPE FILE FILES
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxabstractooxmlfile.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxabstractsheet.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxabstractsheet_p.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxcellformula.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxcell.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxcelllocation.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxcellrange.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxcellreference.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxchart.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxchartsheet.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxconditionalformatting.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxdatavalidation.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxdatetype.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxdocument.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxformat.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxglobal.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxrichstring.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxworkbook.h"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/header/xlsxworksheet.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xdevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake"
         "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/out/build/release/CMakeFiles/Export/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5" TYPE FILE FILES "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/out/build/release/CMakeFiles/Export/lib/cmake/QXlsxQt5/QXlsxQt5Targets.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5" TYPE FILE FILES "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/out/build/release/CMakeFiles/Export/lib/cmake/QXlsxQt5/QXlsxQt5Targets-release.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QXlsxQt5" TYPE FILE FILES
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/out/build/release/QXlsxQt5Config.cmake"
    "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/out/build/release/QXlsxQt5ConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "E:/BaiduNetdiskDownload/QXlsx-1.4.6/QXlsx-1.4.6/QXlsx/out/build/release/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
