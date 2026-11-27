<?xml version="1.0" encoding="UTF-8"?>
<!-- move.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <move direction>step</move> in an action: while its key is held, that many pixels a second that way; in a rule on meeting, that many pixels that way, once. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: in an action of something with no <velocity> of its own,
       which the keys set every frame -->
  <xsl:template match="action/move" mode="covers">
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="move/@direction" mode="covers" />
  <!-- covered: up, down, left or right in a group's rule on meeting -->
  <xsl:template match="group[member]/collisions/collision[not(@edge)]/move[@direction = 'up' or @direction = 'down' or @direction = 'left' or @direction = 'right'][not(*)]" mode="covers">
    <xsl:apply-templates select="text()" mode="covers" />
  </xsl:template>
  <xsl:template match="action/move[ancestor::object[1]/velocity | ancestor::group[1]/member/velocity]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;move&gt; for something with a &lt;velocity&gt;'" /></xsl:call-template>
  </xsl:template>

  <xsl:template match="move" mode="touch">
    <xsl:param name="who" />
    <xsl:param name="self" />
    <xsl:variable name="along">
      <xsl:choose>
        <xsl:when test="@direction = 'up'">.position.y -= </xsl:when>
        <xsl:when test="@direction = 'down'">.position.y += </xsl:when>
        <xsl:when test="@direction = 'left'">.position.x -= </xsl:when>
        <xsl:otherwise>.position.x += </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:value-of select="concat($who, $along)" />
    <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$self" /></xsl:call-template>
    <xsl:text>;</xsl:text>
  </xsl:template>

  <xsl:template match="move" mode="held">
    <xsl:param name="who" />
    <xsl:variable name="along">
      <xsl:choose>
        <xsl:when test="@direction = 'up'">.velocity.y -= </xsl:when>
        <xsl:when test="@direction = 'down'">.velocity.y += </xsl:when>
        <xsl:when test="@direction = 'left'">.velocity.x -= </xsl:when>
        <xsl:otherwise>.velocity.x += </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:value-of select="concat($who, $along)" />
    <xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template>
    <xsl:text>;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
