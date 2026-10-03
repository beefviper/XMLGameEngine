# deploy_libraries.cmake
# XML Game Engine
# author: beefviper
# date: Oct 3, 2026
#
# Run after each program is built with MSVC (cmake -P, from xge_place_program()
# in output.cmake), with PROGRAM_DIR, LIBRARY_DIR, ASSEMBLY_NAME and
# ASSEMBLY_ARCH set.
#
# 1. Moves every DLL next to the program into libraries/. vcpkg copies the
#    DLLs a program needs next to it after each build (it follows what each
#    DLL links to, so it finds them all); this moves them on, with their .pdb.
# 2. Writes libraries/libraries.manifest, the private assembly the programs'
#    own manifests name, listing every DLL in libraries/. Windows loads a DLL
#    from there only if it is listed, so the list is made from the folder
#    itself, after windeployqt has put Qt's DLLs in it.
#
# Every program shares the one folder and parallel builds can run this for
# several at once, so the work is done under a lock, and the manifest is
# replaced in one rename so a program starting meanwhile never reads half of it.
# The lock is held until cmake exits (GUARD FILE crashes CMake 3.28 in -P mode).

file(MAKE_DIRECTORY "${LIBRARY_DIR}")
file(LOCK "${LIBRARY_DIR}" DIRECTORY GUARD PROCESS TIMEOUT 300)

file(GLOB stray_dlls LIST_DIRECTORIES false "${PROGRAM_DIR}/*.dll")
foreach(dll IN LISTS stray_dlls)
	get_filename_component(dll_name "${dll}" NAME)
	get_filename_component(dll_stem "${dll}" NAME_WLE)
	file(COPY_FILE "${dll}" "${LIBRARY_DIR}/${dll_name}" ONLY_IF_DIFFERENT)
	file(REMOVE "${dll}")

	if (EXISTS "${PROGRAM_DIR}/${dll_stem}.pdb")
		file(COPY_FILE "${PROGRAM_DIR}/${dll_stem}.pdb" "${LIBRARY_DIR}/${dll_stem}.pdb" ONLY_IF_DIFFERENT)
		file(REMOVE "${PROGRAM_DIR}/${dll_stem}.pdb")
	endif()
endforeach()

file(GLOB dlls RELATIVE "${LIBRARY_DIR}" LIST_DIRECTORIES false "${LIBRARY_DIR}/*.dll")
list(SORT dlls CASE INSENSITIVE)

set(manifest "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>
<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">
  <assemblyIdentity type=\"win32\" name=\"${ASSEMBLY_NAME}\" version=\"1.0.0.0\" processorArchitecture=\"${ASSEMBLY_ARCH}\"/>
")
foreach(dll IN LISTS dlls)
	string(APPEND manifest "  <file name=\"${dll}\"/>\n")
endforeach()
string(APPEND manifest "</assembly>\n")

set(manifest_path "${LIBRARY_DIR}/${ASSEMBLY_NAME}.manifest")
set(old_manifest "")
if (EXISTS "${manifest_path}")
	file(READ "${manifest_path}" old_manifest)
endif()

if (NOT old_manifest STREQUAL manifest)
	file(WRITE "${manifest_path}.tmp" "${manifest}")
	file(RENAME "${manifest_path}.tmp" "${manifest_path}")
endif()
