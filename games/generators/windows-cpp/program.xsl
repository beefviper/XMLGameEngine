<?xml version="1.0" encoding="UTF-8"?>
<!-- program.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- The rest of a windows-cpp program: source/main.cpp, the loop on SFML
     (keys, a frame at the game's rate, the picture), its build and its
     readme. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- What the game is called: its window's name, or else the program's. -->
  <xsl:variable name="title">
    <xsl:choose>
      <xsl:when test="/game/window/@name != ''"><xsl:value-of select="/game/window/@name" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:template name="main-file">
    <xsl:call-template name="header"><xsl:with-param name="file" select="'main.cpp'" /></xsl:call-template>
    <xsl:value-of select="concat('//&#10;// ', $title, ' on SFML: the keys, a frame at ')" />
    <xsl:call-template name="framerate" />
    <xsl:value-of select="concat(' a second, the picture.&#10;&#10;#include &quot;', $name, '.h&quot;&#10;')" />
    <xsl:text>
#include "engine/keys.h"
#include "window/sfml.h"
#include "window/window.h"

#include &lt;chrono&gt;
#include &lt;thread&gt;

int main()
{
	</xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:text>::</xsl:text>
    <xsl:call-template name="class-name" />
    <xsl:text> game;

	xge::SFMLWindow window(game.window);
	window.init(game.objects);

	using Clock = std::chrono::steady_clock;
	const auto period = std::chrono::duration_cast&lt;Clock::duration&gt;(std::chrono::duration&lt;double&gt;(1.0 / game.window.framerate));
	auto next = Clock::now();

	xge::HeldKeys keys;
	while (window.isOpen())
	{
		for (const auto&amp; [key, pressed] : window.pollEvents())
		{
			if (!pressed) { keys.release(key); game.key(key, false); }
			else if (keys.press(key)) { game.key(key, true); }
		}

		game.frame();
		xge::drawFrame(window, game.objects, game.window.background, [&amp;](const xge::Object&amp; object) { return game.inPlay(object); });

		next += period;
		std::this_thread::sleep_until(next);
	}
}
</xsl:text>
  </xsl:template>

  <xsl:template name="framerate">
    <xsl:choose>
      <xsl:when test="/game/window/framerate"><xsl:value-of select="normalize-space(/game/window/framerate)" /></xsl:when>
      <xsl:otherwise>60</xsl:otherwise>
    </xsl:choose>
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

# SFML 3 for the window: as installed (vcpkg install sfml), or else fetched
# and built here.
find_package(SFML 3 COMPONENTS Graphics QUIET)
if (NOT SFML_FOUND)
	message(STATUS "SFML 3 not found, using FetchContent to download and build it.")
	include(FetchContent)
	set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
	set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 3.1.0 EXCLUDE_FROM_ALL SYSTEM)
	FetchContent_MakeAvailable(SFML)
endif()

add_executable(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>
	source/main.cpp
	source/</xsl:text>
    <xsl:value-of select="concat($name, '.h&#10;&#9;source/', $name, '.cpp&#10;')" />
    <xsl:text>	# the engine's small functions it calls, as they are in xge/lib/
</xsl:text>
    <xsl:for-each select="$runtime">
      <xsl:value-of select="concat('&#9;engine/', substring-after(., '/'), '&#10;')" />
    </xsl:for-each>
    <xsl:value-of select="concat(')&#10;target_include_directories(', $name, ' PRIVATE engine)&#10;target_compile_features(', $name, ' PRIVATE cxx_std_20)&#10;target_link_libraries(', $name, ' PRIVATE SFML::Graphics)&#10;')" />
    <xsl:text>
# The warnings the engine is built with (xge_warnings in its
# scripts/cmake/platform.cmake), for its code and the game's alike.
target_compile_options(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE
	$&lt;$&lt;CXX_COMPILER_ID:MSVC&gt;:/W4
		/w14242 /w14254 /w14263 /w14265 /w14287 /w14296 /w14311 /w14545 /w14546 /w14547
		/w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
		/external:anglebrackets /external:W0 /analyze:external- /utf-8&gt;
	$&lt;$&lt;CXX_COMPILER_ID:GNU,Clang,AppleClang&gt;:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
		-Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wunused
		-Wnull-dereference -Wformat=2 -Wimplicit-fallthrough
		-Wold-style-cast -Wmisleading-indentation&gt;
	$&lt;$&lt;CXX_COMPILER_ID:GNU&gt;:-Wdouble-promotion -Wduplicated-cond -Wduplicated-branches
		-Wlogical-op -Wuseless-cast&gt;)

</xsl:text>
    <xsl:value-of select="concat('set_property(TARGET ', $name, ' PROPERTY VS_DEBUGGER_WORKING_DIRECTORY &quot;$&lt;TARGET_FILE_DIR:', $name, '&gt;&quot;)&#10;')" />
    <xsl:text>
# Visual Studio starts the game when you press F5, rather than ALL_BUILD.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>)

# On Windows the program needs SFML's DLLs beside it when they are shared.
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
    <xsl:value-of select="concat('# ', $title, '&#10;&#10;Generated from `', $source, '` by `xgecli --generate windows-cpp`: the game written&#10;')" />
    <xsl:text>as a person would write it, a program of its own on SFML 3. Its numbers are
constants, its objects variables and its rules functions; it calls the engine's
own small functions (the frame's order, bounce, deflect, stick, the numbers),
copied into `engine/`, so it plays as xgecli plays the file.

| File | What |
|---|---|
| `source/</xsl:text>
    <xsl:value-of select="concat($name, '.cpp` | the game: its numbers, its objects in order, what happens at an edge, on a touch and on a key |&#10;| `source/', $name, '.h` | its class |&#10;')" />
    <xsl:text>| `source/main.cpp` | the loop: keys, a frame, the picture, on SFML |
| `engine/` | the engine's small functions it calls, as they are in its `xge/lib/` |
| `LICENSE` | the engine's licence (MIT), which goes with its code |
| `CMakeLists.txt` | the build; it uses installed SFML 3, or downloads and builds it |
| `.gitignore` | leaves out `build/` and `output/` |

## Build and play

On Windows with Visual Studio and vcpkg (`vcpkg install sfml`), from this folder:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
output\Release\</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.exe
```

Without vcpkg, leave out the toolchain file: SFML is downloaded and built with the game.
</xsl:text>
  </xsl:template>
</xsl:stylesheet>
