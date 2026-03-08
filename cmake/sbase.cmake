# --- 1. Build the sbase internal library ---
file(GLOB SBASE_LIB_SOURCES 
    "${CMAKE_SOURCE_DIR}/extern/sbase/libutil/*.c"
    "${CMAKE_SOURCE_DIR}/extern/sbase/libutf/*.c"
)

# We use a custom command to make libsbase.a so it uses your cross-compiler
set(SBASE_LIB "${CMAKE_CURRENT_BINARY_DIR}/libsbase.a")
set(SBASE_LIB_OBJS "")

foreach(SRC ${SBASE_LIB_SOURCES})
    get_filename_component(NAME ${SRC} NAME_WE)
    set(OBJ "${CMAKE_CURRENT_BINARY_DIR}/sbase_objs/${NAME}.o")
    list(APPEND SBASE_LIB_OBJS ${OBJ})
    
    add_custom_command(
        OUTPUT ${OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/sbase_objs"
        COMMAND i686-myos-gcc ${COMMON_USER_FLAGS} -I"${CMAKE_SOURCE_DIR}/extern/sbase" -c ${SRC} -o ${OBJ}
        DEPENDS ${SRC} newlib_project-flatten
    )
endforeach()

add_custom_command(
    OUTPUT ${SBASE_LIB}
    COMMAND i686-myos-ar rcs ${SBASE_LIB} ${SBASE_LIB_OBJS}
    DEPENDS ${SBASE_LIB_OBJS}
)

# --- 2. The App Loop (Simplified) ---
file(GLOB SBASE_TOOLS "${CMAKE_SOURCE_DIR}/extern/sbase/*.c")

foreach(TOOL_SRC ${SBASE_TOOLS})
    get_filename_component(TOOL_NAME ${TOOL_SRC} NAME_WE)
    
    if(TOOL_NAME MATCHES "sed|tar|getconf")
        continue()
    endif()

    set(TOOL_ELF "${USER_OUTPUT_DIR}/${TOOL_NAME}.elf")

    add_custom_command(
        OUTPUT "${TOOL_ELF}"
        COMMAND i686-myos-gcc ${COMMON_USER_FLAGS} 
                -I"${CMAKE_SOURCE_DIR}/extern/sbase"
                -o "${TOOL_ELF}" 
                "${NEWLIB_SYSROOT}/usr/lib/crt0.o" 
                "${TOOL_SRC}"
                ${SBASE_LIB} 
                -L${NEWLIB_SYSROOT}/usr/lib -lc -lnosys
        DEPENDS "${TOOL_SRC}" ${SBASE_LIB}
        COMMENT "Building sbase tool: ${TOOL_NAME}"
        VERBATIM
    )

    add_custom_target(${TOOL_NAME}_target ALL DEPENDS "${TOOL_ELF}")
endforeach()
