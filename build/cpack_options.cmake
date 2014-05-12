set(CPACK_PACKAGE_NAME "ignition_transport")
set(CPACK_PACKAGE_VENDOR "osrfoundation.org")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "A set of transport classes for robot applications.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "ignition_transport")
set(CPACK_RESOURCE_FILE_LICENSE "/home/caguero/workspace/ign_transport/LICENSE")
set(CPACK_RESOURCE_FILE_README "/home/caguero/workspace/ign_transport/README")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/home/caguero/workspace/ign_transport/README")
set(CPACK_PACKAGE_MAINTAINER "OSR Foundation <info@osrfoundation.org>")
set(CPACK_PACKAGE_CONTACT "OSR Foundation <info@osrfoundation.org>")

set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libprotobuf-dev, libprotoc-dev")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
  "A set of transport classes for robot applications.")

set(CPACK_RPM_PACKAGE_ARCHITECTURE "")
set(CPACK_RPM_PACKAGE_REQUIRES "libprotobuf-dev, libprotoc-dev")
set(CPACK_RPM_PACKAGE_DESCRIPTION
  "A set of transport classes for robot applications.")

set (CPACK_PACKAGE_FILE_NAME
  "ignition_transport-0.1.0")
set (CPACK_SOURCE_PACKAGE_FILE_NAME
  "ignition_transport-0.1.0")
