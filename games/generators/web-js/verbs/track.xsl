<?xml version="1.0" encoding="UTF-8"?>
<!-- track.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <track /> in the action the mouse (or pad 1's stick) triggers: the thing's middle follows it across the window, kept inside it (game.xsl, mouse()). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, alone in an action that only the mouse across triggers -->
  <xsl:template match="action/track[not(@* or node())]" mode="covers">
    <xsl:variable name="action" select=".." />
    <xsl:variable name="object" select="ancestor::object[1]" />
    <xsl:variable name="by" select="$states/inputs//input/trigger[@object = $object/@name and @action = $action/@name]" />
    <xsl:if test="count($action/*) != 1 or not($by) or $by[not(../@axis = 'mouse.x' or ../@axis = 'pad1.x')]">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;track /&gt; other than alone in an action the mouse or a stick triggers'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
