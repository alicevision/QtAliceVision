if(BUILD_IMAGEIO)
	add_subdirectory(qtAliceVisionImageIO)
endif()

if(BUILD_DEPTHMAPENTITY)
	add_subdirectory(depthMapEntity)
endif()

if(BUILD_SFM)
	add_subdirectory(qtAliceVision)
endif()
