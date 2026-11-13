<?xml version="1.0" encoding="UTF-8"?>
<!-- trigger.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- <trigger object action /> on a key: that object's action (its own, or
     its group's), each of its verbs in mode action. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="trigger" mode="key">
    <xsl:variable name="named" select="@object" />
    <xsl:variable name="action" select="@action" />
    <xsl:variable name="object" select="$objects[@name = $named]" />
    <xsl:variable name="done" select="($object/actions/action[@name = $action] | $object/parent::group/actions/action[@name = $action])[1]" />
    <xsl:if test="not($object) or not($done)">
      <xsl:message terminate="yes">windows-cpp cannot write the trigger of '<xsl:value-of select="$named" />' '<xsl:value-of select="$action" />' yet (an object of the state and an action of its own or its group's); use engine-cpp for this game</xsl:message>
    </xsl:if>
    <xsl:variable name="who"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template>()</xsl:variable>
    <xsl:for-each select="$done/*">
      <xsl:text> </xsl:text>
      <xsl:apply-templates select="." mode="action">
        <xsl:with-param name="who" select="$who" />
        <xsl:with-param name="self" select="$object" />
      </xsl:apply-templates>
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
