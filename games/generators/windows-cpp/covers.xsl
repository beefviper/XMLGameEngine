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
  <!-- Looks: texts, rectangles of one colour (only their size differs) or
       pictures; or the pictures of an animation of what is fired. -->
  <xsl:template match="object[count(sprite) &gt; 1] | group[count(sprite) &gt; 1]" mode="covers" priority="2">
    <xsl:variable name="color" select="string(sprite[1]/rectangle/color)" />
    <xsl:variable name="animated" select="animation and (count(. | $fired) = count($fired) or count(. | $screen-animated) = count($screen-animated))" />
    <xsl:if test="(self::group and (count(. | $grids) != count($grids) or group or sprite[not(rectangle or bitmap)] or sprite/rectangle/*[not(self::width or self::height or self::color)]))
        or (self::object and not(not(sprite[not(text/content)])
        or (not(sprite[not(rectangle)]) and not(sprite/rectangle[string(color) != $color]))
        or (not(sprite[not(bitmap)]) and not(animation))
        or ($animated and not(sprite[not(@name = current()/animation/frame/@sprite)]) and not(sprite[not(bitmap)]))))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'looks other than texts, pictures or rectangles of one colour'" /></xsl:call-template>
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
      <xsl:when test="count($grid | $grids) = count($grids) and not($grid/group) and @object and $things[@name = current()/@object] and collision and not(*[not(self::collision[@sprite])])">
        <xsl:for-each select="collision">
          <xsl:if test="@*[local-name() != 'sprite'] or not($grid/sprite[@name = current()/@sprite])">
            <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; in a grid rule other than by one of its looks'" /></xsl:call-template>
          </xsl:if>
          <xsl:apply-templates select="*" mode="covers" />
        </xsl:for-each>
      </xsl:when>
      <xsl:when test="count($grid | $grids) = count($grids) and not($grid/group) and ($fired[@name = current()/@object] or $fired[@class = current()/@class])">
        <xsl:apply-templates select="@* | * | text()" mode="covers" />
      </xsl:when>
      <xsl:when test="../../member and ((@class and ($grids[@class = current()/@class] or $things[@class = current()/@class])) or $things[@name = current()/@object])">
        <xsl:apply-templates select="@* | * | text()" mode="covers" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; on a touch in a group, other than a grid meeting one thing and its cell dying or by its looks, a grid meeting what is fired, or members meeting a grid or a thing'" /></xsl:call-template>
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
  <!-- a kind's sound where the grid meets a thing, or its rule on a
       thing while it has one of its own looks -->
  <xsl:template match="group/group/collisions/collision" mode="covers" priority="3">
    <xsl:choose>
      <xsl:when test="@sprite and not(@edge or @class) and $things[@name = current()/@object] and ../../sprite[@name = current()/@sprite]">
        <xsl:apply-templates select="*" mode="covers" />
      </xsl:when>
      <xsl:when test="@edge or not(@object) or *[not(self::play)] or count(*) != 1 or not(../../../collisions/collision[@object = current()/@object])">
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; in a grid group other than one sound where the grid meets that thing, or a rule on a thing by look'" /></xsl:call-template>
      </xsl:when>
    </xsl:choose>
  </xsl:template>
  <!-- a kind's looks that last a while, each a timer on it -->
  <xsl:template match="group/group/timers" mode="covers" priority="3">
    <xsl:for-each select="timer">
      <xsl:if test="not(@sprite) or @*[local-name() != 'sprite'] or not(after) or *[not(self::after or self::become)] or not(../../sprite[@name = current()/@sprite]) or count(become) != 1
          or not(../../sprite[@name = current()/become/@sprite]) or become/@*[local-name() != 'sprite'] or string(number(after)) = 'NaN'">
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;timer&gt; in a grid group other than a look lasting a while'" /></xsl:call-template>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>
  <xsl:template match="group/group/rows | group/group/group | group/group/velocity/y | group/group/padding/y" mode="covers" priority="3">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; in a grid group')" /></xsl:call-template>
  </xsl:template>
  <!-- a kind's own columns, spacing across and way across: a lane -->
  <xsl:template match="group/group/columns | group/group/padding | group/group/velocity" mode="covers" priority="3">
    <xsl:if test="../rows or ../../rows &gt; 1">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; in a grid group of more than one row')" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="* | text()" mode="covers" />
  </xsl:template>
  <!-- a kind's look: a colour of the grid's rectangles, or a picture of its
       own or the grid's in a colour of its own -->
  <xsl:template match="group/group/sprite" mode="covers" priority="3">
    <xsl:if test="@*[local-name() != 'name'] or (@name and count(../sprite) = 1) or *[not(self::rectangle or self::bitmap)] or rectangle/*[not(self::color)] or (rectangle and not(../../sprite/rectangle))
        or bitmap/@*[local-name() != 'from'] or bitmap/*[not(self::color or self::scale)] or (bitmap and not(bitmap/@from or ../../sprite/bitmap))
        or (../sprite[2] and not(bitmap/@from))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;sprite&gt; in a grid group other than a colour or a picture'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="member/variables | member/sprite" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; of a member')" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="text[count(content) &gt; 1 or count(number) &gt; 1 or number/following-sibling::content
        or (not(content | number) and not(parent::sprite/parent::group[rows]/row/sprite/text/content))] | text[not(@style)][not(parent::sprite/parent::row)]
      | row/sprite/text[@style or count(content) != 1 or number] | style[font or (not(size) and @name = //text/@style)]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a text other than a &lt;content&gt;, a &lt;number&gt; or the two in a style with a size'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="sound[*[not(self::note or self::volume or self::part)] or (part and note) or part/@*[local-name() != 'wave'] or part/*[not(self::note)] or (note | part/note)/@*[not(local-name() = 'pitch' or local-name() = 'to' or local-name() = 'wave')]
      or (@wave | part/@wave | (note | part/note)/@wave)[not(. = 'square' or . = 'triangle' or . = 'sine' or . = 'sawtooth' or . = 'noise')] or @*[not(local-name() = 'name' or local-name() = 'wave')]]" mode="covers" priority="2">
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
  <!-- The mouse, or pad 1's stick, across the window, steering a thing. -->
  <xsl:template match="input[@axis]" mode="covers" priority="3">
    <xsl:if test="(@axis != 'mouse.x' and @axis != 'pad1.x') or * [not(self::trigger)] or count(trigger) != 1
        or not($things[@name = current()/trigger/@object]/actions/action[@name = current()/trigger/@action][count(*) = 1]/track[not(node())])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'an axis other than mouse.x or pad1.x steering a thing by &lt;track /&gt;'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="centered[offset and *[not(self::offset)]] | centered[count(*[not(self::offset)]) &gt; 1] | centered[not(ancestor::*[sprite][1]/sprite/rectangle or ancestor::*[sprite][1]/sprite/text or ancestor::*[sprite][1]/sprite/bitmap)]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;centered&gt; other than on a rectangle, a text or a picture, on the window or on a value'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="group[not(member) and not(sprite/text)][(columns &gt; 1 and position/x//centered) or (rows &gt; 1 and position/y//centered)]" mode="covers" priority="3">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;centered&gt; across a block of more than one'" /></xsl:call-template>
  </xsl:template>

  <!-- A box pulled in from an object's picture's sides, each by a number. -->
  <xsl:template match="object/collisions/hitbox" mode="covers" priority="2">
    <xsl:if test="@* or *[not(self::all or self::horizontal or self::vertical or self::top or self::bottom or self::left or self::right)] or *[string(number(.)) = 'NaN'] or ../type">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;hitbox&gt; other than one pulled in from the sides by numbers'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- A rule passed over while a class (or grid) is touched too. -->
  <xsl:template match="object/collisions/collision[not(@edge)]/@unless" mode="covers" priority="2">
    <xsl:if test="not($grids[@class = current() or @name = current()] or $things[(@class | parent::group/@class) = current() or @name = current()])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('unless=', $quote, ., $quote, ', not a class of things or grids')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Touching by the lit pixels of a picture. -->
  <xsl:template match="collisions/type" mode="covers" priority="2">
    <xsl:if test="normalize-space(.) != 'pixel' or not(../../sprite/bitmap)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a collision &lt;type&gt; other than pixel on a picture'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- A grid's rows by number: a way across of their own; or, in a grid
       that marches, its pictures by the names of the grid's, and numbers
       of the grid's. -->
  <xsl:template match="row[number]" mode="covers" priority="2">
    <xsl:variable name="grid" select=".." />
    <xsl:if test="not(../parent::objects) or count(.. | $grids) != count($grids) or *[not(self::number or self::velocity or self::sprite or self::variables)] or velocity/*[not(self::x)] or velocity/x/*
        or ((sprite or variables) and count(.. | $marching) != count($marching))
        or sprite[@*[local-name() != 'name'] or *[not(self::bitmap)] or bitmap/@*[local-name() != 'from'] or bitmap/* or not($grid/sprite/@name = @name)]
        or (sprite and count(sprite) != count($grid/sprite))
        or variables/variable[* or @*[local-name() != 'name'] or not($grid/variables/variable/@name = @name) or string(number(.)) = 'NaN']">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;row&gt; by number other than a way across in a grid, or pictures and numbers of a grid that marches'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- A grid that marches (kinds.xsl): its pictures in turn, every so long,
       each step its sounds; its way across, the way it faces, and at the
       window's sides back, round and down, all together. -->
  <xsl:template match="group/animation" mode="covers" priority="2">
    <xsl:if test="count(.. | $marching) != count($marching) or @* or *[not(self::seconds or self::stepped or self::frame)] or count(seconds) != 1 or seconds/@*[not(local-name() = 'update' and . = 'live')]
        or stepped/node() or frame/@*[local-name() != 'sprite'] or frame[not(../../sprite/@name = @sprite)] or frame/*[not(self::play)] or (frame/play and frame[count(play) != 1]) or frame/play/@*[local-name() != 'sound']">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'an &lt;animation&gt; of a grid other than its pictures in turn as it marches, a sound a step or none'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="seconds/node()" mode="covers" />
  </xsl:template>
  <xsl:template match="group/velocity | group/facing | group/collisions/lockstep" mode="covers" priority="2">
    <xsl:if test="count(ancestor::group[1] | $marching) != count($marching) or (self::velocity and *[not(self::x)]) or (self::facing and not(. = 'up' or . = 'down' or . = 'left' or . = 'right')) or (self::lockstep and node())">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; in a group other than a grid that marches')" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="self::velocity/*" mode="covers" />
  </xsl:template>
  <xsl:template match="group/collisions/collision[@edge = 'horizontal'][bounce]" mode="covers" priority="3">
    <xsl:if test="count(ancestor::group[1] | $marching) != count($marching) or @*[local-name() != 'edge'] or *[1][not(self::bounce[not(@* or node())])] or *[position() &gt; 1][not(self::move[@direction = 'down'])]">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; at the sides of a grid that marches, other than a bounce and moves down'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="move/node()" mode="covers" />
  </xsl:template>
  <!-- Timers of a grid's cells, each its own, every so often. -->
  <xsl:template match="group/timers" mode="covers" priority="2">
    <xsl:if test="count(.. | $grids) != count($grids) or ../group or timer[@* or not(every) or *[not(self::every or self::fire or self::play)]]">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;timers&gt; in a group other than every so often of a grid'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="timer/*" mode="covers" />
  </xsl:template>

  <!-- No more than so many of a grid's cells left, the grid by name or class. -->
  <xsl:template match="condition/remaining" mode="covers" priority="2">
    <xsl:if test="not($grids[@name = current()/../@object] or $grids[@class = current()/../@class]) or (../@class and ../@object) or string(number(.)) = 'NaN'">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;remaining&gt; other than of a grid'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- A grid's cells, any of them (highest, lowest) or how many (count). -->
  <xsl:template match="conditions/condition/@total" mode="covers" priority="2">
    <xsl:if test="not($grids[@class = current()/../@class]) or ../@object or not(. = 'highest' or . = 'lowest' or . = 'count') or (. = 'count' and ../@variable) or (. != 'count' and not(../@variable = 'x' or ../@variable = 'y'))
        or ../*[self::equal or self::unequal]">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('total=', $quote, ., $quote, ' other than the highest, lowest or count of a grid', $quote, 's cells')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Scenes: of the state that is played, each from= the one before it,
       its ways leading to the next, naming the first's numbers; the first
       takes the pieces. -->
  <xsl:template match="state/scenes" mode="covers" priority="2">
    <xsl:choose>
      <xsl:when test="count(.. | $played) != 1">
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;scenes&gt; in a &lt;state&gt; that is not played'" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:apply-templates select="@* | * | text()" mode="covers" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="scenes/scene" mode="covers" priority="2">
    <xsl:variable name="before" select="preceding-sibling::scene[1]" />
    <xsl:variable name="named" select="variables/variable/@name" />
    <xsl:if test="($before and objects) or ($before and string(@from) != string($before/@name)) or (not($before) and @from)
        or timers or count($named) != count($scene-numbers) or $scene-numbers[not(@name = $named)]">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;scene&gt; other than one from= the one before it, naming the same numbers, the first with the objects'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="scene/ways/way" mode="covers" priority="2">
    <xsl:if test="string(@scene) != string(../../following-sibling::scene[1]/@name)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;way&gt; to a scene other than the next'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="scene/variables/variable[*]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a number of a scene other than a plain one'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="scene/objects/object" mode="covers" priority="2">
    <xsl:if test="* or @*[local-name() != 'name'] or not($pieces[@name = current()/@name])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'an object of a scene other than a piece it takes as it is'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- A piece: one a scene takes, or one that is fired. -->
  <xsl:template match="game/objects/object" mode="covers" priority="2">
    <xsl:choose>
      <xsl:when test="count(. | $taken | $fired) != count($taken | $fired) or count(. | $taken) + count(. | $fired) = 2">
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'a piece other than one the scenes take or one that is fired'" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:apply-templates select="@* | * | text()" mode="covers" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <!-- Timers of the state that is played, every so often; of another, one
       after a while that ends with leaving it. -->
  <xsl:template match="state/timers" mode="covers" priority="2">
    <xsl:if test="count(.. | $played) != 1 and (count(timer) != 1 or timer[not(after) or not(*[last()][self::pop])])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;timers&gt; in a &lt;state&gt; that is not played, other than one after a while that pops it'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="*" mode="covers" />
  </xsl:template>
  <xsl:template match="state/timers/timer" mode="covers" priority="2">
    <xsl:if test="@* or not(every or after) or (every and after) or *[not(self::every or self::after)][not(self::play or self::inc or self::dec or self::set or self::reveal or self::despawn or self::pop or self::trigger)]
        or after[* or string(number(.)) = 'NaN']">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;timer&gt; of a state other than every so often, or once after a while'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="*[not(self::after)]" mode="covers" />
  </xsl:template>
  <!-- Timers: a thing's, every so often. -->
  <xsl:template match="object/timers/timer" mode="covers" priority="2">
    <xsl:if test="@* or not(every) or *[not(self::every)][not(self::fire or self::reverse or self::play or self::inc or self::dec or self::set)] or count(../.. | $things) != count($things)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;timer&gt; other than every so often of a thing'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="*" mode="covers" />
  </xsl:template>
  <!-- An animation: of what is fired, its pictures round and round, each as
       long; or once through, each as long as it says, then what it does
       last of all, ending taken away. -->
  <xsl:template match="object/animation" mode="covers" priority="2">
    <xsl:if test="(count(.. | $fired) != count($fired) and (count(.. | $screen-animated) != count($screen-animated) or once)) or *[not(self::seconds or self::frame or self::once or self::then)] or frame/@*[local-name() != 'sprite'] or not(seconds)
        or frame/*[not(self::seconds)] or (not(once) and (frame/* or then)) or (once and not(then/*[last()][self::despawn])) or frame/seconds[string(number(.)) = 'NaN'] or string(number(seconds)) = 'NaN' or once/node()">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'an &lt;animation&gt; other than of what is fired or shown on a screen that is not played, each picture as long, or once through then taken away'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="seconds | then/*" mode="covers" />
  </xsl:template>
  <!-- A thing that starts out of play, waiting for a <reveal>. -->
  <xsl:template match="object/waiting" mode="covers" priority="2">
    <xsl:if test="@* or node() or count(.. | $things) != count($things)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;waiting /&gt; of other than a thing'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- update="live": where a picture on a screen that is not played is
       drawn follows a thing's x or y. -->
  <xsl:template match="object/position/x/@update | object/position/y/@update" mode="covers" priority="2">
    <xsl:variable name="named" select="normalize-space(..)" />
    <xsl:if test=". != 'live' or ../* or count(../../.. | $screen-animated) != count($screen-animated)
        or not($things[@name = substring-before($named, '.')] and substring-after($named, '.') = local-name(current()/..))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('update= on &lt;', local-name(..), '&gt; other than a picture on a screen that is not played at a thing', $quote, 's place')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="object/facing" mode="covers" priority="2">
    <xsl:if test="not(. = 'up' or . = 'down' or . = 'left' or . = 'right')">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;facing&gt; other than up, down, left or right'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template match="object/most" mode="covers" priority="2">
    <xsl:if test="count(.. | $fired) != count($fired) or string(number(.)) = 'NaN'">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;most&gt; other than a number of what is fired'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Maps of cells (cells.xsl). -->
  <!-- covered: one map of cells in the played state, each piece one cell
       picked by its letter -->
  <xsl:template match="objects/map" mode="covers" priority="2">
    <xsl:choose>
      <xsl:when test="not(cells) or cells/node() or cells/@* or count(../.. | $played) != 1 or count($cell-maps) != 1 or *[not(self::tile or self::position or self::cells or self::row or self::group or self::object)]
          or not(row) or (group | object)[not(@char) or string-length(@char) != 1]">
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;map&gt; other than one of cells in the played state, its pieces each one cell picked by a letter'" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:apply-templates select="@* | tile | position | group | object" mode="covers" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="map/@name | map/group/@char | map/object/@char | chain/@object" mode="covers" />
  <xsl:template match="map/group | map/object | map/tile | tile/width | tile/height | map/position | map/group/sprite | map/object/sprite | map/group/collisions | map/object/collisions | map/object/actions" mode="covers" priority="2">
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <!-- a piece's look: a rectangle or a circle of one colour -->
  <xsl:template match="map/group[count(sprite) != 1] | map/object[count(sprite) != 1] | map/*/sprite[not(rectangle or circle) or count(*) != 1 or @*]" mode="covers" priority="3">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a piece of a map other than a rectangle or a circle'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="map/*/sprite/circle | circle/radius | circle/color" mode="covers" priority="2">
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="map/object/facing" mode="covers" priority="3">
    <xsl:if test="not(. = 'up' or . = 'down' or . = 'left' or . = 'right')">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;facing&gt; other than up, down, left or right'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- a chain: the links of a list of the map behind a piece that goes -->
  <xsl:template match="map/object/chain" mode="covers" priority="2">
    <xsl:if test="node() or @*[local-name() != 'object'] or not(../../group[@name = current()/@object]) or count(.. | $goers) != count($goers)">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;chain&gt; other than of a list of its map, on a piece that goes'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- a rule on stepping into a cell: a piece that goes meeting a class of
       the map, or a list met by one that goes -->
  <xsl:template match="map/*/collisions/collision" mode="covers" priority="3">
    <xsl:variable name="owner" select="../.." />
    <xsl:variable name="classes" select="$owner/../*[self::group or self::object]/@class" />
    <xsl:choose>
      <xsl:when test="@*[local-name() != 'class'] or not(@class) or (count($owner | $goers) = count($goers) and not($classes = @class))
          or (count($owner | $goers) != count($goers) and not($goers/@class = @class))">
        <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; on a map other than by the class of what is stepped into, or of what steps'" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:apply-templates select="*" mode="covers" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template match="map/object/actions/action" mode="covers" priority="2">
    <xsl:if test="*[not(self::go)] or count(go) != 1">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="'an action of a piece of a map other than one &lt;go&gt;'" /></xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="@* | *" mode="covers" />
  </xsl:template>

  <!-- The parts covered: the game, its window and numbers, its states of
       objects and groups (rectangles, blocks of them and texts), their
       rules, actions and keys, the seats and the computer's, the screens on
       the stack, the sounds and what is kept. -->
  <xsl:template mode="covers" match="
      game | game/window | window/width | window/height | window/framerate | window/background | game/objects
      | state/scenes | scenes/scene | scene/ways | scene/variables | scene/objects | scene/conditions
      | object/timers | timer/every | animation/seconds | bitmap/color | style/scale
      | game/variables | variables/variable | game/styles | styles/style | style/size | style/color
      | game/keeps | game/sounds | sounds/sound | sound/note
      | game/states | states/objects | states/state | state/objects | state/inputs | state/shows | shows/show
      | state/conditions | conditions/condition | state/computer | computer/condition | condition/condition
      | objects/object | objects/group | group/member
      | object/sprite | object/position | object/velocity | object/collisions | object/variables | object/actions
      | group/sprite | group/position | group/collisions | group/actions | group/variables
      | group/columns | group/rows | group/padding | padding/x | padding/y | group/group | group/row | row/sprite
      | sound/volume | sound/part | part/note | text/color | x/right | x/left | y/top | y/bottom | right/offset | left/offset | top/offset | bottom/offset
      | member/position | member/velocity
      | sprite/rectangle | rectangle/width | rectangle/height | rectangle/color
      | sprite/text | text/content | text/number | text/size
      | game/pictures | pictures/bitmap | bitmap/row | bitmap/scale | sprite/bitmap
      | position/x | position/y | velocity/x | velocity/y
      | collisions/collision | collision/any | collision/condition | condition/any | any/condition | any/any
      | condition/atleast | condition/atmost | condition/above | condition/below | condition/equal | condition/unequal
      | actions/action | inputs/input | inputs/seat | seat/input | input/button
      | add | subtract | multiply | divide | sign | sine | cosine | ceiling | value
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
      | row/@number | part/@wave | bitmap/@name | bitmap/@from | note/@to | note/@wave | sound/@wave
      | show/@object | show/@state | condition/@object | condition/@class | condition/@variable
      | scene/@name | scene/@from | condition/@name | color/@char | bitmap/@style" />

  <!-- A name in a value must be one the program has: a number, a
       variable of the game, an object's variable (one drawn at random only
       where it starts), a thing's x, y, velocity.x, velocity.y, width or
       height, or a text's own width or height. -->
  <xsl:template mode="covers" match="text()[normalize-space(.) != ''][parent::width or parent::height or parent::x or parent::y
      or parent::value or parent::min or parent::max or parent::variable or parent::move or parent::bounce or parent::deflect
      or parent::offset or parent::number or parent::set or parent::inc or parent::atleast or parent::atmost or parent::above
      or parent::below or parent::equal or parent::unequal or parent::every or parent::seconds or parent::go or parent::radius]">
    <xsl:variable name="text" select="normalize-space(.)" />
    <xsl:variable name="first" select="substring-before($text, '.')" />
    <xsl:variable name="rest" select="substring-after($text, '.')" />
    <xsl:variable name="owner" select="$everything[@name = $first]" />
    <xsl:variable name="variable" select="($owner/variables/variable | $owner/parent::group/variables/variable)[@name = $rest]" />
    <xsl:variable name="here" select="ancestor::*[self::object or self::member or self::group][1]" />
    <xsl:variable name="random-here" select="$variable//random and ($here = $owner or $here/member = $owner) and (ancestor::position or ancestor::velocity or ancestor::variable)" />
    <xsl:if test="not(string(number($text)) != 'NaN'
        or /game/variables/variable[@name = $text]
        or (ancestor::set[@group] and $text = concat(ancestor::set/@group, '.1.1.', ancestor::set/@variable))
        or $scene-numbers[@name = $text]
        or ($variable and (not($variable//random) or $random-here or $written[. = $text]))
        or ($owner and count($owner | $things) = count($things) and ($rest = 'x' or $rest = 'y' or $rest = 'velocity.x' or $rest = 'velocity.y' or $rest = 'width' or $rest = 'height'))
        or ($owner and count($owner | $texts) = count($texts) and ($rest = 'width' or $rest = 'height') and generate-id($here) = generate-id($owner) and ancestor::position))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('the name ', $quote, $text, $quote)" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <xsl:template mode="covers" match="text()[normalize-space(.) != ''][parent::content or parent::size or parent::color or parent::note or parent::columns or parent::rows or parent::volume
      or parent::row[parent::bitmap] or parent::scale or parent::type or parent::number[parent::row] or parent::background or parent::most
      or parent::variable[ancestor::scene]]" />

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
