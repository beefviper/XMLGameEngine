<?xml version="1.0" encoding="UTF-8"?>
<!-- game.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- The game itself: source/<game>.h, its numbers, its records and its
     class, and source/<game>.cpp, its rules. Each thing is a Thing (where it
     is, how it moves, how big it is), or a record built on one when it
     counts something; the screens are an enum and a stack. key() and pad()
     note which buttons are down and answer the presses of the screen on
     top; frame() plays the played screen: the keys held (and the computer's),
     everything moving, the window's edges, the meetings and the game's own
     tests. Each command is written by its verb's file (verbs/) in one of four
     modes: held (a key held, on who), edge (at one edge, on who and that
     edge), touch (on a meeting, on who and other) and do (anywhere). Nothing
     of the engine is in it. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <!-- What the game has, and so what it is written with. -->
  <xsl:variable name="class"><xsl:call-template name="class-name" /></xsl:variable>
  <xsl:variable name="inputs" select="$states/inputs//input" />
  <xsl:variable name="held-inputs" select="$inputs[trigger]" />
  <xsl:variable name="press-inputs" select="$inputs[not(trigger)]" />
  <xsl:variable name="held-list">
    <xsl:for-each select="$held-inputs/@button | $held-inputs/button">
      <b><xsl:call-template name="button-of" /></b>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="held-keys" select="exsl:node-set($held-list)/b[not(. = preceding-sibling::b)][not(starts-with(., 'pad'))]" />
  <xsl:variable name="held-pads" select="exsl:node-set($held-list)/b[not(. = preceding-sibling::b)][starts-with(., 'pad')]" />
  <xsl:variable name="held-buttons" select="$held-keys | $held-pads" />
  <xsl:variable name="all-list">
    <xsl:for-each select="$inputs/@button | $inputs/button">
      <b><xsl:call-template name="button-of" /></b>
    </xsl:for-each>
  </xsl:variable>
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
  <!-- what has a start function: a thing with a record, numbers drawn at
       random or a reset, and every grid; a plain record starts as it is
       declared ({}) -->
  <xsl:variable name="started" select="($things | $records)[not(@name = $kept/@name)]
      [(count(. | $things) = count($things) and count(. | $records) = count($records)) or .//random or @name = $reset-names] | $grids" />
  <!-- members that go back where they started: their start kept in their record -->
  <xsl:variable name="returners" select="$states/objects/group[member][collisions/collision/reset[not(@* or node())]]/member" />
  <xsl:variable name="randoms" select="$everything//random" />
  <xsl:variable name="deflects" select="$played//deflect" />
  <xsl:variable name="sounds" select="/game/sounds/sound" />
  <xsl:variable name="uses-people" select="boolean(//people or $computer)" />
  <xsl:variable name="uses-sign" select="boolean($everything//sign)" />
  <xsl:variable name="uses-angles" select="boolean($everything//sine | $everything//cosine | $deflects)" />
  <xsl:variable name="touch-bounces" select="$played/objects/object/collisions/collision[not(@edge)]/bounce" />
  <xsl:variable name="mouse-input" select="$states/inputs//input[@axis = 'mouse.x']" />
  <!-- the actions rules run, each a function of the game's -->
  <xsl:variable name="called" select="$everything/actions/action[@name = //collision/trigger[not(@object)]/@action]" />

  <!-- source/<game>.h -->
  <xsl:template name="header-file">
    <xsl:call-template name="header"><xsl:with-param name="file" select="concat($name, '.h')" /></xsl:call-template>
    <xsl:value-of select="concat('//&#10;// ', $title, ': its numbers, its things and its rules. main.cpp draws it')" />
    <xsl:if test="$sounds"> and plays its sounds</xsl:if>
    <xsl:text>.&#10;
#pragma once

#include &lt;SFML/System/Vector2.hpp&gt;
#include &lt;SFML/Window/Keyboard.hpp&gt;
</xsl:text>
    <xsl:if test="$randoms or $kept or $screens or $sounds or $grids or $pictures">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="$randoms"><xsl:text>#include &lt;random&gt;&#10;</xsl:text></xsl:if>
      <xsl:if test="$kept or $pictures"><xsl:text>#include &lt;string&gt;&#10;</xsl:text></xsl:if>
      <xsl:if test="$screens or $sounds or $grids or $pictures"><xsl:text>#include &lt;vector&gt;&#10;</xsl:text></xsl:if>
    </xsl:if>
    <xsl:text>&#10;namespace </xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:text>
{
	constexpr float windowWidth = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/width" /></xsl:call-template>
    <xsl:text>;&#10;&#9;constexpr float windowHeight = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/height" /></xsl:call-template>
    <xsl:text>;&#10;</xsl:text>
    <xsl:call-template name="constants" />
    <xsl:if test="$screens or $sounds">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="$screens">
        <xsl:text>&#9;enum class Screen { </xsl:text>
        <xsl:for-each select="$states">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        </xsl:for-each>
        <xsl:text> };&#10;</xsl:text>
      </xsl:if>
      <xsl:if test="$sounds">
        <xsl:text>&#9;enum class Sound { </xsl:text>
        <xsl:for-each select="$sounds">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        </xsl:for-each>
        <xsl:text> };&#10;</xsl:text>
      </xsl:if>
    </xsl:if>
    <xsl:text>
	// Something on the screen: where it is, how it moves, how big it is.
	struct Thing
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		sf::Vector2f size;
	};
</xsl:text>
    <xsl:for-each select="$pictures">
      <xsl:value-of select="concat('&#10;&#9;// The ', @name, ' picture, a row a line: lit where it is not a dot.&#10;&#9;inline const std::vector&lt;std::string&gt; ')" />
      <xsl:call-template name="picture-name" />
      <xsl:text>{</xsl:text>
      <xsl:for-each select="row">
        <xsl:if test="position() &gt; 1">,</xsl:if>
        <xsl:value-of select="concat(' &quot;', normalize-space(.), '&quot;')" />
      </xsl:for-each>
      <xsl:text> };&#10;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$records[not(self::member) or not(preceding-sibling::member[count(. | $records) = count($records)])] | $grids">
      <xsl:choose>
        <xsl:when test="self::group"><xsl:call-template name="grid-record" /></xsl:when>
        <xsl:otherwise><xsl:call-template name="record" /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:if test="$uses-pads">
      <xsl:text>
	// A game pad's buttons, named for an Xbox pad's.
	enum class PadButton { A, B, X, Y, LB, RB, Back, Start, Up, Down, Left, Right };
</xsl:text>
    </xsl:if>
    <xsl:value-of select="concat('&#10;&#9;class ', $class, '&#10;&#9;{&#10;&#9;public:&#10;')" />
    <xsl:choose>
      <xsl:when test="$randoms">
        <xsl:value-of select="concat('&#9;&#9;explicit ', $class, '(unsigned seed = std::random_device{}());&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;&#9;', $class, '();&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$kept">
      <xsl:text>
		// What is kept from one game to the next: read from this file, and
		// written to it by keep().
		void keepIn(const std::string&amp; file);
		void keep() const;
</xsl:text>
    </xsl:if>
    <xsl:text>&#10;&#9;&#9;void key(sf::Keyboard::Key key, bool down);&#10;</xsl:text>
    <xsl:if test="$uses-pads">
      <xsl:text>&#9;&#9;void pad(unsigned pad, PadButton button, bool down);&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$mouse-input">
      <xsl:text>&#9;&#9;void mouse(float x);&#10;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;&#9;void frame(float elapsed);&#10;</xsl:text>
    <xsl:if test="$screens or $sounds">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="$screens"><xsl:text>&#9;&#9;Screen screen() const { return screens.back(); }&#10;</xsl:text></xsl:if>
      <xsl:if test="$sounds">
        <xsl:text>		// The sounds asked for since the last time.
		std::vector&lt;Sound&gt; takeSounds();
</xsl:text>
      </xsl:if>
    </xsl:if>
    <xsl:text>&#10;</xsl:text>
    <xsl:if test="$uses-people"><xsl:text>&#9;&#9;int people{ 1 };&#10;</xsl:text></xsl:if>
    <xsl:for-each select="$everything[count(. | $things | $records) = count($things | $records)] | $grids">
      <xsl:text>&#9;&#9;</xsl:text>
      <xsl:choose>
        <xsl:when test="self::group">
          <xsl:text>std::vector&lt;</xsl:text>
          <xsl:call-template name="cell-type" />
          <xsl:text>&gt;</xsl:text>
        </xsl:when>
        <xsl:otherwise><xsl:call-template name="type-name" /></xsl:otherwise>
      </xsl:choose>
      <xsl:text> </xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:variable name="private">
      <xsl:if test="$screens or $sounds or $kept">
        <part>
          <xsl:if test="$screens">
            <xsl:text>&#9;&#9;std::vector&lt;Screen&gt; screens{ </xsl:text>
            <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$states[1]/@name" /></xsl:call-template>
            <xsl:text> };&#10;</xsl:text>
          </xsl:if>
          <xsl:if test="$sounds"><xsl:text>&#9;&#9;std::vector&lt;Sound&gt; sounds;&#10;</xsl:text></xsl:if>
          <xsl:if test="$kept"><xsl:text>&#9;&#9;std::string keptFile;&#10;</xsl:text></xsl:if>
        </part>
      </xsl:if>
      <xsl:if test="$randoms">
        <part>
          <xsl:text>		std::mt19937 random;
		float between(float min, float max);
</xsl:text>
        </part>
      </xsl:if>
      <xsl:if test="$held-buttons">
        <part>
          <xsl:for-each select="$held-keys | $held-pads[not($held-keys)]">
            <xsl:text>&#9;&#9;bool </xsl:text>
            <xsl:call-template name="held-name"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
            <xsl:text>{ false };&#10;</xsl:text>
          </xsl:for-each>
          <xsl:for-each select="$held-pads[$held-keys]">
            <xsl:text>&#9;&#9;bool </xsl:text>
            <xsl:call-template name="held-name"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
            <xsl:text>{ false };&#10;</xsl:text>
          </xsl:for-each>
        </part>
      </xsl:if>
      <xsl:if test="$new-game or $started or $computer or $called">
        <part>
          <xsl:if test="$new-game"><xsl:text>&#9;&#9;void newGame();&#10;</xsl:text></xsl:if>
          <xsl:for-each select="$started">
            <xsl:text>&#9;&#9;void </xsl:text>
            <xsl:call-template name="start-name" />
            <xsl:text>();&#10;</xsl:text>
            <xsl:if test="self::group[group]">
              <xsl:text>&#9;&#9;void </xsl:text>
              <xsl:call-template name="add-name" />
              <xsl:text>(</xsl:text>
              <xsl:call-template name="add-parameters" />
              <xsl:text>);&#10;</xsl:text>
            </xsl:if>
          </xsl:for-each>
          <xsl:for-each select="$called">
            <xsl:text>&#9;&#9;void </xsl:text>
            <xsl:call-template name="action-name"><xsl:with-param name="action" select="@name" /></xsl:call-template>
            <xsl:text>();&#10;</xsl:text>
          </xsl:for-each>
          <xsl:if test="$computer">
            <xsl:text>&#9;&#9;void computerHolds(</xsl:text>
            <xsl:call-template name="computer-parameters" />
            <xsl:text>) const;&#10;</xsl:text>
          </xsl:if>
        </part>
      </xsl:if>
    </xsl:variable>
    <xsl:if test="exsl:node-set($private)/part">
      <xsl:text>&#10;&#9;private:&#10;</xsl:text>
      <xsl:for-each select="exsl:node-set($private)/part">
        <xsl:if test="position() &gt; 1"><xsl:text>&#10;</xsl:text></xsl:if>
        <xsl:value-of select="." />
      </xsl:for-each>
    </xsl:if>
    <xsl:text>	};
}
</xsl:text>
  </xsl:template>

  <!-- The game's <variables> and the objects' that nothing changes, each a
       constant: those the game reads. -->
  <xsl:template name="constants">
    <xsl:variable name="written">
      <xsl:for-each select="/game/variables/variable">
        <xsl:variable name="named" select="@name" />
        <xsl:if test="//*[not(*)][normalize-space(.) = $named][not(self::variable[@name = $named])]">
          <xsl:text>&#9;constexpr float </xsl:text>
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
          <xsl:text>&#9;constexpr float </xsl:text>
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

  <!-- struct Ball : Thing { int hits{ 0 }; }: a record, its looks and its
       live variables, starting as the file says. -->
  <xsl:template name="record">
    <xsl:variable name="object" select="." />
    <xsl:variable name="looks" select="sprite[@name]" />
    <xsl:text>&#10;&#9;struct </xsl:text>
    <xsl:call-template name="record-name" />
    <xsl:if test="count(. | $things) = count($things)"> : Thing</xsl:if>
    <xsl:text>&#10;&#9;{&#10;</xsl:text>
    <xsl:if test="count(sprite) &gt; 1">
      <xsl:text>&#9;&#9;enum class Look { </xsl:text>
      <xsl:for-each select="sprite">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text> };&#10;&#9;&#9;Look look{ Look::</xsl:text>
      <xsl:call-template name="type-word"><xsl:with-param name="name" select="sprite[1]/@name" /></xsl:call-template>
      <xsl:text> };&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="count(. | $returners) = count($returners)">
      <xsl:text>&#9;&#9;sf::Vector2f start;&#10;</xsl:text>
    </xsl:if>
    <xsl:for-each select="variables/variable | parent::group/variables/variable">
      <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
      <xsl:if test="$kind = 'live'">
        <xsl:text>&#9;&#9;</xsl:text>
        <xsl:call-template name="variable-type" />
        <xsl:text> </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>{ </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$object" /></xsl:call-template>
        <xsl:text> };&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>&#9;};&#10;</xsl:text>
  </xsl:template>

  <!-- int for a count (whole numbers in and whole numbers added), else float. -->
  <xsl:template name="variable-type">
    <xsl:variable name="owner" select="../.." />
    <xsl:variable name="named" select="@name" />
    <xsl:variable name="changes" select="//*[self::set or self::inc or self::dec][substring-after(@variable, '.') = $named and (substring-before(@variable, '.') = $owner/@name or substring-before(@variable, '.') = $owner/member/@name)]" />
    <xsl:choose>
      <xsl:when test="not(*) and floor(number(.)) = number(.) and not($changes[*[not(self::self)] or (not(*) and normalize-space(.) != '' and floor(number(.)) != number(.))])">int</xsl:when>
      <xsl:otherwise>float</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- startBall: where it starts, and starts again. -->
  <xsl:template name="start-name">
    <xsl:param name="object" select="." />
    <xsl:text>start</xsl:text>
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="$object/@name" /></xsl:call-template>
  </xsl:template>

  <!-- source/<game>.cpp -->
  <xsl:template name="game-file">
    <xsl:call-template name="header"><xsl:with-param name="file" select="concat($name, '.cpp')" /></xsl:call-template>
    <xsl:value-of select="concat('&#10;#include &quot;', $name, '.h&quot;&#10;')" />
    <xsl:if test="$deflects or $uses-angles or $edge-owners//bounce or $touch-bounces or $kept or $sounds or $mouse-input">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="$deflects or $mouse-input"><xsl:text>#include &lt;algorithm&gt;&#10;</xsl:text></xsl:if>
      <xsl:if test="$deflects or $uses-angles or $edge-owners//bounce or $touch-bounces"><xsl:text>#include &lt;cmath&gt;&#10;</xsl:text></xsl:if>
      <xsl:if test="$kept"><xsl:text>#include &lt;fstream&gt;&#10;</xsl:text></xsl:if>
      <xsl:if test="$sounds"><xsl:text>#include &lt;utility&gt;&#10;</xsl:text></xsl:if>
    </xsl:if>
    <xsl:text>&#10;namespace </xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:text>&#10;{&#10;</xsl:text>
    <xsl:call-template name="helpers" />
    <xsl:call-template name="constructor" />
    <xsl:if test="$randoms">
      <xsl:value-of select="concat('&#10;&#9;float ', $class, '::between(float min, float max)&#10;&#9;{&#10;&#9;&#9;return std::uniform_real_distribution&lt;float&gt;(min, max)(random);&#10;&#9;}&#10;')" />
    </xsl:if>
    <xsl:if test="$new-game">
      <xsl:call-template name="new-game-function" />
    </xsl:if>
    <xsl:for-each select="$started">
      <xsl:choose>
        <xsl:when test="self::group"><xsl:call-template name="grid-start-function" /></xsl:when>
        <xsl:otherwise><xsl:call-template name="start-function" /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:if test="$kept">
      <xsl:call-template name="keep-functions" />
    </xsl:if>
    <xsl:if test="$sounds">
      <xsl:value-of select="concat('&#10;&#9;std::vector&lt;Sound&gt; ', $class, '::takeSounds()&#10;&#9;{&#10;&#9;&#9;return std::exchange(sounds, {});&#10;&#9;}&#10;')" />
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
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- The small functions the rules are written with, those they use. -->
  <xsl:template name="helpers">
    <xsl:if test="$uses-sign or $uses-angles or $touch-owners or $touch-bounces">
      <xsl:text>	namespace
	{
</xsl:text>
      <xsl:variable name="parts">
        <xsl:if test="$uses-angles">
          <part>		constexpr float pi = 3.14159265358979f;

		float sine(float degrees) { return std::sin(degrees * pi / 180); }
		float cosine(float degrees) { return std::cos(degrees * pi / 180); }
</part>
        </xsl:if>
        <xsl:if test="$uses-sign">
          <part>		float sign(float number) { return number &gt; 0 ? 1.0f : number &lt; 0 ? -1.0f : 0.0f; }
</part>
        </xsl:if>
        <xsl:if test="$touch-owners">
          <part>		bool touching(const Thing&amp; a, const Thing&amp; b)
		{
			return a.position.x &lt; b.position.x + b.size.x &amp;&amp; b.position.x &lt; a.position.x + a.size.x
				&amp;&amp; a.position.y &lt; b.position.y + b.size.y &amp;&amp; b.position.y &lt; a.position.y + a.size.y;
		}
</part>
        </xsl:if>
        <xsl:if test="$pixel-touchers">
          <part>		// Whether a lit pixel of `picture`, drawn over `thing`, overlaps `other`.
		bool touchingPixels(const Thing&amp; thing, const std::vector&lt;std::string&gt;&amp; picture, const Thing&amp; other)
		{
			if (!touching(thing, other)) { return false; }
			const sf::Vector2f pixel{ thing.size.x / static_cast&lt;float&gt;(picture.front().size()), thing.size.y / static_cast&lt;float&gt;(picture.size()) };
			for (std::size_t row = 0; row &lt; picture.size(); ++row)
			{
				for (std::size_t column = 0; column &lt; picture[row].size(); ++column)
				{
					if (picture[row][column] == '.') { continue; }
					const sf::Vector2f corner{ static_cast&lt;float&gt;(column) * pixel.x, static_cast&lt;float&gt;(row) * pixel.y };
					if (touching({ thing.position + corner, {}, pixel }, other)) { return true; }
				}
			}
			return false;
		}
</part>
        </xsl:if>
        <xsl:if test="$touch-bounces">
          <part>		// Off the side of `other` it hit: its speed into that side turned round.
		void bounce(Thing&amp; thing, const Thing&amp; other)
		{
			const sf::Vector2f apart = (thing.position + thing.size / 2.0f) - (other.position + other.size / 2.0f);
			const sf::Vector2f reach = (thing.size + other.size) / 2.0f;
			if (reach.x - std::abs(apart.x) &lt; reach.y - std::abs(apart.y))
			{
				thing.velocity.x = apart.x &lt; 0 ? -std::abs(thing.velocity.x) : std::abs(thing.velocity.x);
			}
			else
			{
				thing.velocity.y = apart.y &lt; 0 ? -std::abs(thing.velocity.y) : std::abs(thing.velocity.y);
			}
		}
</part>
        </xsl:if>
        <xsl:if test="$deflects">
          <part>		// Off the side of `other` it hit, at the same speed, up to `degrees` off
		// straight out: steeper the further from that side's middle, towards
		// the end it hit nearer to.
		void deflect(Thing&amp; thing, const Thing&amp; other, float degrees)
		{
			const sf::Vector2f middle = thing.position + thing.size / 2.0f;
			const sf::Vector2f otherMiddle = other.position + other.size / 2.0f;
			const sf::Vector2f apart = middle - otherMiddle;
			const sf::Vector2f reach = (thing.size + other.size) / 2.0f;
			const float speed = std::hypot(thing.velocity.x, thing.velocity.y);

			if (reach.x - std::abs(apart.x) &lt; reach.y - std::abs(apart.y))
			{
				// its left or right side
				const float away = apart.x &lt; 0 ? -1.0f : 1.0f;
				const float angle = degrees * std::clamp(apart.y / (other.size.y / 2), -1.0f, 1.0f);
				thing.velocity = { away * speed * cosine(angle), speed * sine(angle) };
				thing.position.x = otherMiddle.x + away * reach.x - thing.size.x / 2;
			}
			else
			{
				// its top or bottom
				const float away = apart.y &lt; 0 ? -1.0f : 1.0f;
				const float angle = degrees * std::clamp(apart.x / (other.size.x / 2), -1.0f, 1.0f);
				thing.velocity = { speed * sine(angle), away * speed * cosine(angle) };
				thing.position.y = otherMiddle.y + away * reach.y - thing.size.y / 2;
			}
		}
</part>
        </xsl:if>
      </xsl:variable>
      <xsl:for-each select="exsl:node-set($parts)/part">
        <xsl:if test="position() &gt; 1"><xsl:text>&#10;</xsl:text></xsl:if>
        <xsl:value-of select="." />
      </xsl:for-each>
      <xsl:text>	}

</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The constructor: a new game, or each thing where the file puts it. -->
  <xsl:template name="constructor">
    <xsl:choose>
      <xsl:when test="$randoms">
        <xsl:value-of select="concat('&#9;', $class, '::', $class, '(unsigned seed) : random(seed)&#10;&#9;{&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;', $class, '::', $class, '()&#10;&#9;{&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="$new-game"><xsl:text>&#9;&#9;newGame();&#10;</xsl:text></xsl:when>
      <xsl:otherwise><xsl:call-template name="starts" /></xsl:otherwise>
    </xsl:choose>
    <xsl:text>	}&#10;</xsl:text>
  </xsl:template>

  <!-- Each thing and record that is not kept, as it starts. -->
  <xsl:template name="starts">
    <xsl:for-each select="($things | $records)[not(@name = $kept/@name)] | $grids">
      <xsl:text>&#9;&#9;</xsl:text>
      <xsl:choose>
        <xsl:when test="count(. | $started) = count($started)">
          <xsl:call-template name="start-name" />
          <xsl:text>();&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="count(. | $things) != count($things)">
          <xsl:call-template name="object-name" />
          <xsl:text> = {};&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="object-name" />
          <xsl:text> = { </xsl:text>
          <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity | parent::group/velocity[not(current()/velocity)]" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="size" />
          <xsl:text> };&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="new-game-function">
    <xsl:value-of select="concat('&#10;&#9;// A new game: everything as it starts but what is kept, from the first screen.&#10;&#9;void ', $class, '::newGame()&#10;&#9;{&#10;')" />
    <xsl:call-template name="starts" />
    <xsl:if test="$screens">
      <xsl:text>&#9;&#9;screens = { </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$states[1]/@name" /></xsl:call-template>
      <xsl:text> };&#10;</xsl:text>
    </xsl:if>
    <xsl:text>	}&#10;</xsl:text>
  </xsl:template>

  <!-- { x, y } of a <position> or <velocity>, 0 for an axis it leaves out;
       {} when there is none. -->
  <xsl:template name="pair">
    <xsl:param name="of" />
    <xsl:param name="self" select="." />
    <xsl:param name="start" select="false()" />
    <xsl:choose>
      <xsl:when test="$of">
        <xsl:text>{ </xsl:text>
        <xsl:call-template name="axis-value"><xsl:with-param name="node" select="$of/x" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'x'" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="axis-value"><xsl:with-param name="node" select="$of/y" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'y'" /><xsl:with-param name="start" select="$start" /></xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise>{}</xsl:otherwise>
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

  <!-- { width, height } of its rectangle or picture, or its group's. -->
  <xsl:template name="size">
    <xsl:variable name="sprite" select="(sprite | parent::group/sprite)[1]" />
    <xsl:variable name="bitmap" select="$pictures[@name = $sprite/bitmap/@from]" />
    <xsl:choose>
      <xsl:when test="$bitmap">
        <xsl:variable name="scale"><xsl:call-template name="picture-scale"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat('{ ', string-length(normalize-space($bitmap/row[1])) * $scale, ', ', count($bitmap/row) * $scale, ' }')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>{ </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/width" /><xsl:with-param name="self" select="." /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/height" /><xsl:with-param name="self" select="." /></xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- startBall(): its numbers drawn at random (drawn again each time),
       then its size, place and way, its live variables and its look. -->
  <xsl:template name="start-function">
    <xsl:variable name="object" select="." />
    <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:text>&#10;</xsl:text>
    <xsl:if test="@name = $reset-names">
      <xsl:value-of select="concat('&#9;// Where the ', @name, ' starts, and starts again.&#10;')" />
    </xsl:if>
    <xsl:text>&#9;void </xsl:text>
    <xsl:value-of select="concat($class, '::')" />
    <xsl:call-template name="start-name" />
    <xsl:text>()&#10;&#9;{&#10;</xsl:text>
    <xsl:for-each select="variables/variable | parent::group/variables/variable">
      <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
      <xsl:if test="$kind = 'start'">
        <xsl:text>&#9;&#9;const float </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$object" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
        <xsl:text>;&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:if test="count(. | $things) = count($things)">
      <xsl:value-of select="concat('&#9;&#9;', $thing, '.size = ')" />
      <xsl:call-template name="size" />
      <xsl:value-of select="concat(';&#10;&#9;&#9;', $thing, '.position = ')" />
      <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
      <xsl:if test="count(. | $returners) = count($returners)">
        <xsl:value-of select="concat(';&#10;&#9;&#9;', $thing, '.start = ', $thing, '.position')" />
      </xsl:if>
      <xsl:value-of select="concat(';&#10;&#9;&#9;', $thing, '.velocity = ')" />
      <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity | parent::group/velocity[not(current()/velocity)]" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="count(sprite) &gt; 1">
      <xsl:value-of select="concat('&#9;&#9;', $thing, '.look = ')" />
      <xsl:call-template name="look-name"><xsl:with-param name="object" select="." /><xsl:with-param name="look" select="sprite[1]/@name" /></xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:for-each select="variables/variable | parent::group/variables/variable">
      <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
      <xsl:if test="$kind = 'live'">
        <xsl:value-of select="concat('&#9;&#9;', $thing, '.')" />
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$object" /><xsl:with-param name="start" select="true()" /></xsl:call-template>
        <xsl:text>;&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>&#9;}&#10;</xsl:text>
  </xsl:template>

  <!-- keepIn() and keep(): each kept object's look and live variables, a
       line each. -->
  <xsl:template name="keep-functions">
    <xsl:variable name="kept-list">
      <xsl:for-each select="$kept">
        <xsl:variable name="object" select="." />
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:if test="count(sprite) &gt; 1">
          <value local="{$thing}Look" member="{$thing}.look" type="int" count="{count(sprite)}">
            <xsl:attribute name="record"><xsl:call-template name="record-name" /></xsl:attribute>
            <xsl:attribute name="words"><xsl:value-of select="concat(@name, &quot;'s look&quot;)" /></xsl:attribute>
          </value>
        </xsl:if>
        <xsl:for-each select="variables/variable | parent::group/variables/variable">
          <xsl:variable name="kind"><xsl:call-template name="variable-kind" /></xsl:variable>
          <xsl:if test="$kind = 'live'">
            <xsl:variable name="camel"><xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
            <xsl:variable name="local"><xsl:call-template name="camel"><xsl:with-param name="name" select="concat($object/@name, ' ', @name)" /></xsl:call-template></xsl:variable>
            <value local="{$local}" member="{$thing}.{$camel}">
              <xsl:attribute name="words">
                <xsl:choose>
                  <xsl:when test="count($object/sprite) &gt; 1 or preceding-sibling::variable"><xsl:value-of select="concat('its ', @name)" /></xsl:when>
                  <xsl:otherwise><xsl:value-of select="concat($object/@name, &quot;'s &quot;, @name)" /></xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>
              <xsl:attribute name="type"><xsl:call-template name="variable-type" /></xsl:attribute>
            </value>
          </xsl:if>
        </xsl:for-each>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="values" select="exsl:node-set($kept-list)/value" />
    <xsl:text>&#10;&#9;// One line each: </xsl:text>
    <xsl:for-each select="$values">
      <xsl:if test="position() &gt; 1 and position() = last()">, then </xsl:if>
      <xsl:if test="position() &gt; 1 and position() != last()">, </xsl:if>
      <xsl:if test="not(starts-with(@words, 'its '))">the </xsl:if>
      <xsl:value-of select="@words" />
    </xsl:for-each>
    <xsl:text>.&#10;</xsl:text>
    <xsl:value-of select="concat('&#9;void ', $class, '::keepIn(const std::string&amp; file)&#10;&#9;{&#10;&#9;&#9;keptFile = file;&#10;&#9;&#9;std::ifstream in(file);&#10;')" />
    <xsl:for-each select="$values">
      <xsl:value-of select="concat('&#9;&#9;', @type, ' ', @local, ' = 0;&#10;')" />
    </xsl:for-each>
    <xsl:text>&#9;&#9;if (in</xsl:text>
    <xsl:for-each select="$values">
      <xsl:value-of select="concat(' &gt;&gt; ', @local)" />
    </xsl:for-each>
    <xsl:for-each select="$values[@count]">
      <xsl:value-of select="concat(' &amp;&amp; ', @local, ' &gt;= 0 &amp;&amp; ', @local, ' &lt; ', @count)" />
    </xsl:for-each>
    <xsl:text>)&#10;&#9;&#9;{&#10;</xsl:text>
    <xsl:for-each select="$values">
      <xsl:choose>
        <xsl:when test="@count">
          <xsl:value-of select="concat('&#9;&#9;&#9;', @member, ' = static_cast&lt;', @record, '::Look&gt;(', @local, ');&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&#9;&#9;&#9;', @member, ' = ', @local, ';&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>		}
	}
</xsl:text>
    <xsl:value-of select="concat('&#10;&#9;void ', $class, '::keep() const&#10;&#9;{&#10;&#9;&#9;if (keptFile.empty()) { return; }&#10;&#9;&#9;std::ofstream out(keptFile);&#10;&#9;&#9;out')" />
    <xsl:for-each select="$values">
      <xsl:choose>
        <xsl:when test="@count"><xsl:value-of select="concat(' &lt;&lt; static_cast&lt;int&gt;(', @member, ')')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat(' &lt;&lt; ', @member)" /></xsl:otherwise>
      </xsl:choose>
      <xsl:text> &lt;&lt; '\n'</xsl:text>
    </xsl:for-each>
    <xsl:text>;
	}
</xsl:text>
  </xsl:template>

  <!-- key(): which keys are down, then the presses the screen on top
       answers. -->
  <xsl:template name="key-function">
    <xsl:variable name="keys" select="$held-buttons[not(starts-with(., 'pad'))]" />
    <xsl:variable name="presses">
      <xsl:call-template name="presses"><xsl:with-param name="pads" select="false()" /></xsl:call-template>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="not($keys) and string($presses) = ''">
        <xsl:value-of select="concat('&#10;&#9;void ', $class, '::key(sf::Keyboard::Key, bool)&#10;&#9;{&#10;&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#10;&#9;void ', $class, '::key(sf::Keyboard::Key key, bool down)&#10;&#9;{&#10;&#9;&#9;using Key = sf::Keyboard::Key;&#10;')" />
        <xsl:if test="$keys">
          <xsl:text>&#9;&#9;switch (key)&#10;&#9;&#9;{&#10;</xsl:text>
          <xsl:for-each select="$keys">
            <xsl:text>&#9;&#9;case </xsl:text>
            <xsl:call-template name="key-code"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
            <xsl:text>: </xsl:text>
            <xsl:call-template name="held-name"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
            <xsl:text> = down; break;&#10;</xsl:text>
          </xsl:for-each>
          <xsl:text>		default: break;
		}
</xsl:text>
        </xsl:if>
        <xsl:if test="string($presses) != ''">
          <xsl:text>&#9;&#9;if (!down) { return; }&#10;&#10;</xsl:text>
          <xsl:value-of select="$presses" />
        </xsl:if>
        <xsl:text>	}&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- pad(): which pad buttons are down, then the presses. -->
  <xsl:template name="pad-function">
    <xsl:value-of select="concat('&#10;&#9;void ', $class, '::pad(unsigned pad, PadButton button, bool down)&#10;&#9;{&#10;')" />
    <xsl:for-each select="$held-buttons[starts-with(., 'pad')]">
      <xsl:text>&#9;&#9;if (</xsl:text>
      <xsl:call-template name="pad-test"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
      <xsl:text>) { </xsl:text>
      <xsl:call-template name="held-name"><xsl:with-param name="button" select="string(.)" /></xsl:call-template>
      <xsl:text> = down; }&#10;</xsl:text>
    </xsl:for-each>
    <xsl:variable name="presses">
      <xsl:call-template name="presses"><xsl:with-param name="pads" select="true()" /></xsl:call-template>
    </xsl:variable>
    <xsl:if test="string($presses) != ''">
      <xsl:text>&#9;&#9;if (!down) { return; }&#10;&#10;</xsl:text>
      <xsl:value-of select="$presses" />
    </xsl:if>
    <xsl:text>	}&#10;</xsl:text>
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
                      <xsl:text>key == </xsl:text>
                      <xsl:call-template name="key-code"><xsl:with-param name="button" select="$button" /></xsl:call-template>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:if>
              </xsl:for-each>
            </xsl:variable>
            <xsl:if test="$test != ''">
              <xsl:variable name="parts" select="substring($test, 2)" />
              <line>
                <xsl:text>if (</xsl:text>
                <xsl:call-template name="join-tests"><xsl:with-param name="tests" select="$parts" /><xsl:with-param name="bracket" select="$pads" /></xsl:call-template>
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
        <xsl:for-each select="$all/line"><xsl:value-of select="concat('&#9;&#9;', ., '&#10;')" /></xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>&#9;&#9;switch (screen())&#10;&#9;&#9;{&#10;</xsl:text>
        <xsl:for-each select="$all">
          <xsl:value-of select="concat('&#9;&#9;case ', @screen, ':&#10;')" />
          <xsl:for-each select="line"><xsl:value-of select="concat('&#9;&#9;&#9;', ., '&#10;')" /></xsl:for-each>
          <xsl:text>&#9;&#9;&#9;break;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:if test="count($all) &lt; count($states)">
          <xsl:text>&#9;&#9;default: break;&#10;</xsl:text>
        </xsl:if>
        <xsl:text>&#9;&#9;}&#10;</xsl:text>
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
    <xsl:value-of select="concat('&#10;&#9;void ', $class, '::frame(float elapsed)&#10;&#9;{&#10;')" />
    <xsl:for-each select="$states[count(. | $played) = 2][conditions/condition]">
      <!-- a screen that waits for keys, testing as it waits -->
      <xsl:text>&#9;&#9;if (screen() == </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="@name" /></xsl:call-template>
      <xsl:text>)&#10;&#9;&#9;{&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;&#9;&#9;// the ', @name, ' screen', &quot;'s own tests, the first that holds&#10;&quot;)" />
      <xsl:for-each select="conditions/condition">
        <xsl:call-template name="state-condition">
          <xsl:with-param name="before">
            <xsl:if test="position() &gt; 1">else </xsl:if>
          </xsl:with-param>
          <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:text>&#9;&#9;&#9;return;&#10;&#9;&#9;}&#10;</xsl:text>
    </xsl:for-each>
    <xsl:if test="$screens">
      <xsl:text>&#9;&#9;if (screen() != </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$played/@name" /></xsl:call-template>
      <xsl:text>) { return; }&#10;&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$moved">
      <xsl:choose>
        <xsl:when test="$computer">
          <xsl:value-of select="concat('&#9;&#9;// the keys held, seat ', $computer/@seat, &quot;'s by the computer while nobody has it&#10;&quot;)" />
          <xsl:for-each select="$played/inputs/seat[@number = $computer/@seat]/input[trigger]">
            <xsl:text>&#9;&#9;bool </xsl:text>
            <xsl:call-template name="seat-held" />
            <xsl:text> = </xsl:text>
            <xsl:call-template name="held-test" />
            <xsl:text>;&#10;</xsl:text>
          </xsl:for-each>
          <xsl:value-of select="concat('&#9;&#9;if (people &lt; ', $computer/@seat, ') { computerHolds(')" />
          <xsl:call-template name="computer-arguments" />
          <xsl:text>); }&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>&#9;&#9;// the keys held&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:for-each select="$moved">
        <xsl:call-template name="held-moves" />
      </xsl:for-each>
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:text>		// everything moves
</xsl:text>
    <xsl:for-each select="$things">
      <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;', $thing, '.position += ', $thing, '.velocity * elapsed;&#10;')" />
    </xsl:for-each>
    <xsl:for-each select="$moving-grids">
      <xsl:variable name="cell"><xsl:call-template name="cell-name" /></xsl:variable>
      <xsl:text>&#9;&#9;for (</xsl:text>
      <xsl:call-template name="cell-type" />
      <xsl:value-of select="concat('&amp; ', $cell, ' : ')" />
      <xsl:call-template name="object-name" />
      <xsl:value-of select="concat(') { ', $cell, '.position += ', $cell, '.velocity * elapsed; }&#10;')" />
    </xsl:for-each>
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
    <xsl:text>	}&#10;</xsl:text>
  </xsl:template>

  <!-- One of the game's own tests and what it does; one holding a
       condition is tested with it (&&), and does what that one does. -->
  <xsl:template name="state-condition">
    <xsl:param name="before" />
    <xsl:param name="tests" select="''" />
    <xsl:param name="indent" select="'&#9;&#9;'" />
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
    <xsl:value-of select="concat('&#10;&#9;// The mouse across the window: the ', $object/@name, &quot;'s middle follows it, kept inside the window.&#10;&#9;void &quot;, $class, '::mouse(float x)&#10;&#9;{&#10;')" />
    <xsl:if test="$screens">
      <xsl:text>&#9;&#9;if (screen() != </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="$input/ancestor::state/@name" /></xsl:call-template>
      <xsl:text>) { return; }&#10;</xsl:text>
    </xsl:if>
    <xsl:value-of select="concat('&#9;&#9;', $thing, '.position.x = std::clamp(x - ', $thing, '.size.x / 2, 0.0f, windowWidth - ', $thing, '.size.x);&#10;&#9;}&#10;')" />
  </xsl:template>

  <!-- An action a rule runs: faster(), its verbs on the thing it is of. -->
  <xsl:template name="action-function">
    <xsl:variable name="object" select="ancestor::object[1]" />
    <xsl:value-of select="concat('&#10;&#9;void ', $class, '::')" />
    <xsl:call-template name="action-name"><xsl:with-param name="action" select="@name" /></xsl:call-template>
    <xsl:text>()&#10;&#9;{&#10;</xsl:text>
    <xsl:variable name="body">
      <xsl:for-each select="*">
        <line><xsl:apply-templates select="." mode="do"><xsl:with-param name="self" select="$object" /></xsl:apply-templates></line>
      </xsl:for-each>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
      <xsl:with-param name="indent" select="'&#9;&#9;'" />
    </xsl:call-template>
    <xsl:text>&#9;}&#10;</xsl:text>
  </xsl:template>

  <!-- seat2Up: a held input of the computer's seat, by its first button. -->
  <xsl:template name="seat-held">
    <xsl:param name="input" select="." />
    <xsl:call-template name="camel">
      <xsl:with-param name="name" select="concat('seat ', $input/ancestor::seat/@number, ' ', normalize-space(($input/@button | $input/button)[1]))" />
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

  <!-- A thing the keys move: still, then each held key's moves. -->
  <xsl:template name="held-moves">
    <xsl:variable name="object" select="." />
    <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#9;&#9;', $thing, '.velocity = {};&#10;')" />
    <xsl:for-each select="$played/inputs//input[not(@axis)][trigger/@object = $object/@name]">
      <xsl:text>&#9;&#9;if (</xsl:text>
      <xsl:choose>
        <xsl:when test="$computer and ancestor::seat/@number = $computer/@seat"><xsl:call-template name="seat-held" /></xsl:when>
        <xsl:otherwise><xsl:call-template name="held-test" /></xsl:otherwise>
      </xsl:choose>
      <xsl:text>) {</xsl:text>
      <xsl:for-each select="trigger[@object = $object/@name]">
        <xsl:variable name="action" select="@action" />
        <xsl:variable name="done" select="($object/actions/action[@name = $action] | $object/parent::group/actions/action[@name = $action])[1]" />
        <xsl:if test="not($done)">
          <xsl:message terminate="yes">windows-cpp cannot find the action '<xsl:value-of select="$action" />' of '<xsl:value-of select="$object/@name" />'</xsl:message>
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
        <xsl:text>&#9;&#9;for (</xsl:text>
        <xsl:call-template name="cell-type" />
        <xsl:value-of select="concat('&amp; ', $cell, ' : ')" />
        <xsl:call-template name="object-name" />
        <xsl:text>)&#10;&#9;&#9;{&#10;</xsl:text>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($cell, '.')" />
            <xsl:with-param name="self" select="../.." />
            <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:text>&#9;&#9;}&#10;</xsl:text>
      </xsl:when>
      <xsl:when test="self::group">
        <xsl:variable name="one">
          <xsl:call-template name="camel">
            <xsl:with-param name="name"><xsl:call-template name="singular"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:with-param>
          </xsl:call-template>
        </xsl:variable>
        <xsl:text>&#9;&#9;for (</xsl:text>
        <xsl:call-template name="type-name"><xsl:with-param name="object" select="member[1]" /></xsl:call-template>
        <xsl:value-of select="concat('* ', $one, ' : { ')" />
        <xsl:for-each select="member">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:text>&amp;</xsl:text>
          <xsl:call-template name="object-name" />
        </xsl:for-each>
        <xsl:text> })&#10;&#9;&#9;{&#10;</xsl:text>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($one, '-&gt;')" />
            <xsl:with-param name="self" select="../.." />
            <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:text>&#9;&#9;}&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($thing, '.')" />
            <xsl:with-param name="self" select="../.." />
            <xsl:with-param name="indent" select="'&#9;&#9;'" />
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
       share one loop, in the file's order; a grid's list is looped over
       with its cells' own rules (grids.xsl), even where the thing has none
       for them. -->
  <xsl:template name="meetings">
    <xsl:choose>
      <xsl:when test="self::group">
        <!-- each member in turn, the group's rules on it -->
        <xsl:variable name="one"><xsl:call-template name="loop-name"><xsl:with-param name="group" select="." /></xsl:call-template></xsl:variable>
        <xsl:variable name="each">
          <xsl:call-template name="object-meetings"><xsl:with-param name="thing" select="concat('*', $one)" /></xsl:call-template>
        </xsl:variable>
        <xsl:text>&#9;&#9;for (</xsl:text>
        <xsl:call-template name="type-name"><xsl:with-param name="object" select="member[1]" /></xsl:call-template>
        <xsl:value-of select="concat('* ', $one, ' : { ')" />
        <xsl:for-each select="member">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:text>&amp;</xsl:text>
          <xsl:call-template name="object-name" />
        </xsl:for-each>
        <xsl:text> })&#10;&#9;&#9;{&#10;&#9;</xsl:text>
        <xsl:call-template name="replace">
          <xsl:with-param name="text" select="substring($each, 1, string-length($each) - 1)" />
          <xsl:with-param name="from" select="'&#10;'" />
          <xsl:with-param name="to" select="'&#10;&#9;'" />
        </xsl:call-template>
        <xsl:text>&#10;&#9;&#9;}&#10;</xsl:text>
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
       touchingPixels(*rocket, rocketsPicture, cell). -->
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
    <xsl:variable name="one">
      <xsl:choose>
        <xsl:when test="count($others) = 1"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$others" /></xsl:call-template></xsl:when>
        <xsl:when test="$class"><xsl:call-template name="camel"><xsl:with-param name="name" select="$class" /></xsl:call-template></xsl:when>
        <xsl:otherwise>other</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="other">
      <xsl:if test="count($others) &gt; 1">*</xsl:if>
      <xsl:value-of select="$one" />
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
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$others">
        <!-- their record when they share one, else Thing -->
        <xsl:variable name="first"><xsl:call-template name="type-name"><xsl:with-param name="object" select="$others[1]" /></xsl:call-template></xsl:variable>
        <xsl:variable name="unlike">
          <xsl:for-each select="$others">
            <xsl:variable name="each"><xsl:call-template name="type-name" /></xsl:variable>
            <xsl:if test="$each != $first">x</xsl:if>
          </xsl:for-each>
        </xsl:variable>
        <xsl:variable name="type">
          <xsl:choose>
            <xsl:when test="$unlike = ''"><xsl:value-of select="$first" /></xsl:when>
            <xsl:otherwise>Thing</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;&#9;for (const ', $type, '* ', $one, ' : { ')" />
        <xsl:for-each select="$others">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:text>&amp;</xsl:text>
          <xsl:call-template name="object-name" />
        </xsl:for-each>
        <xsl:text> })&#10;&#9;&#9;{&#10;</xsl:text>
        <xsl:call-template name="write-block">
          <xsl:with-param name="header">
            <xsl:text>if (</xsl:text>
            <xsl:call-template name="touch-test"><xsl:with-param name="object" select="$object" /><xsl:with-param name="thing" select="$thing" /><xsl:with-param name="other" select="$other" /></xsl:call-template>
            <xsl:text>)</xsl:text>
          </xsl:with-param>
          <xsl:with-param name="lines" select="$lines" />
          <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
        </xsl:call-template>
        <xsl:text>&#9;&#9;}&#10;</xsl:text>
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

  <!-- A condition's (or <any>'s) test as C++: what it looks at, compared.
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
            <xsl:when test="normalize-space($condition/remaining) = '0'"><xsl:value-of select="concat($list, '.empty()')" /></xsl:when>
            <xsl:otherwise><xsl:value-of select="concat($list, '.size() &lt;= ', normalize-space($condition/remaining))" /></xsl:otherwise>
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
              <xsl:when test="$compare/self::equal"> == </xsl:when>
              <xsl:otherwise> != </xsl:otherwise>
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

  <!-- bool& seat2Up, bool& seat2Down: the computer's seat's held inputs. -->
  <xsl:template name="computer-parameters">
    <xsl:for-each select="$played/inputs/seat[@number = $computer/@seat]/input[trigger]">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:text>bool&amp; </xsl:text>
      <xsl:call-template name="seat-held" />
    </xsl:for-each>
  </xsl:template>
  <xsl:template name="computer-arguments">
    <xsl:for-each select="$played/inputs/seat[@number = $computer/@seat]/input[trigger]">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="seat-held" />
    </xsl:for-each>
  </xsl:template>

  <!-- computerHolds(): every one of its conditions that holds holds its
       buttons. -->
  <xsl:template name="computer-function">
    <xsl:value-of select="concat('&#10;&#9;// The computer in seat ', $computer/@seat, ': the keys it holds.&#10;&#9;void ', $class, '::computerHolds(')" />
    <xsl:call-template name="computer-parameters" />
    <xsl:text>) const&#10;&#9;{&#10;</xsl:text>
    <xsl:for-each select="$played/inputs/seat[@number = $computer/@seat]/input[trigger]">
      <xsl:text>&#9;&#9;</xsl:text>
      <xsl:call-template name="seat-held" />
      <xsl:text> = false;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:variable name="body">
      <xsl:for-each select="$computer/condition">
        <xsl:call-template name="computer-condition" />
      </xsl:for-each>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
      <xsl:with-param name="indent" select="'&#9;&#9;'" />
    </xsl:call-template>
    <xsl:text>	}&#10;</xsl:text>
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
       the header, then the lines one deeper between braces. -->
  <xsl:template name="block-lines">
    <xsl:param name="header" />
    <xsl:param name="lines" />
    <xsl:param name="otherwise" select="''" />
    <xsl:choose>
      <xsl:when test="count($lines) = 1 and not($lines/@depth) and not(contains($lines, '&#10;')) and $otherwise = ''">
        <line><xsl:value-of select="concat($header, ' { ', $lines, ' }')" /></line>
      </xsl:when>
      <xsl:otherwise>
        <line><xsl:value-of select="$header" /></line>
        <line>{</line>
        <xsl:for-each select="$lines">
          <line depth="{1 + number(concat('0', @depth))}"><xsl:value-of select="." /></line>
        </xsl:for-each>
        <line>}</line>
        <xsl:if test="$otherwise != ''">
          <line><xsl:value-of select="concat('else { ', $otherwise, ' }')" /></line>
        </xsl:if>
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
    <xsl:param name="otherwise" select="''" />
    <xsl:variable name="block">
      <xsl:call-template name="block-lines">
        <xsl:with-param name="header" select="$header" />
        <xsl:with-param name="lines" select="$lines" />
        <xsl:with-param name="otherwise" select="$otherwise" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($block)/line" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
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

  <!-- A verb windows-cpp has no words for yet. -->
  <xsl:template match="*" mode="do">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; here yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="held">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; in an action yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
</xsl:stylesheet>
