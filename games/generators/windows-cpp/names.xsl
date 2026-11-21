<?xml version="1.0" encoding="UTF-8"?>
<!-- names.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- What the parts of a game are called in its C++: the class, its
     namespace, the objects (a group's members each one of their own, taking
     the group's sprite, rules and actions) and the keys. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- The one state's objects, in the file's order: its objects and its
       groups' members, which is the order they move and are drawn in. -->
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

  <!-- An object's (or member's) name in C++: ball, paddle1. -->
  <xsl:template name="object-name">
    <xsl:param name="object" select="." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name" select="$object/@name" />
    </xsl:call-template>
  </xsl:template>





  <!-- A key's name as SFML's (w Key::W, up Key::Up, 1 Key::Num1, lshift LShift,
     pageup PageUp, numpad4 Numpad4); a mouse button, a pad or a sign key
     not yet. -->
  <xsl:template name="key-code">
    <xsl:param name="button" />
    <xsl:variable name="first" select="substring($button, 1, 1)" />
    <xsl:choose>
      <xsl:when test="string-length($button) = 1 and contains($digits, $button)">
        <xsl:value-of select="concat('Key::Num', $button)" />
      </xsl:when>
      <xsl:when test="translate($button, concat($lower, $digits), '') != ''">
        <xsl:message terminate="yes">windows-cpp cannot write the button '<xsl:value-of select="$button" />' yet; use engine-cpp for this game</xsl:message>
      </xsl:when>
      <xsl:when test="($first = 'l' or $first = 'r') and contains('|control|shift|alt|system|', concat('|', substring($button, 2), '|'))">
        <xsl:value-of select="concat('Key::', translate($first, $lower, $upper), translate(substring($button, 2, 1), $lower, $upper), substring($button, 3))" />
      </xsl:when>
      <xsl:when test="starts-with($button, 'page')">
        <xsl:value-of select="concat('Key::Page', translate(substring($button, 5, 1), $lower, $upper), substring($button, 6))" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('Key::', translate($first, $lower, $upper), substring($button, 2))" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
