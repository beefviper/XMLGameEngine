<?xml version="1.0" encoding="UTF-8"?>
<!-- generate.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- The windows-cpp target: a game file in, a C++ program that plays it on SFML 3
     out, written as a person would write it, with nothing of the engine in it.

     Run by xgecli (generate.cpp) with libxslt, or by hand:

       xsltproc -o pong_min-windows-cpp/manifest.xml - -stringparam name pong_min
         - -stringparam source pong_min.xml generators/windows-cpp/generate.xsl games/pong_min.xml

     (with "- -" written as two dashes). It writes three files with EXSLT's
     exsl:document, beside the output named: main.cpp (the game, main.xsl),
     CMakeLists.txt (its build) and README.md. Its own output is a list of what it
     wrote and of what xgecli copies beside them: the modules the game uses
     (modules/: physics.h, pictures.h, and sound.h with sound.cpp), the game's
     assets (its font and pictures), and each <svg> for xgecli to draw into a
     picture as the engine draws it. xgecli reads the list.

     It covers part of the language so far, all of Pong (check.xsl); anything
     else stops it with a message saying what and where. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:include href="values.xsl" />
  <xsl:include href="check.xsl" />
  <xsl:include href="main.xsl" />

  <xsl:output method="xml" indent="yes" encoding="UTF-8" />

  <!-- The program's name (pong_min) and the game file it was made from (pong_min.xml). -->
  <xsl:param name="name" select="'game'" />
  <xsl:param name="source" select="concat($name, '.xml')" />

  <xsl:template match="/">
    <xsl:if test="not(game)">
      <xsl:message terminate="yes">windows-cpp: this is not a game file (no &lt;game&gt;)</xsl:message>
    </xsl:if>

    <xsl:apply-templates select="game" mode="check" />

    <exsl:document href="main.cpp" method="text" encoding="UTF-8">
      <xsl:call-template name="generate-file" />
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
      <xsl:if test="$physics">
        <module path="physics.h" />
      </xsl:if>
      <xsl:if test="$drawn-rows or $drawn-lines">
        <module path="pictures.h" />
      </xsl:if>
      <xsl:if test="$audio">
        <module path="sound.h" />
        <module path="sound.cpp" />
      </xsl:if>
      <xsl:if test="$texts">
        <asset path="assets/tuffy.ttf" />
      </xsl:if>
      <xsl:for-each select="$image-paths">
        <asset path="{normalize-space(.)}" />
      </xsl:for-each>
      <!-- an <svg>, drawn by xgecli as the engine draws it, into a picture -->
      <xsl:for-each select="$drawn-svgs">
        <xsl:variable name="svg" select="svg" />
        <xsl:variable name="file"><xsl:call-template name="drawn-file" /></xsl:variable>
        <picture path="{$file}" svg="{normalize-space($svg/path)}">
          <xsl:if test="$svg/width">
            <xsl:attribute name="x"><xsl:call-template name="svg-number"><xsl:with-param name="node" select="$svg/x" /></xsl:call-template></xsl:attribute>
            <xsl:attribute name="y"><xsl:call-template name="svg-number"><xsl:with-param name="node" select="$svg/y" /></xsl:call-template></xsl:attribute>
            <xsl:attribute name="width"><xsl:call-template name="svg-number"><xsl:with-param name="node" select="$svg/width" /></xsl:call-template></xsl:attribute>
            <xsl:attribute name="height"><xsl:call-template name="svg-number"><xsl:with-param name="node" select="$svg/height" /></xsl:call-template></xsl:attribute>
          </xsl:if>
          <xsl:if test="$svg/scale">
            <xsl:attribute name="scale"><xsl:call-template name="svg-number"><xsl:with-param name="node" select="$svg/scale" /></xsl:call-template></xsl:attribute>
          </xsl:if>
          <xsl:for-each select="$svg/hide">
            <hide id="{normalize-space(.)}" />
          </xsl:for-each>
        </picture>
      </xsl:for-each>
    </generated>
  </xsl:template>

  <!-- A number an <svg> takes, as it is or as the game variable it names. -->
  <xsl:template name="svg-number">
    <xsl:param name="node" />
    <xsl:variable name="text" select="normalize-space($node)" />
    <xsl:choose>
      <xsl:when test="string(number($text)) != 'NaN'"><xsl:value-of select="$text" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="normalize-space(/game/variables/variable[@name = $text][last()])" /></xsl:otherwise>
    </xsl:choose>
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
find_package(SFML 3 COMPONENTS Graphics</xsl:text>
    <xsl:if test="$audio"> Audio</xsl:if>
    <xsl:text> QUIET)

if (NOT SFML_FOUND)
	message(STATUS "SFML 3 not found, using FetchContent to download and build it.")
	include(FetchContent)
	set(SFML_BUILD_AUDIO </xsl:text>
    <xsl:choose>
      <xsl:when test="$audio">ON</xsl:when>
      <xsl:otherwise>OFF</xsl:otherwise>
    </xsl:choose>
    <xsl:text> CACHE BOOL "" FORCE)
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
    <xsl:text> main.cpp</xsl:text>
    <xsl:if test="$physics"> physics.h</xsl:if>
    <xsl:if test="$audio"> sound.h sound.cpp</xsl:if>
    <xsl:text>)
target_compile_features(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE cxx_std_20)
target_link_libraries(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE SFML::Graphics</xsl:text>
    <xsl:if test="$audio"> SFML::Audio</xsl:if>
    <xsl:text>)
</xsl:text>
    <xsl:if test="$texts or $images or $drawn-svgs">
      <xsl:text>
# The game opens its font and pictures from assets/ beside where it runs:
# copied beside the program, and the folder Visual Studio starts it in.
add_custom_command(TARGET </xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text> POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$&lt;TARGET_FILE_DIR:</xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text>&gt;/assets")
set_property(TARGET </xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text> PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$&lt;TARGET_FILE_DIR:</xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text>&gt;")
</xsl:text>
    </xsl:if>
    <xsl:text>
# Visual Studio starts the game when you press F5, rather than ALL_BUILD.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>)

# On Windows the program needs SFML's DLLs beside it when SFML is a shared library.
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
plays, written out as a C++ program on SFML 3, with nothing of the engine in it.

| File | What |
|---|---|
| `main.cpp` | the game: its window, tunables, screens, objects and sounds, then `main` and the game loop, then a function per screen and per object |
</xsl:text>
    <xsl:if test="$physics">
      <xsl:text>| `physics.h` | where things are, whether they touch, and bouncing, sticking and deflecting; it works with any SFML shape, sprite or text |
</xsl:text>
    </xsl:if>
    <xsl:if test="$drawn-rows or $drawn-lines">
      <xsl:text>| `pictures.h` | the pictures it draws itself when it starts: rows of text, and straight lines |
</xsl:text>
    </xsl:if>
    <xsl:if test="$audio">
      <xsl:text>| `sound.h`, `sound.cpp` | 8-bit sounds written as notes, made into samples when the game starts |
</xsl:text>
    </xsl:if>
    <xsl:if test="$texts or $images or $drawn-svgs">
      <xsl:text>| `assets/` | the font and pictures it opens</xsl:text>
      <xsl:if test="$drawn-svgs"> (`assets/drawn/` holds its SVG drawings, drawn as the engine draws them)</xsl:if>
      <xsl:text>, copied beside the program when it is built |
</xsl:text>
    </xsl:if>
    <xsl:text>| `CMakeLists.txt` | the build; it uses an installed SFML 3, or downloads and builds one |

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
