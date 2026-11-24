<?xml version="1.0" encoding="UTF-8"?>
<!-- die.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <die /> in a grid's rule: the cell that met goes from its list (grids.xsl): wall.splice(wall.indexOf(brick), 1). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, last in a grid's rule on meeting a thing -->
  <xsl:template match="group/collisions/collision[not(@edge)]/die[not(@* or node())][not(following-sibling::*)]" mode="covers" />
</xsl:stylesheet>
