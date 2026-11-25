<?xml version="1.0" encoding="UTF-8"?>
<!-- generate.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- The web-js target: a game file in, one web page out (<game>.html),
     written as a person would write that one game in plain JavaScript on a
     canvas. Its numbers are constants, its objects plain objects (where, how
     fast, how big), its rules plain code in one frame() and a few small
     functions. It does what the file says, in its own words: nothing of the
     engine goes with it, and nothing in it names the engine. It is
     windows-cpp's twin for the browser, and covers what windows-cpp covers;
     web-asm covers every game, as WebAssembly nobody reads.

     Run by xgecli (xge/cli/source/generate.cpp) with libxslt, after the engine
     has read and checked the game. It writes <game>.html (game.xsl) and
     README.md with EXSLT's exsl:document; its own output is the list of
     those and of what xgecli copies in. Before it writes, it walks the game
     (windows-cpp's covers.xsl, and each verb's file in verbs/): every part it
     has no words for yet is listed, and it stops. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:include href="../engine-cpp/text.xsl" />
  <xsl:include href="../windows-cpp/covers.xsl" />
  <xsl:include href="../windows-cpp/kinds.xsl" />
  <xsl:include href="names.xsl" />
  <xsl:include href="values.xsl" />
  <xsl:include href="game.xsl" />
  <xsl:include href="grids.xsl" />
  <xsl:include href="verbs/become.xsl" />
  <xsl:include href="verbs/bounce.xsl" />
  <xsl:include href="verbs/dec.xsl" />
  <xsl:include href="verbs/deflect.xsl" />
  <xsl:include href="verbs/die.xsl" />
  <xsl:include href="verbs/hold.xsl" />
  <xsl:include href="verbs/inc.xsl" />
  <xsl:include href="verbs/move.xsl" />
  <xsl:include href="verbs/people.xsl" />
  <xsl:include href="verbs/play.xsl" />
  <xsl:include href="verbs/pop.xsl" />
  <xsl:include href="verbs/push.xsl" />
  <xsl:include href="verbs/reset.xsl" />
  <xsl:include href="verbs/set.xsl" />
  <xsl:include href="verbs/stick.xsl" />
  <xsl:include href="verbs/track.xsl" />
  <xsl:include href="verbs/trigger.xsl" />
  <xsl:include href="verbs/wrap.xsl" />

  <xsl:output method="xml" indent="yes" encoding="UTF-8" />

  <!-- The page's name (pong_min), the game file it is made from
       (pong_min.xml) and the date for its header. -->
  <xsl:param name="name" select="'game'" />
  <xsl:param name="source" select="concat($name, '.xml')" />
  <xsl:param name="function" select="'game'" />
  <xsl:param name="date" select="''" />

  <xsl:template match="/">
    <xsl:if test="not(game)">
      <xsl:message terminate="yes">web-js: this is not a game file (no &lt;game&gt;)</xsl:message>
    </xsl:if>
    <!-- every part it has no words for yet, once each: then it stops -->
    <xsl:variable name="walked">
      <xsl:apply-templates select="game" mode="covers" />
    </xsl:variable>
    <xsl:variable name="gaps" select="exsl:node-set($walked)/gap" />
    <xsl:if test="$gaps">
      <xsl:message terminate="yes">
        <xsl:value-of select="concat('web-js cannot write these parts of ', $source, ' yet; use web-asm for this game:')" />
        <xsl:for-each select="$gaps[not(. = preceding-sibling::gap)]">
          <xsl:value-of select="concat('&#10;  ', .)" />
        </xsl:for-each>
      </xsl:message>
    </xsl:if>

    <exsl:document href="{$name}.html" method="text" encoding="UTF-8">
      <xsl:for-each select="game"><xsl:call-template name="page-file" /></xsl:for-each>
    </exsl:document>
    <exsl:document href="README.md" method="text" encoding="UTF-8">
      <xsl:call-template name="readme-file" />
    </exsl:document>

    <generated target="web-js" name="{$name}">
      <file path="{$name}.html" />
      <file path="README.md" />
      <license path="LICENSE" from="LICENSE" />
    </generated>
  </xsl:template>

  <xsl:template name="readme-file">
    <xsl:value-of select="concat('# ', $title, '&#10;&#10;', $title, ' from `', $source, '`, written as one web page of its own in plain JavaScript.&#10;')" />
    <xsl:text>
| File | What |
|---|---|
| `</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.html` | the game: its numbers, its things, its rules, and the canvas they are drawn on |
| `LICENSE` | the licence (MIT) |

## Play

Open `</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.html` in a browser. It needs nothing else: no build, no server.
</xsl:text>
  </xsl:template>
</xsl:stylesheet>
