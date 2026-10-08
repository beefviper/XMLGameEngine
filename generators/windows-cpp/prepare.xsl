<?xml version="1.0" encoding="UTF-8"?>
<!-- prepare.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 8, 2026 -->

<!-- What the game file is turned into before it is generated (xgecli runs
     this first, then generate.xsl on what it makes). The game stays as it is
     but for one thing: a member of a group that a screen shows by its own
     name, the group itself shown on none, becomes an object of that name, as
     if it had been written so (Pitfall!'s scorpions, one under each of two
     screens). The object is the group with the member's own parts in place
     of the group's, axis by axis for a position or a velocity. Generating a
     group whose members are shown on screens of their own would need a flag
     for each member on each screen; a person would write two objects. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="xml" indent="no" />

  <xsl:variable name="shown" select="/game/states/state/shows/show/@object" />

  <xsl:template match="@* | node()">
    <xsl:copy>
      <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
  </xsl:template>

  <!-- a group shown only member by member: its members that are shown, as
       objects, where the group was -->
  <xsl:template match="/game/objects/group[not(@name = /game/states/state/shows/show/@object)][member/@name = /game/states/state/shows/show/@object]">
    <xsl:variable name="group" select="." />
    <xsl:if test="/game//*[@object = $group/@name]">
      <xsl:message terminate="yes">windows-cpp cannot generate a group shown member by member that something names (object="<xsl:value-of select="@name" />") yet (in game > objects > group <xsl:value-of select="@name" />)</xsl:message>
    </xsl:if>
    <xsl:for-each select="member[@name = $shown]">
      <xsl:variable name="member" select="." />
      <object name="{@name}">
        <xsl:copy-of select="$group/@class" />
        <xsl:for-each select="$group/*[not(self::member or self::row or self::column or self::cell or self::columns or self::rows)] | *[not($group/*[local-name() = local-name(current())])]">
          <xsl:variable name="tag" select="local-name()" />
          <xsl:variable name="own" select="$member/*[local-name() = $tag]" />
          <xsl:choose>
            <xsl:when test="(self::position or self::velocity) and $own and count(. | $own) = 2">
              <xsl:copy>
                <xsl:copy-of select="@*" />
                <xsl:choose>
                  <xsl:when test="$own/x"><xsl:copy-of select="$own/x" /></xsl:when>
                  <xsl:otherwise><xsl:copy-of select="x" /></xsl:otherwise>
                </xsl:choose>
                <xsl:choose>
                  <xsl:when test="$own/y"><xsl:copy-of select="$own/y" /></xsl:when>
                  <xsl:otherwise><xsl:copy-of select="y" /></xsl:otherwise>
                </xsl:choose>
              </xsl:copy>
            </xsl:when>
            <xsl:when test="$own and count(. | $own) = 2 and self::sprite">
              <xsl:message terminate="yes">windows-cpp cannot generate a member shown on its own with a sprite of its own yet (in game > objects > group <xsl:value-of select="$group/@name" /> > member <xsl:value-of select="$member/@name" />)</xsl:message>
            </xsl:when>
            <xsl:when test="$own and count(. | $own) = 2">
              <xsl:apply-templates select="$own" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="." />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </object>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
