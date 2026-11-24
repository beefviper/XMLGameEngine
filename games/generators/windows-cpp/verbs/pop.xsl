<?xml version="1.0" encoding="UTF-8"?>
<!-- pop.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <pop /> goes back to the screen beneath. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, anywhere -->
  <xsl:template match="pop[not(@* or node())]" mode="covers" />

  <xsl:template match="pop" mode="do">screens.pop_back();</xsl:template>
</xsl:stylesheet>
