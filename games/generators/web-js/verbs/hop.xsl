<?xml version="1.0" encoding="UTF-8"?>
<!-- hop.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- <hop direction>distance</hop> in an action its key presses: at once that
     far that way, or not at all when it would land outside the window, and
     then nothing after it in the action happens:
     if (chicken.position.y - cell < 0) { return; } chicken.position.y -= cell; -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: up, down, left or right by a distance, at once, in an action
       of hops and sounds -->
  <xsl:template match="action/hop[@direction = 'up' or @direction = 'down' or @direction = 'left' or @direction = 'right'][not(*)]" mode="covers">
    <xsl:if test="../*[not(self::hop or self::play)]">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;hop&gt; in an action that does more than hop and play'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="text()" mode="covers" />
  </xsl:template>
  <xsl:template match="hop/@direction" mode="covers" />

  <xsl:template match="hop" mode="do">
    <xsl:param name="who" />
    <xsl:param name="self" />
    <xsl:variable name="distance"><xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$self" /><xsl:with-param name="wrap" select="2" /></xsl:call-template></xsl:variable>
    <xsl:variable name="axis" select="substring('yx', 1 + number(@direction = 'left' or @direction = 'right'), 1)" />
    <xsl:variable name="at" select="concat($who, '.position.', $axis)" />
    <xsl:choose>
      <xsl:when test="@direction = 'up' or @direction = 'left'">
        <xsl:value-of select="concat('if (', $at, ' - ', $distance, ' &lt; 0) { return; }&#10;', $at, ' -= ', $distance, ';')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="edge">
          <xsl:choose>
            <xsl:when test="$axis = 'y'">windowHeight</xsl:when>
            <xsl:otherwise>windowWidth</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat('if (', $at, ' + ', $distance, ' &gt; ', $edge, ' - ', $who, '.size.', $axis, ') { return; }&#10;', $at, ' += ', $distance, ';')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
