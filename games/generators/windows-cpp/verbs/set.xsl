<?xml version="1.0" encoding="UTF-8"?>
<!-- set.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <set variable>value</set> gives a variable, or a thing's x, y or velocity, that value: ball.velocity.x *= 1.3f when it multiplies itself. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere, of a variable it can name -->
  <xsl:template match="set[@variable]" mode="covers">
    <xsl:apply-templates select="* | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="set/@variable" mode="covers" />

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
      <xsl:otherwise>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$self" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
