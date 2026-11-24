<?xml version="1.0" encoding="UTF-8"?>
<!-- hold.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <hold button /> in a computer's condition: it holds that button of its seat (game.xsl, computerHolds()). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: in the computer's conditions, a held input of its seat -->
  <xsl:template match="computer//condition/hold[@button and not(node())]" mode="covers">
    <xsl:variable name="seat" select="ancestor::computer/@seat" />
    <xsl:if test="not(ancestor::state/inputs/seat[@number = $seat]/input[trigger][@button = current()/@button or button = current()/@button])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;hold&gt; of ', $quote, @button, $quote, ', a button its seat does not hold')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="hold" mode="do">
    <xsl:variable name="seat" select="ancestor::computer/@seat" />
    <xsl:call-template name="seat-held">
      <xsl:with-param name="input" select="ancestor::state/inputs/seat[@number = $seat]/input[trigger][@button = current()/@button or button = current()/@button]" />
    </xsl:call-template>
    <xsl:text> = true;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
