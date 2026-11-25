<?xml version="1.0" encoding="UTF-8"?>
<!-- scenes.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 25, 2026 -->

<!-- The scenes of the played state (windows-cpp's kinds.xsl) are a table:
     an object of their numbers each, waves, the first first. The page keeps
     which it is at (waveAt) and reads the numbers of that one
     (wave().pace). Starting one starts the things it takes anew; its
     tests run when none of the state's own holds, and scenes whose tests
     are written the same share them. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:variable name="scene-word"><xsl:call-template name="scene-word" /></xsl:variable>
  <!-- Wave, waves, waveAt, startWave -->
  <xsl:variable name="scene-type"><xsl:call-template name="type-word"><xsl:with-param name="name" select="$scene-word" /></xsl:call-template></xsl:variable>
  <xsl:variable name="scene-list"><xsl:call-template name="camel"><xsl:with-param name="name" select="concat($scene-word, 's')" /></xsl:call-template></xsl:variable>
  <xsl:variable name="scene-at"><xsl:call-template name="camel"><xsl:with-param name="name" select="concat($scene-word, ' at')" /></xsl:call-template></xsl:variable>
  <xsl:variable name="scene-one"><xsl:call-template name="camel"><xsl:with-param name="name" select="$scene-word" /></xsl:call-template></xsl:variable>
  <xsl:variable name="scene-start" select="concat('start', $scene-type)" />

  <!-- const waves = [...], waveAt and wave(): the table of them, and the one it is at. -->
  <xsl:template name="scene-table">
    <xsl:value-of select="concat('&#10;// The ', $scene-word, 's: their numbers each, the first first, and the one it is at.&#10;const ', $scene-list, ' = [&#10;')" />
    <xsl:for-each select="$scenes">
      <xsl:variable name="scene" select="." />
      <xsl:text>&#9;{ </xsl:text>
      <xsl:for-each select="$scene-numbers">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>: </xsl:text>
        <xsl:value-of select="normalize-space($scene/variables/variable[@name = current()/@name])" />
      </xsl:for-each>
      <xsl:text> },&#10;</xsl:text>
    </xsl:for-each>
    <xsl:value-of select="concat('];&#10;let ', $scene-at, ' = 0;&#10;function ', $scene-one, '() { return ', $scene-list, '[', $scene-at, ']; }&#10;')" />
  </xsl:template>

  <!-- startWave(): which one it is, and what it takes as it starts. -->
  <xsl:template name="scene-start-function">
    <xsl:value-of select="concat('&#10;// A ', $scene-word, ' starts: the things it takes, anew.&#10;function ', $scene-start, '(at) {&#10;&#9;', $scene-at, ' = at;&#10;')" />
    <xsl:for-each select="$taken[count(. | $started) = count($started)]">
      <xsl:text>&#9;</xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text> = </xsl:text>
      <xsl:call-template name="start-name" />
      <xsl:text>();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>}&#10;</xsl:text>
  </xsl:template>

  <!-- The scenes' tests, after the state's own (`else` when it has some):
       scenes one after another whose tests are written the same, each run
       of them behind a test of which it is at. -->
  <xsl:template name="scene-tests">
    <xsl:param name="after" />
    <xsl:variable name="written">
      <xsl:for-each select="$scenes">
        <xsl:variable name="ids"><xsl:call-template name="scene-conditions"><xsl:with-param name="scene" select="." /></xsl:call-template></xsl:variable>
        <xsl:variable name="conditions" select="$scenes/conditions/condition[generate-id() = exsl:node-set($ids)/c/@id]" />
        <scene at="{position() - 1}" ids="{exsl:node-set($ids)/c/@id}">
          <xsl:attribute name="code">
            <xsl:for-each select="exsl:node-set($ids)/c">
              <xsl:variable name="id" select="@id" />
              <xsl:for-each select="$conditions[generate-id() = $id]">
                <xsl:call-template name="state-condition"><xsl:with-param name="before" select="''" /></xsl:call-template>
              </xsl:for-each>
            </xsl:for-each>
          </xsl:attribute>
          <xsl:copy-of select="exsl:node-set($ids)/c" />
        </scene>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="all" select="exsl:node-set($written)/scene" />
    <xsl:variable name="count" select="count($all)" />
    <xsl:if test="$all[@code != '']">
      <xsl:value-of select="concat('&#9;// then the ', $scene-word, $quote, 's own&#10;')" />
    </xsl:if>
    <xsl:for-each select="$all[@code != ''][not(preceding-sibling::scene[1]/@code = @code)]">
      <xsl:variable name="from" select="number(@at)" />
      <xsl:variable name="code" select="@code" />
      <!-- where the run of the same ends -->
      <xsl:variable name="ends" select="following-sibling::scene[@code != $code][1]/@at" />
      <xsl:variable name="to">
        <xsl:choose>
          <xsl:when test="$ends"><xsl:value-of select="$ends - 1" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$count - 1" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="test">
        <xsl:choose>
          <xsl:when test="$from = 0 and $to = $count - 1" />
          <xsl:when test="$from = $to"><xsl:value-of select="concat($scene-at, ' === ', $from)" /></xsl:when>
          <xsl:when test="$from = 0"><xsl:value-of select="concat($scene-at, ' &lt; ', $to + 1)" /></xsl:when>
          <xsl:when test="$to = $count - 1"><xsl:value-of select="concat($scene-at, ' &gt;= ', $from)" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="concat($scene-at, ' &gt;= ', $from, ' &amp;&amp; ', $scene-at, ' &lt; ', $to + 1)" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="first-run" select="not(preceding-sibling::scene[@code != ''])" />
      <xsl:for-each select="c">
        <xsl:variable name="id" select="@id" />
        <xsl:variable name="nth" select="position()" />
        <xsl:for-each select="$scenes/conditions/condition[generate-id() = $id]">
          <xsl:if test="$to = $count - 1 and .//enter">
            <xsl:message terminate="yes">web-js cannot write the last scene entering another yet; use web-asm for this game</xsl:message>
          </xsl:if>
          <xsl:call-template name="state-condition">
            <xsl:with-param name="before">
              <xsl:if test="$after or not($first-run) or $nth &gt; 1">else </xsl:if>
            </xsl:with-param>
            <xsl:with-param name="tests" select="$test" />
          </xsl:call-template>
        </xsl:for-each>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
