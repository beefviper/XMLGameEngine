<?xml version="1.0" encoding="UTF-8"?>
<!-- become.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <become object sprite /> gives an object that look: computer.look = "smart"; and a thing whose looks differ in size, that size. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere, naming an object with that look -->
  <xsl:template match="become[@object and @sprite and not(node())]" mode="covers">
    <xsl:variable name="object" select="$everything[@name = current()/@object] | $states[@name = substring-before(current()/@object, '.')]/objects/object[@name = substring-after(current()/@object, '.')]" />
    <xsl:if test="not($object/sprite[@name = current()/@sprite])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;become&gt; of ', $quote, @object, $quote, ' to a look it has not')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="become" mode="do">
    <xsl:variable name="object" select="$everything[@name = current()/@object] | $states[@name = substring-before(current()/@object, '.')]/objects/object[@name = substring-after(current()/@object, '.')]" />
    <xsl:variable name="thing"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:variable name="look" select="$object/sprite[@name = current()/@sprite]/rectangle" />
    <xsl:value-of select="concat($thing, '.look = ')" />
    <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@sprite" /></xsl:call-template>
    <xsl:text>;</xsl:text>
    <!-- a size that differs between its looks, this look's -->
    <xsl:if test="$look and $object/sprite/rectangle[string(width) != string($look/width)]">
      <xsl:value-of select="concat('&#10;', $thing, '.size.x = ')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="$look/width" /><xsl:with-param name="self" select="$object" /></xsl:call-template>
      <xsl:text>;</xsl:text>
    </xsl:if>
    <xsl:if test="$look and $object/sprite/rectangle[string(height) != string($look/height)]">
      <xsl:value-of select="concat('&#10;', $thing, '.size.y = ')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="$look/height" /><xsl:with-param name="self" select="$object" /></xsl:call-template>
      <xsl:text>;</xsl:text>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
