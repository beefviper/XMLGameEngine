<?xml version="1.0" encoding="UTF-8"?>
<!-- trigger.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <trigger object action /> on a key: that action's verbs, every frame the key is held (game.xsl, held-moves). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: on a key, naming an object and its action -->
  <xsl:template match="input/trigger[@object and @action]" mode="covers">
    <xsl:apply-templates select="@* | *" mode="covers" />
  </xsl:template>
  <xsl:template match="trigger/@object | trigger/@action" mode="covers" />
</xsl:stylesheet>
