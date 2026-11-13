<?xml version="1.0" encoding="UTF-8"?>
<!-- stick.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <stick />: stopped at the window's edge, against it (sticksAt says
     which, so the frame does not carry it past first). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in a rule at an edge -->
  <xsl:template match="collision[@edge]/stick[not(@* or node())]" mode="covers" />

  <xsl:template match="stick" mode="edge">
    <xsl:if test="@* or node()">
      <xsl:message terminate="yes">windows-cpp can write only a bare &lt;stick /&gt; yet; use engine-cpp for this game</xsl:message>
    </xsl:if>
    <xsl:text>stick(object, window, edge);</xsl:text>
  </xsl:template>
</xsl:stylesheet>
