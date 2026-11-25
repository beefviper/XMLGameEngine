<?xml version="1.0" encoding="UTF-8"?>
<!-- reverse.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 25, 2026 -->

<!-- <reverse /> sends a thing back the way it came: bomber.velocity = -bomber.velocity. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in a thing's timer -->
  <xsl:template match="object/timers/timer/reverse[not(@* or node())]" mode="covers" />

  <xsl:template match="reverse" mode="do">
    <xsl:param name="self" />
    <xsl:variable name="thing"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$self" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat($thing, '.velocity = -', $thing, '.velocity;')" />
  </xsl:template>
</xsl:stylesheet>
