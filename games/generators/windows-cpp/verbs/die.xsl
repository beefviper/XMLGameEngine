<?xml version="1.0" encoding="UTF-8"?>
<!-- die.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <die /> in a grid's rule: the cell that met goes from its list (grids.xsl): brick = wall.erase(brick);
     in a thing's action or its own rule: out of play (not drawn, moved or met, and its
     actions do nothing) until it is revealed: frog.inPlay = false; -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, last in a grid's rule on meeting a thing -->
  <xsl:template match="group/collisions/collision[not(@edge)]/die[not(@* or node())][not(following-sibling::*)]" mode="covers" />
  <xsl:template match="object/actions/action/die[not(@* or node())]" mode="covers" />
  <!-- and bare, last in a thing's rule -->
  <xsl:template match="object/collisions/collision/die[not(@* or node())][not(following-sibling::*)]" mode="covers">
    <xsl:if test="count(ancestor::object[1] | $things) != count($things)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;die&gt; in a rule of other than a thing'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="action/die | object/collisions/collision/die" mode="do">
    <xsl:param name="self" />
    <xsl:call-template name="object-name"><xsl:with-param name="object" select="$self" /></xsl:call-template>
    <xsl:text>.inPlay = false;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
