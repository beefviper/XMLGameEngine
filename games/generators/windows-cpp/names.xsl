<?xml version="1.0" encoding="UTF-8"?>
<!-- names.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- What the parts of a game are called in its C++: the class, the objects
     (a group's members each one of their own, taking the group's sprite,
     rules and actions), and the tests of which object is which. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- The one state's objects, in the file's order: its objects and its
       groups' members. Their place in this list is their place in the
       game's objects. -->
  <xsl:variable name="state" select="/game/states/state[1]" />
  <xsl:variable name="objects" select="$state/objects/object | $state/objects/group/member" />

  <!-- The class the game is: its window's name, in words run together
       (Pong, SpaceRace). -->
  <xsl:template name="class-name">
    <xsl:variable name="camel">
      <xsl:call-template name="camel">
        <xsl:with-param name="name" select="/game/window/@name" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat(translate(substring($camel, 1, 1), $lower, $upper), substring($camel, 2))" />
  </xsl:template>

  <!-- The program's namespace: its name, made a C++ name (pong_min). -->
  <xsl:template name="namespace-name">
    <xsl:value-of select="translate($name, '-. ', '___')" />
  </xsl:template>

  <!-- An object's (or member's) name in C++, which is also its accessor:
       ball, paddle1. -->
  <xsl:template name="object-name">
    <xsl:param name="object" select="." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name" select="$object/@name" />
    </xsl:call-template>
  </xsl:template>

  <!-- Its full name, as the engine names it: under its state (playing.ball). -->
  <xsl:template name="full-name">
    <xsl:param name="object" select="." />
    <xsl:value-of select="concat($state/@name, '.', $object/@name)" />
  </xsl:template>

  <!-- Its place in the game's objects, from 0. -->
  <xsl:template name="index-of">
    <xsl:param name="object" select="." />
    <xsl:for-each select="$objects">
      <xsl:if test="generate-id(.) = generate-id($object)"><xsl:value-of select="position() - 1" /></xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- The test of whether an object is this one (isBall) or in this group
       (inPaddles), as the rules ask it. -->
  <xsl:template name="test-name">
    <xsl:param name="owner" select="." />
    <xsl:variable name="camel">
      <xsl:call-template name="camel">
        <xsl:with-param name="name" select="$owner/@name" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="word">
      <xsl:choose>
        <xsl:when test="local-name($owner) = 'group'">in</xsl:when>
        <xsl:otherwise>is</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:value-of select="concat($word, translate(substring($camel, 1, 1), $lower, $upper), substring($camel, 2))" />
  </xsl:template>

  <!-- Whether an object's start draws a <random>, so a reset draws it again. -->
  <xsl:template name="draws-random">
    <xsl:param name="object" select="." />
    <xsl:if test="$object/variables/variable[random] or $object/position//random or $object/velocity//random">yes</xsl:if>
  </xsl:template>

  <!-- A key's name as a KeyCode (w W, up Up, 1 Num1, lshift LShift,
     pageup PageUp, numpad4 Numpad4); a mouse button, a pad or a sign key
     not yet. -->
  <xsl:template name="key-code">
    <xsl:param name="button" />
    <xsl:variable name="first" select="substring($button, 1, 1)" />
    <xsl:choose>
      <xsl:when test="string-length($button) = 1 and contains($digits, $button)">
        <xsl:value-of select="concat('KeyCode::Num', $button)" />
      </xsl:when>
      <xsl:when test="translate($button, concat($lower, $digits), '') != ''">
        <xsl:message terminate="yes">windows-cpp cannot write the button '<xsl:value-of select="$button" />' yet; use engine-cpp for this game</xsl:message>
      </xsl:when>
      <xsl:when test="($first = 'l' or $first = 'r') and contains('|control|shift|alt|system|', concat('|', substring($button, 2), '|'))">
        <xsl:value-of select="concat('KeyCode::', translate($first, $lower, $upper), translate(substring($button, 2, 1), $lower, $upper), substring($button, 3))" />
      </xsl:when>
      <xsl:when test="starts-with($button, 'page')">
        <xsl:value-of select="concat('KeyCode::Page', translate(substring($button, 5, 1), $lower, $upper), substring($button, 6))" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('KeyCode::', translate($first, $lower, $upper), substring($button, 2))" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
