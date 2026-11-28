<?xml version="1.0" encoding="UTF-8"?>
<!-- grow.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 28, 2026 -->

<!-- <grow /> on a piece with a chain: a link more at the tail as it next
     steps, head.growing += 1 (cells.xsl). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in a rule of a piece of a map with a chain -->
  <xsl:template match="map/object[chain]/collisions/collision/grow[not(@* or node())]" mode="covers" />

  <xsl:template match="grow" mode="do">
    <xsl:param name="self" select="/.." />
    <xsl:call-template name="object-name"><xsl:with-param name="object" select="$self" /></xsl:call-template>
    <xsl:text>.growing += 1;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
