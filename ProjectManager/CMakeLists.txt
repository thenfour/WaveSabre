set(PROJECT_MANAGER_SOURCES
	Program.cs
	ProjectDetails.cs
	ProjectDetails.Designer.cs
	ProjectManager.cs
	ProjectManager.Designer.cs

	Properties/AssemblyInfo.cs
	Properties/Resources.Designer.cs
	Properties/Settings.Designer.cs

	Properties/Resources.resx
	Properties/Settings.settings

	logicoma_basic_CoA_icon.ico

	ProjectDetails.resx
	ProjectManager.resx
)

add_executable(ProjectManager
	App.config

	${PROJECT_MANAGER_SOURCES}
)

target_link_libraries(ProjectManager WaveSabreConvert)

set_property(TARGET ProjectManager PROPERTY VS_GLOBAL_ApplicationIcon "${CMAKE_CURRENT_SOURCE_DIR}/logicoma_basic_CoA_icon.ico")
csharp_set_designer_cs_properties(${PROJECT_MANAGER_SOURCES})
csharp_set_windows_forms_properties(${PROJECT_MANAGER_SOURCES})
# set_property(TARGET ProjectManager PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
set_property(TARGET ProjectManager PROPERTY VS_DOTNET_REFERENCES 
	"System"
	"System.Core"
	"System.Xml.Linq"
	"System.Data.DataSetExtensions"
	"Microsoft.CSharp"
	"System.Data"
	"System.Xml"
	"System.Windows"
	"System.Windows.Forms"
	"System.Drawing"
)
set(_PROJECT_MANAGER_PLATFORM_TARGET x64)
if(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
	set(_PROJECT_MANAGER_PLATFORM_TARGET x86)
endif()

set_target_properties(ProjectManager PROPERTIES
	VS_GLOBAL_ROOTNAMESPACE ProjectManager
	VS_GLOBAL_PlatformTarget ${_PROJECT_MANAGER_PLATFORM_TARGET}
	VS_GLOBAL_Prefer32Bit false)

add_custom_command(TARGET ProjectManager POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:WaveSabreStandAlonePlayer> $<TARGET_FILE_DIR:ProjectManager>)

foreach(target ${VST_TARGETS})
	add_custom_command(TARGET ProjectManager POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:ProjectManager>)
	# Conditionally copy PDB only if it exists (e.g., Debug/RelWithDebInfo). Skip for MinSizeRel/Release where PDB may be absent.
	add_custom_command(TARGET ProjectManager POST_BUILD 
		COMMAND ${CMAKE_COMMAND} -DSRC="$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.pdb" -DDST="$<TARGET_FILE_DIR:ProjectManager>" -P "${CMAKE_CURRENT_LIST_DIR}/CopyIfExists.cmake")
endforeach()
