<?xml version="1.0" encoding="UTF-8"?>
<!-- values.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- A value of the game as a C++ expression, worked out in the order the
     engine works it out (load/expr): an operation's values one after the
     other from the first, so a * b * c is (a * b) * c in both. A name is the
     constant or local it is written as; a number with a point is a float. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- $node holds the value: its text, or one operation, <random> or
       <centered />. $self is the object the value is of (for its own
       variables and size); $axis is x or y in a <position>; $nested, that
       it is inside another operation and wants brackets. -->
  <xsl:template name="value">
    <xsl:param name="node" />
    <xsl:param name="self" />
    <xsl:param name="axis" select="''" />
    <xsl:param name="nested" select="false()" />
    <xsl:variable name="tag" select="$node/*[1]" />
    <xsl:choose>
      <xsl:when test="not($tag)">
        <xsl:call-template name="name-or-number">
          <xsl:with-param name="text" select="normalize-space($node)" />
          <xsl:with-param name="self" select="$self" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="local-name($tag) = 'random'">
        <xsl:text>randomBetween(</xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/min" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/max" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:when test="local-name($tag) = 'centered' and not($tag/node())">
        <!-- its middle on the window's: the window's middle less half its size -->
        <xsl:variable name="window">
          <xsl:choose>
            <xsl:when test="$axis = 'x'">windowWidth</xsl:when>
            <xsl:otherwise>windowHeight</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="size">
          <xsl:call-template name="own-size">
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="axis" select="$axis" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$nested">(</xsl:if>
        <xsl:value-of select="concat($window, ' / 2 - ', $size, ' / 2')" />
        <xsl:if test="$nested">)</xsl:if>
      </xsl:when>
      <xsl:when test="local-name($tag) = 'sign' or local-name($tag) = 'sine' or local-name($tag) = 'cosine'">
        <xsl:value-of select="concat(local-name($tag), '(')" />
        <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/value" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:when test="local-name($tag) = 'add' or local-name($tag) = 'subtract' or local-name($tag) = 'multiply' or local-name($tag) = 'divide'">
        <xsl:variable name="sign">
          <xsl:choose>
            <xsl:when test="local-name($tag) = 'add'"> + </xsl:when>
            <xsl:when test="local-name($tag) = 'subtract'"> - </xsl:when>
            <xsl:when test="local-name($tag) = 'multiply'"> * </xsl:when>
            <xsl:otherwise> / </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:if test="$nested">(</xsl:if>
        <xsl:for-each select="$tag/value">
          <xsl:if test="position() &gt; 1"><xsl:value-of select="$sign" /></xsl:if>
          <xsl:call-template name="value">
            <xsl:with-param name="node" select="." />
            <xsl:with-param name="self" select="$self" />
            <!-- the first is worked out first anyway; the rest keep their own order -->
            <xsl:with-param name="nested" select="position() &gt; 1 or $sign = ' * ' or $sign = ' / '" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:if test="$nested">)</xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">windows-cpp cannot write a &lt;<xsl:value-of select="local-name($tag)" />&gt; value yet; use engine-cpp for this game</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A number (a float when it has a point: 0.5f), a variable of the game
       (its constant), of the window, or of the object itself (its local). -->
  <xsl:template name="name-or-number">
    <xsl:param name="text" />
    <xsl:param name="self" />
    <xsl:variable name="own" select="substring-after($text, '.')" />
    <xsl:choose>
      <xsl:when test="string(number($text)) != 'NaN'">
        <xsl:value-of select="$text" />
        <xsl:if test="contains($text, '.')">f</xsl:if>
      </xsl:when>
      <xsl:when test="$text = 'window.width' or $text = 'window.right'">windowWidth</xsl:when>
      <xsl:when test="$text = 'window.height' or $text = 'window.bottom'">windowHeight</xsl:when>
      <xsl:when test="$text = 'window.left' or $text = 'window.top'">0</xsl:when>
      <xsl:when test="/game/variables/variable[@name = $text]">
        <xsl:call-template name="camel"><xsl:with-param name="name" select="$text" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$self and (starts-with($text, concat($self/@name, '.')) or starts-with($text, 'self.')) and $self/variables/variable[@name = $own]">
        <xsl:call-template name="camel"><xsl:with-param name="name" select="$own" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">windows-cpp cannot write the name '<xsl:value-of select="$text" />' yet; use engine-cpp for this game</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The object's own width (x) or height (y): its rectangle's. -->
  <xsl:template name="own-size">
    <xsl:param name="self" />
    <xsl:param name="axis" />
    <xsl:variable name="sprite" select="($self/sprite | $self/parent::group/sprite)[1]" />
    <xsl:choose>
      <xsl:when test="$sprite/rectangle and $axis = 'x'">
        <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/width" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="nested" select="true()" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$sprite/rectangle">
        <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/height" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="nested" select="true()" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">windows-cpp can centre only a rectangle yet; use engine-cpp for this game</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
