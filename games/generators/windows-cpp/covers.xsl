<?xml version="1.0" encoding="UTF-8"?>
<!-- covers.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- What windows-cpp has words for yet, said as the parts of a game it
     covers. Before it writes anything the generator walks the game in mode
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
  <xsl:template match="states[count(state) &gt; 1]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'more than one &lt;state&gt;'" /></xsl:call-template>
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="object[count(sprite) &gt; 1] | group[count(sprite) &gt; 1]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'more than one &lt;sprite&gt;'" /></xsl:call-template>
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template match="game/variables/variable[.//random]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;random&gt; in the game variables'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="group/collisions/collision[not(@edge)]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'a &lt;collision&gt; on a touch in a group'" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="group/variables | member/variables | member/sprite" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; of a group or member')" /></xsl:call-template>
  </xsl:template>
  <xsl:template match="centered[node()] | centered[not(ancestor::*[sprite][1]/sprite/rectangle)]" mode="covers" priority="2">
    <xsl:call-template name="gap"><xsl:with-param name="what" select="'&lt;centered&gt; other than a bare one, on a rectangle'" /></xsl:call-template>
  </xsl:template>

  <!-- The parts covered: the game, its window and numbers, one state of
       objects and groups of rectangles, their rules, actions and keys. -->
  <xsl:template mode="covers" match="
      game | game/window | window/width | window/height | window/framerate
      | game/variables | variables/variable
      | game/states | states/state | state/objects | state/inputs
      | objects/object | objects/group | group/member
      | object/sprite | object/position | object/velocity | object/collisions | object/variables | object/actions
      | group/sprite | group/collisions | group/actions
      | member/position | member/velocity
      | sprite/rectangle | rectangle/width | rectangle/height
      | position/x | position/y | velocity/x | velocity/y
      | collisions/collision | actions/action | inputs/input | input/button
      | add | subtract | multiply | divide | sign | sine | cosine | value
      | random | random/min | random/max | centered">
    <xsl:apply-templates select="@* | * | text()" mode="covers" />
  </xsl:template>
  <xsl:template mode="covers" match="
      game/@*[namespace-uri() != ''] | window/@name | variable/@name | state/@name
      | object/@name | group/@name | member/@name | action/@name | collision/@edge
      | input/@button" />

  <!-- A name in a value must be one the program has: a number, the
       window's sides, a variable of the game, or, where it starts (its
       position, velocity and variables), one of the object's own. -->
  <xsl:template mode="covers" match="text()[normalize-space(.) != ''][parent::width or parent::height or parent::x or parent::y
      or parent::value or parent::min or parent::max or parent::variable or parent::move or parent::bounce or parent::deflect]">
    <xsl:variable name="text" select="normalize-space(.)" />
    <xsl:variable name="self" select="ancestor::object[1]" />
    <xsl:variable name="own" select="substring-after($text, '.')" />
    <xsl:if test="not(string(number($text)) != 'NaN'
        or $text = 'window.width' or $text = 'window.height' or $text = 'window.left' or $text = 'window.right'
        or $text = 'window.top' or $text = 'window.bottom'
        or /game/variables/variable[@name = $text]
        or ($self and (ancestor::position or ancestor::velocity or ancestor::variable) and (starts-with($text, concat($self/@name, '.')) or starts-with($text, 'self.')) and $self/variables/variable[@name = $own]))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('the name ', $quote, $text, $quote)" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>
