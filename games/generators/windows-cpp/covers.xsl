<?xml version="1.0" encoding="UTF-8"?>
<!-- covers.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- What windows-cpp has words for yet, said as the parts of a game it
     covers; web-js, its twin in JavaScript, walks the game with it too. Before it writes anything the generator walks the game in mode
     covers: a part it covers goes on to its attributes, children and text;
     any other is a gap, and the walk does not go into it. Every gap, once
     each, is listed and the generator stops (generate.xsl), so nothing is
     written half way or approximated. A verb says where it is covered in its
     own file (verbs/). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="quote">'</xsl:variable>

  <!-- One gap: the part, and the object it is in. -->
  <xsl:template name="gap">
    <xsl:param name="what" />
    <xsl:variable name="owner" select="ancestor-or-self::*[self::object or self::group or self::member][1]" />
    <gap>
      <xsl:value-of select="$what" />
      <xsl:if test="$owner">
        <xsl:value-of select="concat(' (', local-name($owner), ' ', $quote, $owner/@name, $quote, ')')" />
      </xsl:if>
    </gap>
  </xsl:template>

  <!-- Not covered: named, and not gone into. -->
  <xsl:template match="*" mode="covers">
    <xsl:call-template name="gap">
      <xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; in &lt;', local-name(..), '&gt;')" />
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="@*" mode="covers">
    <xsl:call-template name="gap">
      <xsl:with-param name="what" select="concat(local-name(), '= on &lt;', local-name(..), '&gt;')" />
    </xsl:call-template>
  </xsl:template>
  <xsl:template match="text()" mode="covers" />

  <!-- Gaps said better than by their tag; one that holds parts like the
       covered ones goes on into them, so they are listed too. -->
  <xsl:template match="game/states" mode="covers" priority="2">
    <xsl:if test="count(state[objects//*[count(. | $things | $grids) = count($things | $grids)] or computer or .//trigger]) &gt; 1">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'things moving in more than one &lt;state&gt;'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <!-- Looks: texts, or rectangles of one colour (only their size
       differs). -->
  <xsl:template match="object[count(sprite) &gt; 1] | group[count(sprite) &gt; 1]" mode="covers" priority="2">
    <xsl:variable name="color" select="string(sprite[1]/rectangle/color)" />
    <xsl:if test="self::group or not(not(sprite[not(text/content)])
        or (not(sprite[not(rectangle)]) and not(sprite/rectangle[string(color) != $color])))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'looks other than texts or rectangles of one colour'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="game/variables/variable[.//random]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;random&gt; in the game variables'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="group/collisions/collision[not(@edge)]" mode="covers" priority="2">
    <xsl:variable name="grid" select="ancestor::group[parent::objects]" />
    <xsl:choose>
      <xsl:when test="count($grid | $grids) = count($grids) and @object and $things[@name = current()/@object] and *[last()][self::die] and not(ancestor::group[2])">
        <xsl:apply-templates select="@* | * | text()" mode="covers" />
      </xsl:when>
      <xsl:when test="../../member and @class and $grids[@class = current()/@class]">
        <xsl:apply-templates select="@* | * | text()" mode="covers" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; on a touch in a group, other than a grid meeting one thing and its cell dying, or members meeting a grid'" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="group/collisions/collision[@edge][ancestor::group[parent::objects][not(member)]][*[not(self::wrap)]]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; at an edge in a grid, other than wrapping round'" /></xsl:call-template>
  </xsl:template>

  <!-- A group in a grid: one kind of its cells, in rows of their own, which
       may give them a colour, a place, numbers and a sound when they meet. -->
  <xsl:template match="objects/group/group" mode="covers" priority="2">
    <xsl:choose>
      <xsl:when test="count(.. | $grids) = count($grids)">
        <xsl:apply-templates select="@* | * | text()" mode="covers" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;group&gt; in a &lt;group&gt; other than a grid'" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="group/group/collisions/collision" mode="covers" priority="3">
    <xsl:if test="@edge or not(@object) or *[not(self::play)] or count(*) != 1 or not(../../../collisions/collision[@object = current()/@object])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; in a grid group other than one sound where the grid meets that thing'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="group/group/columns | group/group/rows | group/group/padding | group/group/velocity | group/group/group" mode="covers" priority="3">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; in a grid group')" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="group/group/sprite[*[not(self::rectangle)] or rectangle/*[not(self::color)]]" mode="covers" priority="3">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;sprite&gt; in a grid group other than a colour'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="member/variables | member/sprite" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; of a member')" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="text[count(content | number) != 1 and not(parent::sprite/parent::group[rows]/row/sprite/text/content)] | text[not(@style)][not(parent::sprite/parent::row)]
      | row/sprite/text[@style or color or count(content) != 1] | style[font or not(size)]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a text other than one &lt;content&gt; or &lt;number&gt; in a style with a size'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="sound[*[not(self::note or self::volume)] or note/@*[not(local-name() = 'pitch' or local-name() = 'to' or local-name() = 'wave')]
      or (@wave | note/@wave)[not(. = 'square' or . = 'triangle' or . = 'sine' or . = 'sawtooth' or . = 'noise')] or @*[not(local-name() = 'name' or local-name() = 'wave')]]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;sound&gt; other than notes of the usual waves'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="keeps/keep" mode="covers" priority="2">
    <xsl:if test="not($everything[@name = current()/@object][count(. | $things) != count($things)])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;keep&gt; of a thing that moves'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="computer[not(@seat)] | state[count(computer) &gt; 1]/computer" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;computer&gt; other than one for a seat'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="conditions/condition[count(condition) &gt; 1 or first or otherwise or (condition and *[not(self::condition or self::any or self::remaining
      or self::atleast or self::atmost or self::above or self::below or self::equal or self::unequal)])]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a state &lt;condition&gt; holding conditions'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="input[trigger and *[not(self::button or self::trigger)]] | input[@on]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a key that triggers and does more, or on= a moment'" /></xsl:call-template>
  </xsl:template>
  <!-- The mouse across the window, steering a thing. -->
  <xsl:template match="input[@axis]" mode="covers" priority="3">
    <xsl:if test="@axis != 'mouse.x' or * [not(self::trigger)] or count(trigger) != 1
        or not($things[@name = current()/trigger/@object]/actions/action[@name = current()/trigger/@action][count(*) = 1]/track[not(node())])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'an axis other than mouse.x steering a thing by &lt;track /&gt;'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="centered[offset and *[not(self::offset)]] | centered[count(*[not(self::offset)]) &gt; 1] | centered[not(ancestor::*[sprite][1]/sprite/rectangle or ancestor::*[sprite][1]/sprite/text or ancestor::*[sprite][1]/sprite/bitmap)]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;centered&gt; other than on a rectangle, a text or a picture, on the window or on a value'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="group[not(member) and not(sprite/text)][(columns &gt; 1 and position/x//centered) or (rows &gt; 1 and position/y//centered)]" mode="covers" priority="3">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;centered&gt; across a block of more than one'" /></xsl:call-template>
  </xsl:template>

  <!-- Touching by the lit pixels of a picture. -->
  <xsl:template match="collisions/type" mode="covers" priority="2">
    <xsl:if test="normalize-space(.) != 'pixel' or not(../../sprite/bitmap)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a collision &lt;type&gt; other than pixel on a picture'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- A grid's rows by number: a way across of their own. -->
  <xsl:template match="row[number]" mode="covers" priority="2">
    <xsl:if test="not(../parent::objects) or count(.. | $grids) != count($grids) or *[not(self::number or self::velocity)] or velocity/*[not(self::x)] or velocity/x/*">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;row&gt; by number other than a way across in a grid'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- No more than so many of a grid's cells left. -->
  <xsl:template match="condition/remaining" mode="covers" priority="2">
    <xsl:if test="not($grids[@name = current()/../@object]) or ../@class or string(number(.)) = 'NaN'">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;remaining&gt; other than of a grid'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- The parts covered: the game, its window and numbers, its states of
       objects and groups (rectangles, blocks of them and texts), their
       rules, actions and keys, the seats and the computer's, the screens on
       the stack, the sounds and what is kept. -->
  <xsl:template mode="covers" match="
      game | game/window | window/width | window/height | window/framerate
      | game/variables | variables/variable | game/styles | styles/style | style/size | style/color
      | game/keeps | game/sounds | sounds/sound | sound/note
      | game/states | states/objects | states/state | state/objects | state/inputs | state/shows | shows/show
      | state/conditions | conditions/condition | state/computer | computer/condition | condition/condition
      | objects/object | objects/group | group/member
      | object/sprite | object/position | object/velocity | object/collisions | object/variables | object/actions
      | group/sprite | group/position | group/collisions | group/actions | group/variables
      | group/columns | group/rows | group/padding | padding/x | padding/y | group/group | group/row | row/sprite
      | sound/volume | text/color | x/right | x/left | y/top | y/bottom | right/offset | left/offset | top/offset | bottom/offset
      | member/position | member/velocity
      | sprite/rectangle | rectangle/width | rectangle/height | rectangle/color
      | sprite/text | text/content | text/number | text/size
      | game/pictures | pictures/bitmap | bitmap/row | bitmap/scale | sprite/bitmap
      | position/x | position/y | velocity/x | velocity/y
      | collisions/collision | collision/any | collision/condition | condition/any | any/condition | any/any
      | condition/atleast | condition/atmost | condition/above | condition/below | condition/equal | condition/unequal
      | actions/action | inputs/input | inputs/seat | seat/input | input/button
      | add | subtract | multiply | divide | sign | sine | cosine | value
      | random | random/min | random/max | centered | centered/offset
      | value/window | x/window | y/window | width/window | height/window | min/window | max/window
      | atleast/window | atmost/window | above/window | below/window | equal/window | unequal/window
      | window/right | window/bottom | window/x | window/y | window/x/center | window/y/center">
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template mode="covers" match="
      game/@*[namespace-uri() != ''] | window/@name | variable/@name | state/@name | style/@name | sound/@name | note/@pitch
      | object/@name | group/@name | member/@name | group/@class | object/@class | sprite/@name | text/@style
      | action/@name | collision/@edge | collision/@class | collision/@object | input/@button | seat/@number | computer/@seat
      | row/@number | bitmap/@name | bitmap/@from | note/@to | note/@wave | sound/@wave
      | show/@object | show/@state | condition/@object | condition/@class | condition/@variable" />

  <!-- A name in a value must be one the program has: a number, a
       variable of the game, an object's variable (one drawn at random only
       where it starts), a thing's x, y, velocity.x, velocity.y, width or
       height, or a text's own width or height. -->
  <xsl:template mode="covers" match="text()[normalize-space(.) != ''][parent::width or parent::height or parent::x or parent::y
      or parent::value or parent::min or parent::max or parent::variable or parent::move or parent::bounce or parent::deflect
      or parent::offset or parent::number or parent::set or parent::inc or parent::atleast or parent::atmost or parent::above
      or parent::below or parent::equal or parent::unequal]">
    <xsl:variable name="text" select="normalize-space(.)" />
    <xsl:variable name="first" select="substring-before($text, '.')" />
    <xsl:variable name="rest" select="substring-after($text, '.')" />
    <xsl:variable name="owner" select="$everything[@name = $first]" />
    <xsl:variable name="variable" select="($owner/variables/variable | $owner/parent::group/variables/variable)[@name = $rest]" />
    <xsl:variable name="here" select="ancestor::*[self::object or self::member or self::group][1]" />
    <xsl:variable name="random-here" select="$variable//random and ($here = $owner or $here/member = $owner) and (ancestor::position or ancestor::velocity or ancestor::variable)" />
    <xsl:if test="not(string(number($text)) != 'NaN'
        or /game/variables/variable[@name = $text]
        or ($variable and (not($variable//random) or $random-here or $written[. = $text]))
        or ($owner and count($owner | $things) = count($things) and ($rest = 'x' or $rest = 'y' or $rest = 'velocity.x' or $rest = 'velocity.y' or $rest = 'width' or $rest = 'height'))
        or ($owner and count($owner | $texts) = count($texts) and ($rest = 'width' or $rest = 'height') and generate-id($here) = generate-id($owner) and ancestor::position))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('the name ', $quote, $text, $quote)" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template mode="covers" match="text()[normalize-space(.) != ''][parent::content or parent::size or parent::color or parent::note or parent::columns or parent::rows or parent::volume
      or parent::row[parent::bitmap] or parent::scale or parent::type or parent::number[parent::row]]" />

  <!-- <self> names one of the object's own variables, or a grid cell's. -->
  <xsl:template mode="covers" match="self">
    <xsl:variable name="self" select="ancestor::*[self::object or self::member or self::group[count(. | $grids) = count($grids)]][1]" />
    <xsl:if test="not(($self/variables/variable | $self/parent::group/variables/variable | $self/group/variables/variable)[@name = normalize-space(current())])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;self&gt;', normalize-space(.), '&lt;/self&gt;')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- A button: a key, or a pad's in a seat 1 or 2 or named for its pad;
       not the mouse. -->
  <xsl:template mode="covers" match="input/button/text() | input/@button" priority="2">
    <xsl:variable name="button"><xsl:call-template name="button-of"><xsl:with-param name="button" select="(self::text()/.. | self::*)[1] | self::text()/.." /></xsl:call-template></xsl:variable>
    <xsl:variable name="padded" select="starts-with($button, 'pad1.') or starts-with($button, 'pad2.')" />
    <xsl:if test="starts-with($button, 'mouse') or (starts-with($button, 'pad') and not($padded))
        or translate($button, concat($lower, $digits, '.'), '') != ''">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('the button ', $quote, $button, $quote)" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
