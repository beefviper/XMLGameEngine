<?xml version="1.0" encoding="UTF-8"?>
<!-- move.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <move direction>step</move> in an action: held while its key is. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: in an action, a step (no <wait>) -->
  <xsl:template match="action/move" mode="covers">
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="move/@direction" mode="covers" />

  <xsl:template match="move" mode="action">
    <xsl:param name="who" />
    <xsl:param name="self" />
    <xsl:if test="@*[local-name() != 'direction']">
      <xsl:message terminate="yes">windows-cpp cannot write &lt;move&gt; with <xsl:value-of select="local-name(@*[local-name() != 'direction'])" />= yet; use engine-cpp for this game</xsl:message>
    </xsl:if>
    <xsl:value-of select="concat('holdMove(', $who, ', Direction::', translate(substring(@direction, 1, 1), $lower, $upper), substring(@direction, 2), ', ')" />
    <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$self" /></xsl:call-template>
    <xsl:text>, pressed);</xsl:text>
  </xsl:template>
</xsl:stylesheet>
