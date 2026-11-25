<?xml version="1.0" encoding="UTF-8"?>
<!-- game.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- The page: <game>.html, its numbers, its things and its rules in a
     module script (its names its own, apart from the page's, such as
     window.top), then the few lines that play them on a canvas. Each thing is a
     plain object (where it is, how it moves, how big it is, and what it
     counts); the screens are a stack of their names. key() and pad() note
     which buttons are down and answer the presses of the screen on top;
     frame() plays the played screen: the keys held (and the computer's),
     everything moving, the window's edges, the meetings and the game's own
     tests. Each command is written by its verb's file (verbs/) in one of
     four modes: held (a key held, on who), edge (at one edge, on who and
     that edge), touch (on a meeting, on who and other) and do (anywhere).
     Nothing of the engine is in it. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <!-- What the game has, and so what it is written with. -->
  <xsl:variable name="inputs" select="$states/inputs//input" />
  <xsl:variable name="held-inputs" select="$inputs[trigger]" />
  <xsl:variable name="held-list">
    <xsl:for-each select="$held-inputs/@button | $held-inputs/button">
      <b><xsl:call-template name="button-of" /></b>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="held-keys" select="exsl:node-set($held-list)/b[not(. = preceding-sibling::b)][not(starts-with(., 'pad'))]" />
  <xsl:variable name="held-pads" select="exsl:node-set($held-list)/b[not(. = preceding-sibling::b)][starts-with(., 'pad')]" />
  <xsl:variable name="all-list">
    <xsl:for-each select="$inputs/@button | $inputs/button">
      <b><xsl:call-template name="button-of" /></b>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="all-keys" select="exsl:node-set($all-list)/b[not(. = preceding-sibling::b)][not(starts-with(., 'pad'))]" />
  <xsl:variable name="uses-pads" select="boolean(exsl:node-set($all-list)/b[starts-with(., 'pad')])" />
  <xsl:variable name="computer" select="$played/computer" />
  <xsl:variable name="moved" select="$things[@name = $held-inputs/trigger/@object]" />
  <xsl:variable name="owners" select="$played/objects/object[count(. | $things) = count($things)] | $played/objects/group[member[count(. | $things) = count($things)]]" />
  <xsl:variable name="edge-owners" select="$owners[collisions/collision[@edge]] | $grids[collisions/collision[@edge]]" />
  <!-- grids whose cells move: the way across of their rows -->
  <xsl:variable name="moving-grids" select="$grids[row/velocity]" />
  <xsl:variable name="touch-owners" select="$owners[self::object][collisions/collision[not(@edge)] or @name = $grids/collisions/collision/@object]
      | $owners[self::group][collisions/collision[not(@edge)]]" />
  <!-- what touches by its picture's lit pixels -->
  <xsl:variable name="pixel-touchers" select="$owners[collisions/type = 'pixel']" />
  <xsl:variable name="new-game" select="boolean($inputs/reset[not(@object)])" />
  <xsl:variable name="reset-names" select="//reset/@object | //collision/reset[not(@object)]/ancestor::object[1]/@name" />
  <xsl:variable name="started" select="($things | $records)[not(@name = $kept/@name)][count(. | $things) = count($things)]
      [count(. | $records) = count($records) or .//random or @name = $reset-names] | $grids" />
  <!-- members that go back where they started: their start kept in their record -->
  <xsl:variable name="returners" select="$states/objects/group[member][collisions/collision/reset[not(@* or node())]]/member" />
  <xsl:variable name="randoms" select="$everything//random" />
  <xsl:variable name="deflects" select="$played//deflect" />
  <xsl:variable name="sounds" select="/game/sounds/sound" />
  <xsl:variable name="uses-people" select="boolean(//people or $computer)" />
  <xsl:variable name="uses-angles" select="boolean($everything//sine | $everything//cosine | $deflects)" />
  <xsl:variable name="styles" select="/game/styles/style[@name = ($texts | $text-rows)/sprite/text/@style]" />
  <xsl:variable name="touch-bounces" select="$played/objects/object/collisions/collision[not(@edge)]/bounce" />
  <xsl:variable name="mouse-input" select="$states/inputs//input[@axis = 'mouse.x']" />
  <!-- the actions rules run, each a function of its own -->
  <xsl:variable name="called" select="$everything/actions/action[@name = //collision/trigger[not(@object)]/@action]" />
  <!-- sounds that name their wave, slide or loudness, or have several notes -->
  <xsl:variable name="shaped-sounds" select="$sounds[@wave or note/@wave or note/@to or volume or count(note) &gt; 1]" />

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
&lt;script type="module"&gt;
const windowWidth = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/width" /></xsl:call-template>
    <xsl:text>;&#10;const windowHeight = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/height" /></xsl:call-template>
    <xsl:text>;&#10;</xsl:text>
    <xsl:call-template name="constants" />
    <xsl:call-template name="helpers" />
    <xsl:for-each select="$pictures">
      <xsl:value-of select="concat('&#10;// The ', @name, ' picture, a row a line: lit where it is not a dot.&#10;const ')" />
      <xsl:call-template name="picture-name" />
      <xsl:text> = [</xsl:text>
      <xsl:for-each select="row">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:value-of select="concat('&quot;', normalize-space(.), '&quot;')" />
      </xsl:for-each>
      <xsl:text>];&#10;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$started">
      <xsl:choose>
        <xsl:when test="count(. | $grids) = count($grids)"><xsl:call-template name="grid-start-function" /></xsl:when>
        <xsl:otherwise><xsl:call-template name="start-function" /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>
// Each thing on the screen: where it is, how it moves, how big it is</xsl:text>
    <xsl:if test="$records">, and what it counts</xsl:if>
    <xsl:text>.&#10;</xsl:text>
    <xsl:if test="$uses-people"><xsl:text>let people = 1;&#10;</xsl:text></xsl:if>
    <xsl:for-each select="$everything[count(. | $things | $records) = count($things | $records)] | $grids">
      <xsl:text>let </xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text> = </xsl:text>
      <xsl:call-template name="start-value" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:if test="$screens">
      <xsl:text>&#10;// The screens, the one on top played.&#10;let screens = [</xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$states[1]/@name" /></xsl:call-template>
      <xsl:text>];
function screen() { return screens[screens.length - 1]; }
</xsl:text>
    </xsl:if>
    <xsl:if test="$new-game">
      <xsl:text>
// A new game: everything as it starts but what is kept, from the first screen.
function newGame() {
</xsl:text>
      <xsl:for-each select="($things | $records)[not(@name = $kept/@name)] | $grids">
        <xsl:text>&#9;</xsl:text>
        <xsl:call-template name="object-name" />
        <xsl:text> = </xsl:text>
        <xsl:call-template name="start-value" />
        <xsl:text>;&#10;</xsl:text>
      </xsl:for-each>
      <xsl:if test="$screens">
        <xsl:text>&#9;screens = [</xsl:text>
        <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$states[1]/@name" /></xsl:call-template>
        <xsl:text>];&#10;</xsl:text>
      </xsl:if>
      <xsl:text>}&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$kept">
      <xsl:call-template name="keep-functions" />
    </xsl:if>
    <xsl:if test="$sounds and not($shaped-sounds)">
      <xsl:call-template name="sound-function" />
    </xsl:if>
    <xsl:if test="$shaped-sounds">
      <xsl:call-template name="shaped-sound-function" />
    </xsl:if>
    <xsl:call-template name="key-function" />
    <xsl:if test="$uses-pads">
      <xsl:call-template name="pad-function" />
    </xsl:if>
    <xsl:if test="$mouse-input">
      <xsl:call-template name="mouse-function" />
    </xsl:if>
    <xsl:call-template name="frame-function" />
    <xsl:for-each select="$called">
      <xsl:call-template name="action-function" />
    </xsl:for-each>
    <xsl:if test="$computer">
      <xsl:call-template name="computer-function" />
    </xsl:if>
    <xsl:call-template name="page-script" />
    <xsl:text>&lt;/script&gt;
&lt;/body&gt;
&lt;/html&gt;
</xsl:text>
  </xsl:template>

  <!-- The game's <variables> and the objects' that nothing changes, each a
       constant: those the game reads. -->
  <xsl:template name="constants">
    <xsl:variable name="written">
      <xsl:for-each select="/game/variables/variable">
        <xsl:variable name="named" select="@name" />
        <xsl:if test="//*[not(*)][normalize-space(.) = $named][not(self::variable[@name = $named])]">
          <xsl:text>const </xsl:text>
          <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text> = </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:for-each select="$everything/variables/variable | $states/objects/group/variables/variable">
        <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
        <xsl:variable name="owner" select="../.." />
        <xsl:variable name="named" select="@name" />
        <xsl:if test="$kind = 'constant' and //*[not(*)][normalize-space(.) = concat($owner/@name, '.', $named) or normalize-space(.) = concat($owner/member/@name, '.', $named)
            or (self::self and normalize-space(.) = $named and ancestor::*[@name = $owner/@name])]">
          <xsl:text>const </xsl:text>
          <xsl:call-template name="constant-name" />
          <xsl:text> = </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:if test="string($written) != ''">
      <xsl:value-of select="concat('&#10;', $written)" />
    </xsl:if>
  </xsl:template>

  <!-- startBall: the ball as it starts, and starts again. -->
  <xsl:template name="start-name">
    <xsl:param name="object" select="." />
    <xsl:variable name="camel"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('start', translate(substring($camel, 1, 1), $lower, $upper), substring($camel, 2))" />
  </xsl:template>

  <!-- What a thing or record is as it starts: its start function's, or
       written out. -->
  <xsl:template name="start-value">
    <xsl:choose>
      <xsl:when test="count(. | $started) = count($started)">
        <xsl:call-template name="start-name" />
        <xsl:text>()</xsl:text>
      </xsl:when>
      <xsl:when test="count(. | $things) = count($things)">
        <xsl:text>{ position: </xsl:text>
        <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
        <xsl:text>, velocity: </xsl:text>
        <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity | parent::group/velocity[not(current()/velocity)]" /></xsl:call-template>
        <xsl:text>, size: </xsl:text>
        <xsl:call-template name="size" />
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="parts">
          <xsl:call-template name="record-parts" />
        </xsl:variable>
        <xsl:text>{ </xsl:text>
        <xsl:for-each select="exsl:node-set($parts)/part">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:value-of select="." />
        </xsl:for-each>
        <xsl:text> }</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- look: "easy", level: 1: a record's look and live variables as they start. -->
  <xsl:template name="record-parts">
    <xsl:param name="start" select="false()" />
    <xsl:variable name="object" select="." />
    <xsl:if test="count(sprite) &gt; 1">
      <part><xsl:value-of select="concat('look: &quot;', sprite[1]/@name, '&quot;')" /></part>
    </xsl:if>
    <xsl:for-each select="variables/variable | parent::group/variables/variable">
      <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
      <xsl:if test="$kind = 'live'">
        <part>
          <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>: </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$object" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        </part>
      </xsl:if>
    </xsl:for-each>
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
      <xsl:if test="$pixel-touchers">
        <part>// Whether a lit pixel of `picture`, drawn over `thing`, overlaps `other`.
function touchingPixels(thing, picture, other) {
	if (!touching(thing, other)) { return false; }
	const pixel = { x: thing.size.x / picture[0].length, y: thing.size.y / picture.length };
	for (let row = 0; row &lt; picture.length; ++row) {
		for (let column = 0; column &lt; picture[row].length; ++column) {
			if (picture[row][column] === ".") { continue; }
			const lit = { position: { x: thing.position.x + column * pixel.x, y: thing.position.y + row * pixel.y }, size: pixel };
			if (touching(lit, other)) { return true; }
		}
	}
	return false;
}
</part>
      </xsl:if>
      <xsl:if test="$touch-bounces">
        <part>// Off the side of `other` it hit: its speed into that side turned round.
function bounce(thing, other) {
	const apart = {
		x: thing.position.x + thing.size.x / 2 - (other.position.x + other.size.x / 2),
		y: thing.position.y + thing.size.y / 2 - (other.position.y + other.size.y / 2),
	};
	const reach = { x: (thing.size.x + other.size.x) / 2, y: (thing.size.y + other.size.y) / 2 };
	if (reach.x - Math.abs(apart.x) &lt; reach.y - Math.abs(apart.y)) {
		thing.velocity.x = apart.x &lt; 0 ? -Math.abs(thing.velocity.x) : Math.abs(thing.velocity.x);
	} else {
		thing.velocity.y = apart.y &lt; 0 ? -Math.abs(thing.velocity.y) : Math.abs(thing.velocity.y);
	}
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
    <xsl:param name="start" select="false()" />
    <xsl:choose>
      <xsl:when test="$of">
        <xsl:text>{ x: </xsl:text>
        <xsl:call-template name="axis-value"><xsl:with-param name="node" select="$of/x" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'x'" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text>, y: </xsl:text>
        <xsl:call-template name="axis-value"><xsl:with-param name="node" select="$of/y" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'y'" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise>{ x: 0, y: 0 }</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- One axis of a pair: its value, or 0 when the file leaves it out. -->
  <xsl:template name="axis-value">
    <xsl:param name="node" />
    <xsl:param name="self" />
    <xsl:param name="axis" />
    <xsl:param name="start" />
    <xsl:choose>
      <xsl:when test="$node"><xsl:call-template name="value"><xsl:with-param name="node" select="$node" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="$axis" /><xsl:with-param name="start" select="$start" /></xsl:call-template></xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- { x: width, y: height } of its rectangle or picture, or its group's. -->
  <xsl:template name="size">
    <xsl:variable name="sprite" select="(sprite | parent::group/sprite)[1]" />
    <xsl:variable name="bitmap" select="$pictures[@name = $sprite/bitmap/@from]" />
    <xsl:choose>
      <xsl:when test="$bitmap">
        <xsl:variable name="scale"><xsl:call-template name="picture-scale"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat('{ x: ', string-length(normalize-space($bitmap/row[1])) * $scale, ', y: ', count($bitmap/row) * $scale, ' }')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>{ x: </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/width" /><xsl:with-param name="self" select="." /></xsl:call-template>
        <xsl:text>, y: </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/height" /><xsl:with-param name="self" select="." /></xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- startBall(): its numbers drawn at random (drawn again each time),
       then the thing: its place, way and size, and what it counts. -->
  <xsl:template name="start-function">
    <xsl:variable name="object" select="." />
    <xsl:text>&#10;</xsl:text>
    <xsl:if test="@name = $reset-names">
      <xsl:value-of select="concat('// The ', @name, ' as it starts, and starts again.&#10;')" />
    </xsl:if>
    <xsl:text>function </xsl:text>
    <xsl:call-template name="start-name" />
    <xsl:text>() {&#10;</xsl:text>
    <xsl:for-each select="variables/variable | parent::group/variables/variable">
      <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
      <xsl:if test="$kind = 'start'">
        <xsl:text>&#9;const </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$object" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
        <xsl:text>;&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:variable name="returns" select="count(. | $returners) = count($returners)" />
    <xsl:choose>
      <xsl:when test="$returns">
        <!-- where it starts, kept to go back to -->
        <xsl:text>&#9;const position = </xsl:text>
        <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
        <xsl:text>;&#10;&#9;return {&#10;&#9;&#9;position,</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#9;return {&#10;&#9;&#9;position: </xsl:text>
        <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
        <xsl:text>,</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&#10;&#9;&#9;velocity: </xsl:text>
    <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity | parent::group/velocity[not(current()/velocity)]" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
    <xsl:text>,&#10;&#9;&#9;size: </xsl:text>
    <xsl:call-template name="size" />
    <xsl:text>,&#10;</xsl:text>
    <xsl:if test="$returns"><xsl:text>&#9;&#9;start: { ...position },&#10;</xsl:text></xsl:if>
    <xsl:variable name="parts"><xsl:call-template name="record-parts"><xsl:with-param name="start" select="true()" /></xsl:call-template></xsl:variable>
    <xsl:for-each select="exsl:node-set($parts)/part">
      <xsl:value-of select="concat('&#9;&#9;', ., ',&#10;')" />
    </xsl:for-each>
    <xsl:text>&#9;};&#10;}&#10;</xsl:text>
  </xsl:template>

  <!-- keep() and its reading back: each kept object's look and live
       variables, in the browser's storage for this page. -->
  <xsl:template name="keep-functions">
    <xsl:variable name="kept-list">
      <xsl:for-each select="$kept">
        <xsl:variable name="object" select="." />
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:if test="count(sprite) &gt; 1">
          <value member="{$thing}.look" check="[{concat('&quot;', sprite[1]/@name, '&quot;')}{concat(', &quot;', sprite[position() &gt; 1]/@name, '&quot;')}].includes(kept.{$thing}Look)">
            <xsl:attribute name="check">
              <xsl:text>[</xsl:text>
              <xsl:for-each select="sprite">
                <xsl:if test="position() &gt; 1">, </xsl:if>
                <xsl:value-of select="concat('&quot;', @name, '&quot;')" />
              </xsl:for-each>
              <xsl:value-of select="concat('].includes(kept.', $thing, 'Look)')" />
            </xsl:attribute>
            <xsl:attribute name="key"><xsl:value-of select="concat($thing, 'Look')" /></xsl:attribute>
          </value>
        </xsl:if>
        <xsl:for-each select="variables/variable | parent::group/variables/variable">
          <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
          <xsl:if test="$kind = 'live'">
            <xsl:variable name="camel"><xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
            <xsl:variable name="key"><xsl:call-template name="camel"><xsl:with-param name="name" select="concat($object/@name, ' ', @name)" /></xsl:call-template></xsl:variable>
            <value member="{$thing}.{$camel}" key="{$key}" check="typeof kept.{$key} === &quot;number&quot;" />
          </xsl:if>
        </xsl:for-each>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="values" select="exsl:node-set($kept-list)/value" />
    <xsl:value-of select="concat('&#10;// What is kept from one game to the next, in the browser for this page.&#10;function keep() {&#10;&#9;try {&#10;&#9;&#9;localStorage.setItem(&quot;', $name, '-kept&quot;, JSON.stringify({ ')" />
    <xsl:for-each select="$values">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:value-of select="concat(@key, ': ', @member)" />
    </xsl:for-each>
    <xsl:text> }));
	} catch {
		// a page the browser keeps nothing for plays on
	}
}

function readKept() {
	try {
		const kept = JSON.parse(localStorage.getItem("</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>-kept")) ?? {};
</xsl:text>
    <xsl:for-each select="$values">
      <xsl:value-of select="concat('&#9;&#9;if (', @check, ') { ', @member, ' = kept.', @key, '; }&#10;')" />
    </xsl:for-each>
    <xsl:text>	} catch {
		// nothing kept yet
	}
}
</xsl:text>
  </xsl:template>

  <!-- play(): a sound, a square wave each, as the page can once a key is
       pressed. -->
  <xsl:template name="sound-function">
    <xsl:text>
// The sounds, a square wave each: how high, how long.
const sounds = {
</xsl:text>
    <xsl:for-each select="$sounds">
      <xsl:variable name="hertz"><xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(note/@pitch)" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#9;', @name, ': { hertz: ', $hertz, ', seconds: ', normalize-space(note), ' },&#10;')" />
    </xsl:for-each>
    <xsl:text>};
let audio = null;

// A sound, faded in and out over two thousandths of a second so it doesn't
// click. The browser lets a page make sounds once a key has been pressed.
function play(sound) {
	audio ??= new AudioContext();
	const { hertz, seconds } = sounds[sound];
	const now = audio.currentTime;
	const tone = audio.createOscillator();
	const volume = audio.createGain();
	tone.type = "square";
	tone.frequency.value = hertz;
	volume.gain.setValueAtTime(0, now);
	volume.gain.linearRampToValueAtTime(0.15, now + 0.002);
	volume.gain.setValueAtTime(0.15, now + seconds - 0.002);
	volume.gain.linearRampToValueAtTime(0, now + seconds);
	tone.connect(volume).connect(audio.destination);
	tone.start(now);
	tone.stop(now + seconds);
}
</xsl:text>
  </xsl:template>

  <!-- play(): a sound, as the page can once a key is pressed: each its
       note or notes, and the wave, slide and loudness of those that have
       their own. -->
  <xsl:template name="shaped-sound-function">
    <xsl:variable name="noisy" select="$sounds[@wave = 'noise' or note/@wave = 'noise']" />
    <xsl:variable name="tunes" select="$sounds[count(note) &gt; 1]" />
    <xsl:text>
// The sounds: how high, how long, and where a sound has its own, its wave,
// the pitch it slides to and how loud it is</xsl:text>
    <xsl:if test="$tunes">; a tune is its notes, one after another</xsl:if>
    <xsl:text>.
const sounds = {
</xsl:text>
    <xsl:for-each select="$sounds">
      <xsl:value-of select="concat('&#9;', @name, ': {')" />
      <xsl:choose>
        <xsl:when test="count(note) &gt; 1">
          <xsl:if test="@wave"><xsl:value-of select="concat(' wave: &quot;', @wave, '&quot;,')" /></xsl:if>
          <xsl:if test="volume"><xsl:value-of select="concat(' volume: ', normalize-space(volume), ',')" /></xsl:if>
          <xsl:text> notes: [&#10;</xsl:text>
          <xsl:for-each select="note">
            <xsl:text>&#9;&#9;{ </xsl:text>
            <xsl:call-template name="note-parts"><xsl:with-param name="wave" select="@wave" /></xsl:call-template>
            <xsl:text> },&#10;</xsl:text>
          </xsl:for-each>
          <xsl:text>&#9;] },&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text> </xsl:text>
          <xsl:for-each select="note"><xsl:call-template name="note-parts"><xsl:with-param name="wave" select="(@wave | ../@wave)[1]" /></xsl:call-template></xsl:for-each>
          <xsl:if test="volume"><xsl:value-of select="concat(', volume: ', normalize-space(volume))" /></xsl:if>
          <xsl:text> },&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>};
let audio = null;
</xsl:text>
    <xsl:if test="$noisy">
      <xsl:text>
// Noise, `seconds` long: a new random level every half cycle, held in
// between, so the pitch sets how coarse the hiss is.
function noise(hertz, to, seconds) {
	const buffer = audio.createBuffer(1, Math.ceil(seconds * audio.sampleRate), audio.sampleRate);
	const samples = buffer.getChannelData(0);
	let phase = 0;
	let held = Math.random() * 2 - 1;
	for (let i = 0; i &lt; samples.length; ++i) {
		samples[i] = held;
		phase += hertz * Math.pow(to / hertz, i / samples.length) / audio.sampleRate;
		if (phase &gt;= 0.5) {
			held = Math.random() * 2 - 1;
			phase -= 0.5;
		}
	}
	const source = audio.createBufferSource();
	source.buffer = buffer;
	return source;
}
</xsl:text>
    </xsl:if>
    <xsl:text>
// A sound</xsl:text>
    <xsl:if test="$tunes">, note by note</xsl:if>
    <xsl:text>, sliding evenly in pitch where it slides, faded in and
// out over two thousandths of a second so it doesn't click, at half its
// loudness as every sound here is. The browser lets a page make sounds once
// a key has been pressed.
function play(name) {
	audio ??= new AudioContext();
	const sound = sounds[name];
	const volume = sound.volume ?? 0.3;
	let at = audio.currentTime;
	for (const { hertz, seconds, wave = sound.wave ?? "square", to = hertz } of </xsl:text>
    <xsl:choose>
      <xsl:when test="$tunes">sound.notes ?? [sound]</xsl:when>
      <xsl:otherwise>[sound]</xsl:otherwise>
    </xsl:choose>
    <xsl:text>) {
</xsl:text>
    <xsl:choose>
      <xsl:when test="$noisy">
        <xsl:text>		let tone;
		if (wave === "noise") {
			tone = noise(hertz, to, seconds);
		} else {
			tone = audio.createOscillator();
			tone.type = wave;
			tone.frequency.setValueAtTime(hertz, at);
			tone.frequency.exponentialRampToValueAtTime(to, at + seconds);
		}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>		const tone = audio.createOscillator();
		tone.type = wave;
		tone.frequency.setValueAtTime(hertz, at);
		tone.frequency.exponentialRampToValueAtTime(to, at + seconds);
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>		const fade = audio.createGain();
		fade.gain.setValueAtTime(0, at);
		fade.gain.linearRampToValueAtTime(volume / 2, at + 0.002);
		fade.gain.setValueAtTime(volume / 2, at + seconds - 0.002);
		fade.gain.linearRampToValueAtTime(0, at + seconds);
		tone.connect(fade).connect(audio.destination);
		tone.start(at);
		tone.stop(at + seconds);
		at += seconds;
	}
}
</xsl:text>
  </xsl:template>

  <!-- hertz: 329.63, seconds: 0.4, wave: "triangle", to: 82.41: a note's
       pitch and length, and its wave and slide where it has them. -->
  <xsl:template name="note-parts">
    <xsl:param name="wave" />
    <xsl:variable name="hertz"><xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(@pitch)" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('hertz: ', $hertz, ', seconds: ', normalize-space(.))" />
    <xsl:if test="$wave"><xsl:value-of select="concat(', wave: &quot;', $wave, '&quot;')" /></xsl:if>
    <xsl:if test="@to">
      <xsl:text>, to: </xsl:text>
      <xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(@to)" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- key(): which keys are down, then the presses the screen on top
       answers. -->
  <xsl:template name="key-function">
    <xsl:text>
// Which keys are down.
</xsl:text>
    <xsl:for-each select="$held-keys | $held-pads">
      <xsl:text>let </xsl:text>
      <xsl:call-template name="held-name"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
      <xsl:text> = false;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:variable name="presses">
      <xsl:call-template name="presses"><xsl:with-param name="pads" select="false()" /></xsl:call-template>
    </xsl:variable>
    <xsl:text>
// The keys the game uses, kept from the page (the arrows would scroll it).
const gameKeys = new Set([</xsl:text>
    <xsl:for-each select="$all-keys">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="key-code"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
    </xsl:for-each>
    <xsl:text>]);

function key(code, down) {
</xsl:text>
    <xsl:if test="$held-keys">
      <xsl:text>&#9;switch (code) {&#10;</xsl:text>
      <xsl:for-each select="$held-keys">
        <xsl:text>&#9;case </xsl:text>
        <xsl:call-template name="key-code"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
        <xsl:text>: </xsl:text>
        <xsl:call-template name="held-name"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
        <xsl:text> = down; break;&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>&#9;}&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="string($presses) != ''">
      <xsl:text>&#9;if (!down) { return; }&#10;&#10;</xsl:text>
      <xsl:value-of select="$presses" />
    </xsl:if>
    <xsl:text>}&#10;</xsl:text>
  </xsl:template>

  <!-- pad(): which pad buttons are down, then the presses. -->
  <xsl:template name="pad-function">
    <xsl:text>&#10;function pad(pad, button, down) {&#10;</xsl:text>
    <xsl:for-each select="$held-pads">
      <xsl:text>&#9;if (</xsl:text>
      <xsl:call-template name="pad-test"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
      <xsl:text>) { </xsl:text>
      <xsl:call-template name="held-name"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
      <xsl:text> = down; }&#10;</xsl:text>
    </xsl:for-each>
    <xsl:variable name="presses">
      <xsl:call-template name="presses"><xsl:with-param name="pads" select="true()" /></xsl:call-template>
    </xsl:variable>
    <xsl:if test="string($presses) != ''">
      <xsl:text>&#9;if (!down) { return; }&#10;&#10;</xsl:text>
      <xsl:value-of select="$presses" />
    </xsl:if>
    <xsl:text>}&#10;</xsl:text>
  </xsl:template>

  <!-- The presses of key() (or of pad()): with screens, a case for each
       screen that answers some. -->
  <xsl:template name="presses">
    <xsl:param name="pads" />
    <xsl:variable name="cases">
      <xsl:for-each select="$states">
        <xsl:variable name="lines">
          <xsl:for-each select="inputs//input[not(trigger)]">
            <xsl:variable name="test">
              <xsl:for-each select="@button | button">
                <xsl:variable name="button"><xsl:call-template name="button-of" /></xsl:variable>
                <xsl:if test="starts-with($button, 'pad') = $pads">
                  <xsl:text>|</xsl:text>
                  <xsl:choose>
                    <xsl:when test="$pads"><xsl:call-template name="pad-test"><xsl:with-param name="button" select="$button" /></xsl:call-template></xsl:when>
                    <xsl:otherwise>
                      <xsl:text>code === </xsl:text>
                      <xsl:call-template name="key-code"><xsl:with-param name="button" select="$button" /></xsl:call-template>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:if>
              </xsl:for-each>
            </xsl:variable>
            <xsl:if test="$test != ''">
              <line>
                <xsl:text>if (</xsl:text>
                <xsl:call-template name="join-tests"><xsl:with-param name="tests" select="substring($test, 2)" /><xsl:with-param name="bracket" select="$pads" /></xsl:call-template>
                <xsl:text>) {</xsl:text>
                <xsl:for-each select="*[not(self::button or self::people)] | people">
                  <xsl:text> </xsl:text>
                  <xsl:apply-templates select="." mode="do" />
                </xsl:for-each>
                <xsl:text> }</xsl:text>
              </line>
            </xsl:if>
          </xsl:for-each>
        </xsl:variable>
        <xsl:if test="exsl:node-set($lines)/line">
          <case>
            <xsl:attribute name="screen"><xsl:call-template name="screen-name"><xsl:with-param name="state" select="@name" /></xsl:call-template></xsl:attribute>
            <xsl:copy-of select="exsl:node-set($lines)/line" />
          </case>
        </xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="all" select="exsl:node-set($cases)/case" />
    <xsl:choose>
      <xsl:when test="not($all)" />
      <xsl:when test="not($screens)">
        <xsl:for-each select="$all/line"><xsl:value-of select="concat('&#9;', ., '&#10;')" /></xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#9;switch (screen()) {&#10;</xsl:text>
        <xsl:for-each select="$all">
          <xsl:value-of select="concat('&#9;case ', @screen, ':&#10;')" />
          <xsl:for-each select="line"><xsl:value-of select="concat('&#9;&#9;', ., '&#10;')" /></xsl:for-each>
          <xsl:text>&#9;&#9;break;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- a|b|c as a || b || c, each bracketed when it is an && itself. -->
  <xsl:template name="join-tests">
    <xsl:param name="tests" />
    <xsl:param name="bracket" />
    <xsl:param name="many" select="contains($tests, '|')" />
    <xsl:variable name="one">
      <xsl:choose>
        <xsl:when test="contains($tests, '|')"><xsl:value-of select="substring-before($tests, '|')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="$tests" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$bracket and $many"><xsl:value-of select="concat('(', $one, ')')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$one" /></xsl:otherwise>
    </xsl:choose>
    <xsl:if test="contains($tests, '|')">
      <xsl:text> || </xsl:text>
      <xsl:call-template name="join-tests">
        <xsl:with-param name="tests" select="substring-after($tests, '|')" />
        <xsl:with-param name="bracket" select="$bracket" />
        <xsl:with-param name="many" select="true()" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- frame(): the keys held, everything moving, the window's edges, the
       meetings, the game's own tests. -->
  <xsl:template name="frame-function">
    <xsl:text>&#10;function frame(elapsed) {&#10;</xsl:text>
    <xsl:for-each select="$states[count(. | $played) = 2][conditions/condition]">
      <!-- a screen that waits for keys, testing as it waits -->
      <xsl:text>&#9;if (screen() === </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="@name" /></xsl:call-template>
      <xsl:text>) {&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;&#9;// the ', @name, ' screen', &quot;'s own tests, the first that holds&#10;&quot;)" />
      <xsl:for-each select="conditions/condition">
        <xsl:call-template name="state-condition">
          <xsl:with-param name="before">
            <xsl:if test="position() &gt; 1">else </xsl:if>
          </xsl:with-param>
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:text>&#9;&#9;return;&#10;&#9;}&#10;</xsl:text>
    </xsl:for-each>
    <xsl:if test="$screens">
      <xsl:text>&#9;if (screen() !== </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$played/@name" /></xsl:call-template>
      <xsl:text>) { return; }&#10;&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$moved">
      <xsl:choose>
        <xsl:when test="$computer">
          <xsl:value-of select="concat('&#9;// the keys held, seat ', $computer/@seat, &quot;'s by the computer while nobody has it&#10;&#9;let seat&quot;, $computer/@seat, ' = { ')" />
          <xsl:for-each select="$played/inputs/seat[@number = $computer/@seat]/input[trigger]">
            <xsl:if test="position() &gt; 1">, </xsl:if>
            <xsl:call-template name="seat-key" />
            <xsl:text>: </xsl:text>
            <xsl:call-template name="held-test" />
          </xsl:for-each>
          <xsl:value-of select="concat(' };&#10;&#9;if (people &lt; ', $computer/@seat, ') { seat', $computer/@seat, ' = computerHolds(); }&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>&#9;// the keys held&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:for-each select="$moved">
        <xsl:call-template name="held-moves" />
      </xsl:for-each>
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:text>	// everything moves
	for (const thing of </xsl:text>
    <xsl:call-template name="all-things"><xsl:with-param name="spread" select="$moving-grids" /></xsl:call-template>
    <xsl:text>) {
		thing.position.x += thing.velocity.x * elapsed;
		thing.position.y += thing.velocity.y * elapsed;
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
    <xsl:if test="$played/conditions/condition">
      <xsl:text>
	// the game's own tests, the first that holds
</xsl:text>
      <xsl:for-each select="$played/conditions/condition">
        <xsl:call-template name="state-condition">
          <xsl:with-param name="before">
            <xsl:if test="position() &gt; 1">else </xsl:if>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:for-each>
    </xsl:if>
    <xsl:text>}&#10;</xsl:text>
  </xsl:template>

  <!-- One of the game's own tests and what it does; one holding a
       condition is tested with it (&&), and does what that one does. -->
  <xsl:template name="state-condition">
    <xsl:param name="before" />
    <xsl:param name="tests" select="''" />
    <xsl:param name="indent" select="'&#9;'" />
    <xsl:variable name="test">
      <xsl:call-template name="test"><xsl:with-param name="joined" select="$tests != '' or condition" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="all">
      <xsl:if test="$tests != ''"><xsl:value-of select="concat($tests, ' &amp;&amp; ')" /></xsl:if>
      <xsl:value-of select="$test" />
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="condition">
        <xsl:for-each select="condition">
          <xsl:call-template name="state-condition">
            <xsl:with-param name="before" select="$before" />
            <xsl:with-param name="tests" select="$all" />
            <xsl:with-param name="indent" select="$indent" />
          </xsl:call-template>
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="body">
          <xsl:for-each select="*[not(self::any or self::remaining or self::atleast or self::atmost or self::above or self::below or self::equal or self::unequal)]">
            <line><xsl:apply-templates select="." mode="do" /></line>
          </xsl:for-each>
        </xsl:variable>
        <xsl:call-template name="write-block">
          <xsl:with-param name="header" select="concat($before, 'if (', $all, ')')" />
          <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- mouse(): the thing the mouse steers, its middle at the mouse, kept in the window. -->
  <xsl:template name="mouse-function">
    <xsl:variable name="input" select="$mouse-input[1]" />
    <xsl:variable name="object" select="$things[@name = $input/trigger/@object]" />
    <xsl:variable name="thing"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('&#10;// The mouse across the window: the ', $object/@name, &quot;'s middle follows it, kept inside the window.&#10;function mouse(x) {&#10;&quot;)" />
    <xsl:if test="$screens">
      <xsl:text>&#9;if (screen() !== </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$input/ancestor::state/@name" /></xsl:call-template>
      <xsl:text>) { return; }&#10;</xsl:text>
    </xsl:if>
    <xsl:value-of select="concat('&#9;', $thing, '.position.x = Math.max(0, Math.min(windowWidth - ', $thing, '.size.x, x - ', $thing, '.size.x / 2));&#10;}&#10;')" />
  </xsl:template>

  <!-- An action a rule runs: faster(), its verbs on the thing it is of. -->
  <xsl:template name="action-function">
    <xsl:variable name="object" select="ancestor::object[1]" />
    <xsl:text>&#10;function </xsl:text>
    <xsl:call-template name="action-name"><xsl:with-param name="action" select="@name" /></xsl:call-template>
    <xsl:text>() {&#10;</xsl:text>
    <xsl:variable name="body">
      <xsl:for-each select="*">
        <line><xsl:apply-templates select="." mode="do"><xsl:with-param name="self" select="$object" /></xsl:apply-templates></line>
      </xsl:for-each>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
      <xsl:with-param name="indent" select="'&#9;'" />
    </xsl:call-template>
    <xsl:text>}&#10;</xsl:text>
  </xsl:template>

  <!-- up: a held input of the computer's seat, by its first button. -->
  <xsl:template name="seat-key">
    <xsl:param name="input" select="." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name" select="normalize-space(($input/@button | $input/button)[1])" />
    </xsl:call-template>
  </xsl:template>

  <!-- wHeld || pad1UpHeld: any of an input's buttons down. -->
  <xsl:template name="held-test">
    <xsl:param name="input" select="." />
    <xsl:for-each select="$input/@button | $input/button">
      <xsl:if test="position() &gt; 1"> || </xsl:if>
      <xsl:call-template name="held-name"><xsl:with-param name="button"><xsl:call-template name="button-of" /></xsl:with-param></xsl:call-template>
    </xsl:for-each>
  </xsl:template>

  <!-- [ball, paddle1, paddle2]: these things, or every one. -->
  <xsl:template name="all-things">
    <xsl:param name="things" select="$things" />
    <xsl:param name="spread" select="/.." />
    <xsl:text>[</xsl:text>
    <xsl:for-each select="$things">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="object-name" />
    </xsl:for-each>
    <!-- and every cell of these grids -->
    <xsl:for-each select="$spread">
      <xsl:text>, ...</xsl:text>
      <xsl:call-template name="object-name" />
    </xsl:for-each>
    <xsl:text>]</xsl:text>
  </xsl:template>

  <!-- A thing the keys move: still, then each held key's moves. -->
  <xsl:template name="held-moves">
    <xsl:variable name="object" select="." />
    <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#9;', $thing, '.velocity = { x: 0, y: 0 };&#10;')" />
    <xsl:for-each select="$played/inputs//input[not(@axis)][trigger/@object = $object/@name]">
      <xsl:text>&#9;if (</xsl:text>
      <xsl:choose>
        <xsl:when test="$computer and ancestor::seat/@number = $computer/@seat">
          <xsl:value-of select="concat('seat', $computer/@seat, '.')" />
          <xsl:call-template name="seat-key" />
        </xsl:when>
        <xsl:otherwise><xsl:call-template name="held-test" /></xsl:otherwise>
      </xsl:choose>
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
      <xsl:when test="count(. | $grids) = count($grids)">
        <xsl:variable name="cell"><xsl:call-template name="cell-name" /></xsl:variable>
        <xsl:value-of select="concat('&#9;for (const ', $cell, ' of ')" />
        <xsl:call-template name="object-name" />
        <xsl:text>) {&#10;</xsl:text>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($cell, '.')" />
            <xsl:with-param name="self" select="../.." />
            <xsl:with-param name="indent" select="'&#9;&#9;'" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:when>
      <xsl:when test="self::group">
        <xsl:variable name="one">
          <xsl:call-template name="camel">
            <xsl:with-param name="name"><xsl:call-template name="singular"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:with-param>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;for (const ', $one, ' of ')" />
        <xsl:call-template name="all-things"><xsl:with-param name="things" select="member" /></xsl:call-template>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($one, '.')" />
            <xsl:with-param name="self" select="../.." />
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
            <xsl:with-param name="self" select="../.." />
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
    <xsl:param name="self" />
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
          <xsl:for-each select="$rule/*">
            <line>
              <xsl:apply-templates select="." mode="edge">
                <xsl:with-param name="of" select="$of" />
                <xsl:with-param name="self" select="$self" />
                <xsl:with-param name="edge" select="$edge" />
              </xsl:apply-templates>
            </line>
          </xsl:for-each>
        </rule>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="rules" select="exsl:node-set($written)/rule" />
    <xsl:variable name="first-code">
      <xsl:for-each select="$rules[1]/line"><xsl:value-of select="concat(., '|')" /></xsl:for-each>
    </xsl:variable>
    <xsl:variable name="differ">
      <xsl:for-each select="$rules">
        <xsl:variable name="code"><xsl:for-each select="line"><xsl:value-of select="concat(., '|')" /></xsl:for-each></xsl:variable>
        <xsl:if test="$code != $first-code">x</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="not($rule/*[not(self::wrap)])">
        <!-- a wrap writes its own test: its lines as they are -->
        <xsl:call-template name="print-lines">
          <xsl:with-param name="lines" select="$rules/line" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="count($rules) &gt; 1 and $differ = ''">
        <xsl:variable name="header">
          <xsl:text>if (</xsl:text>
          <xsl:for-each select="$rules">
            <xsl:if test="position() &gt; 1"> || </xsl:if>
            <xsl:value-of select="test" />
          </xsl:for-each>
          <xsl:text>)</xsl:text>
        </xsl:variable>
        <xsl:call-template name="write-block">
          <xsl:with-param name="header" select="$header" />
          <xsl:with-param name="lines" select="$rules[1]/line" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="$rules">
          <xsl:call-template name="write-block">
            <xsl:with-param name="header" select="concat('if (', test, ')')" />
            <xsl:with-param name="lines" select="line" />
            <xsl:with-param name="indent" select="$indent" />
          </xsl:call-template>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- An object's <collision>s naming no edge: on meeting another thing,
       of the class or the object it names, or any. Rules that name the same
       share one loop, in the file's order; a grid's array is looped over
       with its cells' own rules (grids.xsl), even where the thing has none
       for them. -->
  <xsl:template name="meetings">
    <xsl:choose>
      <xsl:when test="self::group">
        <!-- each member in turn, the group's rules on it -->
        <xsl:variable name="one"><xsl:call-template name="loop-name"><xsl:with-param name="group" select="." /></xsl:call-template></xsl:variable>
        <xsl:variable name="each">
          <xsl:call-template name="object-meetings"><xsl:with-param name="thing" select="$one" /></xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;for (const ', $one, ' of ')" />
        <xsl:call-template name="all-things"><xsl:with-param name="things" select="member" /></xsl:call-template>
        <xsl:text>) {&#10;&#9;</xsl:text>
        <xsl:call-template name="replace">
          <xsl:with-param name="text" select="substring($each, 1, string-length($each) - 1)" />
          <xsl:with-param name="from" select="'&#10;'" />
          <xsl:with-param name="to" select="'&#10;&#9;'" />
        </xsl:call-template>
        <xsl:text>&#10;&#9;}&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise><xsl:call-template name="object-meetings" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The meetings of an object, or of a group's member ($thing). -->
  <xsl:template name="object-meetings">
    <xsl:param name="thing"><xsl:call-template name="object-name" /></xsl:param>
    <xsl:variable name="object" select="." />
    <xsl:variable name="touches" select="collisions/collision[not(@edge)]" />
    <xsl:for-each select="$touches">
      <xsl:variable name="rule" select="." />
      <xsl:variable name="alike" select="$touches[string(@class) = string($rule/@class) and string(@object) = string($rule/@object)]" />
      <xsl:if test="generate-id($alike[1]) = generate-id($rule)">
        <xsl:call-template name="meeting">
          <xsl:with-param name="object" select="$object" />
          <xsl:with-param name="thing" select="$thing" />
          <xsl:with-param name="rules" select="$alike" />
        </xsl:call-template>
      </xsl:if>
    </xsl:for-each>
    <xsl:for-each select="$grids[collisions/collision/@object = $object/@name][not((@class | @name) = $touches/@class or @name = $touches/@object)]">
      <xsl:call-template name="grid-meeting">
        <xsl:with-param name="grid" select="." />
        <xsl:with-param name="thing" select="$thing" />
        <xsl:with-param name="object" select="$object" />
        <xsl:with-param name="rules" select="/.." />
      </xsl:call-template>
    </xsl:for-each>
  </xsl:template>

  <!-- touching(ball, paddle), or by the lit pixels of a picture,
       touchingPixels(rocket, rocketsPicture, cell). -->
  <xsl:template name="touch-test">
    <xsl:param name="object" />
    <xsl:param name="thing" />
    <xsl:param name="other" />
    <xsl:choose>
      <xsl:when test="$object/collisions/type = 'pixel'">
        <xsl:value-of select="concat('touchingPixels(', $thing, ', ')" />
        <xsl:call-template name="picture-name"><xsl:with-param name="bitmap" select="$pictures[@name = $object/sprite/bitmap/@from]" /></xsl:call-template>
        <xsl:value-of select="concat(', ', $other, ')')" />
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('touching(', $thing, ', ', $other, ')')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The rules of one thing that name the same class, object or any. -->
  <xsl:template name="meeting">
    <xsl:param name="object" />
    <xsl:param name="thing" />
    <xsl:param name="rules" />
    <xsl:variable name="class" select="@class" />
    <xsl:variable name="named" select="@object" />
    <xsl:variable name="grid" select="$grids[($class and (@class = $class or @name = $class)) or @name = $named]" />
    <xsl:variable name="others" select="$things[generate-id() != generate-id($object)]
        [($named and @name = $named) or ($class and (@class | parent::group/@class) = $class) or (not($named) and not($class))]" />
    <xsl:variable name="other">
      <xsl:choose>
        <xsl:when test="count($others) = 1"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$others" /></xsl:call-template></xsl:when>
        <xsl:when test="$class"><xsl:call-template name="camel"><xsl:with-param name="name" select="$class" /></xsl:call-template></xsl:when>
        <xsl:otherwise>other</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="body">
      <xsl:for-each select="$rules">
        <xsl:call-template name="rule-lines">
          <xsl:with-param name="who" select="$thing" />
          <xsl:with-param name="self" select="$object" />
          <xsl:with-param name="other" select="$other" />
        </xsl:call-template>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="lines" select="exsl:node-set($body)/line" />
    <xsl:choose>
      <xsl:when test="$grid">
        <xsl:call-template name="grid-meeting">
          <xsl:with-param name="grid" select="$grid" />
          <xsl:with-param name="thing" select="$thing" />
          <xsl:with-param name="object" select="$object" />
          <xsl:with-param name="rules" select="$rules" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="count($others) = 1">
        <xsl:call-template name="write-block">
          <xsl:with-param name="header">
            <xsl:text>if (</xsl:text>
            <xsl:call-template name="touch-test"><xsl:with-param name="object" select="$object" /><xsl:with-param name="thing" select="$thing" /><xsl:with-param name="other" select="$other" /></xsl:call-template>
            <xsl:text>)</xsl:text>
          </xsl:with-param>
          <xsl:with-param name="lines" select="$lines" />
          <xsl:with-param name="indent" select="'&#9;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$others">
        <xsl:value-of select="concat('&#9;for (const ', $other, ' of ')" />
        <xsl:call-template name="all-things"><xsl:with-param name="things" select="$others" /></xsl:call-template>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:call-template name="write-block">
          <xsl:with-param name="header">
            <xsl:text>if (</xsl:text>
            <xsl:call-template name="touch-test"><xsl:with-param name="object" select="$object" /><xsl:with-param name="thing" select="$thing" /><xsl:with-param name="other" select="$other" /></xsl:call-template>
            <xsl:text>)</xsl:text>
          </xsl:with-param>
          <xsl:with-param name="lines" select="$lines" />
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- A rule's lines: its commands, behind the tests it starts with. -->
  <xsl:template name="rule-lines">
    <xsl:param name="who" />
    <xsl:param name="self" />
    <xsl:param name="other" />
    <xsl:param name="skip" select="/.." />
    <xsl:variable name="tests" select="*[self::any or self::condition][not(preceding-sibling::*[not(self::any or self::condition)])]" />
    <xsl:variable name="commands">
      <xsl:for-each select="*[count(. | $tests) != count($tests)][count(. | $skip) != count($skip)]">
        <line>
          <xsl:apply-templates select="." mode="touch">
            <xsl:with-param name="who" select="$who" />
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="other" select="$other" />
          </xsl:apply-templates>
        </line>
      </xsl:for-each>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$tests">
        <xsl:variable name="header">
          <xsl:text>if (</xsl:text>
          <xsl:for-each select="$tests">
            <xsl:if test="position() &gt; 1"> &amp;&amp; </xsl:if>
            <xsl:call-template name="test"><xsl:with-param name="self" select="$self" /><xsl:with-param name="joined" select="count($tests) &gt; 1" /></xsl:call-template>
          </xsl:for-each>
          <xsl:text>)</xsl:text>
        </xsl:variable>
        <xsl:call-template name="block-lines">
          <xsl:with-param name="header" select="$header" />
          <xsl:with-param name="lines" select="exsl:node-set($commands)/line" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="exsl:node-set($commands)/line" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A condition's (or <any>'s) test: what it looks at, compared.
       $joined: it stands beside another with &&, so an || is bracketed. -->
  <xsl:template name="test">
    <xsl:param name="condition" select="." />
    <xsl:param name="self" select="/.." />
    <xsl:param name="joined" select="false()" />
    <xsl:variable name="written">
      <xsl:choose>
        <xsl:when test="$condition/self::any">
          <xsl:for-each select="$condition/condition | $condition/any">
            <xsl:if test="position() &gt; 1"> || </xsl:if>
            <xsl:call-template name="test"><xsl:with-param name="self" select="$self" /></xsl:call-template>
          </xsl:for-each>
        </xsl:when>
        <xsl:when test="$condition/any">
          <xsl:call-template name="test"><xsl:with-param name="condition" select="$condition/any" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
        </xsl:when>
        <xsl:when test="$condition/remaining">
          <!-- no more than so many of a grid's cells left -->
          <xsl:variable name="list"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$grids[@name = $condition/@object]" /></xsl:call-template></xsl:variable>
          <xsl:choose>
            <xsl:when test="normalize-space($condition/remaining) = '0'"><xsl:value-of select="concat($list, '.length === 0')" /></xsl:when>
            <xsl:otherwise><xsl:value-of select="concat($list, '.length &lt;= ', normalize-space($condition/remaining))" /></xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="compare" select="$condition/*[self::atleast or self::atmost or self::above or self::below or self::equal or self::unequal][1]" />
          <xsl:variable name="sign">
            <xsl:choose>
              <xsl:when test="$compare/self::atleast"> &gt;= </xsl:when>
              <xsl:when test="$compare/self::atmost"> &lt;= </xsl:when>
              <xsl:when test="$compare/self::above"> &gt; </xsl:when>
              <xsl:when test="$compare/self::below"> &lt; </xsl:when>
              <xsl:when test="$compare/self::equal"> === </xsl:when>
              <xsl:otherwise> !== </xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:variable name="against">
            <xsl:call-template name="value"><xsl:with-param name="node" select="$compare" /><xsl:with-param name="self" select="$self" /></xsl:call-template>
          </xsl:variable>
          <xsl:variable name="looked" select="$things[$condition/@class and (@class | parent::group/@class) = $condition/@class]
              | $everything[@name = $condition/@object] | $self[not($condition/@object or $condition/@class)]" />
          <xsl:for-each select="$looked">
            <xsl:if test="position() &gt; 1"> || </xsl:if>
            <xsl:call-template name="name-or-number">
              <xsl:with-param name="text" select="concat(@name, '.', $condition/@variable)" />
              <xsl:with-param name="self" select="." />
            </xsl:call-template>
            <xsl:value-of select="concat($sign, $against)" />
          </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$joined and contains($written, ' || ')"><xsl:value-of select="concat('(', $written, ')')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$written" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- computerHolds(): every one of its conditions that holds holds its
       buttons. -->
  <xsl:template name="computer-function">
    <xsl:value-of select="concat('&#10;// The computer in seat ', $computer/@seat, ': the keys it holds.&#10;function computerHolds() {&#10;&#9;const holds = { ')" />
    <xsl:for-each select="$played/inputs/seat[@number = $computer/@seat]/input[trigger]">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="seat-key" />
      <xsl:text>: false</xsl:text>
    </xsl:for-each>
    <xsl:text> };&#10;</xsl:text>
    <xsl:variable name="body">
      <xsl:for-each select="$computer/condition">
        <xsl:call-template name="computer-condition" />
      </xsl:for-each>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
      <xsl:with-param name="indent" select="'&#9;'" />
    </xsl:call-template>
    <xsl:text>&#9;return holds;&#10;}&#10;</xsl:text>
  </xsl:template>

  <!-- One of the computer's conditions: its test, then the buttons it
       holds or the conditions inside it; one holding a single condition
       and nothing else is one test with it. -->
  <xsl:template name="computer-condition">
    <xsl:param name="before" select="''" />
    <xsl:variable name="test">
      <xsl:call-template name="test"><xsl:with-param name="joined" select="$before != '' or (count(condition) = 1 and not(hold))" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="tests">
      <xsl:if test="$before != ''"><xsl:value-of select="concat($before, ' &amp;&amp; ')" /></xsl:if>
      <xsl:value-of select="$test" />
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="count(condition) = 1 and not(hold)">
        <xsl:for-each select="condition">
          <xsl:call-template name="computer-condition"><xsl:with-param name="before" select="$tests" /></xsl:call-template>
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="body">
          <xsl:for-each select="hold">
            <line><xsl:apply-templates select="." mode="do" /></line>
          </xsl:for-each>
          <xsl:for-each select="condition">
            <xsl:call-template name="computer-condition" />
          </xsl:for-each>
        </xsl:variable>
        <xsl:call-template name="block-lines">
          <xsl:with-param name="header" select="concat('if (', $tests, ')')" />
          <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A block as lines: `header { line }` when it is one plain line, else
       the header and the lines one deeper, closed. -->
  <xsl:template name="block-lines">
    <xsl:param name="header" />
    <xsl:param name="lines" />
    <xsl:choose>
      <xsl:when test="count($lines) = 1 and not($lines/@depth) and not(contains($lines, '&#10;'))">
        <line><xsl:value-of select="concat($header, ' { ', $lines, ' }')" /></line>
      </xsl:when>
      <xsl:otherwise>
        <line><xsl:value-of select="concat($header, ' {')" /></line>
        <xsl:for-each select="$lines">
          <line depth="{1 + number(concat('0', @depth))}"><xsl:value-of select="." /></line>
        </xsl:for-each>
        <line>}</line>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The lines written out, each at its depth. -->
  <xsl:template name="print-lines">
    <xsl:param name="lines" />
    <xsl:param name="indent" />
    <xsl:for-each select="$lines">
      <xsl:call-template name="print-line">
        <xsl:with-param name="text" select="string(.)" />
        <xsl:with-param name="indent">
          <xsl:value-of select="$indent" />
          <xsl:if test="@depth">
            <xsl:value-of select="substring('&#9;&#9;&#9;&#9;&#9;&#9;&#9;&#9;&#9;&#9;', 1, @depth)" />
          </xsl:if>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:for-each>
  </xsl:template>

  <!-- A line, or a line of several (a verb that writes two statements), each at the indent. -->
  <xsl:template name="print-line">
    <xsl:param name="text" />
    <xsl:param name="indent" />
    <xsl:choose>
      <xsl:when test="contains($text, '&#10;')">
        <xsl:value-of select="concat($indent, substring-before($text, '&#10;'), '&#10;')" />
        <xsl:call-template name="print-line">
          <xsl:with-param name="text" select="substring-after($text, '&#10;')" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="concat($indent, $text, '&#10;')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A block written out. -->
  <xsl:template name="write-block">
    <xsl:param name="header" />
    <xsl:param name="lines" />
    <xsl:param name="indent" />
    <xsl:variable name="block">
      <xsl:call-template name="block-lines">
        <xsl:with-param name="header" select="$header" />
        <xsl:with-param name="lines" select="$lines" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($block)/line" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
  </xsl:template>

  <!-- The page's part: the canvas, the keys and pads in, a frame each time
       the browser draws, and the picture. -->
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

// Keys in, a key held down saying so once. Away from the page nothing stays
// held.
const held = new Set();
addEventListener("keydown", (event) =&gt; {
	if (!gameKeys.has(event.code)) { return; }
	event.preventDefault();
	if (!event.repeat) { held.add(event.code); key(event.code, true); }
});
addEventListener("keyup", (event) =&gt; {
	if (!gameKeys.has(event.code)) { return; }
	event.preventDefault();
	held.delete(event.code);
	key(event.code, false);
});
addEventListener("blur", () =&gt; {
	for (const code of held) { key(code, false); }
	held.clear();
});
</xsl:text>
    <xsl:if test="$mouse-input">
      <xsl:text>
// The mouse across the canvas, as the game's x.
canvas.addEventListener("mousemove", (event) =&gt; {
	const area = canvas.getBoundingClientRect();
	mouse((event.clientX - area.left) * windowWidth / area.width);
});
</xsl:text>
    </xsl:if>
    <xsl:if test="$uses-pads">
      <xsl:text>
// The pads' buttons by the browser's standard layout, an Xbox pad's, and
// their directions by the d-pad or the left stick past halfway; a button is
// pressed or let go when it is not as it was.
const padButtons = { a: 0, b: 1, x: 2, y: 3, lb: 4, rb: 5, back: 8, start: 9, up: 12, down: 13, left: 14, right: 15 };
const padsWere = [{}, {}];
function readPads() {
	const pads = navigator.getGamepads ? navigator.getGamepads() : [];
	for (let number = 0; number &lt; 2; ++number) {
		const found = pads[number];
		const now = {};
		for (const [button, index] of Object.entries(padButtons)) { now[button] = !!found?.buttons[index]?.pressed; }
		if (found) {
			now.up ||= found.axes[1] &lt; -0.5;
			now.down ||= found.axes[1] &gt; 0.5;
			now.left ||= found.axes[0] &lt; -0.5;
			now.right ||= found.axes[0] &gt; 0.5;
		}
		for (const button in now) {
			if (now[button] !== !!padsWere[number][button]) { pad(number + 1, button, now[button]); }
		}
		padsWere[number] = now;
	}
}
</xsl:text>
    </xsl:if>
    <xsl:if test="$texts">
      <xsl:text>
// The words' styles: how big, what colour.
</xsl:text>
      <xsl:for-each select="$styles">
        <xsl:text>const </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:value-of select="concat(' = { size: ', normalize-space(size), ', color: ')" />
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(color)" /></xsl:call-template>
        <xsl:text> };&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>
// How big words are drawn: the box their letters cover.
function measure(words, style) {
	context.font = style.size + "px sans-serif";
	const letters = context.measureText(words);
	return {
		x: letters.actualBoundingBoxLeft + letters.actualBoundingBoxRight,
		y: letters.actualBoundingBoxAscent + letters.actualBoundingBoxDescent,
	};
}

// Words with their letters' top left at x, y.
function text(words, style, x, y) {
	context.font = style.size + "px sans-serif";
	const letters = context.measureText(words);
	context.fillStyle = style.color;
	context.fillText(words, x + letters.actualBoundingBoxLeft, y + letters.actualBoundingBoxAscent);
}
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$grids[count(group) &gt; 1][group/sprite/rectangle/color]">
      <xsl:variable name="word"><xsl:call-template name="cell-word" /></xsl:variable>
      <xsl:value-of select="concat('&#10;// a', substring('n', 1, number(contains('aeiou', substring($word, 1, 1)))), ' ', $word)" />
      <xsl:text>'s colour by its look&#10;</xsl:text>
      <xsl:value-of select="concat('const ', $word, 'Colors = { ')" />
      <xsl:for-each select="group">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>: </xsl:text>
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space((sprite/rectangle/color | ../sprite/rectangle/color)[1])" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text> };&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>
function box(x, y, width, height, color = "white") {
	context.fillStyle = color;
	context.fillRect(x, y, width, height);
}
</xsl:text>
    <xsl:if test="$pictures">
      <xsl:text>
// The lit pixels of a picture's rows over the box at x, y, width by height.
function pixels(rows, x, y, width, height, color = "white") {
	const pixel = { x: width / rows[0].length, y: height / rows.length };
	for (let row = 0; row &lt; rows.length; ++row) {
		for (let column = 0; column &lt; rows[row].length; ++column) {
			if (rows[row][column] !== ".") { box(x + column * pixel.x, y + row * pixel.y, pixel.x, pixel.y, color); }
		}
	}
}
</xsl:text>
    </xsl:if>
    <xsl:if test="$screens">
      <xsl:call-template name="draw-functions" />
    </xsl:if>
    <xsl:text>
// A frame each time the browser draws: the time since the last one, at most
// a quarter of a second, then the picture.
let last = performance.now();
function playFrame(now) {
</xsl:text>
    <xsl:if test="$uses-pads"><xsl:text>&#9;readPads();&#10;</xsl:text></xsl:if>
    <xsl:text>	frame(Math.min(Math.max(now - last, 0) / 1000, 0.25));
	last = now;

	context.setTransform(canvas.width / windowWidth, 0, 0, canvas.height / windowHeight, 0, 0);
	context.fillStyle = "black";
	context.fillRect(0, 0, windowWidth, windowHeight);
</xsl:text>
    <xsl:choose>
      <xsl:when test="$screens"><xsl:text>&#9;draw();&#10;</xsl:text></xsl:when>
      <xsl:otherwise>
        <xsl:variable name="drawn">
          <xsl:for-each select="$states[1]/objects/*"><xsl:call-template name="draw-lines" /></xsl:for-each>
        </xsl:variable>
        <xsl:call-template name="print-lines">
          <xsl:with-param name="lines" select="exsl:node-set($drawn)/line" />
          <xsl:with-param name="indent" select="'&#9;'" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>	requestAnimationFrame(playFrame);
}
</xsl:text>
    <xsl:if test="$kept">
      <xsl:text>readKept();
addEventListener("pagehide", keep);
</xsl:text>
    </xsl:if>
    <xsl:text>requestAnimationFrame(playFrame);
</xsl:text>
  </xsl:template>

  <!-- A colour as the page says it. -->
  <xsl:template name="color">
    <xsl:param name="color" />
    <xsl:variable name="table" select="'|green 0, 255, 0|grey 128, 128, 128|darkgrey 64, 64, 64|lightgrey 192, 192, 192|brown 139, 69, 19|orange 255, 165, 0|purple 128, 0, 128|darkblue 0, 0, 139|darkgreen 0, 100, 0|forestgreen 34, 139, 34|gold 230, 180, 30|peach 255, 214, 170|'" />
    <xsl:choose>
      <xsl:when test="$color = ''">"white"</xsl:when>
      <xsl:when test="contains($table, concat('|', $color, ' '))">
        <xsl:value-of select="concat('&quot;rgb(', substring-before(substring-after($table, concat('|', $color, ' ')), '|'), ')&quot;')" />
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('&quot;', $color, '&quot;')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- drawTitle(), drawPlaying() ...: each screen's objects, then those it
       shows; draw(): the screen on top, over those it shows still. -->
  <xsl:template name="draw-functions">
    <xsl:for-each select="$states">
      <xsl:variable name="state" select="." />
      <xsl:variable name="drawn">
        <xsl:for-each select="objects/*"><xsl:call-template name="draw-lines" /></xsl:for-each>
        <xsl:for-each select="$shared[@name = $state/shows/show/@object]"><xsl:call-template name="draw-lines" /></xsl:for-each>
      </xsl:variable>
      <xsl:if test="exsl:node-set($drawn)/line">
        <xsl:text>&#10;function draw</xsl:text>
        <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>() {&#10;</xsl:text>
        <xsl:call-template name="print-lines">
          <xsl:with-param name="lines" select="exsl:node-set($drawn)/line" />
          <xsl:with-param name="indent" select="'&#9;'" />
        </xsl:call-template>
        <xsl:text>}&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>
// The screen on top, over those it shows still beneath it.
function draw() {
	switch (screen()) {
</xsl:text>
    <xsl:for-each select="$states">
      <xsl:text>&#9;case </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="@name" /></xsl:call-template>
      <xsl:text>:</xsl:text>
      <xsl:for-each select="$states[@name = current()/shows/show/@state] | .">
        <xsl:variable name="state" select="." />
        <xsl:if test="objects/* or $shared[@name = $state/shows/show/@object]">
          <xsl:text> draw</xsl:text>
          <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>();</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text> break;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>	}
}
</xsl:text>
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

  <!-- The lines that draw one object, group or member. -->
  <xsl:template name="draw-lines">
    <xsl:variable name="sprite" select="(sprite | parent::group/sprite)[1]" />
    <xsl:variable name="color">
      <xsl:if test="$sprite/rectangle/color">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space($sprite/rectangle/color)" /></xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="count(. | $grids) = count($grids)">
        <xsl:variable name="cell"><xsl:call-template name="cell-name" /></xsl:variable>
        <xsl:variable name="word"><xsl:call-template name="cell-word" /></xsl:variable>
        <xsl:variable name="list"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:variable name="cell-color">
          <xsl:choose>
            <xsl:when test="count(group) &gt; 1 and group/sprite/rectangle/color"><xsl:value-of select="concat(', ', $word, 'Colors[', $cell, '.look]')" /></xsl:when>
            <xsl:when test="group/sprite/rectangle/color">
              <xsl:text>, </xsl:text>
              <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(group[1]/sprite/rectangle/color)" /></xsl:call-template>
            </xsl:when>
            <xsl:otherwise><xsl:value-of select="$color" /></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <line><xsl:value-of select="concat('for (const ', $cell, ' of ', $list, ') { box(', $cell, '.position.x, ', $cell, '.position.y, ', $cell, '.size.x, ', $cell, '.size.y', $cell-color, '); }')" /></line>
      </xsl:when>
      <xsl:when test="self::group[member]">
        <xsl:for-each select="member"><xsl:call-template name="draw-lines" /></xsl:for-each>
      </xsl:when>
      <xsl:when test="count(. | $text-rows) = count($text-rows)">
        <xsl:call-template name="text-row-lines" />
      </xsl:when>
      <xsl:when test="count(. | $things) = count($things) and $pictures[@name = $sprite/bitmap/@from]">
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:variable name="rows"><xsl:call-template name="picture-name"><xsl:with-param name="bitmap" select="$pictures[@name = $sprite/bitmap/@from]" /></xsl:call-template></xsl:variable>
        <line><xsl:value-of select="concat('pixels(', $rows, ', ', $thing, '.position.x, ', $thing, '.position.y, ', $thing, '.size.x, ', $thing, '.size.y', $color, ');')" /></line>
      </xsl:when>
      <xsl:when test="count(. | $things) = count($things)">
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <line><xsl:value-of select="concat('box(', $thing, '.position.x, ', $thing, '.position.y, ', $thing, '.size.x, ', $thing, '.size.y', $color, ');')" /></line>
      </xsl:when>
      <xsl:when test="$sprite/rectangle">
        <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="size">
          <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/width" /><xsl:with-param name="self" select="." /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/height" /><xsl:with-param name="self" select="." /></xsl:call-template>
        </xsl:variable>
        <xsl:variable name="across"><xsl:call-template name="cell-step"><xsl:with-param name="along" select="'x'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="down"><xsl:call-template name="cell-step"><xsl:with-param name="along" select="'y'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="columns" select="number(concat('0', normalize-space(columns)))" />
        <xsl:variable name="rows" select="number(concat('0', normalize-space(rows)))" />
        <xsl:variable name="at-x">
          <xsl:value-of select="$x" />
          <xsl:if test="$columns &gt; 1"><xsl:value-of select="concat(' + ', $across, ' * column')" /></xsl:if>
        </xsl:variable>
        <xsl:variable name="at-y">
          <xsl:value-of select="$y" />
          <xsl:if test="$rows &gt; 1"><xsl:value-of select="concat(' + ', $down, ' * row')" /></xsl:if>
        </xsl:variable>
        <xsl:variable name="draw" select="concat('box(', $at-x, ', ', $at-y, ', ', $size, $color, ');')" />
        <xsl:choose>
          <xsl:when test="$columns &gt; 1 and $rows &gt; 1">
            <line><xsl:value-of select="concat('for (let row = 0; row &lt; ', $rows, '; ++row) {')" /></line>
            <line depth="1"><xsl:value-of select="concat('for (let column = 0; column &lt; ', $columns, '; ++column) { ', $draw, ' }')" /></line>
            <line>}</line>
          </xsl:when>
          <xsl:when test="$columns &gt; 1 or $rows &gt; 1">
            <xsl:variable name="along">
              <xsl:choose>
                <xsl:when test="$rows &gt; 1">row</xsl:when>
                <xsl:otherwise>column</xsl:otherwise>
              </xsl:choose>
            </xsl:variable>
            <line><xsl:value-of select="concat('for (let ', $along, ' = 0; ', $along, ' &lt; ', $rows + $columns - 1, '; ++', $along, ') { ', $draw, ' }')" /></line>
          </xsl:when>
          <xsl:otherwise><line><xsl:value-of select="$draw" /></line></xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$sprite/text">
        <xsl:call-template name="text-lines" />
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- How far one cell of a block is from the next. -->
  <xsl:template name="cell-step">
    <xsl:param name="along" />
    <xsl:variable name="rectangle" select="(sprite | parent::group/sprite)[1]/rectangle" />
    <xsl:variable name="size" select="$rectangle/*[local-name() = substring('widthheight', 1 + 5 * number($along = 'y'), 5 + number($along = 'y'))]" />
    <xsl:variable name="padding" select="padding/*[local-name() = $along]" />
    <xsl:choose>
      <xsl:when test="not($padding)">
        <xsl:call-template name="value"><xsl:with-param name="node" select="$size" /><xsl:with-param name="self" select="." /><xsl:with-param name="wrap" select="3" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="string(number($size)) != 'NaN' and string(number($padding)) != 'NaN'">
        <xsl:value-of select="number($size) + number($padding)" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>(</xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$size" /><xsl:with-param name="self" select="." /></xsl:call-template>
        <xsl:text> + </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$padding" /><xsl:with-param name="self" select="." /><xsl:with-param name="wrap" select="1" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A text: its words (a look's, or a number the game counts), measured
       when where it goes depends on its size, then drawn. -->
  <xsl:template name="text-lines">
    <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="base"><xsl:call-template name="camel"><xsl:with-param name="name" select="sprite[1]/text/@style" /></xsl:call-template></xsl:variable>
    <xsl:variable name="first-style"><xsl:call-template name="text-style"><xsl:with-param name="sprite" select="sprite[1]" /><xsl:with-param name="style" select="$base" /></xsl:call-template></xsl:variable>
    <xsl:variable name="styled">
      <xsl:for-each select="sprite[position() &gt; 1]">
        <xsl:variable name="own"><xsl:call-template name="text-style"><xsl:with-param name="sprite" select="." /><xsl:with-param name="style" select="$base" /></xsl:call-template></xsl:variable>
        <xsl:if test="$own != $first-style">yes</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="colored" select="$styled != ''" />
    <xsl:variable name="style">
      <xsl:choose>
        <xsl:when test="$colored"><xsl:value-of select="concat($named, 'Style')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="$first-style" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="words">
      <xsl:choose>
        <xsl:when test="count(sprite) &gt; 1">
          <xsl:for-each select="sprite[position() &gt; 1]">
            <xsl:value-of select="concat($named, '.look === &quot;', @name, '&quot; ? ')" />
            <xsl:call-template name="string"><xsl:with-param name="text" select="text/content" /></xsl:call-template>
            <xsl:text> : </xsl:text>
          </xsl:for-each>
          <xsl:call-template name="string"><xsl:with-param name="text" select="sprite[1]/text/content" /></xsl:call-template>
        </xsl:when>
        <xsl:when test="sprite/text/number">
          <xsl:text>String(</xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/text/number" /><xsl:with-param name="self" select="." /></xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="string"><xsl:with-param name="text" select="sprite/text/content" /></xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="measured" select="contains(concat($x, $y), concat($named, 'Size'))" />
    <xsl:variable name="literal" select="starts-with($words, '&quot;')" />
    <xsl:variable name="shown">
      <xsl:choose>
        <xsl:when test="$measured and not($literal)"><xsl:value-of select="concat($named, 'Text')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="$words" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$colored">
      <line>
        <xsl:value-of select="concat('const ', $named, 'Style = ')" />
        <xsl:for-each select="sprite[position() &gt; 1]">
          <xsl:value-of select="concat($named, '.look === &quot;', @name, '&quot; ? ')" />
          <xsl:call-template name="text-style"><xsl:with-param name="sprite" select="." /><xsl:with-param name="style" select="$base" /></xsl:call-template>
          <xsl:text> : </xsl:text>
        </xsl:for-each>
        <xsl:call-template name="text-style"><xsl:with-param name="sprite" select="sprite[1]" /><xsl:with-param name="style" select="$base" /></xsl:call-template>
        <xsl:text>;</xsl:text>
      </line>
    </xsl:if>
    <xsl:if test="$measured and not($literal)">
      <line><xsl:value-of select="concat('const ', $named, 'Text = ', $words, ';')" /></line>
    </xsl:if>
    <xsl:if test="$measured">
      <line><xsl:value-of select="concat('const ', $named, 'Size = measure(', $shown, ', ', $style, ');')" /></line>
    </xsl:if>
    <line><xsl:value-of select="concat('text(', $shown, ', ', $style, ', ', $x, ', ', $y, ');')" /></line>
  </xsl:template>

  <!-- A text's style: the style it names, or that style with a size or
       colour of the text's own. -->
  <xsl:template name="text-style">
    <xsl:param name="sprite" />
    <xsl:param name="style" />
    <xsl:choose>
      <xsl:when test="$sprite/text/color or $sprite/text/size">
        <xsl:value-of select="concat('{ ...', $style)" />
        <xsl:if test="$sprite/text/size"><xsl:value-of select="concat(', size: ', normalize-space($sprite/text/size))" /></xsl:if>
        <xsl:if test="$sprite/text/color">
          <xsl:text>, color: </xsl:text>
          <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space($sprite/text/color)" /></xsl:call-template>
        </xsl:if>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$style" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A group of texts in rows: each row's words, centred on themselves
       across, the first row placed as the group is and each next one the
       padding further down. -->
  <xsl:template name="text-row-lines">
    <xsl:variable name="group" select="." />
    <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="style"><xsl:call-template name="camel"><xsl:with-param name="name" select="sprite/text/@style" /></xsl:call-template></xsl:variable>
    <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="size" select="concat($named, 'Size.')" />
    <xsl:variable name="first" select="concat($named, '1Size.')" />
    <xsl:variable name="padding" select="padding/y" />
    <xsl:for-each select="row">
      <xsl:sort select="@number" data-type="number" />
      <xsl:variable name="number" select="normalize-space(@number)" />
      <xsl:variable name="row-size" select="concat($named, $number, 'Size.')" />
      <xsl:variable name="words"><xsl:call-template name="string"><xsl:with-param name="text" select="sprite/text/content" /></xsl:call-template></xsl:variable>
      <xsl:variable name="at-x"><xsl:call-template name="replace"><xsl:with-param name="text" select="$x" /><xsl:with-param name="from" select="$size" /><xsl:with-param name="to" select="$row-size" /></xsl:call-template></xsl:variable>
      <xsl:variable name="at-y">
        <xsl:call-template name="replace"><xsl:with-param name="text" select="$y" /><xsl:with-param name="from" select="$size" /><xsl:with-param name="to" select="$first" /></xsl:call-template>
        <xsl:if test="$number &gt; 1 and $padding">
          <xsl:text> + </xsl:text>
          <xsl:choose>
            <xsl:when test="string(number($padding)) != 'NaN'"><xsl:value-of select="($number - 1) * number($padding)" /></xsl:when>
            <xsl:otherwise>
              <xsl:if test="$number &gt; 2"><xsl:value-of select="concat($number - 1, ' * ')" /></xsl:if>
              <xsl:call-template name="value"><xsl:with-param name="node" select="$padding" /><xsl:with-param name="self" select="$group" /><xsl:with-param name="wrap" select="3" /></xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:if>
      </xsl:variable>
      <xsl:if test="contains(concat($at-x, $at-y), concat($named, $number, 'Size'))">
        <line><xsl:value-of select="concat('const ', $named, $number, 'Size = measure(', $words, ', ', $style, ');')" /></line>
      </xsl:if>
      <line><xsl:value-of select="concat('text(', $words, ', ', $style, ', ', $at-x, ', ', $at-y, ');')" /></line>
    </xsl:for-each>
  </xsl:template>

  <!-- Every `from` in `text` written as `to`. -->
  <xsl:template name="replace">
    <xsl:param name="text" />
    <xsl:param name="from" />
    <xsl:param name="to" />
    <xsl:choose>
      <xsl:when test="contains($text, $from)">
        <xsl:value-of select="concat(substring-before($text, $from), $to)" />
        <xsl:call-template name="replace"><xsl:with-param name="text" select="substring-after($text, $from)" /><xsl:with-param name="from" select="$from" /><xsl:with-param name="to" select="$to" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$text" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A verb at an edge or on a touch that says nothing of either is
       written as anywhere else. -->
  <xsl:template match="*" mode="edge">
    <xsl:param name="self" />
    <xsl:apply-templates select="." mode="do"><xsl:with-param name="self" select="$self" /></xsl:apply-templates>
  </xsl:template>
  <xsl:template match="*" mode="touch">
    <xsl:param name="self" />
    <xsl:apply-templates select="." mode="do"><xsl:with-param name="self" select="$self" /></xsl:apply-templates>
  </xsl:template>

  <!-- A verb web-js has no words for yet. -->
  <xsl:template match="*" mode="do">
    <xsl:message terminate="yes">web-js cannot write &lt;<xsl:value-of select="local-name()" />&gt; here yet; use web-asm for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="held">
    <xsl:message terminate="yes">web-js cannot write &lt;<xsl:value-of select="local-name()" />&gt; in an action yet; use web-asm for this game</xsl:message>
  </xsl:template>
</xsl:stylesheet>
