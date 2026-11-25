<?xml version="1.0" encoding="UTF-8"?>
<!-- kinds.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- What each part of a game is, as a program written by hand sees it,
     whatever language it is written in; web-js reads the game with it too.

     * A thing moves or meets: a rectangle with a velocity, rules, actions or
       variables, or one a rule names by its class. The game holds it, and
       its rules are the frame's.
     * A text, or a rectangle or block of them nothing moves or meets, is
       only drawn: the window draws it where the file puts it.
     * A record is what the game remembers of one: the numbers it changes
       (live variables) and its look, when it has more than one.
     * An object's variable is a constant when nothing changes it, a local of
       its start when it is drawn at random there, and live when changed.
     * A picture is a bitmap a thing is drawn with: its rows of pixels as
       the file writes them.
     * The screens are the states; the one whose things move is played,
       the others wait for keys (and test their own conditions). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    xmlns:math="http://exslt.org/math"
    extension-element-prefixes="exsl math">

  <xsl:variable name="states" select="/game/states/state" />
  <xsl:variable name="screens" select="count($states) &gt; 1" />
  <xsl:variable name="shared" select="/game/states/objects/object" />

  <!-- every object and member in the game, the shared ones first, in the file's order -->
  <xsl:variable name="everything" select="/game/states/objects/object | $states/objects/object | $states/objects/group/member" />

  <!-- the names commands change: set, inc and dec say a variable, copy into one -->
  <xsl:variable name="written" select="//set/@variable | //inc/@variable | //dec/@variable | //copy/@into" />

  <!-- the classes rules meet -->
  <xsl:variable name="met-classes" select="//collision/@class" />

  <!-- Things: rectangles or pictures that move, meet or count, in the
       file's order. -->
  <xsl:variable name="things" select="$everything[(sprite | parent::group/sprite)[1][rectangle or bitmap]]
      [velocity or collisions or actions or variables
       or parent::group[velocity or collisions or actions or variables]
       or (@class | parent::group/@class) = $met-classes
       or @name = //trigger/@object]" />

  <!-- Grids: a group of cells in rows and columns, in groups of its own
       (one kind of cell each) or not, whose cells meet: a list of them. -->
  <xsl:variable name="grids" select="$states/objects/group[not(member)][(sprite | group/sprite)/rectangle]
      [collisions or group/collisions or @class = $met-classes or @name = //collision/@object]" />

  <!-- Pictures: bitmaps of the game's things, square pixels a row a line. -->
  <xsl:variable name="pictures" select="/game/pictures/bitmap[@name = $everything/sprite/bitmap/@from or @name = $everything/parent::group/sprite/bitmap/@from]" />

  <!-- Rows of words: a group of texts, a row's words each. -->
  <xsl:variable name="text-rows" select="$states/objects/group[not(member)][sprite/text]" />

  <!-- The state that is played: its things move, its rules run. -->
  <xsl:variable name="played" select="($states[objects//*[count(. | $things | $grids) = count($things | $grids)] or conditions or computer or .//trigger])[1]" />

  <!-- Texts. -->
  <xsl:variable name="texts" select="$everything[(sprite | parent::group/sprite)[1]/text]" />

  <!-- Records: what has looks of its own, or live variables. -->
  <xsl:variable name="looked" select="$everything[count(sprite) &gt; 1]" />

  <xsl:variable name="record-list">
    <xsl:for-each select="$everything">
      <xsl:variable name="has"><xsl:call-template name="has-record" /></xsl:variable>
      <xsl:if test="$has = 'yes'"><name><xsl:value-of select="@name" /></name></xsl:if>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="records" select="$everything[@name = exsl:node-set($record-list)/name]" />

  <xsl:variable name="kept" select="$everything[@name = /game/keeps/keep/@object]" />

  <!-- 'live', 'constant' or 'start' for the variable that is the context:
       a variable of an object or of a group. -->
  <xsl:template name="variable-kind">
    <xsl:param name="variable" select="." />
    <xsl:variable name="owner" select="$variable/../.." />
    <xsl:variable name="named" select="$variable/@name" />
    <xsl:choose>
      <xsl:when test="$written[substring-after(., '.') = $named and (substring-before(., '.') = $owner/@name or substring-before(., '.') = $owner/member/@name)]
          or $owner//*[self::set or self::inc or self::dec][@variable = $named]">live</xsl:when>
      <xsl:when test="$variable//random">start</xsl:when>
      <xsl:otherwise>constant</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Whether the object (or member) that is the context has a record:
       live variables, its own or its group's, or looks. -->
  <xsl:template name="has-record">
    <xsl:param name="object" select="." />
    <xsl:variable name="kinds">
      <xsl:for-each select="$object/variables/variable | $object/parent::group/variables/variable">
        <xsl:call-template name="variable-kind" />
      </xsl:for-each>
    </xsl:variable>
    <xsl:if test="contains($kinds, 'live') or count($object/sprite) &gt; 1">yes</xsl:if>
  </xsl:template>

  <!-- The object a name's first part names, or its group: ball, paddle1, paddles. -->
  <xsl:template name="named-owner">
    <xsl:param name="name" />
    <xsl:copy-of select="$everything[@name = $name] | /game/states/objects/group[@name = $name] | $states/objects/group[@name = $name]" />
  </xsl:template>

  <!-- A button as the input it is in means it: pad.up in seat 2 is pad2.up. -->
  <xsl:template name="button-of">
    <xsl:param name="button" select="." />
    <xsl:variable name="text" select="normalize-space($button)" />
    <xsl:variable name="seat" select="$button/ancestor::seat[1]/@number" />
    <xsl:choose>
      <xsl:when test="starts-with($text, 'pad.') and $seat"><xsl:value-of select="concat('pad', $seat, substring($text, 4))" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$text" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The singular of a group's name, for one of its members: paddles a paddle. -->
  <xsl:template name="singular">
    <xsl:param name="name" />
    <xsl:choose>
      <xsl:when test="substring($name, string-length($name)) = 's'"><xsl:value-of select="substring($name, 1, string-length($name) - 1)" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat($name, '-one')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- How many across a picture's pixel is: its <scale>, or 1. -->
  <xsl:template name="picture-scale">
    <xsl:param name="bitmap" />
    <xsl:choose>
      <xsl:when test="$bitmap/scale"><xsl:value-of select="normalize-space($bitmap/scale)" /></xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A pitch in hertz: a number as it is, or a note (a letter, a sharp or
       flat, an octave; A4 is 440) to a hundredth. -->
  <xsl:template name="hertz">
    <xsl:param name="pitch" />
    <xsl:choose>
      <xsl:when test="string(number($pitch)) != 'NaN'"><xsl:value-of select="$pitch" /></xsl:when>
      <xsl:otherwise>
        <xsl:variable name="letter" select="translate(substring($pitch, 1, 1), 'abcdefg', 'ABCDEFG')" />
        <xsl:variable name="accidental" select="substring($pitch, 2, 1)" />
        <xsl:variable name="shift" select="number($accidental = '#') - number($accidental = 'b')" />
        <xsl:variable name="octave" select="number(substring($pitch, 2 + number($shift != 0)))" />
        <xsl:variable name="letter-at" select="string-length(substring-before('A_B_C_D_E_F_G', $letter)) div 2" />
        <xsl:variable name="midi" select="($octave + 1) * 12 + number(substring('09110002040507', 1 + 2 * $letter-at, 2)) + $shift" />
        <xsl:value-of select="format-number(440 * math:power(2, ($midi - 69) div 12), '0.##')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
