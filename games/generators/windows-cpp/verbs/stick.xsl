<?xml version="1.0" encoding="UTF-8"?>
<!-- stick.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <stick /> at an edge: put back against it. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in a rule at an edge -->
  <xsl:template match="collision[@edge]/stick[not(@* or node())]" mode="covers" />

  <xsl:template match="stick" mode="edge">
    <xsl:param name="of" />
    <xsl:param name="edge" />
    <xsl:choose>
      <xsl:when test="$edge = 'top'"><xsl:value-of select="concat($of, 'position.y = 0;')" /></xsl:when>
      <xsl:when test="$edge = 'bottom'"><xsl:value-of select="concat($of, 'position.y = windowHeight - ', $of, 'size.y;')" /></xsl:when>
      <xsl:when test="$edge = 'left'"><xsl:value-of select="concat($of, 'position.x = 0;')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat($of, 'position.x = windowWidth - ', $of, 'size.x;')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
