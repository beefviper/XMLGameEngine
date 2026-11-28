<?xml version="1.0" encoding="UTF-8"?>
<!-- hole.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 28, 2026 -->

<!-- <hole>radius<rough /></hole> in a grid cell's rule chips a hole in the
     cell's own picture where the other struck it: cutHole(*pylon, *beam, 9,
     0.5f), written with the game when it is used (game.xsl). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: a number, and a number rough, in the rule of a grid of
       pictures that touch by their pixels -->
  <xsl:template match="group/collisions/collision/hole" mode="covers">
    <xsl:if test="@* or *[not(self::rough)] or rough/* or string(number(normalize-space(text()))) = 'NaN' or string(number(rough)) = 'NaN' and rough
        or not(ancestor::group[1]/collisions/type = 'pixel' and ancestor::group[1]/sprite/bitmap)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;hole&gt; other than a number, in a grid of pictures touching by their pixels'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="hole" mode="touch">
    <xsl:param name="who" />
    <xsl:param name="other" />
    <xsl:variable name="rough" select="normalize-space(rough)" />
    <xsl:value-of select="concat('cutHole(', $who, ', ', $other, ', ', normalize-space(text()), ', ')" />
    <xsl:choose>
      <xsl:when test="$rough = ''">0</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$rough" />
        <xsl:if test="contains($rough, '.')">f</xsl:if>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>);</xsl:text>
  </xsl:template>
</xsl:stylesheet>
