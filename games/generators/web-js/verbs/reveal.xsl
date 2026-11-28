<?xml version="1.0" encoding="UTF-8"?>
<!-- reveal.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- <reveal object /> puts a thing out of play (<die />, <waiting />) back
     in, where it started, going its first way, its numbers drawn at random
     drawn again: frog.inPlay = true; frog.position = { ...frog.start }; -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: one, of a thing that dies or waits -->
  <xsl:template match="reveal[@object and not(node())]" mode="covers">
    <xsl:if test="not($things[@name = current()/@object][actions/action/die or collisions/collision/die or waiting])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;reveal&gt; of ', $quote, @object, $quote, ', not a thing that dies')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="reveal/@object" mode="covers" />

  <xsl:template match="reveal" mode="do">
    <xsl:variable name="object" select="$things[@name = current()/@object]" />
    <xsl:variable name="thing"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat($thing, '.inPlay = true;&#10;', $thing, '.position = { ...', $thing, '.start };&#10;', $thing, '.velocity = ')" />
    <xsl:call-template name="pair"><xsl:with-param name="of" select="$object/velocity" /><xsl:with-param name="self" select="$object" /></xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:for-each select="$object/variables/variable[.//random]">
      <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
      <xsl:if test="$kind = 'live'">
        <xsl:value-of select="concat('&#10;', $thing, '.')" />
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$object" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
        <xsl:text>;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
