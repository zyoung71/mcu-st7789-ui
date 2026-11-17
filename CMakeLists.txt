if (NOT TARGET mcu-st7789-ui)

    add_library(mcu-st7789-ui STATIC
        src/display/ST7789.cpp
    )

    target_include_directories(mcu-st7789-ui PUBLIC
        include
    )

    add_subdirectory(lib/mcu-interactive-ui)
    add_subdirectory(lib/mcu-platform)

    target_link_libraries(mcu-st7789-ui
        mcu-platform-interface
        mcu-interactive-ui
    )
    
endif()