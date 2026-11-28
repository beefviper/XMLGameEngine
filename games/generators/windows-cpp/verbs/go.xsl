<?xml version="1.0" encoding="UTF-8"?>
<!-- go.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 28, 2026 -->

<!-- <go direction> in an action of a piece of a map: off that way, so many
     cells a second, until told again: go({ 0, -1 }, status.speed) (cells.xsl). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: up, down, left or right, in an action of a piece of a map -->
  <xsl:template match="map/object/actions/action/go[@direction = 'up' or @direction = 'down' or @direction = 'left' or @direction = 'right'][not(*)]" mode="covers">
    <xsl:apply-templates select="text()" mode="covers" />
  </xsl:template>
  <xsl:template match="go/@direction" mode="covers" />

  <xsl:template match="go" mode="do">
    <xsl:param name="self" select="/.." />
    <xsl:call-template name="go-name"><xsl:with-param name="goer" select="$self" /></xsl:call-template>
    <xsl:choose>
      <xsl:when test="@direction = 'up'">({ 0, -1 }, </xsl:when>
      <xsl:when test="@direction = 'down'">({ 0, 1 }, </xsl:when>
      <xsl:when test="@direction = 'left'">({ -1, 0 }, </xsl:when>
      <xsl:otherwise>({ 1, 0 }, </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$self" /></xsl:call-template>
    <xsl:text>);</xsl:text>
  </xsl:template>
</xsl:stylesheet>
