<?xml version="1.0" encoding="UTF-8"?>
<!-- game.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- The page: <game>.html, its numbers, its things and its rules in a
     script, then the few lines that play them on a canvas. Each object is a
     plain object (where it is, how it moves, how big it is); a frame is the
     keys held, everything moving, the window's edges and the meetings, each
     rule written by its verb's file (verbs/) in one of three modes: held (a
     key held, on who), edge (at one edge, on who and that edge) and touch (on
     a meeting, on self and other). Nothing of the engine is in it. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:key name="buttons" match="input/@button | input/button" use="normalize-space(.)" />

  <!-- What the game has, and so what it is written with. -->
  <xsl:variable name="owners" select="$state/objects/object | $state/objects/group" />
  <xsl:variable name="edge-owners" select="$owners[collisions/collision[@edge]]" />
  <xsl:variable name="touch-owners" select="$state/objects/object[collisions/collision[not(@edge)]]" />
  <xsl:variable name="started" select="$state/objects/object[variables/variable or collisions/collision/reset]" />
  <xsl:variable name="inputs" select="$state/inputs/input[trigger]" />
  <xsl:variable name="all-buttons" select="$inputs/@button | $inputs/button" />
  <xsl:variable name="buttons" select="$all-buttons[generate-id() = generate-id(key('buttons', normalize-space(.))[1])]" />
  <xsl:variable name="moved" select="$objects[@name = $inputs/trigger/@object]" />
  <xsl:variable name="randoms" select="$state//random" />
  <xsl:variable name="deflects" select="$state//deflect" />
  <xsl:variable name="uses-angles" select="boolean($state//sine | $state//cosine | $deflects)" />

  <!-- What the game is called: its window's name, or else the program's. -->
  <xsl:variable name="title">
    <xsl:choose>
      <xsl:when test="/game/window/@name != ''"><xsl:value-of select="/game/window/@name" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- <game>.html -->
  <xsl:template name="page-file">
    <xsl:text>&lt;!DOCTYPE html&gt;&#10;</xsl:text>
    <xsl:value-of select="concat('&lt;!-- ', $name, '.html --&gt;&#10;&lt;!-- ', $title, ' --&gt;&#10;&lt;!-- author: beefviper --&gt;&#10;&lt;!-- date: ', $date, ' --&gt;&#10;')" />
    <xsl:value-of select="concat('&#10;&lt;!-- ', $title, ': its numbers, its things and its rules, then the page that&#10;     plays them. Open it in a browser. --&gt;&#10;')" />
    <xsl:text>&lt;html lang="en"&gt;
&lt;head&gt;
&lt;meta charset="utf-8"&gt;
&lt;title&gt;</xsl:text>
    <xsl:value-of select="$title" />
    <xsl:text>&lt;/title&gt;
&lt;style&gt;
	html, body { margin: 0; height: 100%; background: black; overflow: hidden; }
	body { display: flex; align-items: center; justify-content: center; }
&lt;/style&gt;
&lt;/head&gt;
&lt;body&gt;
&lt;canvas id="game"&gt;&lt;/canvas&gt;
&lt;script&gt;
"use strict";

const windowWidth = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/width" /></xsl:call-template>
    <xsl:text>;&#10;const windowHeight = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/height" /></xsl:call-template>
    <xsl:text>;&#10;</xsl:text>
    <xsl:call-template name="constants" />
    <xsl:call-template name="helpers" />
    <xsl:for-each select="$started">
      <xsl:call-template name="start-function" />
    </xsl:for-each>
    <xsl:text>
// Each thing on the screen: where it is, how it moves, how big it is.
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:text>let </xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text> = </xsl:text>
      <xsl:choose>
        <xsl:when test="count(. | $started) = count($started)">
          <xsl:call-template name="start-name" />
          <xsl:text>();&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>{ position: </xsl:text>
          <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
          <xsl:text>, velocity: </xsl:text>
          <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity" /></xsl:call-template>
          <xsl:text>, size: </xsl:text>
          <xsl:call-template name="size" />
          <xsl:text> };&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:call-template name="key-function" />
    <xsl:call-template name="frame-function" />
    <xsl:call-template name="page-script" />
    <xsl:text>&lt;/script&gt;
&lt;/body&gt;
&lt;/html&gt;
</xsl:text>
  </xsl:template>

  <!-- The game's <variables>, each a constant: those the game reads. -->
  <xsl:template name="constants">
    <xsl:variable name="read" select="/game/variables/variable[@name = $state//*[not(*)] or @name = /game/variables/variable//*[not(*)] or @name = /game/variables/variable[not(*)]]" />
    <xsl:if test="$read">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:for-each select="/game/variables/variable">
      <xsl:variable name="named" select="@name" />
      <xsl:if test="$state//*[not(*)][normalize-space(.) = $named] | /game/variables/variable[@name != $named]//*[normalize-space(.) = $named] | /game/variables/variable[@name != $named][normalize-space(.) = $named]">
        <xsl:text>const </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template>
        <xsl:text>;&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- wHeld, upHeld: whether that key is down. -->
  <xsl:template name="held-name">
    <xsl:param name="button" select="normalize-space(.)" />
    <xsl:call-template name="camel"><xsl:with-param name="name" select="$button" /></xsl:call-template>
    <xsl:text>Held</xsl:text>
  </xsl:template>

  <!-- startBall: the ball as it starts, and starts again. -->
  <xsl:template name="start-name">
    <xsl:param name="object" select="." />
    <xsl:variable name="camel"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('start', translate(substring($camel, 1, 1), $lower, $upper), substring($camel, 2))" />
  </xsl:template>

  <!-- The small functions the rules are written with, those they use. -->
  <xsl:template name="helpers">
    <xsl:variable name="parts">
      <xsl:if test="$uses-angles">
        <part>function sine(degrees) { return Math.sin(degrees * Math.PI / 180); }
function cosine(degrees) { return Math.cos(degrees * Math.PI / 180); }
</part>
      </xsl:if>
      <xsl:if test="$randoms">
        <part>function between(min, max) { return min + Math.random() * (max - min); }
</part>
      </xsl:if>
      <xsl:if test="$touch-owners">
        <part>function touching(a, b) {
	return a.position.x &lt; b.position.x + b.size.x &amp;&amp; b.position.x &lt; a.position.x + a.size.x
		&amp;&amp; a.position.y &lt; b.position.y + b.size.y &amp;&amp; b.position.y &lt; a.position.y + a.size.y;
}
</part>
      </xsl:if>
      <xsl:if test="$deflects">
        <part>// Off the side of `other` it hit, at the same speed, up to `degrees` off
// straight out: steeper the further from that side's middle, towards the
// end it hit nearer to.
function deflect(thing, other, degrees) {
	const middle = { x: thing.position.x + thing.size.x / 2, y: thing.position.y + thing.size.y / 2 };
	const otherMiddle = { x: other.position.x + other.size.x / 2, y: other.position.y + other.size.y / 2 };
	const apart = { x: middle.x - otherMiddle.x, y: middle.y - otherMiddle.y };
	const reach = { x: (thing.size.x + other.size.x) / 2, y: (thing.size.y + other.size.y) / 2 };
	const speed = Math.hypot(thing.velocity.x, thing.velocity.y);

	if (reach.x - Math.abs(apart.x) &lt; reach.y - Math.abs(apart.y)) {
		// its left or right side
		const away = apart.x &lt; 0 ? -1 : 1;
		const angle = degrees * Math.max(-1, Math.min(1, apart.y / (other.size.y / 2)));
		thing.velocity = { x: away * speed * cosine(angle), y: speed * sine(angle) };
		thing.position.x = otherMiddle.x + away * reach.x - thing.size.x / 2;
	} else {
		// its top or bottom
		const away = apart.y &lt; 0 ? -1 : 1;
		const angle = degrees * Math.max(-1, Math.min(1, apart.x / (other.size.x / 2)));
		thing.velocity = { x: speed * sine(angle), y: away * speed * cosine(angle) };
		thing.position.y = otherMiddle.y + away * reach.y - thing.size.y / 2;
	}
}
</part>
      </xsl:if>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($parts)/part">
      <xsl:text>&#10;</xsl:text>
      <xsl:value-of select="." />
    </xsl:for-each>
  </xsl:template>

  <!-- { x: ..., y: ... } of a <position> or <velocity>; 0, 0 when there is none. -->
  <xsl:template name="pair">
    <xsl:param name="of" />
    <xsl:param name="self" select="." />
    <xsl:choose>
      <xsl:when test="$of">
        <xsl:text>{ x: </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$of/x" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template>
        <xsl:text>, y: </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$of/y" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise>{ x: 0, y: 0 }</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- { x: width, y: height } of its rectangle, or its group's. -->
  <xsl:template name="size">
    <xsl:variable name="rectangle" select="(sprite | parent::group/sprite)[1]/rectangle" />
    <xsl:text>{ x: </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="$rectangle/width" /><xsl:with-param name="self" select="." /></xsl:call-template>
    <xsl:text>, y: </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="$rectangle/height" /><xsl:with-param name="self" select="." /></xsl:call-template>
    <xsl:text> }</xsl:text>
  </xsl:template>

  <!-- startBall(): its own variables (a <random> drawn again each time),
       then the thing, its place, way and size. -->
  <xsl:template name="start-function">
    <xsl:text>&#10;// The </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text> as it starts</xsl:text>
    <xsl:if test="collisions/collision/reset">, and starts again</xsl:if>
    <xsl:text>.&#10;function </xsl:text>
    <xsl:call-template name="start-name" />
    <xsl:text>() {&#10;</xsl:text>
    <xsl:for-each select="variables/variable">
      <xsl:text>&#9;const </xsl:text>
      <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text> = </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="../.." /></xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#9;return {&#10;&#9;&#9;position: </xsl:text>
    <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
    <xsl:text>,&#10;&#9;&#9;velocity: </xsl:text>
    <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity" /></xsl:call-template>
    <xsl:text>,&#10;&#9;&#9;size: </xsl:text>
    <xsl:call-template name="size" />
    <xsl:text>,&#10;&#9;};&#10;}&#10;</xsl:text>
  </xsl:template>

  <!-- key(): which keys are down; whether the game uses that key. -->
  <xsl:template name="key-function">
    <xsl:text>
// Which keys are down.
</xsl:text>
    <xsl:for-each select="$buttons">
      <xsl:text>let </xsl:text>
      <xsl:call-template name="held-name" />
      <xsl:text> = false;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>
function key(code, down) {
	switch (code) {
</xsl:text>
    <xsl:for-each select="$buttons">
      <xsl:text>&#9;case </xsl:text>
      <xsl:call-template name="key-code"><xsl:with-param name="button" select="normalize-space(.)" /></xsl:call-template>
      <xsl:text>: </xsl:text>
      <xsl:call-template name="held-name" />
      <xsl:text> = down; return true;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>	default: return false;
	}
}
</xsl:text>
  </xsl:template>

  <!-- frame(): the keys held, everything moving, the window's edges, the
       meetings. -->
  <xsl:template name="frame-function">
    <xsl:text>&#10;function frame(seconds) {&#10;</xsl:text>
    <xsl:if test="$moved">
      <xsl:text>	// the keys held
</xsl:text>
      <xsl:for-each select="$moved">
        <xsl:call-template name="held-moves" />
      </xsl:for-each>
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:text>	// everything moves
	for (const thing of </xsl:text>
    <xsl:call-template name="all-things" />
    <xsl:text>) {
		thing.position.x += thing.velocity.x * seconds;
		thing.position.y += thing.velocity.y * seconds;
	}
</xsl:text>
    <xsl:if test="$edge-owners">
      <xsl:text>
	// at the window's edges
</xsl:text>
      <xsl:for-each select="$edge-owners">
        <xsl:call-template name="edges" />
      </xsl:for-each>
    </xsl:if>
    <xsl:if test="$touch-owners">
      <xsl:text>
	// what meets what
</xsl:text>
      <xsl:for-each select="$touch-owners">
        <xsl:call-template name="meetings" />
      </xsl:for-each>
    </xsl:if>
    <xsl:text>}&#10;</xsl:text>
  </xsl:template>

  <!-- [ball, paddle1, paddle2]: these things, or every one. -->
  <xsl:template name="all-things">
    <xsl:param name="things" select="$objects" />
    <xsl:text>[</xsl:text>
    <xsl:for-each select="$things">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="object-name" />
    </xsl:for-each>
    <xsl:text>]</xsl:text>
  </xsl:template>

  <!-- A thing the keys move: still, then each held key's moves. -->
  <xsl:template name="held-moves">
    <xsl:variable name="object" select="." />
    <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#9;', $thing, '.velocity = { x: 0, y: 0 };&#10;')" />
    <xsl:for-each select="$inputs[trigger/@object = $object/@name]">
      <xsl:text>&#9;if (</xsl:text>
      <xsl:for-each select="@button | button">
        <xsl:if test="position() &gt; 1"> || </xsl:if>
        <xsl:call-template name="held-name" />
      </xsl:for-each>
      <xsl:text>) {</xsl:text>
      <xsl:for-each select="trigger[@object = $object/@name]">
        <xsl:variable name="action" select="@action" />
        <xsl:variable name="done" select="($object/actions/action[@name = $action] | $object/parent::group/actions/action[@name = $action])[1]" />
        <xsl:if test="not($done)">
          <xsl:message terminate="yes">web-js cannot find the action '<xsl:value-of select="$action" />' of '<xsl:value-of select="$object/@name" />'</xsl:message>
        </xsl:if>
        <xsl:for-each select="$done/*">
          <xsl:text> </xsl:text>
          <xsl:apply-templates select="." mode="held">
            <xsl:with-param name="who" select="$thing" />
            <xsl:with-param name="self" select="$object" />
          </xsl:apply-templates>
        </xsl:for-each>
      </xsl:for-each>
      <xsl:text> }&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- An object's or a group's <collision edge>s: for a group, each of its
       members in turn. -->
  <xsl:template name="edges">
    <xsl:choose>
      <xsl:when test="self::group">
        <xsl:variable name="one">
          <xsl:call-template name="camel">
            <xsl:with-param name="name">
              <xsl:choose>
                <xsl:when test="substring(@name, string-length(@name)) = 's'"><xsl:value-of select="substring(@name, 1, string-length(@name) - 1)" /></xsl:when>
                <xsl:otherwise><xsl:value-of select="concat(@name, '-one')" /></xsl:otherwise>
              </xsl:choose>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;for (const ', $one, ' of ')" />
        <xsl:call-template name="all-things"><xsl:with-param name="things" select="member" /></xsl:call-template>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($one, '.')" />
            <xsl:with-param name="indent" select="'&#9;&#9;'" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($thing, '.')" />
            <xsl:with-param name="indent" select="'&#9;'" />
          </xsl:call-template>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- One <collision edge>: an if for each edge it names, or one if for
       them all when it does the same at each. -->
  <xsl:template name="edge-rule">
    <xsl:param name="of" />
    <xsl:param name="indent" />
    <xsl:variable name="rule" select="." />
    <xsl:variable name="named">
      <xsl:choose>
        <xsl:when test="@edge = 'vertical'"><edge>top</edge><edge>bottom</edge></xsl:when>
        <xsl:when test="@edge = 'horizontal'"><edge>left</edge><edge>right</edge></xsl:when>
        <xsl:when test="@edge = 'all'"><edge>top</edge><edge>bottom</edge><edge>left</edge><edge>right</edge></xsl:when>
        <xsl:otherwise><edge><xsl:value-of select="@edge" /></edge></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="written">
      <xsl:for-each select="exsl:node-set($named)/edge">
        <xsl:variable name="edge" select="string(.)" />
        <rule>
          <test>
            <xsl:choose>
              <xsl:when test="$edge = 'top'"><xsl:value-of select="concat($of, 'position.y &lt; 0')" /></xsl:when>
              <xsl:when test="$edge = 'bottom'"><xsl:value-of select="concat($of, 'position.y &gt; windowHeight - ', $of, 'size.y')" /></xsl:when>
              <xsl:when test="$edge = 'left'"><xsl:value-of select="concat($of, 'position.x &lt; 0')" /></xsl:when>
              <xsl:otherwise><xsl:value-of select="concat($of, 'position.x &gt; windowWidth - ', $of, 'size.x')" /></xsl:otherwise>
            </xsl:choose>
          </test>
          <code>
            <xsl:for-each select="$rule/*">
              <xsl:if test="position() &gt; 1"><xsl:text> </xsl:text></xsl:if>
              <xsl:apply-templates select="." mode="edge">
                <xsl:with-param name="of" select="$of" />
                <xsl:with-param name="edge" select="$edge" />
              </xsl:apply-templates>
            </xsl:for-each>
          </code>
        </rule>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="rules" select="exsl:node-set($written)/rule" />
    <xsl:choose>
      <xsl:when test="count($rules) &gt; 1 and not($rules/code != $rules[1]/code)">
        <xsl:value-of select="concat($indent, 'if (')" />
        <xsl:for-each select="$rules">
          <xsl:if test="position() &gt; 1"> || </xsl:if>
          <xsl:value-of select="test" />
        </xsl:for-each>
        <xsl:value-of select="concat(') { ', $rules[1]/code, ' }&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="$rules">
          <xsl:value-of select="concat($indent, 'if (', test, ') { ', code, ' }&#10;')" />
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- An object's <collision>s naming no edge: on meeting any other thing. -->
  <xsl:template name="meetings">
    <xsl:variable name="object" select="." />
    <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="others" select="$objects[generate-id() != generate-id($object)]" />
    <xsl:variable name="rules" select="collisions/collision[not(@edge)]/*" />
    <xsl:choose>
      <xsl:when test="count($others) = 1">
        <xsl:variable name="other"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$others" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat('&#9;if (touching(', $thing, ', ', $other, ')) {')" />
        <xsl:for-each select="$rules">
          <xsl:text> </xsl:text>
          <xsl:apply-templates select="." mode="touch">
            <xsl:with-param name="self" select="$thing" />
            <xsl:with-param name="other" select="$other" />
          </xsl:apply-templates>
        </xsl:for-each>
        <xsl:text> }&#10;</xsl:text>
      </xsl:when>
      <xsl:when test="$others">
        <xsl:text>&#9;for (const other of </xsl:text>
        <xsl:call-template name="all-things"><xsl:with-param name="things" select="$others" /></xsl:call-template>
        <xsl:value-of select="concat(') {&#10;&#9;&#9;if (touching(', $thing, ', other)) {')" />
        <xsl:for-each select="$rules">
          <xsl:text> </xsl:text>
          <xsl:apply-templates select="." mode="touch">
            <xsl:with-param name="self" select="$thing" />
            <xsl:with-param name="other" select="'other'" />
          </xsl:apply-templates>
        </xsl:for-each>
        <xsl:text> }&#10;&#9;}&#10;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- The page's part: the canvas, the keys in, a frame each time the
       browser draws, and the picture. -->
  <xsl:template name="page-script">
    <xsl:text>
// The page: the canvas as large as the browser's window holds, the game's
// proportions kept, with a canvas pixel for each of the screen's.
const canvas = document.getElementById("game");
const context = canvas.getContext("2d");

function fit() {
	const scale = Math.min(innerWidth / windowWidth, innerHeight / windowHeight);
	canvas.style.width = Math.floor(windowWidth * scale) + "px";
	canvas.style.height = Math.floor(windowHeight * scale) + "px";
	canvas.width = Math.max(1, Math.round(windowWidth * scale * devicePixelRatio));
	canvas.height = Math.max(1, Math.round(windowHeight * scale * devicePixelRatio));
}
addEventListener("resize", fit);
fit();

// Keys in. A key the game uses is kept from the page (the arrows would
// scroll it), and away from the page nothing stays held.
const held = new Set();
addEventListener("keydown", (event) =&gt; {
	if (key(event.code, true)) { held.add(event.code); event.preventDefault(); }
});
addEventListener("keyup", (event) =&gt; {
	if (key(event.code, false)) { held.delete(event.code); event.preventDefault(); }
});
addEventListener("blur", () =&gt; {
	for (const code of held) { key(code, false); }
	held.clear();
});

function draw(thing) {
	context.fillRect(thing.position.x, thing.position.y, thing.size.x, thing.size.y);
}

// A frame each time the browser draws: the time since the last one, at most
// a quarter of a second, then the picture.
let last = performance.now();
function play(now) {
	frame(Math.min(Math.max(now - last, 0) / 1000, 0.25));
	last = now;

	context.setTransform(canvas.width / windowWidth, 0, 0, canvas.height / windowHeight, 0, 0);
	context.fillStyle = "black";
	context.fillRect(0, 0, windowWidth, windowHeight);
	context.fillStyle = "white";
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:text>&#9;draw(</xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text>);&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>	requestAnimationFrame(play);
}
requestAnimationFrame(play);
</xsl:text>
  </xsl:template>

  <!-- A verb web-js has no words for yet, in any mode. -->
  <xsl:template match="*" mode="edge">
    <xsl:message terminate="yes">web-js cannot write &lt;<xsl:value-of select="local-name()" />&gt; at an edge yet; use web-asm for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="touch">
    <xsl:message terminate="yes">web-js cannot write &lt;<xsl:value-of select="local-name()" />&gt; on a touch yet; use web-asm for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="held">
    <xsl:message terminate="yes">web-js cannot write &lt;<xsl:value-of select="local-name()" />&gt; in an action yet; use web-asm for this game</xsl:message>
  </xsl:template>
</xsl:stylesheet>
