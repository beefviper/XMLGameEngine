<?xml version="1.0" encoding="UTF-8"?>
<!-- copy.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 28, 2026 -->

<!-- <copy variable into /> gives one variable another's number: best.score = lens.score; -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, anywhere, of variables of objects -->
  <xsl:template match="copy[@variable and @into and not(node())]" mode="covers">
    <xsl:for-each select="@variable | @into">
      <xsl:variable name="owner" select="$everything[@name = substring-before(current(), '.')]" />
      <xsl:if test="not(($owner/variables/variable | $owner/parent::group/variables/variable)[@name = substring-after(current(), '.')])">
        <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('the name ', $quote, ., $quote)" /></xsl:call-template>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="copy" mode="do">
    <xsl:param name="self" select="/.." />
    <xsl:call-template name="variable-target"><xsl:with-param name="path" select="@into" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
    <xsl:text> = </xsl:text>
    <xsl:call-template name="name-or-number"><xsl:with-param name="text" select="string(@variable)" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
    <xsl:text>;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
