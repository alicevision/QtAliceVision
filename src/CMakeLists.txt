if(BUILD_IMAGEIO)
    add_subdirectory(qtAliceVisionImageIO)
endif()

if(BUILD_SFM)
    add_subdirectory(qtAliceVision)
endif()


if(BUILD_CURVEVIEWER)
    add_subdirectory(curveViewer)
endif()

if(BUILD_MESHVIEWER)
    add_subdirectory(meshViewer)
endif()
