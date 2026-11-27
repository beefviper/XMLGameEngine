<?xml version="1.0" encoding="UTF-8"?>
<!-- ride.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- <ride /> in a thing's rule on meeting another: carried along by it for
     the frame, its speed lent: frog.position += one.velocity * elapsed; -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in an object's rule on meeting others -->
  <xsl:template match="object/collisions/collision[not(@edge)]/ride[not(@* or node())]" mode="covers" />

  <xsl:template match="ride" mode="touch">
    <xsl:param name="who" />
    <xsl:param name="other" />
    <xsl:variable name="of">
      <xsl:choose>
        <xsl:when test="starts-with($who, '*')"><xsl:value-of select="concat(substring($who, 2), '-&gt;')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat($who, '.')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="by">
      <xsl:choose>
        <xsl:when test="starts-with($other, '*')"><xsl:value-of select="concat(substring($other, 2), '-&gt;')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat($other, '.')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:value-of select="concat($of, 'position += ', $by, 'velocity * elapsed;')" />
  </xsl:template>
</xsl:stylesheet>
