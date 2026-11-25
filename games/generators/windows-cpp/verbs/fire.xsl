<?xml version="1.0" encoding="UTF-8"?>
<!-- fire.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 25, 2026 -->

<!-- <fire object /> sends a new one of a piece from a thing: onto its list
     (bombs), from the middle of the side the thing faces, just clear of it,
     that way at the piece's own speed; with no facing, from its top middle at
     the piece's own way. None when the piece's <most> are in flight. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in a thing's timer, of a piece that moves along one
       axis when the thing faces some way -->
  <xsl:template match="object/timers/timer/fire[@object and not(node())]" mode="covers">
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:if test="not($piece/sprite[1]/bitmap or $piece/sprite[1]/rectangle) or (ancestor::object[1]/facing and count($piece/velocity/*) != 1)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;fire&gt; of other than a picture or a rectangle moving along one axis'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="fire" mode="do">
    <xsl:param name="self" />
    <xsl:variable name="piece" select="$fired[@name = current()/@object]" />
    <xsl:variable name="from"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$self" /></xsl:call-template></xsl:variable>
    <xsl:variable name="shot"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:variable name="list"><xsl:call-template name="fired-list-name"><xsl:with-param name="piece" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:variable name="type"><xsl:call-template name="fired-type"><xsl:with-param name="piece" select="$piece" /></xsl:call-template></xsl:variable>
    <xsl:variable name="width"><xsl:call-template name="own-size"><xsl:with-param name="self" select="$piece" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="height"><xsl:call-template name="own-size"><xsl:with-param name="self" select="$piece" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="facing" select="normalize-space($self/facing)" />
    <xsl:variable name="speed">
      <xsl:call-template name="value"><xsl:with-param name="node" select="$piece/velocity/*" /><xsl:with-param name="self" select="$piece" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="across" select="concat($from, '.position.x + ', $from, '.size.x / 2 - ', $width div 2)" />
    <xsl:variable name="down" select="concat($from, '.position.y + ', $from, '.size.y / 2 - ', $height div 2)" />
    <xsl:variable name="position">
      <xsl:choose>
        <xsl:when test="$facing = 'down'"><xsl:value-of select="concat('{ ', $across, ', ', $from, '.position.y + ', $from, '.size.y }')" /></xsl:when>
        <xsl:when test="$facing = 'up'"><xsl:value-of select="concat('{ ', $across, ', ', $from, '.position.y - ', $height, ' }')" /></xsl:when>
        <xsl:when test="$facing = 'left'"><xsl:value-of select="concat('{ ', $from, '.position.x - ', $width, ', ', $down, ' }')" /></xsl:when>
        <xsl:when test="$facing = 'right'"><xsl:value-of select="concat('{ ', $from, '.position.x + ', $from, '.size.x, ', $down, ' }')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat('{ ', $across, ', ', $from, '.position.y }')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="velocity">
      <xsl:choose>
        <xsl:when test="$facing = 'down'"><xsl:value-of select="concat('{ 0, ', $speed, ' }')" /></xsl:when>
        <xsl:when test="$facing = 'up'"><xsl:value-of select="concat('{ 0, -', $speed, ' }')" /></xsl:when>
        <xsl:when test="$facing = 'left'"><xsl:value-of select="concat('{ -', $speed, ', 0 }')" /></xsl:when>
        <xsl:when test="$facing = 'right'"><xsl:value-of select="concat('{ ', $speed, ', 0 }')" /></xsl:when>
        <xsl:otherwise><xsl:call-template name="pair"><xsl:with-param name="of" select="$piece/velocity" /><xsl:with-param name="self" select="$piece" /></xsl:call-template></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="tab">
      <xsl:if test="$piece/most"><xsl:text>&#9;</xsl:text></xsl:if>
    </xsl:variable>
    <xsl:if test="$piece/most">
      <xsl:value-of select="concat('if (', $list, '.size() &lt; ', normalize-space($piece/most), ')&#10;{&#10;')" />
    </xsl:if>
    <xsl:value-of select="concat($tab, $type, ' ', $shot, ';&#10;')" />
    <xsl:value-of select="concat($tab, $shot, '.size = { ', $width, ', ', $height, ' };&#10;')" />
    <xsl:value-of select="concat($tab, $shot, '.position = ', $position, ';&#10;')" />
    <xsl:value-of select="concat($tab, $shot, '.velocity = ', $velocity, ';&#10;')" />
    <xsl:value-of select="concat($tab, $list, '.push_back(', $shot, ');')" />
    <xsl:if test="$piece/most"><xsl:text>&#10;}</xsl:text></xsl:if>
  </xsl:template>
</xsl:stylesheet>
