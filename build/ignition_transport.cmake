if (IGNITION_TRANSPORT_CONFIG_INCLUDED)
  return()
endif()
set(IGNITION_TRANSPORT_CONFIG_INCLUDED TRUE)

list(APPEND IGNITION_TRANSPORT_INCLUDE_DIRS /home/caguero/local/include)

list(APPEND IGNITION_TRANSPORT_LIBRARY_DIRS /home/caguero/local/lib)

list(APPEND IGNITION_TRANSPORT_CFLAGS -I/home/caguero/local/include)

foreach(lib ignition_transport)
  set(onelib "${lib}-NOTFOUND")
  find_library(onelib ${lib}
    PATHS /home/caguero/local/lib
    NO_DEFAULT_PATH
    )
  if(NOT onelib)
    message(FATAL_ERROR "Library '${lib}' in package IGNITION_TRANSPORT is not installed properly")
  endif()
  list(APPEND IGNITION_TRANSPORT_LIBRARIES ${onelib})
endforeach()

foreach(dep Protobuf)
  if(NOT ${dep}_FOUND)
    find_package(${dep} REQUIRED)
  endif()
  list(APPEND IGNITION_TRANSPORT_INCLUDE_DIRS ${${dep}_INCLUDE_DIRS})
  list(APPEND IGNITION_TRANSPORT_LIBRARIES ${${dep_lib}_LIBRARIES})
endforeach()

list(APPEND IGNITION_TRANSPORT_LDFLAGS -L/home/caguero/local/lib)
