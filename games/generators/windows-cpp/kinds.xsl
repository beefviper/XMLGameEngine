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
     * A scene is a row of a table of its numbers; the things it takes are
       the game's, started anew as it starts. What a thing fires is a list.
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

  <!-- Pieces: the top-level objects. A scene takes some (its own copy
       each, started anew when the scene is), a thing fires others: a list
       of them in flight. -->
  <xsl:variable name="pieces" select="/game/objects/object" />
  <xsl:variable name="scenes" select="$states/scenes/scene" />
  <xsl:variable name="taken" select="$pieces[@name = $scenes/objects/object/@name]" />
  <xsl:variable name="fired" select="$pieces[@name = //fire/@object]" />

  <!-- every object and member in the game, the pieces the scenes take and
       the shared ones first, in the file's order; not what is fired -->
  <xsl:variable name="everything" select="$taken | /game/states/objects/object | $states/objects/object | $states/objects/group/member" />

  <!-- the names commands change: set, inc and dec say a variable, copy into one -->
  <xsl:variable name="written" select="//set/@variable | //inc/@variable | //dec/@variable | //copy/@into" />

  <!-- the classes rules meet -->
  <xsl:variable name="met-classes" select="//collision/@class" />

  <!-- Things: rectangles or pictures that move, meet or count, in the
       file's order. -->
  <xsl:variable name="things" select="$everything[(sprite | parent::group/sprite)[1][rectangle or bitmap]]
      [velocity or collisions or actions or variables or timers
       or parent::group[velocity or collisions or actions or variables]
       or (@class | parent::group/@class) = $met-classes
       or @name = //trigger/@object]" />

  <!-- Grids: a group of cells in rows and columns, in groups of its own
       (one kind of cell each) or not, whose cells meet: a list of them. -->
  <xsl:variable name="grids" select="$states/objects/group[not(member)][(sprite | group/sprite)/rectangle or sprite/bitmap]
      [collisions or group/collisions or @class = $met-classes or @name = //collision/@object]" />

  <!-- Pictures: bitmaps of the game's things, square pixels a row a line. -->
  <xsl:variable name="pictures" select="/game/pictures/bitmap[@name = ($everything | $fired)/sprite/bitmap/@from or @name = $everything/parent::group/sprite/bitmap/@from
      or @name = $grids/descendant::bitmap/@from]" />

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

  <!-- The scenes' numbers: those of the first, which every scene names. -->
  <xsl:variable name="scene-numbers" select="$scenes[1]/variables/variable" />

  <!-- One of the scenes, as many of them are called: wave for wave1, or
       scene. -->
  <xsl:template name="scene-word">
    <xsl:variable name="word" select="translate($scenes[1]/@name, $digits, '')" />
    <xsl:choose>
      <xsl:when test="$word != '' and $word != $scenes[1]/@name"><xsl:value-of select="$word" /></xsl:when>
      <xsl:otherwise>scene</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A scene's number is whole (int) when it is in every scene and is not
       a speed, a place or a time. -->
  <xsl:template name="scene-number-whole">
    <xsl:param name="named" select="@name" />
    <xsl:variable name="values" select="$scenes/variables/variable[@name = $named]" />
    <xsl:if test="not($values[* or floor(number(.)) != number(.)])
        and not(//*[self::velocity or self::position or self::every or self::move or self::seconds]//text()[normalize-space(.) = $named])">yes</xsl:if>
  </xsl:template>

  <!-- |wave|upto|: the scenes' whole numbers. -->
  <xsl:variable name="whole-scene-numbers">
    <xsl:text>|</xsl:text>
    <xsl:for-each select="$scene-numbers">
      <xsl:variable name="whole"><xsl:call-template name="scene-number-whole" /></xsl:variable>
      <xsl:if test="$whole = 'yes'"><xsl:value-of select="concat(@name, '|')" /></xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <!-- A scene's conditions, as <c id="" /> each: those of the scene it is
       from= (one of the same name its own instead), then its own others. -->
  <xsl:template name="scene-conditions">
    <xsl:param name="scene" />
    <xsl:variable name="own" select="$scene/conditions/condition" />
    <xsl:variable name="copied">
      <xsl:if test="$scene/@from">
        <xsl:call-template name="scene-conditions"><xsl:with-param name="scene" select="$scenes[@name = $scene/@from]" /></xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($copied)/c">
      <xsl:choose>
        <xsl:when test="@name != '' and $own[@name = current()/@name]"><c id="{generate-id($own[@name = current()/@name])}" name="{@name}" /></xsl:when>
        <xsl:otherwise><xsl:copy-of select="." /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:for-each select="$own[not(@name) or not(@name = exsl:node-set($copied)/c/@name)]">
      <c id="{generate-id()}" name="{@name}" />
    </xsl:for-each>
  </xsl:template>

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
    <xsl:if test="contains($kinds, 'live') or count($object/sprite) &gt; 1 or $object/timers/timer">yes</xsl:if>
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

  <!-- What else its kinds give of their own, each a 'yes': a way across
       (a lane's speed), a count of columns, a padding across, a picture of
       another size. Shared by the grids of each target. -->
  <xsl:template name="kind-speed">
    <xsl:param name="grid" select="." />
    <xsl:if test="$grid/group/velocity">yes</xsl:if>
  </xsl:template>
  <xsl:template name="kind-count">
    <xsl:param name="grid" select="." />
    <xsl:if test="$grid/group/columns">yes</xsl:if>
  </xsl:template>
  <xsl:template name="kind-gap">
    <xsl:param name="grid" select="." />
    <xsl:if test="$grid/group/padding/x">yes</xsl:if>
  </xsl:template>
  <xsl:template name="kind-size">
    <xsl:param name="grid" select="." />
    <xsl:variable name="sizes">
      <xsl:for-each select="$grid/group">
        <xsl:call-template name="kind-size-of" />
        <xsl:text>|</xsl:text>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="first"><xsl:for-each select="$grid/group[1]"><xsl:call-template name="kind-size-of" /></xsl:for-each></xsl:variable>
    <xsl:if test="$grid/group[2]">
      <xsl:call-template name="sizes-differ"><xsl:with-param name="sizes" select="$sizes" /><xsl:with-param name="first" select="$first" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template name="sizes-differ">
    <xsl:param name="sizes" />
    <xsl:param name="first" />
    <xsl:choose>
      <xsl:when test="$sizes = ''" />
      <xsl:when test="substring-before($sizes, '|') != $first">yes</xsl:when>
      <xsl:otherwise><xsl:call-template name="sizes-differ"><xsl:with-param name="sizes" select="substring-after($sizes, '|')" /><xsl:with-param name="first" select="$first" /></xsl:call-template></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A kind's picture: its own, or the grid's. -->
  <xsl:template name="kind-picture">
    <xsl:param name="kind" select="." />
    <xsl:value-of select="($kind/sprite/bitmap/@from | $kind/../sprite/bitmap/@from)[last()]" />
  </xsl:template>

  <!-- A kind's picture's scale: its own, else the grid's picture's, as a
       kind's picture takes what it leaves out from the grid's. -->
  <xsl:template name="kind-scale">
    <xsl:param name="kind" select="." />
    <xsl:variable name="named"><xsl:call-template name="kind-picture"><xsl:with-param name="kind" select="$kind" /></xsl:call-template></xsl:variable>
    <xsl:variable name="own" select="/game/pictures/bitmap[@name = $named]" />
    <xsl:variable name="base" select="/game/pictures/bitmap[@name = $kind/../sprite/bitmap/@from]" />
    <xsl:call-template name="picture-scale"><xsl:with-param name="bitmap" select="($own[scale] | $base[not($own/scale)] | $own[not($base)])[1]" /></xsl:call-template>
  </xsl:template>

  <!-- A kind's picture's colours, a letter's own or the rest's: the grid's
       picture's, then its own picture's, then a colour on its sprite. -->
  <xsl:template name="kind-colors">
    <xsl:param name="kind" select="." />
    <xsl:variable name="named"><xsl:call-template name="kind-picture"><xsl:with-param name="kind" select="$kind" /></xsl:call-template></xsl:variable>
    <xsl:variable name="own" select="/game/pictures/bitmap[@name = $named]" />
    <xsl:variable name="base" select="/game/pictures/bitmap[@name = $kind/../sprite/bitmap/@from][count(. | $own) != count($own)]" />
    <xsl:variable name="rest" select="($kind/sprite/bitmap/color[not(@char)] | $own/color[not(@char)][not($kind/sprite/bitmap/color[not(@char)])] | $base/color[not(@char)][not($own/color[not(@char)] or $kind/sprite/bitmap/color[not(@char)])])[1]" />
    <rest><xsl:value-of select="normalize-space($rest)" /></rest>
    <xsl:for-each select="$base/color[@char][not(@char = $own/color/@char)] | $own/color[@char]">
      <letter char="{@char}"><xsl:value-of select="normalize-space(.)" /></letter>
    </xsl:for-each>
  </xsl:template>

  <!-- How many across a picture's pixel is: its <scale>, its style's, or 1. -->
  <xsl:template name="picture-scale">
    <xsl:param name="bitmap" />
    <xsl:variable name="style" select="/game/styles/style[@name = $bitmap/@style]" />
    <xsl:choose>
      <xsl:when test="$bitmap/scale"><xsl:value-of select="normalize-space($bitmap/scale)" /></xsl:when>
      <xsl:when test="$style/scale"><xsl:value-of select="normalize-space($style/scale)" /></xsl:when>
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
