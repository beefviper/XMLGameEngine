<?xml version="1.0" encoding="UTF-8"?>
<!-- deflect.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <deflect>N</deflect>: off what it met at up to N degrees, by where on
     it it met. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: in a rule on a touch -->
  <xsl:template match="collision[not(@edge)]/deflect[not(@*)]" mode="covers">
    <xsl:apply-templates select="* | text()" mode="covers" />
  </xsl:template>

  <xsl:template match="deflect" mode="touch">
    <xsl:param name="owner" />
    <xsl:param name="self" />
    <xsl:param name="other" />
    <xsl:param name="touch" />
    <xsl:if test="@*">
      <xsl:message terminate="yes">windows-cpp cannot write &lt;deflect&gt; with an attribute yet; use engine-cpp for this game</xsl:message>
    </xsl:if>
    <xsl:value-of select="concat('deflect(', $self, ', ', $other, ', ', $touch, '.edge, ')" />
    <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$owner[self::object]" /></xsl:call-template>
    <xsl:text>);</xsl:text>
  </xsl:template>
</xsl:stylesheet>
