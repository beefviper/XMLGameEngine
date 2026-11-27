<?xml version="1.0" encoding="UTF-8"?>
<!-- reveal.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- <reveal object /> puts a thing out of play (<die />) back in, where it
     started: frog.inPlay = true; frog.position = frog.start; -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: one, of a thing that dies -->
  <xsl:template match="reveal[@object and not(node())]" mode="covers">
    <xsl:if test="not($things[@name = current()/@object][actions/action/die])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;reveal&gt; of ', $quote, @object, $quote, ', not a thing that dies')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="reveal/@object" mode="covers" />

  <xsl:template match="reveal" mode="do">
    <xsl:variable name="thing"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$things[@name = current()/@object]" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat($thing, '.inPlay = true;&#10;', $thing, '.position = ', $thing, '.start;&#10;', $thing, '.velocity = {};')" />
  </xsl:template>
</xsl:stylesheet>
