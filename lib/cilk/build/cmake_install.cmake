# Install script for directory: /export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk/build/install")
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

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/cilk" TYPE DIRECTORY FILES "/export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk/include/cilk" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lib/libcilkrts.so.5"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lib/libcilkrts.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/lib" TYPE SHARED_LIBRARY FILES
    "/export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk/build/libcilkrts.so.5"
    "/export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk/build/libcilkrts.so"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lib/libcilkrts.so.5"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/bin/lib/libcilkrts.so"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/lib" TYPE STATIC_LIBRARY FILES "/export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk/build/libcilkrts.a")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

file(WRITE "/export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk/build/${CMAKE_INSTALL_MANIFEST}" "")
foreach(file ${CMAKE_INSTALL_MANIFEST_FILES})
  file(APPEND "/export/users/avtische/open-sources/_git/cilk/new-git-cilk-Jan-26/projects/compiler-rt/lib/cilk/build/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
endforeach()
