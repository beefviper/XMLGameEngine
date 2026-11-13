<?xml version="1.0" encoding="UTF-8"?>
<!-- move.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <move direction>step</move> in an action: while its key is held, that many pixels a frame that way. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: in an action of something with no <velocity> of its own,
       which the keys set every frame -->
  <xsl:template match="action/move" mode="covers">
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="move/@direction" mode="covers" />
  <xsl:template match="action/move[ancestor::object[1]/velocity | ancestor::group[1]/member/velocity]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;move&gt; for something with a &lt;velocity&gt;'" /></xsl:call-template>
  </xsl:template>

  <xsl:template match="move" mode="held">
    <xsl:param name="who" />
    <xsl:variable name="along">
      <xsl:choose>
        <xsl:when test="@direction = 'up'">.velocity.y -= </xsl:when>
        <xsl:when test="@direction = 'down'">.velocity.y += </xsl:when>
        <xsl:when test="@direction = 'left'">.velocity.x -= </xsl:when>
        <xsl:otherwise>.velocity.x += </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:value-of select="concat($who, $along)" />
    <xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template>
    <xsl:text>;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
