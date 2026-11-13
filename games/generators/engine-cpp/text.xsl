<?xml version="1.0" encoding="UTF-8"?>
<!-- text.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 11, 2026 -->

<!-- Text as C++ wants it: a game's text trimmed as load/xml.cpp trims it,
     written as a string literal, and a name made into a C++ name. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="spaces" select="'&#9;&#10;&#13; '" />
  <xsl:variable name="lower" select="'abcdefghijklmnopqrstuvwxyz'" />
  <xsl:variable name="upper" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />
  <xsl:variable name="digits" select="'0123456789'" />

  <!-- $text without the spaces, tabs and line ends at either end (readText). -->
  <xsl:template name="trim">
    <xsl:param name="text" />
    <xsl:choose>
      <xsl:when test="$text != '' and contains($spaces, substring($text, 1, 1))">
        <xsl:call-template name="trim">
          <xsl:with-param name="text" select="substring($text, 2)" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$text != '' and contains($spaces, substring($text, string-length($text)))">
        <xsl:call-template name="trim">
          <xsl:with-param name="text" select="substring($text, 1, string-length($text) - 1)" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- $text as a C++ string literal. -->
  <xsl:template name="string">
    <xsl:param name="text" />
    <xsl:text>"</xsl:text>
    <xsl:call-template name="escape">
      <xsl:with-param name="text" select="$text" />
    </xsl:call-template>
    <xsl:text>"</xsl:text>
  </xsl:template>

  <!-- The text of $node, trimmed, as a C++ string literal. -->
  <xsl:template name="text">
    <xsl:param name="node" select="." />
    <xsl:call-template name="string">
      <xsl:with-param name="text">
        <xsl:call-template name="trim">
          <xsl:with-param name="text" select="string($node)" />
        </xsl:call-template>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="escape">
    <xsl:param name="text" />
    <xsl:choose>
      <xsl:when test="translate($text, concat('\&quot;', $spaces), '') = translate($text, ' ', '')">
        <xsl:value-of select="$text" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="first" select="substring($text, 1, 1)" />
        <xsl:choose>
          <xsl:when test="$first = '\'">\\</xsl:when>
          <xsl:when test="$first = '&quot;'">\"</xsl:when>
          <xsl:when test="$first = '&#9;'">\t</xsl:when>
          <xsl:when test="$first = '&#10;'">\n</xsl:when>
          <xsl:when test="$first = '&#13;'">\r</xsl:when>
          <xsl:otherwise><xsl:value-of select="$first" /></xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="escape">
          <xsl:with-param name="text" select="substring($text, 2)" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A name in the game as a C++ name: its words (split at anything not a
       letter or a digit) run together in camel case, so pong_min is pongMin,
       and never starting with a digit. -->
  <xsl:template name="camel">
    <xsl:param name="name" />
    <xsl:variable name="others" select="translate($name, concat($lower, $upper, $digits), '')" />
    <xsl:variable name="words" select="normalize-space(translate($name, $others, '                                                                                                    '))" />
    <xsl:variable name="joined">
      <xsl:call-template name="camel-words">
        <xsl:with-param name="words" select="$words" />
        <xsl:with-param name="first" select="true()" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$joined = '' or contains($digits, substring($joined, 1, 1))">n</xsl:if>
    <xsl:value-of select="$joined" />
  </xsl:template>

  <xsl:template name="camel-words">
    <xsl:param name="words" />
    <xsl:param name="first" />
    <xsl:if test="$words != ''">
      <xsl:variable name="word">
        <xsl:choose>
          <xsl:when test="contains($words, ' ')"><xsl:value-of select="substring-before($words, ' ')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$words" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="$first"><xsl:value-of select="$word" /></xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat(translate(substring($word, 1, 1), $lower, $upper), substring($word, 2))" />
        </xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="camel-words">
        <xsl:with-param name="words" select="substring-after($words, ' ')" />
        <xsl:with-param name="first" select="false()" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- The C++ function a named thing of the game is written as: its name in
       camel case (after the words of $prefix, its owner's, when it has one)
       and $suffix, and if one of the $earlier ones already has that, its
       place among them too. -->
  <xsl:template name="function-name">
    <xsl:param name="node" select="." />
    <xsl:param name="earlier" />
    <xsl:param name="suffix" />
    <xsl:param name="prefix" select="''" />
    <xsl:variable name="camel">
      <xsl:call-template name="camel">
        <xsl:with-param name="name" select="concat($prefix, ' ', $node/@name)" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="clashes">
      <xsl:for-each select="$earlier">
        <xsl:variable name="other">
          <xsl:call-template name="camel">
            <xsl:with-param name="name" select="concat($prefix, ' ', @name)" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$other = $camel">x</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:value-of select="$camel" />
    <xsl:if test="$clashes != ''">
      <xsl:value-of select="count($earlier) + 1" />
    </xsl:if>
    <xsl:value-of select="$suffix" />
  </xsl:template>
</xsl:stylesheet>
