enable_language(ASM_NASM)


set(TCC_BIN "${CMAKE_CURRENT_SOURCE_DIR}/tcc.elf")
set(MUSL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/musl")
set(TCC_DIR "${CMAKE_SOURCE_DIR}/extern/tcc")

# --- 2. Build User Executables ---
file(GLOB USER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/user_src/*.c")
set(USER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/BOOT")

foreach(USER_FILE ${USER_SOURCES})
    get_filename_component(USER_NAME ${USER_FILE} NAME_WE)
    set(USER_OBJ "${CMAKE_CURRENT_BINARY_DIR}/user_objs/${USER_NAME}.o")
    set(USER_ELF "${USER_OUTPUT_DIR}/${USER_NAME}.elf")

    # Compile with TCC
    add_custom_command(
        OUTPUT "${USER_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/user_objs"
        COMMAND "${TCC_BIN}" -c "${USER_FILE}" -o "${USER_OBJ}" 
                -nostdinc 
                "-I${TCC_DIR}/include" 
                "-I${MUSL_DIR}/include"
                "-I${MUSL_DIR}/obj/include"
        DEPENDS "${USER_FILE}" "${TCC_BIN}"
        COMMENT "TCC compiling ${USER_NAME}.c"
        VERBATIM # This is the magic key for argument handling
    )

  # LINK EVERYTHING together with TCC
    add_custom_command(
        OUTPUT "${USER_ELF}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${USER_OUTPUT_DIR}"
        # Using -Wl,-Ttext to force the loading address to 0x400000
        COMMAND "${TCC_BIN}" -o "${USER_ELF}" -static -nostdlib
                "-Wl,-Ttext=0x400000"
                "${MUSL_DIR}/obj/crt/crt1.o"
                "${MUSL_DIR}/obj/crt/x86_64/crti.o"
                "${USER_OBJ}"
                "${MUSL_DIR}/obj/crt/x86_64/crtn.o"
                "-L${MUSL_DIR}/lib" -lc
        DEPENDS "${USER_OBJ}" "${TCC_BIN}"
        COMMENT "TCC Linking ${USER_NAME}.elf"
        VERBATIM
    ) 
    add_custom_target(${USER_NAME}_target ALL DEPENDS ${USER_ELF})
endforeach()
