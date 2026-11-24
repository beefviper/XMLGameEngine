<?xml version="1.0" encoding="UTF-8"?>
<!-- names.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- What the parts of a game are called in its JavaScript: the objects (a
     group's members each one of their own, taking the group's sprite, rules
     and actions) and the keys. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <!-- An object's (or member's) name: ball, paddle1. -->
  <xsl:template name="object-name">
    <xsl:param name="object" select="." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name" select="$object/@name" />
    </xsl:call-template>
  </xsl:template>

  <!-- An object's variable that nothing changes: ballSize, or paddleSpeed
       for a group's. -->
  <xsl:template name="constant-name">
    <xsl:param name="variable" select="." />
    <xsl:variable name="owner" select="$variable/../.." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name">
        <xsl:choose>
          <xsl:when test="$owner/self::group">
            <xsl:call-template name="singular"><xsl:with-param name="name" select="$owner/@name" /></xsl:call-template>
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="$owner/@name" /></xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="concat(' ', $variable/@name)" />
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- A screen, a sound and a look are each called by their name in the
       file: "playing", "wall", "smart". -->
  <xsl:template name="screen-name">
    <xsl:param name="state" />
    <xsl:value-of select="concat('&quot;', $state, '&quot;')" />
  </xsl:template>
  <xsl:template name="sound-name">
    <xsl:param name="sound" />
    <xsl:value-of select="concat('&quot;', $sound, '&quot;')" />
  </xsl:template>
  <xsl:template name="look-name">
    <xsl:param name="object" />
    <xsl:param name="look" />
    <xsl:value-of select="concat('&quot;', $look, '&quot;')" />
  </xsl:template>

  <!-- wHeld, pad2UpHeld: whether that button is down. -->
  <xsl:template name="held-name">
    <xsl:param name="button" />
    <xsl:call-template name="camel"><xsl:with-param name="name" select="$button" /></xsl:call-template>
    <xsl:text>Held</xsl:text>
  </xsl:template>

  <!-- A pad's button (pad2.up) as the test in pad(): pad 2 and button "up". -->
  <xsl:template name="pad-test">
    <xsl:param name="button" />
    <xsl:value-of select="concat('pad === ', substring-before(substring-after($button, 'pad'), '.'), ' &amp;&amp; button === &quot;', substring-after($button, '.'), '&quot;')" />
  </xsl:template>

  <!-- A key's name as the page's KeyboardEvent.code ("KeyW", "ArrowUp",
       "Digit1", "ShiftLeft", "PageUp", "Numpad4", "F1"); a mouse button, a
       pad or a sign key not yet. -->
  <xsl:template name="key-code">
    <xsl:param name="button" />
    <xsl:variable name="first" select="substring($button, 1, 1)" />
    <xsl:variable name="rest" select="substring($button, 2)" />
    <xsl:variable name="named" select="exsl:node-set($key-names)/key[@button = $button]" />
    <xsl:text>"</xsl:text>
    <xsl:choose>
      <xsl:when test="$named"><xsl:value-of select="$named" /></xsl:when>
      <xsl:when test="string-length($button) = 1 and contains($digits, $button)">
        <xsl:value-of select="concat('Digit', $button)" />
      </xsl:when>
      <xsl:when test="string-length($button) = 1 and contains($lower, $button)">
        <xsl:value-of select="concat('Key', translate($button, $lower, $upper))" />
      </xsl:when>
      <xsl:when test="starts-with($button, 'num') and string-length($button) = 4 and contains($digits, substring($button, 4))">
        <xsl:value-of select="concat('Digit', substring($button, 4))" />
      </xsl:when>
      <xsl:when test="starts-with($button, 'numpad') and string-length($button) = 7 and contains($digits, substring($button, 7))">
        <xsl:value-of select="concat('Numpad', substring($button, 7))" />
      </xsl:when>
      <xsl:when test="$first = 'f' and string(number($rest)) != 'NaN'">
        <xsl:value-of select="concat('F', $rest)" />
      </xsl:when>
      <xsl:when test="($first = 'l' or $first = 'r') and contains('|control|shift|alt|system|', concat('|', $rest, '|'))">
        <xsl:choose>
          <xsl:when test="$rest = 'system'">Meta</xsl:when>
          <xsl:otherwise><xsl:value-of select="concat(translate(substring($rest, 1, 1), $lower, $upper), substring($rest, 2))" /></xsl:otherwise>
        </xsl:choose>
        <xsl:choose>
          <xsl:when test="$first = 'l'">Left</xsl:when>
          <xsl:otherwise>Right</xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">web-js cannot write the button '<xsl:value-of select="$button" />' yet; use web-asm for this game</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>"</xsl:text>
  </xsl:template>

  <!-- The keys whose page names are not made from the game's. -->
  <xsl:variable name="key-names">
    <key button="up">ArrowUp</key>
    <key button="down">ArrowDown</key>
    <key button="left">ArrowLeft</key>
    <key button="right">ArrowRight</key>
    <key button="space">Space</key>
    <key button="enter">Enter</key>
    <key button="escape">Escape</key>
    <key button="tab">Tab</key>
    <key button="backspace">Backspace</key>
    <key button="pageup">PageUp</key>
    <key button="pagedown">PageDown</key>
    <key button="home">Home</key>
    <key button="end">End</key>
    <key button="insert">Insert</key>
    <key button="delete">Delete</key>
    <key button="pause">Pause</key>
  </xsl:variable>
</xsl:stylesheet>
