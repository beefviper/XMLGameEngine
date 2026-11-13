<?xml version="1.0" encoding="UTF-8"?>
<!-- game.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- The game itself: source/<game>.h, its numbers and its class, and
     source/<game>.cpp, its rules. Each object is a Thing (where it is, how
     it moves, how big it is); a frame is the keys held, everything moving,
     the window's edges and the meetings, each rule written by its verb's
     file (verbs/) in one of three modes: held (a key held, on who), edge (at
     one edge, on who and that edge) and touch (on a meeting, on self and
     other). Nothing of the engine is in it. -->
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
  <xsl:variable name="uses-sign" select="boolean($state//sign)" />
  <xsl:variable name="uses-angles" select="boolean($state//sine | $state//cosine | $deflects)" />

  <!-- source/<game>.h -->
  <xsl:template name="header-file">
    <xsl:variable name="class"><xsl:call-template name="class-name" /></xsl:variable>
    <xsl:call-template name="header"><xsl:with-param name="file" select="concat($name, '.h')" /></xsl:call-template>
    <xsl:value-of select="concat('//&#10;// ', $title, ': its numbers, its things and its rules. main.cpp draws it.&#10;')" />
    <xsl:text>
#pragma once

#include &lt;SFML/System/Vector2.hpp&gt;
#include &lt;SFML/Window/Keyboard.hpp&gt;
</xsl:text>
    <xsl:if test="$randoms">
      <xsl:text>&#10;#include &lt;random&gt;&#10;</xsl:text>
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
    <xsl:text>
	// Something on the screen: where it is, how it moves, how big it is.
	struct Thing
	{
		sf::Vector2f position;
		sf::Vector2f velocity;
		sf::Vector2f size;
	};

</xsl:text>
    <xsl:value-of select="concat('&#9;class ', $class, '&#10;&#9;{&#10;&#9;public:&#10;')" />
    <xsl:choose>
      <xsl:when test="$randoms">
        <xsl:value-of select="concat('&#9;&#9;explicit ', $class, '(unsigned seed = std::random_device{}());&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;&#9;', $class, '();&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>
		void key(sf::Keyboard::Key key, bool down);
		void frame();

</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:text>&#9;&#9;Thing </xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:if test="$randoms or $buttons or $started">
      <xsl:text>&#10;&#9;private:&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$randoms">
      <xsl:text>		std::mt19937 random;
		float between(float min, float max);
</xsl:text>
    </xsl:if>
    <xsl:if test="$buttons">
      <xsl:if test="$randoms"><xsl:text>&#10;</xsl:text></xsl:if>
      <xsl:for-each select="$buttons">
        <xsl:text>&#9;&#9;bool </xsl:text>
        <xsl:call-template name="held-name" />
        <xsl:text>{ false };&#10;</xsl:text>
      </xsl:for-each>
    </xsl:if>
    <xsl:if test="$started">
      <xsl:if test="$randoms or $buttons"><xsl:text>&#10;</xsl:text></xsl:if>
      <xsl:for-each select="$started">
        <xsl:text>&#9;&#9;void </xsl:text>
        <xsl:call-template name="start-name" />
        <xsl:text>();&#10;</xsl:text>
      </xsl:for-each>
    </xsl:if>
    <xsl:text>	};
}
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
        <xsl:text>&#9;constexpr float </xsl:text>
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

  <!-- startBall: where it starts, and starts again. -->
  <xsl:template name="start-name">
    <xsl:param name="object" select="." />
    <xsl:variable name="camel"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('start', translate(substring($camel, 1, 1), $lower, $upper), substring($camel, 2))" />
  </xsl:template>

  <!-- source/<game>.cpp -->
  <xsl:template name="game-file">
    <xsl:variable name="class"><xsl:call-template name="class-name" /></xsl:variable>
    <xsl:call-template name="header"><xsl:with-param name="file" select="concat($name, '.cpp')" /></xsl:call-template>
    <xsl:value-of select="concat('&#10;#include &quot;', $name, '.h&quot;&#10;')" />
    <xsl:if test="$deflects or $uses-angles or $edge-owners//bounce">
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="$deflects"><xsl:text>#include &lt;algorithm&gt;&#10;</xsl:text></xsl:if>
      <xsl:text>#include &lt;cmath&gt;&#10;</xsl:text>
    </xsl:if>
    <xsl:text>&#10;namespace </xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:text>&#10;{&#10;</xsl:text>
    <xsl:call-template name="helpers" />
    <xsl:call-template name="constructor"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    <xsl:if test="$randoms">
      <xsl:value-of select="concat('&#10;&#9;float ', $class, '::between(float min, float max)&#10;&#9;{&#10;&#9;&#9;return std::uniform_real_distribution&lt;float&gt;(min, max)(random);&#10;&#9;}&#10;')" />
    </xsl:if>
    <xsl:for-each select="$started">
      <xsl:call-template name="start-function"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    </xsl:for-each>
    <xsl:call-template name="key-function"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    <xsl:call-template name="frame-function"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- The small functions the rules are written with, those they use. -->
  <xsl:template name="helpers">
    <xsl:if test="$uses-sign or $uses-angles or $touch-owners">
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

  <!-- The constructor: each thing where the file puts it. -->
  <xsl:template name="constructor">
    <xsl:param name="class" />
    <xsl:choose>
      <xsl:when test="$randoms">
        <xsl:value-of select="concat('&#9;', $class, '::', $class, '(unsigned seed) : random(seed)&#10;&#9;{&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;', $class, '::', $class, '()&#10;&#9;{&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:for-each select="$objects">
      <xsl:text>&#9;&#9;</xsl:text>
      <xsl:choose>
        <xsl:when test="count(. | $started) = count($started)">
          <xsl:call-template name="start-name" />
          <xsl:text>();&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="object-name" />
          <xsl:text> = { </xsl:text>
          <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="size" />
          <xsl:text> };&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>	}&#10;</xsl:text>
  </xsl:template>

  <!-- { x, y } of a <position> or <velocity>; {} when there is none. -->
  <xsl:template name="pair">
    <xsl:param name="of" />
    <xsl:param name="self" select="." />
    <xsl:choose>
      <xsl:when test="$of">
        <xsl:text>{ </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$of/x" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$of/y" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise>{}</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- { width, height } of its rectangle, or its group's. -->
  <xsl:template name="size">
    <xsl:variable name="rectangle" select="(sprite | parent::group/sprite)[1]/rectangle" />
    <xsl:text>{ </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="$rectangle/width" /><xsl:with-param name="self" select="." /></xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="$rectangle/height" /><xsl:with-param name="self" select="." /></xsl:call-template>
    <xsl:text> }</xsl:text>
  </xsl:template>

  <!-- startBall(): its own variables (a <random> drawn again each time),
       then its size, place and way. -->
  <xsl:template name="start-function">
    <xsl:param name="class" />
    <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:text>&#10;&#9;// Where the </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text> starts</xsl:text>
    <xsl:if test="collisions/collision/reset">, and starts again</xsl:if>
    <xsl:text>.&#10;&#9;void </xsl:text>
    <xsl:value-of select="concat($class, '::')" />
    <xsl:call-template name="start-name" />
    <xsl:text>()&#10;&#9;{&#10;</xsl:text>
    <xsl:for-each select="variables/variable">
      <xsl:text>&#9;&#9;const float </xsl:text>
      <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text> = </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="../.." /></xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:value-of select="concat('&#9;&#9;', $thing, '.size = ')" />
    <xsl:call-template name="size" />
    <xsl:value-of select="concat(';&#10;&#9;&#9;', $thing, '.position = ')" />
    <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
    <xsl:value-of select="concat(';&#10;&#9;&#9;', $thing, '.velocity = ')" />
    <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity" /></xsl:call-template>
    <xsl:text>;&#10;&#9;}&#10;</xsl:text>
  </xsl:template>

  <!-- key(): which keys are down. -->
  <xsl:template name="key-function">
    <xsl:param name="class" />
    <xsl:choose>
      <xsl:when test="not($buttons)">
        <xsl:value-of select="concat('&#10;&#9;void ', $class, '::key(sf::Keyboard::Key, bool)&#10;&#9;{&#10;&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#10;&#9;void ', $class, '::key(sf::Keyboard::Key key, bool down)&#10;&#9;{&#10;&#9;&#9;using Key = sf::Keyboard::Key;&#10;&#9;&#9;switch (key)&#10;&#9;&#9;{&#10;')" />
        <xsl:for-each select="$buttons">
          <xsl:text>&#9;&#9;case </xsl:text>
          <xsl:call-template name="key-code"><xsl:with-param name="button" select="normalize-space(.)" /></xsl:call-template>
          <xsl:text>: </xsl:text>
          <xsl:call-template name="held-name" />
          <xsl:text> = down; break;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>		default: break;
		}
	}
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- frame(): the keys held, everything moving, the window's edges, the
       meetings. -->
  <xsl:template name="frame-function">
    <xsl:param name="class" />
    <xsl:value-of select="concat('&#10;&#9;void ', $class, '::frame()&#10;&#9;{&#10;')" />
    <xsl:if test="$moved">
      <xsl:text>		// the keys held
</xsl:text>
      <xsl:for-each select="$moved">
        <xsl:call-template name="held-moves" />
      </xsl:for-each>
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:text>		// everything moves
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;', $thing, '.position += ', $thing, '.velocity;&#10;')" />
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
    <xsl:text>	}&#10;</xsl:text>
  </xsl:template>

  <!-- A thing the keys move: still, then each held key's moves. -->
  <xsl:template name="held-moves">
    <xsl:variable name="object" select="." />
    <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#9;&#9;', $thing, '.velocity = {};&#10;')" />
    <xsl:for-each select="$inputs[trigger/@object = $object/@name]">
      <xsl:text>&#9;&#9;if (</xsl:text>
      <xsl:for-each select="@button | button">
        <xsl:if test="position() &gt; 1"> || </xsl:if>
        <xsl:call-template name="held-name" />
      </xsl:for-each>
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
        <xsl:value-of select="concat('&#9;&#9;for (Thing* ', $one, ' : { ')" />
        <xsl:for-each select="member">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:text>&amp;</xsl:text>
          <xsl:call-template name="object-name" />
        </xsl:for-each>
        <xsl:text> })&#10;&#9;&#9;{&#10;</xsl:text>
        <xsl:for-each select="collisions/collision[@edge]">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="of" select="concat($one, '-&gt;')" />
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
        <xsl:value-of select="concat('&#9;&#9;if (touching(', $thing, ', ', $other, ')) {')" />
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
        <xsl:text>&#9;&#9;for (const Thing* other : { </xsl:text>
        <xsl:for-each select="$others">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:text>&amp;</xsl:text>
          <xsl:call-template name="object-name" />
        </xsl:for-each>
        <xsl:value-of select="concat(' })&#10;&#9;&#9;{&#10;&#9;&#9;&#9;if (touching(', $thing, ', *other)) {')" />
        <xsl:for-each select="$rules">
          <xsl:text> </xsl:text>
          <xsl:apply-templates select="." mode="touch">
            <xsl:with-param name="self" select="$thing" />
            <xsl:with-param name="other" select="'*other'" />
          </xsl:apply-templates>
        </xsl:for-each>
        <xsl:text> }&#10;&#9;&#9;}&#10;</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- A verb windows-cpp has no words for yet, in any mode. -->
  <xsl:template match="*" mode="edge">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; at an edge yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="touch">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; on a touch yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="held">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; in an action yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
</xsl:stylesheet>
