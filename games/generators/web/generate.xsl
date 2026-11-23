<?xml version="1.0" encoding="UTF-8"?>
<!-- generate.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 23, 2026 -->

<!-- The web target: a game file in, one web page out (output/<game>.html)
     that plays the game in a browser opened straight from the disk, with
     nothing else beside it. It is engine-cpp built with Emscripten: the game
     written out as a tree of C++ data (engine-cpp's tree.xsl), played by the
     engine's own code, copied in unchanged, with a window that is the page's
     canvas (window/web) and sound that is its Web Audio (audio/web). The
     engine, the game and its assets go into the page as WebAssembly.

     Run by xgecli (xge/cli/source/generate.cpp) with libxslt, or by hand as
     engine-cpp's generate.xsl is. It writes source/<game>.cpp,
     source/main.cpp, source/page.html, CMakeLists.txt, README.md and
     .gitignore; its own output lists those and what xgecli copies in. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:include href="../engine-cpp/text.xsl" />
  <xsl:include href="../engine-cpp/tree.xsl" />
  <xsl:include href="program.xsl" />

  <xsl:output method="xml" indent="yes" encoding="UTF-8" />

  <xsl:param name="name" select="'game'" />
  <xsl:param name="source" select="concat($name, '.xml')" />
  <xsl:param name="function" select="'game'" />
  <xsl:param name="date" select="''" />

  <xsl:variable name="title">
    <xsl:choose>
      <xsl:when test="/game/window/@name != ''"><xsl:value-of select="/game/window/@name" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- engine-cpp's list of the engine's files, with the page's window and
       sound in place of SFML's. -->
  <xsl:variable name="engine" select="document('../engine-cpp/engine.xml')/engine/file[not(contains(., '/sfml.'))] | document('engine.xml')/engine/file" />
  <xsl:variable name="pictures" select="//sprite/image/path | //sprite/svg/path | //pictures/sheet/path" />
  <xsl:variable name="texts" select="//sprite/text" />
  <xsl:variable name="fonts" select="//text/font | //styles/style/font" />
  <xsl:variable name="recordings" select="//sounds/sound/path" />

  <xsl:template match="/">
    <xsl:if test="not(game)">
      <xsl:message terminate="yes">web: this is not a game file (no &lt;game&gt;)</xsl:message>
    </xsl:if>

    <exsl:document href="source/{$name}.cpp" method="text" encoding="UTF-8">
      <xsl:call-template name="tree-file" />
    </exsl:document>

    <exsl:document href="source/main.cpp" method="text" encoding="UTF-8">
      <xsl:call-template name="main-file" />
    </exsl:document>

    <exsl:document href="source/page.html" method="text" encoding="UTF-8">
      <xsl:call-template name="page-file" />
    </exsl:document>

    <exsl:document href="CMakeLists.txt" method="text" encoding="UTF-8">
      <xsl:call-template name="cmake-file" />
    </exsl:document>

    <exsl:document href="README.md" method="text" encoding="UTF-8">
      <xsl:call-template name="readme-file" />
    </exsl:document>

    <exsl:document href=".gitignore" method="text" encoding="UTF-8">
      <xsl:text># where the game is built, and the built page
build/
output/
</xsl:text>
    </exsl:document>

    <generated target="web" name="{$name}">
      <file path="source/{$name}.cpp" />
      <file path="source/main.cpp" />
      <file path="source/page.html" />
      <file path="CMakeLists.txt" />
      <file path="README.md" />
      <file path=".gitignore" />
      <xsl:for-each select="$engine">
        <engine path="engine/{substring-after(., '/')}" from="{.}" />
      </xsl:for-each>
      <license path="LICENSE" from="LICENSE" />
      <xsl:if test="$texts">
        <asset path="assets/tuffy.ttf" />
      </xsl:if>
      <xsl:for-each select="$fonts | $pictures | $recordings">
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

  <xsl:template name="header">
    <xsl:param name="file" />
    <xsl:value-of select="concat('// ', $file, '&#10;// XML Game Engine&#10;// author: beefviper&#10;// date: ', $date, '&#10;')" />
  </xsl:template>
</xsl:stylesheet>
