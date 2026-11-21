<?xml version="1.0" encoding="UTF-8"?>
<!-- bounce.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <bounce /> at an edge: back off it, its speed into the edge turned round. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in a rule at an edge -->
  <xsl:template match="collision[@edge]/bounce[not(@* or node())]" mode="covers" />

  <xsl:template match="bounce" mode="edge">
    <xsl:param name="of" />
    <xsl:param name="edge" />
    <xsl:choose>
      <xsl:when test="$edge = 'top'"><xsl:value-of select="concat($of, 'velocity.y = std::abs(', $of, 'velocity.y);')" /></xsl:when>
      <xsl:when test="$edge = 'bottom'"><xsl:value-of select="concat($of, 'velocity.y = -std::abs(', $of, 'velocity.y);')" /></xsl:when>
      <xsl:when test="$edge = 'left'"><xsl:value-of select="concat($of, 'velocity.x = std::abs(', $of, 'velocity.x);')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat($of, 'velocity.x = -std::abs(', $of, 'velocity.x);')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
