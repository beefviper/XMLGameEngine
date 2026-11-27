<?xml version="1.0" encoding="UTF-8"?>
<!-- despawn.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 25, 2026 -->

<!-- <despawn object /> takes every one of a fired piece away: bombs = [],
     and out of the loop over them when it is in one; a bare <despawn /> in
     the piece's own rule on a meeting takes that one away, spliced from its
     list (game.xsl). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: of a fired piece, anywhere; bare, last in a fired piece's
       rule on meeting a thing -->
  <xsl:template match="despawn[@object and not(node())]" mode="covers">
    <xsl:if test="not($fired[@name = current()/@object])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;despawn&gt; of ', $quote, @object, $quote, ', not a piece that is fired')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="object/animation/then/despawn[not(@* or node())][not(following-sibling::*)]" mode="covers" />
  <xsl:template match="object/collisions/collision[@object or @class][not(@edge)]/despawn[not(@* or node())][not(following-sibling::*)]" mode="covers">
    <xsl:if test="count(ancestor::object[1] | $fired) != count($fired)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a bare &lt;despawn /&gt; other than of a piece that is fired'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="despawn" mode="do">
    <xsl:call-template name="fired-list-name"><xsl:with-param name="piece" select="$fired[@name = current()/@object]" /></xsl:call-template>
    <xsl:text> = [];</xsl:text>
  </xsl:template>
  <xsl:template match="despawn" mode="edge">
    <xsl:param name="of" />
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:variable name="one"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:call-template name="fired-list-name"><xsl:with-param name="piece" select="$piece" /></xsl:call-template>
    <xsl:text> = [];</xsl:text>
    <xsl:if test="$of = concat($one, '.')">&#10;break;</xsl:if>
  </xsl:template>
</xsl:stylesheet>
