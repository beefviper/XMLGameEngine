<?xml version="1.0" encoding="UTF-8"?>
<!-- play.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <play sound /> plays a sound: play("wall"). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere -->
  <xsl:template match="play[@sound and not(*)]" mode="covers" />

  <xsl:template match="play" mode="do">
    <xsl:text>play(</xsl:text>
    <xsl:call-template name="sound-name"><xsl:with-param name="sound" select="@sound" /></xsl:call-template>
    <xsl:text>);</xsl:text>
  </xsl:template>
</xsl:stylesheet>
