<?xml version="1.0" encoding="UTF-8"?>
<!-- become.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <become object sprite /> gives an object that look: computer.look = Computer::Look::Smart. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere, naming an object with that look -->
  <xsl:template match="become[@object and @sprite and not(node())]" mode="covers">
    <xsl:if test="not($everything[@name = current()/@object]/sprite[@name = current()/@sprite])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;become&gt; of ', $quote, @object, $quote, ' to a look it has not')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="become" mode="do">
    <xsl:variable name="object" select="$everything[@name = current()/@object]" />
    <xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template>
    <xsl:text>.look = </xsl:text>
    <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@sprite" /></xsl:call-template>
    <xsl:text>;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
