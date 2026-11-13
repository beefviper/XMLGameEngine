<?xml version="1.0" encoding="UTF-8"?>
<!-- reset.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <reset />: back where it started, its start drawn again first when it
     draws a <random> (a new serve). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="reset" mode="edge">
    <xsl:param name="owner" />
    <xsl:call-template name="reset">
      <xsl:with-param name="owner" select="$owner" />
      <xsl:with-param name="self" select="'object'" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="reset" mode="touch">
    <xsl:param name="owner" />
    <xsl:param name="self" />
    <xsl:call-template name="reset">
      <xsl:with-param name="owner" select="$owner" />
      <xsl:with-param name="self" select="$self" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="reset">
    <xsl:param name="owner" />
    <xsl:param name="self" />
    <xsl:if test="@* or node()">
      <xsl:message terminate="yes">windows-cpp can write only a bare &lt;reset /&gt; yet; use engine-cpp for this game</xsl:message>
    </xsl:if>
    <xsl:variable name="random">
      <xsl:if test="$owner[self::object]"><xsl:call-template name="draws-random"><xsl:with-param name="object" select="$owner" /></xsl:call-template></xsl:if>
    </xsl:variable>
    <xsl:if test="$random = 'yes'">
      <xsl:call-template name="object-name"><xsl:with-param name="object" select="$owner" /></xsl:call-template>
      <xsl:text>Start(); </xsl:text>
    </xsl:if>
    <xsl:value-of select="concat('restart(', $self, ');')" />
  </xsl:template>
</xsl:stylesheet>
