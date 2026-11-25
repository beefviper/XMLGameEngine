<?xml version="1.0" encoding="UTF-8"?>
<!-- enter.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 25, 2026 -->

<!-- <enter way /> in a scene's test: the next scene starts (a scene's ways
     each lead to the next, windows-cpp's covers.xsl): startWave(waveAt + 1). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: by a way, in a scene's test -->
  <xsl:template match="scene/conditions/condition/enter[@way and not(node())]" mode="covers">
    <xsl:if test="not(ancestor::scene/ways/way[@name = current()/@way])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;enter way=', $quote, @way, $quote, '&gt; that its scene has not')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="enter" mode="do">
    <xsl:value-of select="concat($scene-start, '(', $scene-at, ' + 1);')" />
  </xsl:template>
</xsl:stylesheet>
