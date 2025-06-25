function(Doxygen input output)
    find_package(Doxygen)

    if (NOT DOXYGEN_FOUND)
        add_custom_target(doxygen COMMAND false
            COMMENT "Doxygen not found!!"
        )
        return()
    endif()

    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_HTML_OUTPUT ${PROJECT_SOURCE_DIR}/${output}/html)
    set(DOXYGEN_GENERATE_LATEX YES)
    set(DOXYGEN_LATEX_OUTPUT ${PROJECT_SOURCE_DIR}/${output}/latex)
    set(DOXYGEN_EXTRACT_ALL YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    set(DOXYGEN_RECURSIVE YES)
    set(DOXYGEN_QUIET YES)

    doxygen_add_docs(doxygen
        ${PROJECT_SOURCE_DIR}/${input}
        COMMENT "Generating documentation"
    )
    
endfunction()
