# Try to find libatcore.so (probably needs to be checked on completeness)
#
# Defines
#
# ANDOR_FOUND - system has libatcore
# ANDOR_INCLUDE_DIRS - libatcore include directory
# ANDOR_LIBRARIES - libatcore library


find_package(PackageHandleStandardArgs)

find_path(ATCORE_INCLUDE_DIRS atcore.h)
find_library(ATCORE_LIBRARIES atcore)

find_package_handle_standard_args(ATCORE DEFAULT_MSG ATCORE_LIBRARIES ATCORE_INCLUDE_DIRS)

mark_as_advanced(
	ATCORE_INCLUDE_DIRS
	ATCORE_LIBRARIES
)
