# # Force the use of the host compiler for this directory
# # (Usually handles itself if not inheriting global cross-compile flags)
# add_executable(mkfs_myfs 
#     main.c 
# )
#
# # If your OS headers and Host headers conflict, 
# # make sure this tool ONLY sees its own include dir and standard system headers.
# target_include_directories(mkfs_myfs PRIVATE include)
#
# # This defines where the binary goes in the build folder
# set_target_properties(mkfs_myfs PROPERTIES 
#     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
# )
