<?xml version="1.0" encoding="UTF-8"?>
<!-- spawn.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- <spawn object /> in a thing's action: one of the piece onto its list
     (splats), its middle on the thing's middle, at the piece's own speed.
     Its <most> there already, it is refused, and nothing after it in the
     action happens: if (splats.length >= 1) { return; } In a rule on a
     meeting, of a thing or a grid's cell, it is only left out then. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in an object's action, of a picture or a rectangle -->
  <xsl:template match="object/actions/action/spawn[@object and not(node())]" mode="covers">
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:if test="not($piece/sprite[1]/bitmap or $piece/sprite[1]/rectangle) or count(ancestor::object[1] | $things) != count($things)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;spawn&gt; of other than a picture or a rectangle, by a thing'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="collisions/collision[not(@edge)]/spawn[@object and not(node())]" mode="covers">
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:variable name="owner" select="ancestor::*[self::object or self::group][1]" />
    <xsl:if test="not($piece/sprite[1]/bitmap or $piece/sprite[1]/rectangle) or (count($owner | $things) != count($things) and count($owner | $grids) != count($grids))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;spawn&gt; of other than a picture or a rectangle, on a meeting of a thing or a grid'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- covered: in a rule of a list of a map, one more of a list of it in
       a free cell at random, if there is one -->
  <xsl:template match="map/group/collisions/collision/spawn[@object][count(*) = 1][free[not(@* or node())]]" mode="covers">
    <xsl:if test="not(ancestor::map[1]/group[@name = current()/@object])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;spawn&gt; on a map other than of a list of it, in a free cell'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="spawn" mode="cell">
    <xsl:text>const spot = freeSpot(next);&#10;if (spot) { </xsl:text>
    <xsl:call-template name="object-name"><xsl:with-param name="object" select="ancestor::map[1]/group[@name = current()/@object]" /></xsl:call-template>
    <xsl:text>.push(spot); }</xsl:text>
  </xsl:template>
  <xsl:template match="spawn/@object" mode="covers" />

  <xsl:template match="spawn" mode="touch">
    <xsl:param name="who" />
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:variable name="list"><xsl:call-template name="fired-list-name"><xsl:with-param name="piece" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:variable name="lines"><xsl:call-template name="spawned"><xsl:with-param name="from" select="$who" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$piece/most">
        <xsl:value-of select="concat('if (', $list, '.length &lt; ', normalize-space($piece/most), ') {&#10;&#9;')" />
        <xsl:call-template name="replace"><xsl:with-param name="text" select="$lines" /><xsl:with-param name="from" select="'&#10;'" /><xsl:with-param name="to" select="'&#10;&#9;'" /></xsl:call-template>
        <xsl:text>&#10;}</xsl:text>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$lines" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="spawn" mode="do">
    <xsl:param name="self" />
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:variable name="from"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$self" /></xsl:call-template></xsl:variable>
    <xsl:variable name="list"><xsl:call-template name="fired-list-name"><xsl:with-param name="piece" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:if test="$piece/most">
      <xsl:value-of select="concat('if (', $list, '.length &gt;= ', normalize-space($piece/most), ') { return; }&#10;')" />
    </xsl:if>
    <xsl:call-template name="spawned"><xsl:with-param name="from" select="$from" /></xsl:call-template>
  </xsl:template>

  <!-- The new one, from `from` (frog or shard), onto its list. -->
  <xsl:template name="spawned">
    <xsl:param name="from" />
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:variable name="list"><xsl:call-template name="fired-list-name"><xsl:with-param name="piece" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:variable name="width"><xsl:call-template name="own-size"><xsl:with-param name="self" select="$piece" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="height"><xsl:call-template name="own-size"><xsl:with-param name="self" select="$piece" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat($list, '.push({&#10;')" />
    <xsl:value-of select="concat('&#9;position: { x: ', $from, '.position.x + ', $from, '.size.x / 2 - ', $width div 2, ', y: ', $from, '.position.y + ', $from, '.size.y / 2 - ', $height div 2, ' },&#10;')" />
    <xsl:text>&#9;velocity: </xsl:text>
    <xsl:choose>
      <xsl:when test="$piece/velocity"><xsl:call-template name="pair"><xsl:with-param name="of" select="$piece/velocity" /><xsl:with-param name="self" select="$piece" /></xsl:call-template></xsl:when>
      <xsl:otherwise>{ x: 0, y: 0 }</xsl:otherwise>
    </xsl:choose>
    <xsl:text>,&#10;</xsl:text>
    <xsl:value-of select="concat('&#9;size: { x: ', $width, ', y: ', $height, ' },&#10;')" />
    <xsl:if test="$piece/animation"><xsl:text>&#9;age: 0,&#10;</xsl:text></xsl:if>
    <xsl:text>});</xsl:text>
  </xsl:template>
</xsl:stylesheet>
