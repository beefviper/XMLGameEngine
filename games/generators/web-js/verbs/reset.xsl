<?xml version="1.0" encoding="UTF-8"?>
<!-- reset.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <reset /> at an edge: the object starts again (its start function,
     game.xsl), its <random>s drawn anew; <reset object /> that object; and
     on a key, a new game, what is kept written first. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in an object's rule at an edge or on a key; naming an
       object that starts again, anywhere -->
  <xsl:template match="object/collisions/collision[@edge]/reset[not(@* or node())] | input/reset[not(@* or node())]" mode="covers" />
  <!-- and bare in a rule of a group of members that start still: each back
       where it started (its start, kept in its record) -->
  <xsl:template match="group[member][not(velocity or member/velocity)]/collisions/collision/reset[not(@* or node())]" mode="covers" />
  <xsl:template match="reset[@object and not(node())]" mode="covers">
    <xsl:if test="not($everything[@name = current()/@object] | $grids[@name = current()/@object])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;reset&gt; of ', $quote, @object, $quote, ', not an object')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="reset/@object" mode="covers" />

  <xsl:template match="reset[not(@object)][ancestor::group[member]]" mode="edge">
    <xsl:param name="of" />
    <xsl:value-of select="concat($of, 'position = { ...', $of, 'start };&#10;', $of, 'velocity = { x: 0, y: 0 };')" />
  </xsl:template>
  <xsl:template match="reset[not(@object)][ancestor::group[member]]" mode="touch">
    <xsl:param name="who" />
    <xsl:value-of select="concat($who, '.position = { ...', $who, '.start };&#10;', $who, '.velocity = { x: 0, y: 0 };')" />
  </xsl:template>

  <xsl:template match="reset" mode="edge">
    <xsl:call-template name="restart"><xsl:with-param name="object" select="($everything[@name = current()/@object] | ../../parent::*[not(current()/@object)])[1]" /></xsl:call-template>
  </xsl:template>

  <!-- ball = startBall(); -->
  <xsl:template name="restart">
    <xsl:param name="object" />
    <xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template>
    <xsl:text> = </xsl:text>
    <xsl:call-template name="start-name"><xsl:with-param name="object" select="$object" /></xsl:call-template>
    <xsl:text>();</xsl:text>
  </xsl:template>

  <xsl:template match="reset" mode="do">
    <xsl:choose>
      <xsl:when test="@object">
        <xsl:call-template name="restart"><xsl:with-param name="object" select="$everything[@name = current()/@object] | $grids[@name = current()/@object]" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$kept">keep(); newGame();</xsl:when>
      <xsl:otherwise>newGame();</xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
