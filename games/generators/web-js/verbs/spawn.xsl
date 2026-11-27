<?xml version="1.0" encoding="UTF-8"?>
<!-- spawn.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- <spawn object /> in a thing's action: one of the piece onto its list
     (splats), its middle on the thing's middle, at the piece's own speed.
     Its <most> there already, it is refused, and nothing after it in the
     action happens: if (splats.length >= 1) { return; } -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in an object's action, of a picture or a rectangle -->
  <xsl:template match="object/actions/action/spawn[@object and not(node())]" mode="covers">
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:if test="not($piece/sprite[1]/bitmap or $piece/sprite[1]/rectangle) or count(ancestor::object[1] | $things) != count($things)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;spawn&gt; of other than a picture or a rectangle, by a thing'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="spawn/@object" mode="covers" />

  <xsl:template match="spawn" mode="do">
    <xsl:param name="self" />
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:variable name="from"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$self" /></xsl:call-template></xsl:variable>
    <xsl:variable name="list"><xsl:call-template name="fired-list-name"><xsl:with-param name="piece" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:variable name="width"><xsl:call-template name="own-size"><xsl:with-param name="self" select="$piece" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="height"><xsl:call-template name="own-size"><xsl:with-param name="self" select="$piece" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
    <xsl:if test="$piece/most">
      <xsl:value-of select="concat('if (', $list, '.length &gt;= ', normalize-space($piece/most), ') { return; }&#10;')" />
    </xsl:if>
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
