<?xml version="1.0" encoding="UTF-8"?>
<!-- set.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <set variable>value</set> gives a variable, or a thing's x, y or velocity, that value: ball.velocity.x *= 1.3f when it multiplies itself, pace.beat /= 1.25f when it divides. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere, of a variable it can name; with group=, the way
       across or down of every cell of a grid, by itself times a number -->
  <xsl:template match="set[@variable]" mode="covers">
    <xsl:if test="@group and (not($grids[@name = current()/@group]) or not(@variable = 'velocity.x' or @variable = 'velocity.y')
        or not(multiply[count(value) = 2]/value[1][not(*)][normalize-space(.) = concat(current()/@group, '.1.1.', current()/@variable)])
        or multiply/value[2][* or string(number(.)) = 'NaN'])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;set group&gt; other than the way of every cell of a grid by itself times a number'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="* | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="set/@variable" mode="covers" />

  <xsl:template match="set[@group]" mode="do">
    <xsl:variable name="grid" select="$grids[@name = current()/@group]" />
    <xsl:variable name="cell"><xsl:call-template name="cell-name"><xsl:with-param name="grid" select="$grid" /></xsl:call-template></xsl:variable>
    <xsl:text>for (</xsl:text>
    <xsl:call-template name="cell-type"><xsl:with-param name="grid" select="$grid" /></xsl:call-template>
    <xsl:value-of select="concat('&amp; ', $cell, ' : ')" />
    <xsl:call-template name="object-name"><xsl:with-param name="object" select="$grid" /></xsl:call-template>
    <xsl:value-of select="concat(') { ', $cell, '.', @variable, ' *= ')" />
    <xsl:call-template name="name-or-number"><xsl:with-param name="text" select="normalize-space(multiply/value[2])" /></xsl:call-template>
    <xsl:text>; }</xsl:text>
  </xsl:template>

  <xsl:template match="set" mode="do">
    <xsl:param name="self" select="/.." />
    <xsl:variable name="path" select="@variable" />
    <xsl:call-template name="variable-target"><xsl:with-param name="path" select="$path" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
    <xsl:choose>
      <!-- itself times something: *= -->
      <xsl:when test="multiply and count(multiply/value) = 2 and normalize-space(multiply/value[1]) = $path and not(multiply/value[1]/*)">
        <xsl:text> *= </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="multiply/value[2]" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
      </xsl:when>
      <!-- itself shared out: /= -->
      <xsl:when test="divide and count(divide/value) = 2 and normalize-space(divide/value[1]) = $path and not(divide/value[1]/*)">
        <xsl:text> /= </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="divide/value[2]" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$self" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
