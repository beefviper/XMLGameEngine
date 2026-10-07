<?xml version="1.0" encoding="UTF-8"?>
<!-- main.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- main.cpp, written the way a person would write the game by hand on SFML 3:
     the header, the includes (local, third party, standard), the window's
     constants and the game's tunables, the objects (SFML shapes, and a velocity
     for those that move), the functions declared, main (the window, then the
     game loop: events, update, render), and the functions defined below it.
     Each object is a global named as in the game (paddle1), each rule an if
     statement in that object's update function, and only the helper functions
     the game uses are written (functions.xml). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:date="http://exslt.org/dates-and-times"
    exclude-result-prefixes="date">

  <xsl:variable name="tables" select="document('tables.xml')/tables" />
  <xsl:variable name="helpers" select="document('functions.xml')/functions" />

  <xsl:variable name="game" select="/game" />
  <xsl:variable name="state" select="/game/states/state[1]" />

  <!-- The objects the screen shows; the rest never appear, so are left out. -->
  <xsl:variable name="objects" select="/game/objects/object[@name = $state/shows/show/@object]" />

  <!-- Those that move on their own: a velocity that is not 0, 0. -->
  <xsl:variable name="moving" select="$objects[velocity/x/* or velocity/y/* or number(velocity/x) != 0 or number(velocity/y) != 0]" />

  <xsl:variable name="rules" select="$objects/collisions[normalize-space(enabled) = 'true']/collision" />
  <xsl:variable name="edge-rules" select="$rules[@edge]" />
  <xsl:variable name="object-rules" select="$rules[not(@edge)]" />

  <!-- Those that go back to where they started (a <reset />, which puts back
       the place and keeps the velocity, as in the engine). -->
  <xsl:variable name="resetting" select="$objects[collisions[normalize-space(enabled) = 'true']/collision/reset]" />

  <!-- Those with something to do each frame. -->
  <xsl:variable name="updating" select="$objects[count(. | $moving) = count($moving) or collisions[normalize-space(enabled) = 'true']/collision/* or @name = $state/inputs/input/trigger/@object]" />

  <!-- Every word in the parts of the game that are written out, between
       spaces, to see which names and functions it uses. -->
  <xsl:variable name="words">
    <xsl:text> </xsl:text>
    <xsl:for-each select="$game/window//text() | $game/variables//text() | $game/variables//@* | $objects//text() | $objects//@*">
      <xsl:value-of select="translate(., '+-*/(),&#9;&#10;&#13;', '          ')" />
      <xsl:text> </xsl:text>
    </xsl:for-each>
  </xsl:variable>

  <!-- The helper functions the game needs (functions.xml). -->
  <xsl:variable name="used">
    <xsl:text> </xsl:text>
    <xsl:if test="$objects//random"> randomBetween </xsl:if>
    <xsl:if test="contains($words, ' sgn ')"> sign </xsl:if>
    <xsl:if test="$edge-rules[* and (@edge = 'left' or @edge = 'horizontal' or @edge = 'all')]"> left </xsl:if>
    <xsl:if test="$edge-rules[* and (@edge = 'right' or @edge = 'horizontal' or @edge = 'all')]"> right </xsl:if>
    <xsl:if test="$edge-rules[* and (@edge = 'top' or @edge = 'vertical' or @edge = 'all')]"> top </xsl:if>
    <xsl:if test="$edge-rules[* and (@edge = 'bottom' or @edge = 'vertical' or @edge = 'all')]"> bottom </xsl:if>
    <xsl:if test="$object-rules/*"> touching </xsl:if>
    <xsl:if test="$object-rules/bounce"> bounceOff </xsl:if>
    <xsl:if test="$object-rules/deflect"> deflect </xsl:if>
  </xsl:variable>
  <xsl:variable name="used-functions" select="$helpers/function[contains($used, concat(' ', @name, ' '))]" />

  <!-- The standard headers it needs. -->
  <xsl:variable name="headers">
    <xsl:text> optional </xsl:text>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="concat(' ', @uses, ' ')" />
    </xsl:for-each>
    <xsl:if test="$edge-rules/bounce"> cmath </xsl:if>
    <xsl:if test="$edge-rules[ancestor::object[count(. | $moving) = count($moving)]]/stick"> algorithm </xsl:if>
    <xsl:if test="contains($words, ' min ') or contains($words, ' max ')"> algorithm </xsl:if>
    <xsl:if test="contains($words, ' abs ') or contains($words, ' floor ') or contains($words, ' ceil ') or contains($words, ' sqrt ') or contains($words, ' sin ') or contains($words, ' cos ') or contains($words, ' tan ') or contains($words, ' pow ') or contains($words, ' round ')"> cmath </xsl:if>
  </xsl:variable>

  <!-- The date in the header: today's, as Oct 5, 2026. -->
  <xsl:param name="date" select="concat(date:month-abbreviation(), ' ', date:day-in-month(), ', ', date:year())" />

  <!-- ===================================================================== -->
  <!-- The file                                                               -->
  <!-- ===================================================================== -->

  <xsl:template name="generate-file">
    <xsl:call-template name="generate-header" />
    <xsl:call-template name="generate-includes" />
    <xsl:call-template name="generate-globals" />
    <xsl:call-template name="generate-objects" />
    <xsl:call-template name="generate-declarations" />
    <xsl:call-template name="generate-main" />
    <xsl:call-template name="generate-definitions" />
  </xsl:template>

  <xsl:template name="generate-header">
    <xsl:text>// main.cpp
// XML Game Engine
// author: beefviper
// date: </xsl:text>
    <xsl:value-of select="$date" />
    <xsl:text>
//
// </xsl:text>
    <xsl:value-of select="concat($game/window/@name, ', generated from ', $source, ' by xgecli --generate windows-cpp.')" />
    <xsl:text>
// Change the game file and generate it again rather than editing this.
</xsl:text>
  </xsl:template>

  <!-- Local headers (none yet), then SFML, then the standard library. -->
  <xsl:template name="generate-includes">
    <xsl:text>
#include &lt;SFML/Graphics.hpp&gt;

</xsl:text>
    <xsl:for-each select="document('')//xsl:variable[@name = 'standard-headers']/header">
      <xsl:if test="contains($headers, concat(' ', ., ' '))">
        <xsl:value-of select="concat('#include &lt;', ., '&gt;&#10;')" />
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:variable name="standard-headers">
    <header>algorithm</header>
    <header>cmath</header>
    <header>optional</header>
    <header>random</header>
  </xsl:variable>

  <!-- The window's constants, the ones of its names the game uses, and the
       game's own <variables> as tunables. -->
  <xsl:template name="generate-globals">
    <xsl:text>
// window
const float windowWidth = </xsl:text>
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$game/window/width" /></xsl:call-template>
    <xsl:text>;
const float windowHeight = </xsl:text>
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$game/window/height" /></xsl:call-template>
    <xsl:text>;
</xsl:text>
    <xsl:if test="contains($words, ' window.left ')">const float windowLeft = 0.0f;&#10;</xsl:if>
    <xsl:if test="contains($words, ' window.right ')">const float windowRight = windowWidth;&#10;</xsl:if>
    <xsl:if test="contains($words, ' window.top ')">const float windowTop = 0.0f;&#10;</xsl:if>
    <xsl:if test="contains($words, ' window.bottom ')">const float windowBottom = windowHeight;&#10;</xsl:if>
    <xsl:if test="contains($words, ' window.width.center ')">const float windowWidthCenter = windowWidth / 2.0f;&#10;</xsl:if>
    <xsl:if test="contains($words, ' window.height.center ')">const float windowHeightCenter = windowHeight / 2.0f;&#10;</xsl:if>
    <xsl:if test="contains($words, ' pi ')">const float pi = 3.14159265f;&#10;</xsl:if>
    <xsl:text>const unsigned int framerate = </xsl:text>
    <xsl:value-of select="normalize-space($game/window/framerate)" />
    <xsl:text>;
const sf::Color background = </xsl:text>
    <xsl:call-template name="color"><xsl:with-param name="name" select="$game/window/background" /></xsl:call-template>
    <xsl:text>;
</xsl:text>
    <xsl:variable name="variables" select="$game/variables/variable[not(@name = following-sibling::variable/@name)]" />
    <xsl:if test="$variables">
      <xsl:text>
// tunables
</xsl:text>
      <xsl:for-each select="$variables">
        <xsl:text>const float </xsl:text>
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value-bare" />
        <xsl:text>;
</xsl:text>
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <!-- Each object: its SFML shape, the velocity of one that moves, and the
       starting place of one that goes back to it. -->
  <xsl:template name="generate-objects">
    <xsl:text>
// objects</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:variable name="moves" select="count(. | $moving) = count($moving)" />
      <xsl:if test="position() &gt; 1">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:text>
</xsl:text>
      <xsl:choose>
        <xsl:when test="sprite/circle">sf::CircleShape </xsl:when>
        <xsl:otherwise>sf::RectangleShape </xsl:otherwise>
      </xsl:choose>
      <xsl:value-of select="concat($name, ';')" />
      <xsl:if test="$moves">
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Velocity;')" />
      </xsl:if>
      <xsl:for-each select="variables/variable">
        <xsl:text>&#10;float </xsl:text>
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="concat(ancestor::object/@name, '.', @name)" /></xsl:call-template>
        <xsl:text> = 0.0f;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template name="generate-declarations">
    <xsl:text>
// functions
void setup();
</xsl:text>
    <xsl:for-each select="$updating">
      <xsl:text>void update</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$resetting">
      <xsl:text>void start</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="concat(normalize-space(declaration), '&#10;')" />
    </xsl:for-each>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- main                                                                   -->
  <!-- ===================================================================== -->

  <xsl:template name="generate-main">
    <xsl:text>
int main()
{
</xsl:text>
    <xsl:call-template name="generate-window" />
    <xsl:text>
	setup();
</xsl:text>
    <xsl:call-template name="generate-game-loop" />
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <xsl:template name="generate-window">
    <xsl:text>	sf::RenderWindow window(sf::VideoMode({static_cast&lt;unsigned int&gt;(windowWidth), static_cast&lt;unsigned int&gt;(windowHeight)}), </xsl:text>
    <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="$game/window/@name" /></xsl:call-template>
    <xsl:if test="normalize-space($game/window/fullscreen) = 'true'">
      <xsl:text>, sf::Style::Default, sf::State::Fullscreen</xsl:text>
    </xsl:if>
    <xsl:text>);
	window.setFramerateLimit(framerate);
</xsl:text>
  </xsl:template>

  <xsl:template name="generate-game-loop">
    <xsl:text>
	while (window.isOpen())
	{
</xsl:text>
    <xsl:call-template name="generate-events" />
    <xsl:call-template name="generate-update" />
    <xsl:call-template name="generate-render" />
    <xsl:text>	}
</xsl:text>
  </xsl:template>

  <xsl:template name="generate-events">
    <xsl:text>		while (const std::optional event = window.pollEvent())
		{
			if (event-&gt;is&lt;sf::Event::Closed&gt;())
			{
				window.close();
			}
		}
</xsl:text>
  </xsl:template>

  <xsl:template name="generate-update">
    <xsl:if test="$updating">
      <xsl:text>
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$updating">
      <xsl:text>		update</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <xsl:template name="generate-render">
    <xsl:text>
		window.clear(background);
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:text>		window.draw(</xsl:text>
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>);
</xsl:text>
    </xsl:for-each>
    <xsl:text>		window.display();
</xsl:text>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- The functions                                                          -->
  <!-- ===================================================================== -->

  <xsl:template name="generate-definitions">
    <xsl:call-template name="generate-setup" />
    <xsl:for-each select="$updating">
      <xsl:call-template name="generate-object-update" />
    </xsl:for-each>
    <xsl:for-each select="$resetting">
      <xsl:text>
// </xsl:text>
      <xsl:value-of select="@name" />
      <xsl:text>: where it starts, and starts again after a &lt;reset /&gt; (any &lt;random&gt; drawn anew)
void start</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>()
{
</xsl:text>
      <xsl:call-template name="object-start" />
      <xsl:text>}
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="definition" />
    </xsl:for-each>
  </xsl:template>

  <!-- An object's start: its variables (first, as the engine works them out
       first), its position and the velocity of one that moves. -->
  <xsl:template name="object-start">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:for-each select="variables/variable">
      <xsl:sort select="@name" />
      <xsl:text>	</xsl:text>
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="concat(ancestor::object/@name, '.', @name)" /></xsl:call-template>
      <xsl:text> = </xsl:text>
      <xsl:call-template name="value-bare" />
      <xsl:text>;
</xsl:text>
    </xsl:for-each>
    <xsl:value-of select="concat('&#9;', $name, '.setPosition(')" />
    <xsl:call-template name="vector"><xsl:with-param name="node" select="position" /></xsl:call-template>
    <xsl:text>);
</xsl:text>
    <xsl:if test="count(. | $moving) = count($moving)">
      <xsl:value-of select="concat('&#9;', $name, 'Velocity = ')" />
      <xsl:call-template name="vector"><xsl:with-param name="node" select="velocity" /></xsl:call-template>
      <xsl:text>;
</xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template name="generate-setup">
    <xsl:text>
// Every object's shape, color and starting place.
void setup()
{
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:variable name="moves" select="count(. | $moving) = count($moving)" />
      <xsl:variable name="resets" select="count(. | $resetting) = count($resetting)" />
      <xsl:if test="position() &gt; 1">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="sprite/circle">
          <xsl:value-of select="concat('&#9;', $name, '.setRadius(')" />
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="sprite/circle/radius" /></xsl:call-template>
          <xsl:text>);
</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&#9;', $name, '.setSize({')" />
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="sprite/rectangle/width" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="sprite/rectangle/height" /></xsl:call-template>
          <xsl:text>});
</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:value-of select="concat('&#9;', $name, '.setFillColor(')" />
      <xsl:call-template name="color"><xsl:with-param name="name" select="sprite/*/color" /></xsl:call-template>
      <xsl:text>);
</xsl:text>
      <xsl:choose>
        <xsl:when test="$resets">
          <xsl:text>	start</xsl:text>
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>();
</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="object-start" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- An object's frame: the keys that move it, its own move, then its
       collision rules in the order written. Blocks are written each with a
       line break before it, and the first one's taken off. -->
  <xsl:template name="generate-object-update">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="object" select="." />
    <xsl:variable name="blocks">
      <xsl:for-each select="$state/inputs/input[trigger/@object = current()/@name]">
        <xsl:text>
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::</xsl:text>
        <xsl:value-of select="$tables/keys/key[@name = current()/@button]/@sfml" />
        <xsl:text>))
	{
</xsl:text>
        <xsl:for-each select="trigger[@object = $object/@name]">
          <xsl:for-each select="$object/actions/action[@name = current()/@action]/move">
            <xsl:value-of select="concat('&#9;&#9;', $name, '.move(')" />
            <xsl:call-template name="direction"><xsl:with-param name="node" select="." /></xsl:call-template>
            <xsl:text>);
</xsl:text>
          </xsl:for-each>
        </xsl:for-each>
        <xsl:text>	}
</xsl:text>
      </xsl:for-each>
      <xsl:if test="count(. | $moving) = count($moving)">
        <xsl:value-of select="concat('&#10;&#9;', $name, '.move(', $name, 'Velocity);&#10;')" />
      </xsl:if>
      <xsl:for-each select="collisions[normalize-space(enabled) = 'true']/collision[*]">
        <xsl:choose>
          <xsl:when test="@edge">
            <xsl:call-template name="edge-rule"><xsl:with-param name="name" select="$name" /></xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="object-rule"><xsl:with-param name="name" select="$name" /></xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </xsl:variable>
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>, each frame
void update</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>()
{
</xsl:text>
    <xsl:value-of select="substring($blocks, 2)" />
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- A screen-edge rule: an if statement for each edge it is about. -->
  <xsl:template name="edge-rule">
    <xsl:param name="name" />
    <xsl:variable name="edge" select="@edge" />
    <xsl:variable name="rule" select="." />
    <xsl:for-each select="document('')//xsl:variable[@name = 'edges']/edge[@name = $edge or @in = $edge or $edge = 'all']">
      <xsl:variable name="side" select="@name" />
      <xsl:value-of select="concat('&#10;&#9;// ', $side, ':')" />
      <xsl:for-each select="$rule/*">
        <xsl:value-of select="concat(' ', local-name())" />
      </xsl:for-each>
      <xsl:value-of select="concat('&#10;&#9;if (', $side, '(', $name, ') ', @test, ')&#10;&#9;{&#10;')" />
      <xsl:for-each select="$rule/*">
        <xsl:choose>
          <xsl:when test="self::reset">
            <xsl:text>		start</xsl:text>
            <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="ancestor::object/@name" /></xsl:call-template>
            <xsl:text>();
</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <!-- bounce and stick: back inside the edge first -->
            <xsl:variable name="edgeInfo" select="document('')//xsl:variable[@name = 'edges']/edge[@name = $side]" />
            <xsl:variable name="velocity" select="concat($name, 'Velocity.', $edgeInfo/@axis)" />
            <xsl:value-of select="concat('&#9;&#9;', $name, '.move(')" />
            <xsl:choose>
              <xsl:when test="$side = 'top'">{0.0f, -top(<xsl:value-of select="$name" />)}</xsl:when>
              <xsl:when test="$side = 'bottom'">{0.0f, windowHeight - bottom(<xsl:value-of select="$name" />)}</xsl:when>
              <xsl:when test="$side = 'left'">{-left(<xsl:value-of select="$name" />), 0.0f}</xsl:when>
              <xsl:otherwise>{windowWidth - right(<xsl:value-of select="$name" />), 0.0f}</xsl:otherwise>
            </xsl:choose>
            <xsl:text>);
</xsl:text>
            <xsl:choose>
              <xsl:when test="self::bounce">
                <xsl:value-of select="concat('&#9;&#9;', $velocity, ' = ', $edgeInfo/@away, 'std::abs(', $velocity, ');&#10;')" />
              </xsl:when>
              <xsl:when test="self::stick and ancestor::object[count(. | $moving) = count($moving)]">
                <xsl:value-of select="concat('&#9;&#9;', $velocity, ' = std::', $edgeInfo/@stop, '(', $velocity, ', 0.0f);&#10;')" />
              </xsl:when>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:text>	}
</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- What each edge is: when an object is past it, the axis it is on, the
       sign of a velocity heading away from it, and what keeps one from heading
       into it. -->
  <xsl:variable name="edges">
    <edge name="top" in="vertical" test="&lt; 0.0f" axis="y" away="" stop="max" />
    <edge name="bottom" in="vertical" test="&gt; windowHeight" axis="y" away="-" stop="min" />
    <edge name="left" in="horizontal" test="&lt; 0.0f" axis="x" away="" stop="max" />
    <edge name="right" in="horizontal" test="&gt; windowWidth" axis="x" away="-" stop="min" />
  </xsl:variable>

  <!-- A rule about other objects: an if statement for each it can touch. -->
  <xsl:template name="object-rule">
    <xsl:param name="name" />
    <xsl:variable name="rule" select="." />
    <xsl:variable name="self" select="ancestor::object" />
    <xsl:for-each select="$objects[collisions[normalize-space(enabled) = 'true']]
                                  [count(. | $self) != 1]
                                  [not($rule/@object) or @name = $rule/@object]
                                  [not($rule/@class) or @class = $rule/@class]">
      <xsl:variable name="other"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#10;&#9;// ', @name, ':')" />
      <xsl:for-each select="$rule/*">
        <xsl:value-of select="concat(' ', local-name())" />
      </xsl:for-each>
      <xsl:value-of select="concat('&#10;&#9;if (touching(', $name, ', ', $other, '))&#10;&#9;{&#10;')" />
      <xsl:for-each select="$rule/*">
        <xsl:choose>
          <xsl:when test="self::reset">
            <xsl:text>		start</xsl:text>
            <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template>
            <xsl:text>();
</xsl:text>
          </xsl:when>
          <xsl:when test="self::bounce">
            <xsl:value-of select="concat('&#9;&#9;bounceOff(', $name, ', ', $name, 'Velocity, ', $other, ');&#10;')" />
          </xsl:when>
          <xsl:when test="self::deflect">
            <xsl:value-of select="concat('&#9;&#9;deflect(', $name, ', ', $name, 'Velocity, ', $other, ', ')" />
            <xsl:call-template name="value-bare" />
            <xsl:text>);
</xsl:text>
          </xsl:when>
        </xsl:choose>
      </xsl:for-each>
      <xsl:text>	}
</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Small pieces                                                           -->
  <!-- ===================================================================== -->

  <!-- A position or velocity as {x, y}. -->
  <xsl:template name="vector">
    <xsl:param name="node" />
    <xsl:text>{</xsl:text>
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$node/x" /></xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$node/y" /></xsl:call-template>
    <xsl:text>}</xsl:text>
  </xsl:template>

  <!-- A <move direction>'s step as the vector to move by. -->
  <xsl:template name="direction">
    <xsl:param name="node" />
    <xsl:variable name="direction" select="$node/@direction" />
    <xsl:choose>
      <xsl:when test="$direction = 'up' or $direction = 'left'">
        <xsl:if test="$direction = 'up'">{0.0f, </xsl:if>
        <xsl:if test="$direction = 'left'">{</xsl:if>
        <xsl:text>-</xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$node" /></xsl:call-template>
        <xsl:if test="$direction = 'up'">}</xsl:if>
        <xsl:if test="$direction = 'left'">, 0.0f}</xsl:if>
      </xsl:when>
      <xsl:when test="$direction = 'down'">
        <xsl:text>{0.0f, </xsl:text>
        <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$node" /></xsl:call-template>
        <xsl:text>}</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>{</xsl:text>
        <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$node" /></xsl:call-template>
        <xsl:text>, 0.0f}</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A color name as SFML's own (sf::Color::White) or by its numbers. -->
  <xsl:template name="color">
    <xsl:param name="name" />
    <xsl:variable name="color" select="$tables/colors/color[@name = normalize-space($name)]" />
    <xsl:choose>
      <xsl:when test="$color/@sfml">
        <xsl:value-of select="concat('sf::Color::', $color/@sfml)" />
      </xsl:when>
      <xsl:when test="$color">
        <xsl:value-of select="concat('sf::Color(', $color/@rgba, ')')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">windows-cpp: there is no color named "<xsl:value-of select="normalize-space($name)" />"</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
