set(VSTDIR "" CACHE PATH "VST system directory")

set(VST_TARGETS "" CACHE INTERNAL "VST Targets" FORCE)

file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
foreach(child ${children})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
		file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/${child}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/${child}/*.hpp)
		aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/${child} sources)
		file(WRITE "${CMAKE_BINARY_DIR}/${child}.def"
			"LIBRARY ${child}\nEXPORTS\nVSTPluginMain\nWaveSabreDeviceVSTChunkToMinifiedChunk\nWaveSabreFreeChunk\nWaveSabreTestCompression\nmain=VSTPluginMain")

		source_group("Source Files" FILES ${sources})

		add_library(${child} SHARED
			${sources}
			${CMAKE_BINARY_DIR}/${child}.def)
		target_link_libraries(${child} WaveSabreCore WaveSabreVstLib)
		set_property(TARGET ${child} APPEND_STRING PROPERTY LINK_FLAGS_MINSIZEREL
			" /LTCG")
		set_property(TARGET ${child} APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO
			" /LTCG")
		set_property(TARGET ${child} PROPERTY FOLDER VSTs)
		target_compile_features(${child} PUBLIC cxx_std_17)

		if(VSTDIR)
			add_custom_command(TARGET ${child} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${child}> ${VSTDIR})
		endif()

		list(APPEND VST_TARGETS ${child})

	endif()
endforeach()

# Make VST_TARGETS visible in the parent scope
set(VST_TARGETS ${VST_TARGETS} PARENT_SCOPE)

