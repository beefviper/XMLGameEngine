<?xml version="1.0" encoding="UTF-8"?>
<!-- inc.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <inc variable /> adds its number (1 when it has none) to a variable: paddle2.score += 1. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere, of a variable it can name -->
  <xsl:template match="inc[@variable]" mode="covers">
    <xsl:apply-templates select="* | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="inc/@variable" mode="covers" />
  <!-- who="self": the one the rule is of, as without it -->
  <xsl:template match="inc/@who[. = 'self']" mode="covers" />

  <xsl:template match="inc" mode="do">
    <xsl:param name="self" select="/.." />
    <xsl:call-template name="variable-target"><xsl:with-param name="path" select="@variable" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
    <xsl:text> += </xsl:text>
    <xsl:choose>
      <xsl:when test="normalize-space(.) = '' and not(*)">1</xsl:when>
      <xsl:otherwise>
        <!-- a count added to a number that is not one: said outright -->
        <xsl:variable name="target" select="$everything[@name = substring-before(current()/@variable, '.')]/variables/variable[@name = substring-after(current()/@variable, '.')][1]" />
        <xsl:variable name="target-type"><xsl:for-each select="$target"><xsl:call-template name="variable-type" /></xsl:for-each></xsl:variable>
        <xsl:variable name="source" select="($self/variables/variable | $self/parent::group/variables/variable)[@name = normalize-space(current()/self)][1]" />
        <xsl:variable name="source-type"><xsl:for-each select="$source"><xsl:call-template name="variable-type" /></xsl:for-each></xsl:variable>
        <xsl:variable name="cast" select="count(*) = 1 and self and $target-type = 'float' and $source-type = 'int'" />
        <xsl:if test="$cast">static_cast&lt;float&gt;(</xsl:if>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$self" /></xsl:call-template>
        <xsl:if test="$cast">)</xsl:if>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
