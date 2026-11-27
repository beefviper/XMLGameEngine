<?xml version="1.0" encoding="UTF-8"?>
<!-- trigger.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <trigger object action /> on a key: that action's verbs, every frame the key is held (game.xsl, held-moves); in a rule or a test, its function: faster(). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: on a key, naming an object and its action -->
  <xsl:template match="input/trigger[@object and @action]" mode="covers">
    <xsl:apply-templates select="@* | *" mode="covers" />
  </xsl:template>
  <xsl:template match="trigger/@object | trigger/@action" mode="covers" />

  <!-- covered: in a rule or a test of the game's, an action of the object
       it names, or of its own -->
  <xsl:template match="collision/trigger[@action and not(node())] | conditions/condition/trigger[@action and @object and not(node())]" mode="covers">
    <xsl:variable name="object" select="$everything[@name = current()/@object] | ancestor::object[1][not(current()/@object)]" />
    <xsl:if test="not($object/actions/action[@name = current()/@action])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;trigger&gt; of ', $quote, @action, $quote, ', an action it has not')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template match="trigger" mode="do">
    <xsl:call-template name="action-name"><xsl:with-param name="action" select="@action" /></xsl:call-template>
    <xsl:text>();</xsl:text>
  </xsl:template>
</xsl:stylesheet>
