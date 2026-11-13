<?xml version="1.0" encoding="UTF-8"?>
<!-- deflect.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <deflect>degrees</deflect> on a touch: deflect() off the other thing, written with the game when it is used (game.xsl). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: in an object's rule on a touch -->
  <xsl:template match="object/collisions/collision[not(@edge)]/deflect[not(@*)]" mode="covers">
    <xsl:apply-templates select="* | text()" mode="covers" />
  </xsl:template>

  <xsl:template match="deflect" mode="touch">
    <xsl:param name="self" />
    <xsl:param name="other" />
    <xsl:value-of select="concat('deflect(', $self, ', ', $other, ', ')" />
    <xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template>
    <xsl:text>);</xsl:text>
  </xsl:template>
</xsl:stylesheet>
