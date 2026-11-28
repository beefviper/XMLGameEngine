<?xml version="1.0" encoding="UTF-8"?>
<!-- values.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- A value of the game as a JavaScript expression, in the order the file
     nests it, with brackets only where JavaScript would otherwise read it
     another way. A name is the constant, local or property it is written
     as. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- $node holds the value: its text, or one operation, <random>, <window>,
       <self> or <centered>. $self is the object the value is of (for its own
       variables and size); $axis is x or y in a <position>; $in what the
       game's things are reached through (nothing: they are the page's own); $start, that
       it is in the object's start, where its drawn numbers are locals.
       $wrap is how tightly the operation around it binds: 1 for + or -
       (2 on the right of a -), 3 for * or / (4 on the right of a /). -->
  <xsl:template name="value">
    <xsl:param name="node" />
    <xsl:param name="self" />
    <xsl:param name="axis" select="''" />
    <xsl:param name="in" select="''" />
    <xsl:param name="start" select="false()" />
    <xsl:param name="wrap" select="0" />
    <xsl:variable name="tag" select="$node/*[1]" />
    <xsl:variable name="op" select="local-name($tag)" />
    <xsl:choose>
      <xsl:when test="not($tag)">
        <xsl:call-template name="name-or-number">
          <xsl:with-param name="text" select="normalize-space($node)" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="in" select="$in" />
          <xsl:with-param name="start" select="$start" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$op = 'random'">
        <xsl:text>between(</xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/min" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/max" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:when test="$op = 'self' and $self/self::group">
        <!-- a number of a grid's cell or a group's member, in the loop over them -->
        <xsl:call-template name="loop-name"><xsl:with-param name="group" select="$self" /></xsl:call-template>
        <xsl:text>.</xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="normalize-space($tag)" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$op = 'self'">
        <xsl:call-template name="name-or-number">
          <xsl:with-param name="text" select="concat($self/@name, '.', normalize-space($tag))" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="in" select="$in" />
          <xsl:with-param name="start" select="$start" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$op = 'window'">
        <xsl:variable name="measure" select="local-name($tag/*)" />
        <xsl:choose>
          <xsl:when test="$measure = 'width' or $measure = 'right'">windowWidth</xsl:when>
          <xsl:when test="$measure = 'height' or $measure = 'bottom'">windowHeight</xsl:when>
          <xsl:otherwise>
            <xsl:if test="$wrap &gt;= 4">(</xsl:if>
            <xsl:choose>
              <xsl:when test="$measure = 'x'">windowWidth / 2</xsl:when>
              <xsl:otherwise>windowHeight / 2</xsl:otherwise>
            </xsl:choose>
            <xsl:if test="$wrap &gt;= 4">)</xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$op = 'centered'">
        <!-- its middle on the window's (or on a value it holds): that middle
             less half its size, and any offset -->
        <xsl:variable name="window">
          <xsl:choose>
            <xsl:when test="$axis = 'x'">windowWidth</xsl:when>
            <xsl:otherwise>windowHeight</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="size">
          <xsl:call-template name="own-size">
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="axis" select="$axis" />
            <xsl:with-param name="in" select="$in" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$wrap &gt;= 2">(</xsl:if>
        <xsl:choose>
          <xsl:when test="$tag/*[not(self::offset)] or normalize-space($tag/text()) != ''">
            <!-- on a value of its own: its middle there -->
            <xsl:call-template name="value"><xsl:with-param name="node" select="$tag" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="wrap" select="1" /></xsl:call-template>
            <xsl:text> - </xsl:text>
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="concat($window, ' / 2 - ')" /></xsl:otherwise>
        </xsl:choose>
        <xsl:choose>
          <xsl:when test="string(number($size)) != 'NaN'"><xsl:value-of select="$size div 2" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="concat($size, ' / 2')" /></xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="offset"><xsl:with-param name="node" select="$tag/offset" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /></xsl:call-template>
        <xsl:if test="$wrap &gt;= 2">)</xsl:if>
      </xsl:when>
      <xsl:when test="$op = 'left' or $op = 'top'">
        <!-- that side on the window's, so far in -->
        <xsl:choose>
          <xsl:when test="$tag/offset"><xsl:call-template name="value"><xsl:with-param name="node" select="$tag/offset" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="wrap" select="$wrap" /></xsl:call-template></xsl:when>
          <xsl:otherwise>0</xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$op = 'right' or $op = 'bottom'">
        <xsl:variable name="size">
          <xsl:call-template name="own-size">
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="axis" select="$axis" />
            <xsl:with-param name="in" select="$in" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$wrap &gt;= 2">(</xsl:if>
        <xsl:choose>
          <xsl:when test="$op = 'right'">windowWidth - </xsl:when>
          <xsl:otherwise>windowHeight - </xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="$size" />
        <xsl:if test="$tag/offset">
          <xsl:text> - </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/offset" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="wrap" select="2" /></xsl:call-template>
        </xsl:if>
        <xsl:if test="$wrap &gt;= 2">)</xsl:if>
      </xsl:when>
      <xsl:when test="$op = 'ceiling'">
        <xsl:text>Math.ceil(</xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/value" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:when test="$op = 'sign' or $op = 'sine' or $op = 'cosine'">
        <xsl:choose>
          <xsl:when test="$op = 'sign'">Math.sign(</xsl:when>
          <xsl:otherwise><xsl:value-of select="concat($op, '(')" /></xsl:otherwise>
        </xsl:choose>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$tag/value" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:when test="$op = 'add' or $op = 'subtract' or $op = 'multiply' or $op = 'divide'">
        <xsl:variable name="binds">
          <xsl:choose>
            <xsl:when test="$op = 'add' or $op = 'subtract'">1</xsl:when>
            <xsl:otherwise>3</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="sign">
          <xsl:choose>
            <xsl:when test="$op = 'add'"> + </xsl:when>
            <xsl:when test="$op = 'subtract'"> - </xsl:when>
            <xsl:when test="$op = 'multiply'"> * </xsl:when>
            <xsl:otherwise> / </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="bracketed" select="$wrap &gt; $binds" />
        <xsl:if test="$bracketed">(</xsl:if>
        <xsl:for-each select="$tag/value">
          <xsl:if test="position() &gt; 1"><xsl:value-of select="$sign" /></xsl:if>
          <xsl:call-template name="value">
            <xsl:with-param name="node" select="." />
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="in" select="$in" />
            <xsl:with-param name="start" select="$start" />
            <!-- the first is worked out first anyway; after a - or a /, the rest keep their own order -->
            <xsl:with-param name="wrap" select="$binds + number(position() &gt; 1 and ($op = 'subtract' or $op = 'divide'))" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:if test="$bracketed">)</xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">web-js cannot write a &lt;<xsl:value-of select="$op" />&gt; value yet; use web-asm for this game</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- + 70, or - 110 for a number below nothing: an offset added on. -->
  <xsl:template name="offset">
    <xsl:param name="node" />
    <xsl:param name="self" />
    <xsl:param name="in" select="''" />
    <xsl:if test="$node">
      <xsl:choose>
        <xsl:when test="not($node/*) and starts-with(normalize-space($node), '-')">
          <xsl:value-of select="concat(' - ', substring(normalize-space($node), 2))" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:text> + </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="$node" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="in" select="$in" /><xsl:with-param name="wrap" select="1" /></xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- A number, a variable of the game
       (its constant), or an object's: its constant, the local of its start,
       or its member; or what an object is: x and y its place, velocity.x
       and .y its way, width and height its size. -->
  <xsl:template name="name-or-number">
    <xsl:param name="text" />
    <xsl:param name="self" />
    <xsl:param name="in" select="''" />
    <xsl:param name="start" select="false()" />
    <xsl:variable name="first" select="substring-before($text, '.')" />
    <xsl:variable name="rest" select="substring-after($text, '.')" />
    <xsl:variable name="owner" select="$everything[@name = $first]" />
    <xsl:variable name="variable" select="($owner/variables/variable | $owner/parent::group/variables/variable)[@name = $rest][1]" />
    <xsl:choose>
      <xsl:when test="string(number($text)) != 'NaN'">
        <xsl:value-of select="$text" />
      </xsl:when>
      <xsl:when test="/game/variables/variable[@name = $text]">
        <xsl:call-template name="camel"><xsl:with-param name="name" select="$text" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$scene-numbers[@name = $text]">
        <!-- the scene's, of the one it is at -->
        <xsl:value-of select="concat($scene-one, '().')" />
        <xsl:call-template name="camel"><xsl:with-param name="name" select="$text" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$variable">
        <xsl:variable name="kind"><xsl:call-template name="variable-kind"><xsl:with-param name="variable" select="$variable" /></xsl:call-template></xsl:variable>
        <xsl:choose>
          <xsl:when test="$kind = 'constant'">
            <xsl:call-template name="constant-name"><xsl:with-param name="variable" select="$variable" /></xsl:call-template>
          </xsl:when>
          <xsl:when test="$kind = 'start' and $start and generate-id($owner) = generate-id($self)">
            <xsl:call-template name="camel"><xsl:with-param name="name" select="$rest" /></xsl:call-template>
          </xsl:when>
          <xsl:when test="$kind = 'live'">
            <xsl:value-of select="$in" />
            <xsl:call-template name="object-name"><xsl:with-param name="object" select="$owner" /></xsl:call-template>
            <xsl:text>.</xsl:text>
            <xsl:call-template name="camel"><xsl:with-param name="name" select="$rest" /></xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes">web-js cannot read '<xsl:value-of select="$text" />' outside where its object starts yet; use web-asm for this game</xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$owner and count($owner | $texts) = count($texts) and ($rest = 'width' or $rest = 'height')">
        <!-- a text's size, as the page measured it -->
        <xsl:call-template name="object-name"><xsl:with-param name="object" select="$owner" /></xsl:call-template>
        <xsl:choose>
          <xsl:when test="$rest = 'width'">Size.x</xsl:when>
          <xsl:otherwise>Size.y</xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$start and $owner and generate-id($owner) = generate-id($self) and ($rest = 'width' or $rest = 'height')">
        <!-- its own size, in its start, where the thing is not made yet -->
        <xsl:call-template name="own-size">
          <xsl:with-param name="self" select="$owner" />
          <xsl:with-param name="axis" select="substring('xy', 1 + number($rest = 'height'), 1)" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$owner and count($owner | $things) = count($things) and ($rest = 'x' or $rest = 'y' or $rest = 'velocity.x' or $rest = 'velocity.y' or $rest = 'width' or $rest = 'height')">
        <xsl:value-of select="$in" />
        <xsl:call-template name="object-name"><xsl:with-param name="object" select="$owner" /></xsl:call-template>
        <xsl:call-template name="part-name"><xsl:with-param name="part" select="$rest" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">web-js cannot write the name '<xsl:value-of select="$text" />' yet; use web-asm for this game</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What a command changes: a variable, the object's own when it names
       no object (hits in the ball's rule is ball.hits). -->
  <xsl:template name="variable-target">
    <xsl:param name="path" />
    <xsl:param name="self" />
    <xsl:choose>
      <xsl:when test="$self/self::group and not(contains($path, '.'))">
        <!-- of the member or cell the loop is at -->
        <xsl:call-template name="loop-name"><xsl:with-param name="group" select="$self" /></xsl:call-template>
        <xsl:text>.</xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="$path" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="name-or-number">
          <xsl:with-param name="text">
            <xsl:if test="not(contains($path, '.')) or not($everything[@name = substring-before($path, '.')])"><xsl:value-of select="concat($self/@name, '.')" /></xsl:if>
            <xsl:value-of select="$path" />
          </xsl:with-param>
          <xsl:with-param name="self" select="$self" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- .position.x for x, .velocity.y for velocity.y, .size.x for width. -->
  <xsl:template name="part-name">
    <xsl:param name="part" />
    <xsl:choose>
      <xsl:when test="$part = 'x' or $part = 'y'"><xsl:value-of select="concat('.position.', $part)" /></xsl:when>
      <xsl:when test="$part = 'width'">.size.x</xsl:when>
      <xsl:when test="$part = 'height'">.size.y</xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('.', $part)" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The object's own width (x) or height (y): its rectangle's, its
       block's, or its text's as measured. -->
  <xsl:template name="own-size">
    <xsl:param name="self" />
    <xsl:param name="axis" />
    <xsl:param name="in" select="''" />
    <xsl:variable name="sprite" select="($self/sprite | $self/parent::group/sprite)[1]" />
    <xsl:choose>
      <xsl:when test="$sprite/text">
        <xsl:call-template name="object-name"><xsl:with-param name="object" select="$self" /></xsl:call-template>
        <xsl:value-of select="concat('Size.', $axis)" />
      </xsl:when>
      <xsl:when test="$sprite/bitmap">
        <xsl:variable name="bitmap" select="$pictures[@name = $sprite/bitmap/@from]" />
        <xsl:variable name="scale"><xsl:call-template name="picture-scale"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template></xsl:variable>
        <xsl:choose>
          <xsl:when test="$axis = 'x'"><xsl:value-of select="string-length(normalize-space($bitmap/row[1])) * $scale" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="count($bitmap/row) * $scale" /></xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$sprite/rectangle">
        <xsl:variable name="one">
          <xsl:call-template name="value">
            <xsl:with-param name="node" select="$sprite/rectangle/*[local-name() = substring('widthheight', 1 + 5 * number($axis = 'y'), 5 + number($axis = 'y'))]" />
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="in" select="$in" />
            <xsl:with-param name="wrap" select="3" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="count" select="number(normalize-space($self/*[local-name() = substring('columnsrows', 1 + 7 * number($axis = 'y'), 7 - 3 * number($axis = 'y'))]))" />
        <xsl:choose>
          <xsl:when test="not($count &gt; 1)"><xsl:value-of select="$one" /></xsl:when>
          <xsl:otherwise>
            <xsl:message terminate="yes">web-js can centre a block of one row or column only yet; use web-asm for this game</xsl:message>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">web-js can centre only a rectangle or a text yet; use web-asm for this game</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
