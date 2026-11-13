<?xml version="1.0" encoding="UTF-8"?>
<!-- reset.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <reset /> at an edge: the object starts again (its start function, game.xsl), its <random>s drawn anew. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in an object's rule at an edge -->
  <xsl:template match="object/collisions/collision[@edge]/reset[not(@* or node())]" mode="covers" />

  <xsl:template match="reset" mode="edge">
    <xsl:call-template name="start-name"><xsl:with-param name="object" select="../../.." /></xsl:call-template>
    <xsl:text>();</xsl:text>
  </xsl:template>
</xsl:stylesheet>
