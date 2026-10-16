<?xml version="1.0" encoding="UTF-8"?>
<!-- program.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 11, 2026 -->

<!-- The rest of a generated program: source/main.cpp, which plays the game
     with the engine's own Game and Engine on SFML, its build and its readme. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template name="main-file">
    <xsl:call-template name="header">
      <xsl:with-param name="file" select="'main.cpp'" />
    </xsl:call-template>
    <xsl:text>//
// </xsl:text>
    <xsl:value-of select="concat($title, ', generated from ', $source, ' by xgecli --generate windows-cpp. The')" />
    <xsl:text>
// game is in source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp, as the engine's own data; it is played
// by the engine's own code (engine/), on SFML, as xgecli plays it.

#include "audio_sfml.h"
#include "engine.h"
#include "game.h"
#include "window_sfml.h"

#include &lt;cstdlib&gt;
#include &lt;exception&gt;
#include &lt;iostream&gt;
#include &lt;memory&gt;

// The game, as written in </xsl:text>
    <xsl:value-of select="concat($source, ' (source/', $name, '.cpp).')" />
    <xsl:text>
xge::GameData </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>();

int main()
{
	try
	{
		xge::Game game{ </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>() };

		// No sound device is no reason not to play: silence, as xgecli does.
		std::unique_ptr&lt;xge::Audio&gt; audio;
		try
		{
			audio = std::make_unique&lt;xge::SFMLAudio&gt;();
		}
		catch (const std::exception&amp; error)
		{
			std::cerr &lt;&lt; "warning: no sound: " &lt;&lt; error.what() &lt;&lt; '\n';
		}

		xge::Engine engine(game, std::make_unique&lt;xge::SFMLWindow&gt;(game.getWindowDesc()), std::move(audio));

		// What the game keeps (a best score) lasts from one run to the next.
		engine.keepIn("saves/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.xml");
		engine.loop();
	}
	catch (const std::exception&amp; error)
	{
		std::cerr &lt;&lt; "Error: " &lt;&lt; error.what() &lt;&lt; '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
</xsl:text>
  </xsl:template>

  <xsl:template name="cmake-file">
    <xsl:text># CMakeLists.txt
# </xsl:text>
    <xsl:value-of select="concat($title, ', generated from ', $source, ' by xgecli --generate windows-cpp.')" />
    <xsl:text>

cmake_minimum_required(VERSION 3.28)

project(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> LANGUAGES CXX)

# The built game, and any DLLs it needs, go to output/ (with a folder for the
# configuration, Debug or Release, under Visual Studio).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output")

include(FetchContent)

# What the engine is built with: SFML 3 for the window and sound, exprtk for
# the expressions in the game, lunasvg for any &lt;svg&gt; picture. Each as
# installed (vcpkg install sfml exprtk lunasvg), or else fetched and built here.
find_package(SFML 3 COMPONENTS Graphics Audio QUIET)
if (NOT SFML_FOUND)
	message(STATUS "SFML 3 not found, using FetchContent to download and build it.")
	set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
	set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 3.1.0 EXCLUDE_FROM_ALL SYSTEM)
	FetchContent_MakeAvailable(SFML)
endif()

find_path(EXPRTK_INCLUDE_DIR exprtk.hpp)
if (EXPRTK_INCLUDE_DIR)
	add_library(exprtk INTERFACE)
	target_include_directories(exprtk SYSTEM INTERFACE "${EXPRTK_INCLUDE_DIR}")
else()
	message(STATUS "exprtk not found, using FetchContent to download it.")
	FetchContent_Declare(exprtk GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git GIT_TAG 0.0.3-cmake EXCLUDE_FROM_ALL SYSTEM)
	FetchContent_MakeAvailable(exprtk)
endif()

find_package(lunasvg QUIET)
if (NOT lunasvg_FOUND)
	message(STATUS "lunasvg not found, using FetchContent to download and build it.")
	set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	set(PLUTOVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(lunasvg GIT_REPOSITORY https://github.com/sammycage/lunasvg.git GIT_TAG v3.5.0 EXCLUDE_FROM_ALL SYSTEM)
	FetchContent_MakeAvailable(lunasvg)
endif()

add_executable(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>
	source/main.cpp
	source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp
	source/game_parts.h
	# the engine's own code, as it is in xge/lib/
</xsl:text>
    <xsl:for-each select="$engine">
      <xsl:value-of select="concat('&#9;engine/', substring-after(substring-after(., '/'), '/'), '&#10;')" />
    </xsl:for-each>
    <xsl:value-of select="concat(')&#10;target_include_directories(', $name, ' PRIVATE engine)&#10;target_compile_features(', $name, ' PRIVATE cxx_std_20)&#10;target_link_libraries(', $name, ' PRIVATE SFML::Graphics SFML::Audio exprtk lunasvg::lunasvg)&#10;')" />
    <xsl:text>
# The warnings the engine is built with (xge_warnings in its
# scripts/cmake/platform.cmake), for its code and the game's alike; /bigobj
# because exprtk makes more sections than MSVC allows an object file without it.
target_compile_options(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE
	$&lt;$&lt;CXX_COMPILER_ID:MSVC&gt;:/W4
		/w14242 /w14254 /w14263 /w14265 /w14287 /w14296 /w14311 /w14545 /w14546 /w14547
		/w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
		/external:anglebrackets /external:W0 /analyze:external- /bigobj /utf-8&gt;
	$&lt;$&lt;CXX_COMPILER_ID:GNU,Clang,AppleClang&gt;:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
		-Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wunused
		-Wnull-dereference -Wformat=2 -Wimplicit-fallthrough
		-Wold-style-cast -Wmisleading-indentation&gt;
	$&lt;$&lt;CXX_COMPILER_ID:GNU&gt;:-Wdouble-promotion -Wduplicated-cond -Wduplicated-branches
		-Wlogical-op -Wuseless-cast&gt;)

</xsl:text>
    <xsl:if test="$texts or $pictures">
      <xsl:text># The game opens its font and pictures from assets/ beside where it runs:
# copied beside the program, and the folder Visual Studio starts it in.
add_custom_command(TARGET </xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text> POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$&lt;TARGET_FILE_DIR:</xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text>&gt;/assets")
</xsl:text>
    </xsl:if>
    <xsl:value-of select="concat('set_property(TARGET ', $name, ' PROPERTY VS_DEBUGGER_WORKING_DIRECTORY &quot;$&lt;TARGET_FILE_DIR:', $name, '&gt;&quot;)&#10;')" />
    <xsl:text>
# Visual Studio starts the game when you press F5, rather than ALL_BUILD.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>)

# On Windows the program needs its libraries' DLLs beside it when they are shared.
if (WIN32)
	add_custom_command(TARGET </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$&lt;TARGET_RUNTIME_DLLS:</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>&gt;" "$&lt;TARGET_FILE_DIR:</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>&gt;"
		COMMAND_EXPAND_LISTS)
endif()
</xsl:text>
  </xsl:template>

  <xsl:template name="readme-file">
    <xsl:value-of select="concat('# ', $title, '&#10;&#10;Generated from `', $source, '` by `xgecli --generate windows-cpp`. It is the game xgecli and&#10;')" />
    <xsl:text>xgegui play, built as a program of its own on SFML 3: the stylesheets in the engine's
`games/generators/windows-cpp/` write the game file out as C++ data in `source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp`,
and everything that plays it (the frame, the physics and collisions, the keys, the
pictures and the sounds) is the engine's own code, copied unchanged into `engine/`.
Only reading XML and choosing among window and sound libraries are left out.

| File | What |
|---|---|
| `source/</xsl:text>
    <xsl:value-of select="concat($name, '.cpp` | the game, as the engine reads it from `', $source, '` |')" />
    <xsl:text>
| `source/game_parts.h` | the small functions the game is written with, a tag each |
| `source/main.cpp` | makes the game, an SFML window and SFML sound, and runs the engine's loop |
| `engine/` | the engine's own files, as they are in its `xge/lib/` |
| `assets/` | the font and pictures it opens, copied beside the program when it is built |
| `CMakeLists.txt` | the build; it uses installed SFML 3, exprtk and lunasvg, or downloads and builds them |
| `.gitignore` | leaves out `build/` and `output/` |

## Build and play

On Windows with Visual Studio and vcpkg (`vcpkg install sfml exprtk lunasvg`), from this folder:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
output\Release\</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.exe
```

Without vcpkg, leave out the toolchain file: the libraries are downloaded and built with the game.
The built game, its DLLs and its `assets/` are in `output/`; `build/` holds the rest.

Change the game in its XML file and generate it again, rather than editing `source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp`.
</xsl:text>
  </xsl:template>
</xsl:stylesheet>
