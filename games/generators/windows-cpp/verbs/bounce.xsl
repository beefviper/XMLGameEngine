<?xml version="1.0" encoding="UTF-8"?>
<!-- bounce.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <bounce>, keeping a share of its speed (all of it when it says none):
     off the window's edge, or off what it met. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="bounce" mode="edge">
    <xsl:param name="owner" />
    <xsl:text>bounceOffEdge(object, edge, </xsl:text>
    <xsl:call-template name="bounce-share"><xsl:with-param name="owner" select="$owner" /></xsl:call-template>
    <xsl:text>);</xsl:text>
  </xsl:template>

  <xsl:template match="bounce" mode="touch">
    <xsl:param name="owner" />
    <xsl:param name="self" />
    <xsl:param name="other" />
    <xsl:param name="touch" />
    <xsl:value-of select="concat('bounce(', $self, ', ', $other, ', ', $touch, ', ')" />
    <xsl:call-template name="bounce-share"><xsl:with-param name="owner" select="$owner" /></xsl:call-template>
    <xsl:text>);</xsl:text>
  </xsl:template>

  <xsl:template name="bounce-share">
    <xsl:param name="owner" />
    <xsl:if test="@*">
      <xsl:message terminate="yes">windows-cpp cannot write &lt;bounce&gt; with an attribute yet; use engine-cpp for this game</xsl:message>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="normalize-space(.) = '' and not(*)">1</xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$owner[self::object]" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
