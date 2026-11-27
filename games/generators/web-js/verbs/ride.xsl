<?xml version="1.0" encoding="UTF-8"?>
<!-- ride.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- <ride /> in a thing's rule on meeting another: carried along by it for
     the frame, its speed lent: frog.position.x += one.velocity.x * elapsed; -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in an object's rule on meeting others -->
  <xsl:template match="object/collisions/collision[not(@edge)]/ride[not(@* or node())]" mode="covers" />

  <xsl:template match="ride" mode="touch">
    <xsl:param name="who" />
    <xsl:param name="other" />
    <xsl:value-of select="concat($who, '.position.x += ', $other, '.velocity.x * elapsed;&#10;', $who, '.position.y += ', $other, '.velocity.y * elapsed;')" />
  </xsl:template>
</xsl:stylesheet>
