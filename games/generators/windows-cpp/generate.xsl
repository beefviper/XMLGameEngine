<?xml version="1.0" encoding="UTF-8"?>
<!-- generate.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- The windows-cpp target: a game file in, a program of its own out, written
     as a person would write that one game over a weekend, on SFML 3 and
     nothing else. Its numbers are constants, its objects Things (where, how
     fast, how big), its rules plain code in one frame() and a few small
     functions. It does what the file says, in its own words: nothing of the
     engine goes with it, and nothing in it names the engine.

     Run by xgecli (xge/cli/source/generate.cpp) with libxslt, after the engine
     has read and checked the game. It writes source/<game>.h and .cpp (the
     game, game.xsl), source/main.cpp, CMakeLists.txt, README.md and .gitignore
     (program.xsl) with EXSLT's exsl:document; its own output is the list of
     those and of what xgecli copies in. Before it writes, it walks the game
     (covers.xsl, and each verb's file in verbs/): every part it has no
     words for yet is listed, and it stops. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:include href="../engine-cpp/text.xsl" />
  <xsl:include href="names.xsl" />
  <xsl:include href="values.xsl" />
  <xsl:include href="covers.xsl" />
  <xsl:include href="game.xsl" />
  <xsl:include href="program.xsl" />
  <xsl:include href="verbs/bounce.xsl" />
  <xsl:include href="verbs/deflect.xsl" />
  <xsl:include href="verbs/move.xsl" />
  <xsl:include href="verbs/reset.xsl" />
  <xsl:include href="verbs/stick.xsl" />
  <xsl:include href="verbs/trigger.xsl" />

  <xsl:output method="xml" indent="yes" encoding="UTF-8" />

  <!-- The program's name (pong_min), the game file it is made from
       (pong_min.xml) and the date for the files' headers. -->
  <xsl:param name="name" select="'game'" />
  <xsl:param name="source" select="concat($name, '.xml')" />
  <xsl:param name="function" select="'game'" />
  <xsl:param name="date" select="''" />

  <xsl:template match="/">
    <xsl:if test="not(game)">
      <xsl:message terminate="yes">windows-cpp: this is not a game file (no &lt;game&gt;)</xsl:message>
    </xsl:if>
    <!-- every part it has no words for yet, once each: then it stops -->
    <xsl:variable name="walked">
      <xsl:apply-templates select="game" mode="covers" />
    </xsl:variable>
    <xsl:variable name="gaps" select="exsl:node-set($walked)/gap" />
    <xsl:if test="$gaps">
      <xsl:message terminate="yes">
        <xsl:value-of select="concat('windows-cpp cannot write these parts of ', $source, ' yet; use engine-cpp for this game:')" />
        <xsl:for-each select="$gaps[not(. = preceding-sibling::gap)]">
          <xsl:value-of select="concat('&#10;  ', .)" />
        </xsl:for-each>
      </xsl:message>
    </xsl:if>

    <exsl:document href="source/{$name}.h" method="text" encoding="UTF-8">
      <xsl:for-each select="game"><xsl:call-template name="header-file" /></xsl:for-each>
    </exsl:document>
    <exsl:document href="source/{$name}.cpp" method="text" encoding="UTF-8">
      <xsl:for-each select="game"><xsl:call-template name="game-file" /></xsl:for-each>
    </exsl:document>
    <exsl:document href="source/main.cpp" method="text" encoding="UTF-8">
      <xsl:for-each select="game"><xsl:call-template name="main-file" /></xsl:for-each>
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
      <file path="source/{$name}.h" />
      <file path="source/{$name}.cpp" />
      <file path="source/main.cpp" />
      <file path="CMakeLists.txt" />
      <file path="README.md" />
      <file path=".gitignore" />
      <license path="LICENSE" from="LICENSE" />
    </generated>
  </xsl:template>

  <!-- The block every C++ file of the program starts with: the game's own
       name where the engine's files say the engine's. -->
  <xsl:template name="header">
    <xsl:param name="file" />
    <xsl:value-of select="concat('// ', $file, '&#10;// ', $title, '&#10;// author: beefviper&#10;// date: ', $date, '&#10;')" />
  </xsl:template>
</xsl:stylesheet>
