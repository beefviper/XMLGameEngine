<?xml version="1.0" encoding="UTF-8"?>
<!-- prepare.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 27, 2026 -->

<!-- The game, said more plainly, before generate.xsl writes it out: run by
     xgecli on the game file first, its output what generate.xsl reads. It
     is the same game; only groups in groups go, the way a person writing
     it by hand would not keep them:

     * A group that only holds others (no class, no look of its own) gives
       what it says to each of them and goes: Frogger's lanes give the
       river and the traffic their row and their wrap.
     * A group in a grid that holds kinds of its own gives what it says to
       each kind and goes: Frogger's logs give each row of logs their
       picture. A kind's own picture of rows only takes the scale and the
       colours of the one it stands for, written out on the kind.

     Whatever a group says that one inside it says too, the inside one's
     wins; their rules are both kept, the inside one's first. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    exclude-result-prefixes="exsl">

  <xsl:output method="xml" encoding="UTF-8" />

  <xsl:key name="picture" match="/game/pictures/bitmap" use="@name" />

  <!-- a group at a time, until none holds groups of groups -->
  <xsl:template match="/">
    <xsl:call-template name="plainer">
      <xsl:with-param name="game" select="." />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="plainer">
    <xsl:param name="game" />
    <xsl:choose>
      <xsl:when test="$game//group[group/group]">
        <xsl:variable name="once"><xsl:apply-templates select="$game/node()" /></xsl:variable>
        <xsl:call-template name="plainer">
          <xsl:with-param name="game" select="exsl:node-set($once)" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:copy-of select="$game/node()" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="@* | node()">
    <xsl:copy><xsl:apply-templates select="@* | node()" /></xsl:copy>
  </xsl:template>

  <!-- a holder: its groups take its place -->
  <xsl:template match="group[group/group][not(@class) and not(sprite)]">
    <xsl:for-each select="group">
      <xsl:call-template name="given">
        <xsl:with-param name="from" select=".." />
      </xsl:call-template>
    </xsl:for-each>
  </xsl:template>

  <!-- a group with a middle group of kinds: the kinds take its place -->
  <xsl:template match="group[group/group][@class or sprite]">
    <xsl:copy>
      <xsl:apply-templates select="@*" />
      <xsl:for-each select="node()">
        <xsl:choose>
          <xsl:when test="self::group[group]">
            <xsl:for-each select="group">
              <xsl:call-template name="given">
                <xsl:with-param name="from" select=".." />
              </xsl:call-template>
            </xsl:for-each>
          </xsl:when>
          <xsl:otherwise><xsl:apply-templates select="." /></xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </xsl:copy>
  </xsl:template>

  <!-- This group, with what $from says that it does not. -->
  <xsl:template name="given">
    <xsl:param name="from" />
    <xsl:variable name="here" select="." />
    <xsl:copy>
      <xsl:copy-of select="$from/@*[not(name() = 'name')]" />
      <xsl:apply-templates select="@*" />
      <!-- what the outer one says that this one does not -->
      <xsl:for-each select="$from/*[not(self::group or self::collisions)]">
        <xsl:variable name="tag" select="name()" />
        <xsl:if test="not($here/*[name() = $tag])">
          <xsl:copy-of select="." />
        </xsl:if>
      </xsl:for-each>
      <xsl:for-each select="*[not(self::collisions)]">
        <xsl:choose>
          <xsl:when test="self::sprite and $from/sprite">
            <xsl:call-template name="sprite-given">
              <xsl:with-param name="outer" select="$from/sprite[1]" />
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise><xsl:apply-templates select="." /></xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:if test="collisions or $from/collisions">
        <collisions>
          <xsl:apply-templates select="collisions/node()" />
          <xsl:copy-of select="$from/collisions/node()" />
        </collisions>
      </xsl:if>
    </xsl:copy>
  </xsl:template>

  <!-- A kind's picture with what the outer one's gives that it does not. -->
  <xsl:template name="sprite-given">
    <xsl:param name="outer" />
    <xsl:variable name="outer-picture" select="key('picture', $outer/bitmap/@from)" />
    <xsl:copy>
      <xsl:copy-of select="@*" />
      <xsl:for-each select="*">
        <xsl:choose>
          <xsl:when test="self::bitmap[@from] and $outer-picture">
            <xsl:variable name="own" select="key('picture', @from) | ." />
            <xsl:copy>
              <xsl:copy-of select="@* | node()" />
              <xsl:if test="not($own/scale)">
                <xsl:copy-of select="($outer/bitmap/scale | $outer-picture/scale)[1]" />
              </xsl:if>
              <xsl:if test="not($own/color[not(@char)])">
                <xsl:copy-of select="($outer/bitmap/color[not(@char)] | $outer-picture/color[not(@char)])[1]" />
              </xsl:if>
              <xsl:for-each select="$outer-picture/color[@char]">
                <xsl:if test="not($own/color[@char = current()/@char])">
                  <xsl:copy-of select="." />
                </xsl:if>
              </xsl:for-each>
            </xsl:copy>
          </xsl:when>
          <xsl:otherwise><xsl:copy-of select="." /></xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
