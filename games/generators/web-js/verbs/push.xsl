<?xml version="1.0" encoding="UTF-8"?>
<!-- push.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <push state /> puts that screen on top of the stack, its time from 0
     when it keeps one. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere -->
  <xsl:template match="push[@state and not(node())]" mode="covers" />

  <xsl:template match="push" mode="do">
    <xsl:text>screens.push(</xsl:text>
    <xsl:call-template name="screen-name"><xsl:with-param name="state" select="@state" /></xsl:call-template>
    <xsl:text>);</xsl:text>
    <xsl:variable name="state" select="$states[@name = current()/@state]" />
    <xsl:if test="count($state | $timed-screens) = count($timed-screens)">
      <xsl:text>&#10;</xsl:text>
      <xsl:call-template name="screen-time"><xsl:with-param name="state" select="$state" /></xsl:call-template>
      <xsl:text> = 0;</xsl:text>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
