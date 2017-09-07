# Try to find libatutility.so 
 




find_package(PackageHandleStandardArgs)


find_path(ATUTILITY_INCLUDE_DIRS atutility.h)
find_library(ATUTILITY_LIBRARIES atutility)


find_package_handle_standard_args(ATUTILITY DEFAULT_MSG ATUTILITY_LIBRARIES ATUTILITY_INCLUDE_DIRS)


mark_as_advanced(
	ATUTILITY_INCLUDE_DIRS
	ATUTILITY_LIBRARIES
)
