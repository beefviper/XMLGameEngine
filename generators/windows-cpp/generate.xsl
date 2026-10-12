<?xml version="1.0" encoding="UTF-8"?>
<!-- generate.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- The windows-cpp target: a game file in, a program of its own out, that
     plays the game as xgecli and xgegui play it, on SFML 3. The game is
     written out as the engine's own data (data.xsl), and everything that
     plays it is the engine's own code, copied in unchanged.

     Run by xgecli (cli/source/generate.cpp) with libxslt, after
     the engine has read and checked the game, or by hand:

       xsltproc -o pong_min-windows-cpp/manifest.xml
         - -stringparam name pong_min - -stringparam function pongMinGame
         generators/windows-cpp/generate.xsl games/pong_min.xml

     (with "- -" written as two dashes). It writes the program's own files
     with EXSLT's exsl:document, relative to the output named: source/<game>.cpp
     (the game, data.xsl), source/main.cpp, CMakeLists.txt, README.md and
     .gitignore (program.xsl). Its own output is a list of those and of what
     xgecli copies in: game_parts.h (modules/, to source/), the engine's files
     (engine.xml; from lib/ to engine/) and the game's assets (its font and
     pictures). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:include href="text.xsl" />
  <xsl:include href="data.xsl" />
  <xsl:include href="program.xsl" />

  <xsl:output method="xml" indent="yes" encoding="UTF-8" />

  <!-- The program's name (pong_min), the game file it is made from
       (pong_min.xml), the C++ function that returns the game (pongMinGame),
       and the date for the files' headers. -->
  <xsl:param name="name" select="'game'" />
  <xsl:param name="source" select="concat($name, '.xml')" />
  <xsl:param name="function" select="'game'" />
  <xsl:param name="date" select="''" />

  <!-- What the game is called: its window's name, or else the program's. -->
  <xsl:variable name="title">
    <xsl:choose>
      <xsl:when test="/game/window/@name != ''"><xsl:value-of select="/game/window/@name" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- The engine's files the program compiles, and the game's assets: the
       font if it shows text, and every picture and drawing it names. -->
  <xsl:variable name="engine" select="document('engine.xml')/engine/file" />
  <xsl:variable name="pictures" select="//sprite/image/path | //sprite/svg/path" />
  <xsl:variable name="texts" select="//sprite/text" />

  <xsl:template match="/">
    <xsl:if test="not(game)">
      <xsl:message terminate="yes">windows-cpp: this is not a game file (no &lt;game&gt;)</xsl:message>
    </xsl:if>

    <exsl:document href="source/{$name}.cpp" method="text" encoding="UTF-8">
      <xsl:call-template name="data-file" />
    </exsl:document>

    <exsl:document href="source/main.cpp" method="text" encoding="UTF-8">
      <xsl:call-template name="main-file" />
    </exsl:document>

    <exsl:document href="CMakeLists.txt" method="text" encoding="UTF-8">
      <xsl:call-template name="cmake-file" />
    </exsl:document>

    <exsl:document href="README.md" method="text" encoding="UTF-8">
      <xsl:call-template name="readme-file" />
    </exsl:document>

    <exsl:document href=".gitignore" method="text" encoding="UTF-8">
      <xsl:text># where the game is built, and the built game
build/
output/
</xsl:text>
    </exsl:document>

    <generated target="windows-cpp" name="{$name}">
      <file path="source/{$name}.cpp" />
      <file path="source/main.cpp" />
      <file path="CMakeLists.txt" />
      <file path="README.md" />
      <file path=".gitignore" />
      <module path="source/game_parts.h" from="game_parts.h" />
      <xsl:for-each select="$engine">
        <engine path="engine/{substring-after(substring-after(., '/'), '/')}" from="{.}" />
      </xsl:for-each>
      <xsl:if test="$texts">
        <asset path="assets/tuffy.ttf" />
      </xsl:if>
      <xsl:for-each select="$pictures">
        <asset>
          <xsl:attribute name="path">
            <xsl:call-template name="trim">
              <xsl:with-param name="text" select="string(.)" />
            </xsl:call-template>
          </xsl:attribute>
        </asset>
      </xsl:for-each>
    </generated>
  </xsl:template>

  <!-- The block every C++ file of the project starts with. -->
  <xsl:template name="header">
    <xsl:param name="file" />
    <xsl:value-of select="concat('// ', $file, '&#10;// XML Game Engine&#10;// author: beefviper&#10;// date: ', $date, '&#10;')" />
  </xsl:template>
</xsl:stylesheet>
