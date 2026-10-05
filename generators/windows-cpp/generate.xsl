<?xml version="1.0" encoding="UTF-8"?>
<!-- generate.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- The windows-cpp target: a game file in, a C++ program that plays it on SFML 3
     out, with no part of the engine in it.

     Run by xgecli (generate.cpp) with libxslt, or by hand:

       xsltproc -o pong-windows-cpp/manifest.xml - -stringparam name pong
         - -stringparam source pong.xml generators/windows-cpp/generate.xsl games/pong.xml

     (with "- -" written as two dashes). It writes three files with EXSLT's
     exsl:document, beside the output named: main.cpp (the game), CMakeLists.txt
     (its build) and README.md. Its own output is a list of what it wrote and of
     the asset files the program needs, which xgecli copies, since a stylesheet
     cannot copy a file.

     A tag this target cannot generate yet stops it with a message saying which
     and where (game.xsl). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:include href="values.xsl" />
  <xsl:include href="game.xsl" />

  <xsl:output method="xml" indent="yes" encoding="UTF-8" />

  <!-- The program's name (pong) and the game file it was made from (pong.xml). -->
  <xsl:param name="name" select="'game'" />
  <xsl:param name="source" select="concat($name, '.xml')" />

  <xsl:variable name="runtime" select="document('runtime.xml')/runtime" />
  <xsl:variable name="game" select="/game" />

  <!-- The runtime's parts for a place, leaving out those for tags the game
       does not use. -->
  <xsl:template name="runtime">
    <xsl:param name="place" />
    <xsl:for-each select="$runtime/part[@place = $place]">
      <xsl:variable name="when" select="@when" />
      <xsl:if test="not($when) or $game//*[local-name() = $when]">
        <xsl:value-of select="." />
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="/">
    <xsl:if test="not(game)">
      <xsl:message terminate="yes">windows-cpp: this is not a game file (no &lt;game&gt;)</xsl:message>
    </xsl:if>

    <exsl:document href="main.cpp" method="text" encoding="UTF-8">
      <xsl:text>// main.cpp
// </xsl:text>
      <xsl:value-of select="concat(game/window/@name, ', generated from ', $source, ' by xgecli --generate windows-cpp.')" />
      <xsl:text>
// Generated: change the game file and generate it again rather than editing this.

</xsl:text>
      <xsl:call-template name="runtime"><xsl:with-param name="place" select="'top'" /></xsl:call-template>
      <xsl:call-template name="game" />
      <xsl:call-template name="runtime"><xsl:with-param name="place" select="'bottom'" /></xsl:call-template>
    </exsl:document>

    <exsl:document href="CMakeLists.txt" method="text" encoding="UTF-8">
      <xsl:call-template name="cmake" />
    </exsl:document>

    <exsl:document href="README.md" method="text" encoding="UTF-8">
      <xsl:call-template name="readme" />
    </exsl:document>

    <generated target="windows-cpp" name="{$name}">
      <file path="main.cpp" />
      <file path="CMakeLists.txt" />
      <file path="README.md" />
      <xsl:if test="game/objects/object/sprite/text">
        <asset path="assets/tuffy.ttf" />
      </xsl:if>
      <xsl:for-each select="game/objects/object/sprite/image/path[not(. = preceding::path)]">
        <asset path="{normalize-space(.)}" />
      </xsl:for-each>
    </generated>
  </xsl:template>

  <xsl:template name="cmake">
    <xsl:text># CMakeLists.txt
# </xsl:text>
    <xsl:value-of select="concat(game/window/@name, ', generated from ', $source, ' by xgecli --generate windows-cpp.')" />
    <xsl:text>

cmake_minimum_required(VERSION 3.28)

project(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> LANGUAGES CXX)

# SFML 3 as installed (vcpkg install sfml), or else fetched and built here.
find_package(SFML 3 COMPONENTS Graphics Audio QUIET)

if (NOT SFML_FOUND)
	message(STATUS "SFML 3 not found, using FetchContent to download and build it.")
	include(FetchContent)
	set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(SFML
		GIT_REPOSITORY https://github.com/SFML/SFML.git
		GIT_TAG 3.1.0
		EXCLUDE_FROM_ALL
		SYSTEM)
	FetchContent_MakeAvailable(SFML)
endif()

add_executable(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> main.cpp)
target_compile_features(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE cxx_std_20)
target_link_libraries(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE SFML::Graphics SFML::Audio)

# The program looks for assets/ beside itself, and on Windows needs SFML's DLLs
# there too when SFML is a shared library.
add_custom_command(TARGET </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$&lt;TARGET_FILE_DIR:</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>&gt;/assets"
	VERBATIM)

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

  <xsl:template name="readme">
    <xsl:text># </xsl:text>
    <xsl:value-of select="game/window/@name" />
    <xsl:text>

Generated from `</xsl:text>
    <xsl:value-of select="$source" />
    <xsl:text>` by `xgecli --generate windows-cpp`. It is the same game as the engine
plays, written out as one C++ program on SFML 3, with nothing of the engine in it.

| File | What |
|---|---|
| `main.cpp` | the game: the engine's parts the game uses, then the game's own objects, rules and states |
| `CMakeLists.txt` | the build; it uses an installed SFML 3, or downloads and builds one |
| `assets/` | the font and pictures the game uses, copied beside the program when it is built |

## Build and play

On Windows with Visual Studio and vcpkg (`vcpkg install sfml`), from this folder:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
build\Release\</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.exe
```

Without vcpkg, leave out the toolchain file: SFML is downloaded and built with the game.

Change the game in its XML file and generate it again, rather than editing `main.cpp`.
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
