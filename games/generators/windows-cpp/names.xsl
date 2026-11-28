<?xml version="1.0" encoding="UTF-8"?>
<!-- names.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- What the parts of a game are called in its C++: the class, its
     namespace, the objects (a group's members each one of their own, taking
     the group's sprite, rules and actions) and their types, the screens,
     sounds and looks, and the keys. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- The class the game is: its window's name, in words run together
       (Pong, SpaceRace). -->
  <xsl:template name="class-name">
    <xsl:call-template name="type-word">
      <xsl:with-param name="name" select="/game/window/@name" />
    </xsl:call-template>
  </xsl:template>

  <!-- Words run together, each with a capital: space race SpaceRace. -->
  <xsl:template name="type-word">
    <xsl:param name="name" />
    <xsl:variable name="camel">
      <xsl:call-template name="camel">
        <xsl:with-param name="name" select="$name" />
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

  <!-- What an object's record is called: Ball, or for a group's member
       the group's one, Paddle; a grid's cell, its class or its one, Brick. -->
  <xsl:template name="record-name">
    <xsl:param name="object" select="." />
    <xsl:call-template name="type-word">
      <xsl:with-param name="name">
        <xsl:choose>
          <xsl:when test="$object/self::group[@class]"><xsl:call-template name="one-of-class"><xsl:with-param name="class" select="$object/@class" /></xsl:call-template></xsl:when>
          <xsl:when test="$object/self::group">
            <xsl:call-template name="singular"><xsl:with-param name="name" select="$object/@name" /></xsl:call-template>
          </xsl:when>
          <xsl:when test="$object/self::member">
            <xsl:call-template name="singular"><xsl:with-param name="name" select="$object/../@name" /></xsl:call-template>
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="$object/@name" /></xsl:otherwise>
        </xsl:choose>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- An object's type: its record, or a plain Thing. -->
  <xsl:template name="type-name">
    <xsl:param name="object" select="." />
    <xsl:choose>
      <xsl:when test="count($object | $records) = count($records)">
        <xsl:call-template name="record-name"><xsl:with-param name="object" select="$object" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>Thing</xsl:otherwise>
    </xsl:choose>
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

  <!-- One cell of a grid, as a loop names it: brick, or cell (one when cell is
       taken too) when the game has a number of that name. -->
  <xsl:template name="cell-name">
    <xsl:param name="grid" select="." />
    <xsl:variable name="named">
      <xsl:call-template name="camel">
        <xsl:with-param name="name">
          <xsl:choose>
            <xsl:when test="$grid/@class"><xsl:call-template name="one-of-class"><xsl:with-param name="class" select="$grid/@class" /></xsl:call-template></xsl:when>
            <xsl:otherwise><xsl:call-template name="singular"><xsl:with-param name="name" select="$grid/@name" /></xsl:call-template></xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="not(/game/variables/variable[@name = $named] or $everything[@name = $named] or $grid/@name = $named)"><xsl:value-of select="$named" /></xsl:when>
      <xsl:when test="/game/variables/variable[@name = 'cell'] or $everything[@name = 'cell']">one</xsl:when>
      <xsl:otherwise>cell</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What a loop over a group calls the one it is at: a grid's cell, or
     one of its members (rocket for rockets). -->
  <xsl:template name="loop-name">
    <xsl:param name="group" />
    <xsl:choose>
      <xsl:when test="$group/member">
        <xsl:call-template name="camel">
          <xsl:with-param name="name"><xsl:call-template name="singular"><xsl:with-param name="name" select="$group/@name" /></xsl:call-template></xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:call-template name="cell-name"><xsl:with-param name="grid" select="$group" /></xsl:call-template></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The list of a fired piece in flight: bombs, splashes. -->
  <xsl:template name="fired-list-name">
    <xsl:param name="piece" select="." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name"><xsl:call-template name="plural"><xsl:with-param name="name" select="$piece/@name" /></xsl:call-template></xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- What one in flight is: its record (Bomb) when it has an animation or
       numbers of its own, else a Thing. -->
  <xsl:template name="fired-type">
    <xsl:param name="piece" select="." />
    <xsl:choose>
      <xsl:when test="$piece/animation or $piece/variables/variable"><xsl:call-template name="type-word"><xsl:with-param name="name" select="$piece/@name" /></xsl:call-template></xsl:when>
      <xsl:otherwise>Thing</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A timer's count down: untilFire, by its name or its first verb;
       a screen's own, untilTick, or by what its first verb is done to,
       untilComet. -->
  <xsl:template name="timer-name">
    <xsl:param name="timer" select="." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name">
        <xsl:text>until </xsl:text>
        <xsl:choose>
          <xsl:when test="$timer/@name"><xsl:value-of select="$timer/@name" /></xsl:when>
          <xsl:when test="$timer/../parent::state and $timer/*[not(self::every)][1]/@object"><xsl:value-of select="$timer/*[not(self::every)][1]/@object" /></xsl:when>
          <xsl:when test="$timer/../parent::state">tick</xsl:when>
          <xsl:otherwise><xsl:value-of select="local-name($timer/*[not(self::every)][1])" /></xsl:otherwise>
        </xsl:choose>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <!-- How long a screen has been on top: crackedTime. -->
  <xsl:template name="screen-time">
    <xsl:param name="state" select="." />
    <xsl:call-template name="camel"><xsl:with-param name="name" select="concat($state/@name, ' time')" /></xsl:call-template>
  </xsl:template>

  <!-- A picture's pixels: rocketsPicture. -->
  <xsl:template name="picture-name">
    <xsl:param name="bitmap" select="." />
    <xsl:call-template name="camel"><xsl:with-param name="name" select="concat($bitmap/@name, ' picture')" /></xsl:call-template>
  </xsl:template>

  <!-- An action written as a function of its own: faster. -->
  <xsl:template name="action-name">
    <xsl:param name="action" />
    <xsl:call-template name="camel"><xsl:with-param name="name" select="$action" /></xsl:call-template>
  </xsl:template>

  <!-- Screen::Playing, Sound::Wall, Computer::Look::Smart. -->
  <xsl:template name="screen-name">
    <xsl:param name="state" />
    <xsl:text>Screen::</xsl:text>
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="$state" /></xsl:call-template>
  </xsl:template>
  <xsl:template name="sound-name">
    <xsl:param name="sound" />
    <xsl:text>Sound::</xsl:text>
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="$sound" /></xsl:call-template>
  </xsl:template>
  <xsl:template name="look-name">
    <xsl:param name="object" />
    <xsl:param name="look" />
    <xsl:call-template name="record-name"><xsl:with-param name="object" select="$object" /></xsl:call-template>
    <xsl:text>::Look::</xsl:text>
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="$look" /></xsl:call-template>
  </xsl:template>

  <!-- wHeld, pad2UpHeld: whether that button is down. -->
  <xsl:template name="held-name">
    <xsl:param name="button" />
    <xsl:call-template name="camel"><xsl:with-param name="name" select="$button" /></xsl:call-template>
    <xsl:text>Held</xsl:text>
  </xsl:template>

  <!-- A key's name as SFML's (w Key::W, up Key::Up, 1 Key::Num1, lshift LShift,
     pageup PageUp, numpad4 Numpad4); a mouse button or a sign key not yet. -->
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

  <!-- A pad's button (pad2.up) as the test in pad(): pad == 2 && button == PadButton::Up. -->
  <xsl:template name="pad-test">
    <xsl:param name="button" />
    <xsl:variable name="which" select="substring-after($button, '.')" />
    <xsl:value-of select="concat('pad == ', substring-before(substring-after($button, 'pad'), '.'), ' &amp;&amp; button == PadButton::')" />
    <xsl:choose>
      <xsl:when test="$which = 'lb' or $which = 'rb'"><xsl:value-of select="translate($which, $lower, $upper)" /></xsl:when>
      <xsl:otherwise><xsl:call-template name="type-word"><xsl:with-param name="name" select="$which" /></xsl:call-template></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
