<?xml version="1.0" encoding="UTF-8"?>
<!-- main.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- main.cpp, written the way a person would write the game by hand on SFML 3:
     the header, the includes (local, third party, standard), the window's
     constants and the game's tunables, the screens, the objects (SFML shapes,
     sprites and texts, and a velocity for those that move), the sounds, the
     functions declared, main (the window, then the game loop: events, update,
     render), and the functions defined below it. Each object is a global named
     as in the game (paddle1), each rule an if statement in that object's
     update function, each screen's keys and conditions in that screen's update
     function. The physics and the sound are modules of their own beside it
     (modules/physics.h, modules/sound.h and sound.cpp), copied only when the
     game uses them; the few other helpers it needs are in functions.xml. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:date="http://exslt.org/dates-and-times"
    xmlns:exsl="http://exslt.org/common"
    exclude-result-prefixes="date exsl">

  <xsl:variable name="tables" select="document('tables.xml')/tables" />
  <xsl:variable name="helpers" select="document('functions.xml')/functions" />

  <xsl:variable name="game" select="/game" />
  <xsl:variable name="states" select="/game/states/state" />
  <xsl:variable name="first" select="$states[1]" />

  <!-- More than one screen: a stack of them, and a switch over the one on top. -->
  <xsl:variable name="screens" select="count($states) &gt; 1" />

  <!-- The objects and groups some screen shows; the rest never appear, so are
       left out. A group is a std::vector of its members' shapes. -->
  <xsl:variable name="objects" select="/game/objects/object[@name = $states/shows/show/@object]" />
  <xsl:variable name="groups" select="/game/objects/group[@name = $states/shows/show/@object]" />
  <xsl:variable name="things" select="$objects | $groups" />
  <xsl:variable name="texts" select="$objects[sprite/text]" />
  <xsl:variable name="numbers" select="$texts[sprite/text/number]" />
  <xsl:variable name="images" select="$objects[sprite/image] | $groups[.//sprite/image]" />
  <!-- The sprites the game draws itself when it starts: rows of text (a
       <bitmap>), <line>s, or an <svg>, drawn by xgecli into a picture of its
       own. Each is a texture named for what shows it (shipPicture), and a
       group's for each look its members or cells have (groups.xsl). -->
  <xsl:variable name="drawn" select="($objects/sprite | $groups[sprite[2]]/sprite | $group-data[not(@looks)]/look/sprite)[line or bitmap or svg]" />
  <xsl:variable name="drawn-rows" select="$drawn[bitmap]" />
  <xsl:variable name="drawn-lines" select="$drawn[line]" />
  <xsl:variable name="drawn-svgs" select="$drawn[svg]" />
  <xsl:variable name="loads" select="boolean($texts or $images or $drawn)" />
  <!-- Each picture once, however many objects show it. -->
  <xsl:key name="image-path" match="image/path" use="normalize-space(.)" />
  <xsl:variable name="image-paths" select="$images/sprite/image/path[generate-id() = generate-id(key('image-path', normalize-space(.))[count(ancestor::*[parent::objects] | $images) = count($images)][1])]" />

  <!-- Those that move on their own: a velocity that is not 0, 0 (a group's,
       or any of its members'). -->
  <xsl:variable name="moving" select="$things[(velocity | */velocity)[x/* or y/* or number(x) != 0 or number(y) != 0]]" />

  <!-- The groups whose members or cells each have a velocity of their own
       (a member, row, column or cell gives one), kept in a std::vector beside
       the shapes; the rest of the groups that move share one. -->
  <xsl:variable name="member-velocities" select="$groups[*/velocity]" />

  <!-- Those that never move (no velocity of their own, no keys): they never
       meet an edge, so their edge rules are left out, as the engine only looks
       at the edges for what is moving. -->
  <xsl:variable name="still" select="$things[count(. | $moving) != count($moving)][not(actions/action/*)]" />

  <!-- Groups that move as one block (<lockstep>): when one of them bounces off
       a side, all of them turn. -->
  <xsl:variable name="lockstep" select="$groups[normalize-space(collisions/lockstep) = 'true']" />

  <xsl:variable name="rules" select="$things/collisions[normalize-space(enabled) = 'true' or ../@class = 'projectile']/collision[not(@edge and count(ancestor::*[parent::objects] | $still) = count($still))]" />
  <xsl:variable name="edge-rules" select="$rules[@edge]" />
  <!-- Those that can be taken out of play by a <die />: each has a flag (one
       for each member of a group) saying it is still in play, looked at before
       it is moved, drawn, moved by a key or touched. -->
  <xsl:variable name="dying" select="$things[@name = $rules[die]/ancestor::*[parent::objects]/@name] | $projectiles" />

  <!-- Projectiles (class="projectile"): out of play until a <fire> launches
       one, from the middle of the shooter's top at its own velocity. Their
       collisions are on while they fly, whatever <enabled> says, as in the
       engine. -->
  <xsl:variable name="projectiles" select="$things[@class = 'projectile']" />
  <xsl:variable name="object-rules" select="$rules[not(@edge)]" />

  <!-- Those that go back to where they started after a <reset /> (its place
       and velocity, any <random> drawn anew, as in the engine). -->
  <xsl:variable name="resetting" select="$objects[collisions[normalize-space(enabled) = 'true' or ../@class = 'projectile']/collision/reset[not(@object)]]" />

  <!-- Those with something to do each frame: a move, or a rule written in
       their own update (one about another that comes first in the file and
       has rules about it too is written in the other's: Breakout's top row). -->
  <xsl:variable name="update-work">
    <xsl:for-each select="$things[count(. | $moving) = count($moving) or collisions/collision[*][count(. | $rules) = count($rules)]]">
      <work name="{@name}">
        <xsl:if test="count(. | $moving) = count($moving)">moves</xsl:if>
        <xsl:call-template name="update-rules" />
      </work>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="updating" select="$things[@name = exsl:node-set($update-work)/work[string(.) != '']/@name]" />

  <!-- What the keys do: held, a <trigger> of an action of <move>s, looked at
       every frame; pressed, everything else (a <hop> too), once for each press. -->
  <xsl:key name="action" match="object/actions/action" use="concat(../../@name, '|', @name)" />
  <xsl:variable name="held-inputs" select="$states/inputs/input[trigger[key('action', concat(@object, '|', @action))/move]]" />
  <xsl:variable name="pressed-inputs" select="$states/inputs/input[* and not(trigger[key('action', concat(@object, '|', @action))/move])]" />
  <xsl:variable name="hops" select="boolean($pressed-inputs/trigger[key('action', concat(@object, '|', @action))/hop])" />
  <xsl:variable name="fires" select="boolean($pressed-inputs/trigger[key('action', concat(@object, '|', @action))/fire])" />

  <xsl:variable name="sounds" select="/game/sounds/sound" />

  <!-- Every word in the parts of the game that are written out, between
       spaces, to see which names and functions it uses. -->
  <xsl:variable name="words">
    <xsl:text> </xsl:text>
    <xsl:for-each select="$game/window//text() | $game/variables//text() | $game/variables//@* | $things//text() | $things//@* | $states//text() | $states//@*">
      <xsl:value-of select="translate(., '+-*/(),&#9;&#10;&#13;', '          ')" />
      <xsl:text> </xsl:text>
    </xsl:for-each>
  </xsl:variable>

  <!-- The same, without the names of the game's own variables, to see which
       of them are used (an <svg>'s numbers are used when it is generated, not
       by the program). -->
  <xsl:variable name="values-words">
    <xsl:text> </xsl:text>
    <xsl:for-each select="$game/window//text() | $game/variables//text() | $game/sounds//text() | $things//text()[not(ancestor::svg)] | $things//@*[not(local-name() = 'name')] | $states//text() | $states//@*">
      <xsl:value-of select="translate(., '+-*/(),&#9;&#10;&#13;', '          ')" />
      <xsl:text> </xsl:text>
    </xsl:for-each>
  </xsl:variable>

  <!-- Whether an object's size is read (title.width), which physics::width gives. -->
  <xsl:variable name="sizes-read">
    <xsl:for-each select="$game/objects/object">
      <xsl:if test="contains($words, concat(' ', @name, '.width ')) or contains($words, concat(' ', @name, '.height '))">yes</xsl:if>
    </xsl:for-each>
  </xsl:variable>

  <!-- The modules it needs: physics for the edges, touches and sizes, sound for
       its sounds. -->
  <xsl:variable name="physics" select="boolean($rules/* or $sizes-read != '' or $hops or $fires)" />
  <xsl:variable name="audio" select="boolean($sounds)" />

  <!-- The helper functions the game needs (functions.xml). -->
  <xsl:variable name="used">
    <xsl:text> </xsl:text>
    <xsl:if test="$things//random"> randomBetween </xsl:if>
    <xsl:if test="contains($words, ' sgn ')"> sign </xsl:if>
  </xsl:variable>
  <xsl:variable name="used-functions" select="$helpers/function[contains($used, concat(' ', @name, ' '))]" />

  <!-- The standard headers it needs. -->
  <xsl:variable name="headers">
    <xsl:text> optional </xsl:text>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="concat(' ', @uses, ' ')" />
    </xsl:for-each>
    <xsl:if test="contains($words, ' min ') or contains($words, ' max ')"> algorithm </xsl:if>
    <xsl:if test="contains($words, ' abs ') or contains($words, ' floor ') or contains($words, ' ceil ') or contains($words, ' sqrt ') or contains($words, ' sin ') or contains($words, ' cos ') or contains($words, ' tan ') or contains($words, ' pow ') or contains($words, ' round ')"> cmath </xsl:if>
    <xsl:if test="$numbers or $drawn-rows"> string </xsl:if>
    <xsl:if test="$drawn-rows or $drawn-lines"> vector </xsl:if>
    <xsl:if test="$screens or $groups"> vector </xsl:if>
    <xsl:for-each select="$states/conditions/condition[remaining]">
      <xsl:if test="$dying[self::group][(current()/@object and @name = current()/@object) or (current()/@class and @class = current()/@class)]"> algorithm </xsl:if>
    </xsl:for-each>
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
    <xsl:call-template name="generate-screens" />
    <xsl:call-template name="generate-objects" />
    <xsl:call-template name="generate-sounds" />
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

  <!-- The modules beside it, then SFML, then the standard library. -->
  <xsl:template name="generate-includes">
    <xsl:text>
</xsl:text>
    <xsl:if test="$physics">#include "physics.h"&#10;</xsl:if>
    <xsl:if test="$drawn-rows or $drawn-lines">#include "pictures.h"&#10;</xsl:if>
    <xsl:if test="$audio">#include "sound.h"&#10;</xsl:if>
    <xsl:if test="$physics or $audio or $drawn-rows or $drawn-lines">
      <xsl:text>
</xsl:text>
    </xsl:if>
    <xsl:text>#include &lt;SFML/Graphics.hpp&gt;

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
    <header>string</header>
    <header>vector</header>
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
    <xsl:if test="$edge-rules/* or $hops">const sf::FloatRect windowArea({0.0f, 0.0f}, {windowWidth, windowHeight});&#10;</xsl:if>
    <xsl:if test="contains($words, ' pi ')">const float pi = 3.14159265f;&#10;</xsl:if>
    <xsl:text>const unsigned int framerate = </xsl:text>
    <xsl:value-of select="normalize-space($game/window/framerate)" />
    <xsl:text>;
const sf::Color background = </xsl:text>
    <xsl:call-template name="color"><xsl:with-param name="name" select="$game/window/background" /></xsl:call-template>
    <xsl:text>;
</xsl:text>
    <!-- the ones something uses (a name in a value, not its own name) -->
    <xsl:variable name="variables" select="$game/variables/variable[not(@name = following-sibling::variable/@name)][contains($values-words, concat(' ', @name, ' '))]" />
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

  <!-- The screens, one for each <state>, and the stack of them: the one on
       top is showing. -->
  <xsl:template name="generate-screens">
    <xsl:if test="$screens">
      <xsl:text>
// screens: the one on top of the stack is showing
enum class Screen { </xsl:text>
      <xsl:for-each select="$states">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text> };
std::vector&lt;Screen&gt; screens;
</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The font and pictures, then each object: its SFML shape, sprite or text,
       the velocity of one that moves, and its own variables. -->
  <xsl:template name="generate-objects">
    <xsl:text>
// objects</xsl:text>
    <xsl:if test="$texts">
      <xsl:text>
sf::Font font;</xsl:text>
    </xsl:if>
    <xsl:for-each select="$image-paths">
      <xsl:text>
sf::Texture </xsl:text>
      <xsl:call-template name="texture-name"><xsl:with-param name="path" select="." /></xsl:call-template>
      <xsl:text>;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$drawn">
      <xsl:call-template name="declare-picture" />
    </xsl:for-each>
    <xsl:if test="$loads">
      <xsl:text>
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$things">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:if test="position() &gt; 1">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:text>
</xsl:text>
      <xsl:choose>
        <xsl:when test="self::group">
          <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
          <xsl:variable name="cells"><xsl:call-template name="group-size" /></xsl:variable>
          <xsl:value-of select="concat('// ', @name, ': ', $cells, ' of them&#10;std::vector&lt;', $type, '&gt; ', $name, '(', $cells)" />
          <xsl:if test="$type = 'sf::Sprite'">
            <xsl:text>, sf::Sprite(</xsl:text>
            <xsl:for-each select="$group-data[@name = current()/@name]/look[1]/sprite"><xsl:call-template name="sprite-texture" /></xsl:for-each>
            <xsl:text>)</xsl:text>
          </xsl:if>
          <xsl:text>);</xsl:text>
          <xsl:choose>
            <xsl:when test="count(. | $member-velocities) = count($member-velocities)">
              <xsl:value-of select="concat('&#10;std::vector&lt;sf::Vector2f&gt; ', $name, 'Velocity;')" />
            </xsl:when>
            <xsl:when test="count(. | $moving) = count($moving)">
              <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Velocity; // every one of them')" />
            </xsl:when>
          </xsl:choose>
        </xsl:when>
        <xsl:when test="sprite/circle"><xsl:value-of select="concat('sf::CircleShape ', $name, ';')" /></xsl:when>
        <xsl:when test="sprite/rectangle"><xsl:value-of select="concat('sf::RectangleShape ', $name, ';')" /></xsl:when>
        <xsl:when test="sprite/text"><xsl:value-of select="concat('sf::Text ', $name, '(font);')" /></xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('sf::Sprite ', $name, '(')" />
          <xsl:for-each select="sprite[1]"><xsl:call-template name="sprite-texture" /></xsl:for-each>
          <xsl:text>);</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="count(. | $looked) = count($looked)">
        <xsl:call-template name="declare-looks" />
      </xsl:if>
      <xsl:if test="self::object and count(. | $moving) = count($moving)">
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Velocity;')" />
      </xsl:if>
      <xsl:if test="count(. | $dying) = count($dying)">
        <xsl:choose>
          <xsl:when test="self::group">
            <xsl:value-of select="concat('&#10;std::vector&lt;bool&gt; ', $name, 'Alive; // which of them are still in play')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:choose>
              <xsl:when test="@class = 'projectile'">
                <xsl:value-of select="concat('&#10;bool ', $name, 'Alive = false; // in play from when it is fired until it dies')" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="concat('&#10;bool ', $name, 'Alive = true; // in play until it dies')" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
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

  <xsl:template name="generate-sounds">
    <xsl:if test="$audio">
      <xsl:text>
// sounds
</xsl:text>
      <xsl:for-each select="$sounds">
        <xsl:text>sound::Sound </xsl:text>
        <xsl:call-template name="sound-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>;
</xsl:text>
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <xsl:template name="generate-declarations">
    <xsl:text>
// functions
</xsl:text>
    <xsl:choose>
      <xsl:when test="$loads">bool setup();&#10;</xsl:when>
      <xsl:otherwise>void setup();&#10;</xsl:otherwise>
    </xsl:choose>
    <xsl:text>void start();
</xsl:text>
    <xsl:if test="$pressed-inputs">void pressed(sf::Keyboard::Key key);&#10;</xsl:if>
    <xsl:for-each select="$states">
      <xsl:if test="count(. | $updating-states) = count($updating-states)">
        <xsl:text>void update</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:if>
    </xsl:for-each>
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
    <xsl:for-each select="$looked">
      <xsl:call-template name="become-signature" />
      <xsl:text>;
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$unless-classes">
      <xsl:call-template name="touching-signature" />
      <xsl:text>;
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$numbers">
      <xsl:text>void show</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="concat(normalize-space(declaration), '&#10;')" />
    </xsl:for-each>
  </xsl:template>

  <!-- The screens with something to do each frame: keys held, objects that
       move or have rules, or conditions. -->
  <xsl:variable name="updating-states" select="$states[inputs/input[count(. | $held-inputs) = count($held-inputs)] or shows/show/@object = $updating/@name or conditions/condition]" />

  <!-- ===================================================================== -->
  <!-- main                                                                   -->
  <!-- ===================================================================== -->

  <xsl:template name="generate-main">
    <xsl:text>
int main()
{
</xsl:text>
    <xsl:call-template name="generate-window" />
    <xsl:choose>
      <xsl:when test="$loads">
        <xsl:text>
	if (!setup())
	{
		return 1;
	}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>
	setup();
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
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
</xsl:text>
    <xsl:if test="$pressed-inputs">
      <xsl:text>			else if (const auto* key = event-&gt;getIf&lt;sf::Event::KeyPressed&gt;())
			{
				pressed(key-&gt;code);
			}
</xsl:text>
    </xsl:if>
    <xsl:text>		}
</xsl:text>
  </xsl:template>

  <!-- The showing screen's update: called straight out with one screen, the
       one that has one with several. -->
  <xsl:template name="generate-update">
    <xsl:choose>
      <xsl:when test="not($updating-states)" />
      <xsl:when test="not($screens)">
        <xsl:text>
		update</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$first/@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:when>
      <xsl:when test="count($updating-states) = 1">
        <xsl:text>
		if (screens.back() == Screen::</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$updating-states/@name" /></xsl:call-template>
        <xsl:text>)
		{
			update</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$updating-states/@name" /></xsl:call-template>
        <xsl:text>();
		}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>
		switch (screens.back())
		{
</xsl:text>
        <xsl:for-each select="$updating-states">
          <xsl:variable name="title"><xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
          <xsl:value-of select="concat('&#9;&#9;case Screen::', $title, ':&#10;&#9;&#9;&#9;update', $title, '();&#10;&#9;&#9;&#9;break;&#10;')" />
        </xsl:for-each>
        <xsl:text>		default:
			break;
		}
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="generate-render">
    <xsl:text>
		window.clear(background);
</xsl:text>
    <xsl:choose>
      <xsl:when test="$screens">
        <xsl:text>		switch (screens.back())
		{
</xsl:text>
        <xsl:for-each select="$states">
          <xsl:text>		case Screen::</xsl:text>
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>:
</xsl:text>
          <xsl:for-each select="shows/show">
            <xsl:call-template name="draw"><xsl:with-param name="indent" select="'&#9;&#9;&#9;'" /></xsl:call-template>
          </xsl:for-each>
          <xsl:text>			break;
</xsl:text>
        </xsl:for-each>
        <xsl:text>		}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="$first/shows/show">
          <xsl:call-template name="draw"><xsl:with-param name="indent" select="'&#9;&#9;'" /></xsl:call-template>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>		window.display();
</xsl:text>
  </xsl:template>

  <!-- A <show>: the object drawn, or every member of the group. -->
  <xsl:template name="draw">
    <xsl:param name="indent" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="$groups[@name = current()/@object]" />
    <xsl:variable name="dies" select="count($game/objects/*[@name = current()/@object] | $dying) = count($dying)" />
    <xsl:choose>
      <xsl:when test="$group and $dies">
        <xsl:value-of select="concat($indent, 'for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;if (', $name, 'Alive[i])&#10;', $indent, '&#9;{&#10;', $indent, '&#9;&#9;window.draw(', $name, '[i]);&#10;', $indent, '&#9;}&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:when test="$dies">
        <xsl:value-of select="concat($indent, 'if (', $name, 'Alive)&#10;', $indent, '{&#10;', $indent, '&#9;window.draw(', $name, ');&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:when test="$group">
        <xsl:variable name="type"><xsl:for-each select="$group"><xsl:call-template name="sf-type" /></xsl:for-each></xsl:variable>
        <xsl:value-of select="concat($indent, 'for (const ', $type, '&amp; one : ', $name, ')&#10;', $indent, '{&#10;', $indent, '&#9;window.draw(one);&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($indent, 'window.draw(', $name, ');&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- The functions                                                          -->
  <!-- ===================================================================== -->

  <xsl:template name="generate-definitions">
    <xsl:call-template name="generate-setup" />
    <xsl:call-template name="generate-start" />
    <xsl:if test="$pressed-inputs">
      <xsl:call-template name="generate-pressed" />
    </xsl:if>
    <xsl:for-each select="$updating-states">
      <xsl:call-template name="generate-screen-update" />
    </xsl:for-each>
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
      <xsl:call-template name="object-start">
        <xsl:with-param name="variables" select="variables/variable[.//random]" />
      </xsl:call-template>
      <xsl:text>}
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$looked">
      <xsl:call-template name="define-become" />
    </xsl:for-each>
    <xsl:for-each select="$unless-classes">
      <xsl:call-template name="define-touching" />
    </xsl:for-each>
    <xsl:for-each select="$numbers">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:text>
// </xsl:text>
      <xsl:value-of select="concat(@name, ': ', normalize-space(sprite/text/number), ' as it is now, put in its place by its new size')" />
      <xsl:text>
void show</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>()
{
</xsl:text>
      <xsl:value-of select="concat('&#9;', $name, '.setString(std::to_string(static_cast&lt;int&gt;(')" />
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="normalize-space(sprite/text/number)" /></xsl:call-template>
      <xsl:text>)));
</xsl:text>
      <xsl:value-of select="concat('&#9;', $name, '.setOrigin(', $name, '.getLocalBounds().position);&#10;')" />
      <xsl:value-of select="concat('&#9;', $name, '.setPosition(')" />
      <xsl:call-template name="vector"><xsl:with-param name="node" select="position" /></xsl:call-template>
      <xsl:text>);
}
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="definition" />
    </xsl:for-each>
  </xsl:template>

  <!-- Each object's look (its size and color, picture or text), and each sound
       made; then start(). With a font or a picture to load, false if one could
       not be. -->
  <xsl:template name="generate-setup">
    <xsl:choose>
      <xsl:when test="$loads">
        <xsl:text>
// The font and pictures, every object's look and every sound, then the game
// from the start; false if a file could not be loaded</xsl:text>
        <xsl:if test="$drawn-rows or $drawn-lines"> or a picture made</xsl:if>
        <xsl:text> (SFML says which).
bool setup()
{
	if (</xsl:text>
        <xsl:if test="$texts">!font.openFromFile("assets/tuffy.ttf")</xsl:if>
        <xsl:for-each select="$image-paths">
          <xsl:if test="$texts or position() &gt; 1"> || </xsl:if>
          <xsl:text>!</xsl:text>
          <xsl:call-template name="texture-name"><xsl:with-param name="path" select="." /></xsl:call-template>
          <xsl:text>.loadFromFile(</xsl:text>
          <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="normalize-space(.)" /></xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:for-each>
        <xsl:for-each select="$drawn">
          <xsl:if test="$texts or $image-paths or position() &gt; 1">
            <xsl:text>
		|| </xsl:text>
          </xsl:if>
          <xsl:call-template name="load-picture" />
        </xsl:for-each>
        <xsl:text>)
	{
		return false;
	}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>
// Every object's look</xsl:text>
        <xsl:if test="$audio"> and every sound</xsl:if>
        <xsl:text>, then the game from the start.
void setup()
{
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:for-each select="$things">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:if test="position() &gt; 1 or $loads">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="count(. | $looked) = count($looked)">
          <xsl:value-of select="concat('&#9;// ', @name, ': its first look, shown by start()&#10;')" />
        </xsl:when>
        <xsl:when test="self::group">
          <xsl:call-template name="group-looks"><xsl:with-param name="name" select="$name" /></xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:for-each select="sprite">
            <xsl:call-template name="set-look">
              <xsl:with-param name="name" select="$name" />
              <xsl:with-param name="indent" select="'&#9;'" />
            </xsl:call-template>
          </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:if test="$audio">
      <xsl:text>
</xsl:text>
      <xsl:for-each select="$sounds">
        <xsl:call-template name="make-sound" />
      </xsl:for-each>
    </xsl:if>
    <xsl:text>
	start();
</xsl:text>
    <xsl:if test="$loads">
      <xsl:text>	return true;
</xsl:text>
    </xsl:if>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- A <sprite>'s look on the SFML object `name`: its size and color, its
       text, or its picture. With a `base` (a group's look, already set), only
       what is not the same as it: a circle's or rectangle's size or color, or
       another picture. -->
  <xsl:template name="set-look">
    <xsl:param name="name" />
    <xsl:param name="indent" />
    <xsl:param name="base" select="/.." />
    <xsl:variable name="shape" select="*[1]" />
    <xsl:variable name="was" select="$base/*[1][local-name() = local-name($shape)]" />
    <xsl:choose>
      <xsl:when test="circle">
        <xsl:if test="not($was) or string($was/radius/@said) != string($shape/radius/@said)">
          <xsl:value-of select="concat($indent, $name, '.setRadius(')" />
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="circle/radius" /></xsl:call-template>
          <xsl:text>);
</xsl:text>
        </xsl:if>
        <xsl:if test="not($was) or string($was/color/@said) != string($shape/color/@said)">
          <xsl:value-of select="concat($indent, $name, '.setFillColor(')" />
          <xsl:call-template name="color"><xsl:with-param name="name" select="circle/color" /></xsl:call-template>
          <xsl:text>);
</xsl:text>
        </xsl:if>
      </xsl:when>
      <xsl:when test="rectangle">
        <xsl:if test="not($was) or string($was/width/@said) != string($shape/width/@said) or string($was/height/@said) != string($shape/height/@said)">
          <xsl:value-of select="concat($indent, $name, '.setSize({')" />
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="rectangle/width" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="rectangle/height" /></xsl:call-template>
          <xsl:text>});
</xsl:text>
        </xsl:if>
        <xsl:if test="not($was) or string($was/color/@said) != string($shape/color/@said)">
          <xsl:value-of select="concat($indent, $name, '.setFillColor(')" />
          <xsl:call-template name="color"><xsl:with-param name="name" select="rectangle/color" /></xsl:call-template>
          <xsl:text>);
</xsl:text>
        </xsl:if>
      </xsl:when>
      <xsl:when test="text">
        <xsl:if test="text/content">
          <xsl:value-of select="concat($indent, $name, '.setString(')" />
          <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="text/content" /></xsl:call-template>
          <xsl:text>);
</xsl:text>
        </xsl:if>
        <xsl:value-of select="concat($indent, $name, '.setCharacterSize(')" />
        <xsl:call-template name="whole-number"><xsl:with-param name="node" select="text/size" /></xsl:call-template>
        <xsl:text>);
</xsl:text>
        <xsl:value-of select="concat($indent, $name, '.setFillColor(')" />
        <xsl:call-template name="color"><xsl:with-param name="name" select="text/color" /></xsl:call-template>
        <xsl:text>);
</xsl:text>
        <xsl:if test="text/content">
          <xsl:value-of select="$indent" /><xsl:text>// the top left of the letters themselves where it is put, as the engine draws text
</xsl:text>
          <xsl:value-of select="concat($indent, $name, '.setOrigin(', $name, '.getLocalBounds().position);&#10;')" />
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="texture"><xsl:for-each select="ancestor-or-self::sprite"><xsl:call-template name="sprite-texture" /></xsl:for-each></xsl:variable>
        <xsl:variable name="flip" select="normalize-space((image | bitmap | svg)/flip)" />
        <xsl:value-of select="concat($indent, $name, '.setTexture(', $texture, ', true);&#10;')" />
        <xsl:choose>
          <xsl:when test="$flip = 'horizontal'">
            <xsl:value-of select="concat($indent, $name, '.setTextureRect({{static_cast&lt;int&gt;(', $texture, '.getSize().x), 0}, {-static_cast&lt;int&gt;(', $texture, '.getSize().x), static_cast&lt;int&gt;(', $texture, '.getSize().y)}}); // flipped left to right&#10;')" />
          </xsl:when>
          <xsl:when test="$flip = 'vertical'">
            <xsl:value-of select="concat($indent, $name, '.setTextureRect({{0, static_cast&lt;int&gt;(', $texture, '.getSize().y)}, {static_cast&lt;int&gt;(', $texture, '.getSize().x), -static_cast&lt;int&gt;(', $texture, '.getSize().y)}}); // flipped upside down&#10;')" />
          </xsl:when>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A sound made from its notes: wave, pitch, slide and length each. -->
  <xsl:template name="make-sound">
    <xsl:variable name="wave" select="@wave" />
    <xsl:text>	</xsl:text>
    <xsl:call-template name="sound-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>.make(</xsl:text>
    <xsl:choose>
      <xsl:when test="volume"><xsl:call-template name="value-bare"><xsl:with-param name="node" select="volume" /></xsl:call-template></xsl:when>
      <xsl:otherwise>0.3f</xsl:otherwise>
    </xsl:choose>
    <xsl:text>, {</xsl:text>
    <xsl:for-each select="note | rest">
      <xsl:if test="position() &gt; 1">,</xsl:if>
      <xsl:text>
		</xsl:text>
      <xsl:choose>
        <xsl:when test="self::rest">
          <xsl:text>sound::rest(</xsl:text>
          <xsl:call-template name="value-bare" />
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="noteWave">
            <xsl:choose>
              <xsl:when test="@wave"><xsl:value-of select="@wave" /></xsl:when>
              <xsl:otherwise><xsl:value-of select="$wave" /></xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:text>{sound::Wave::</xsl:text>
          <xsl:value-of select="concat(translate(substring($noteWave, 1, 1), $lower, $upper), substring($noteWave, 2))" />
          <xsl:text>, </xsl:text>
          <xsl:call-template name="pitch"><xsl:with-param name="name" select="@pitch" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:choose>
            <xsl:when test="@to"><xsl:call-template name="pitch"><xsl:with-param name="name" select="@to" /></xsl:call-template></xsl:when>
            <xsl:otherwise>0.0f</xsl:otherwise>
          </xsl:choose>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value-bare" />
          <xsl:text>}</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>
	});
</xsl:text>
  </xsl:template>

  <!-- A pitch: hertz as a number, or a note's name (A4) worked out by the module. -->
  <xsl:template name="pitch">
    <xsl:param name="name" />
    <xsl:choose>
      <xsl:when test="contains($digits, substring($name, 1, 1)) or starts-with($name, '.')">
        <xsl:call-template name="cpp-number"><xsl:with-param name="text" select="normalize-space($name)" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>sound::pitch(</xsl:text>
        <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="normalize-space($name)" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The game from the start: the first screen, every object's variables,
       place and velocity, and every number shown. -->
  <xsl:template name="generate-start">
    <xsl:text>
// The game from the start</xsl:text>
    <xsl:if test="$screens">: the first screen, and every object where it starts</xsl:if>
    <xsl:if test="not($screens)">: every object where it starts</xsl:if>
    <xsl:if test="$states//input/reset[not(@object)] or $states//condition/reset[not(@object)]">&#10;// (a &lt;reset /&gt; on a key or a condition does this too)</xsl:if>
    <xsl:text>.
void start()
{
</xsl:text>
    <xsl:if test="$screens">
      <xsl:text>	screens.assign(1, Screen::</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$first/@name" /></xsl:call-template>
      <xsl:text>);
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$things">
      <xsl:if test="position() &gt; 1 or $screens">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="self::group">
          <xsl:call-template name="group-start" />
        </xsl:when>
        <xsl:when test="count(. | $resetting) = count($resetting)">
          <xsl:for-each select="variables/variable[not(.//random)]">
            <xsl:sort select="@name" />
            <xsl:call-template name="set-variable" />
          </xsl:for-each>
          <xsl:text>	start</xsl:text>
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>();
</xsl:text>
        </xsl:when>
        <xsl:when test="count(. | $numbers) = count($numbers)">
          <xsl:for-each select="variables/variable">
            <xsl:sort select="@name" />
            <xsl:call-template name="set-variable" />
          </xsl:for-each>
          <xsl:text>	// put in its place by show</xsl:text>
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>(), below
</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="object-start">
            <xsl:with-param name="variables" select="variables/variable" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="self::object and count(. | $dying) = count($dying)">
        <xsl:text>	</xsl:text>
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:choose>
          <xsl:when test="@class = 'projectile'">Alive = false; // until it is fired
</xsl:when>
          <xsl:otherwise>Alive = true;
</xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:for-each>
    <xsl:if test="$numbers">
      <xsl:text>
	// the numbers shown, once every variable has its first value
</xsl:text>
      <xsl:for-each select="$numbers">
        <xsl:text>	show</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:for-each>
    </xsl:if>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- A group's start: where each member or cell is (cells: groups.xsl), and
       the velocity they share or each one's. What a member leaves out it takes
       from its group. -->
  <xsl:template name="group-start">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="." />
    <xsl:choose>
      <xsl:when test="columns">
        <xsl:call-template name="cells-start"><xsl:with-param name="name" select="$name" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="member">
          <xsl:value-of select="concat('&#9;', $name, '[', count(preceding-sibling::member), '].setPosition(')" />
          <xsl:call-template name="merged-vector">
            <xsl:with-param name="own" select="position" />
            <xsl:with-param name="shared" select="$group/position" />
          </xsl:call-template>
          <xsl:text>);
</xsl:text>
        </xsl:for-each>
        <xsl:choose>
          <xsl:when test="count(. | $member-velocities) = count($member-velocities)">
            <xsl:value-of select="concat('&#9;', $name, 'Velocity = {')" />
            <xsl:for-each select="member">
              <xsl:if test="position() &gt; 1">, </xsl:if>
              <xsl:call-template name="merged-vector">
                <xsl:with-param name="own" select="velocity" />
                <xsl:with-param name="shared" select="$group/velocity" />
              </xsl:call-template>
            </xsl:for-each>
            <xsl:text>};
</xsl:text>
          </xsl:when>
          <xsl:when test="count(. | $moving) = count($moving)">
            <xsl:value-of select="concat('&#9;', $name, 'Velocity = ')" />
            <xsl:call-template name="vector"><xsl:with-param name="node" select="velocity" /></xsl:call-template>
            <xsl:text>;
</xsl:text>
          </xsl:when>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="count(. | $looked) = count($looked)">
      <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;&#9;&#9;')" />
      <xsl:call-template name="become-call">
        <xsl:with-param name="thing" select="." />
        <xsl:with-param name="sprite" select="sprite[1]/@name" />
        <xsl:with-param name="index" select="'i'" />
      </xsl:call-template>
      <xsl:text>
	}
</xsl:text>
    </xsl:if>
    <xsl:if test="count(. | $dying) = count($dying)">
      <xsl:choose>
        <xsl:when test="@class = 'projectile'">
          <xsl:value-of select="concat('&#9;', $name, 'Alive.assign(', $name, '.size(), false); // until they are fired&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&#9;', $name, 'Alive.assign(', $name, '.size(), true);&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- From one cell to the next: its size, and the padding when there is one. -->
  <xsl:template name="grid-step">
    <xsl:param name="size" />
    <xsl:param name="padding" />
    <xsl:choose>
      <xsl:when test="not($padding) or (not($padding/*) and number($padding) = 0)">
        <xsl:value-of select="$size" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('(', $size, ' + ')" />
        <xsl:call-template name="value"><xsl:with-param name="node" select="$padding" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="set-variable">
    <xsl:text>	</xsl:text>
    <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="concat(ancestor::object/@name, '.', @name)" /></xsl:call-template>
    <xsl:text> = </xsl:text>
    <xsl:call-template name="value-bare" />
    <xsl:text>;
</xsl:text>
  </xsl:template>

  <!-- An object's start: the variables given (first, as the engine works them
       out first), its position and the velocity of one that moves. -->
  <xsl:template name="object-start">
    <xsl:param name="variables" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:for-each select="$variables">
      <xsl:sort select="@name" />
      <xsl:call-template name="set-variable" />
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
    <xsl:if test="count(. | $looked) = count($looked)">
      <xsl:text>	</xsl:text>
      <xsl:call-template name="become-call">
        <xsl:with-param name="thing" select="." />
        <xsl:with-param name="sprite" select="sprite[1]/@name" />
      </xsl:call-template>
      <xsl:text>
</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- What a key does when it is pressed, on the screen showing. -->
  <xsl:template name="generate-pressed">
    <xsl:text>
// A key pressed: what it does on the screen showing.
void pressed(sf::Keyboard::Key key)
{
</xsl:text>
    <xsl:choose>
      <xsl:when test="$screens">
        <xsl:text>	switch (screens.back())
	{
</xsl:text>
        <xsl:for-each select="$states[inputs/input[count(. | $pressed-inputs) = count($pressed-inputs)]]">
          <xsl:text>	case Screen::</xsl:text>
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>:
</xsl:text>
          <xsl:call-template name="pressed-keys"><xsl:with-param name="indent" select="'&#9;&#9;'" /></xsl:call-template>
          <xsl:text>		break;
</xsl:text>
        </xsl:for-each>
        <xsl:if test="$states[not(inputs/input[count(. | $pressed-inputs) = count($pressed-inputs)])]">
          <xsl:text>	default:
		break;
</xsl:text>
        </xsl:if>
        <xsl:text>	}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="$first">
          <xsl:call-template name="pressed-keys"><xsl:with-param name="indent" select="'&#9;'" /></xsl:call-template>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- One screen's keys, as an if for each, else-if after the first. -->
  <xsl:template name="pressed-keys">
    <xsl:param name="indent" />
    <xsl:for-each select="inputs/input[count(. | $pressed-inputs) = count($pressed-inputs)]">
      <xsl:value-of select="$indent" />
      <xsl:if test="position() &gt; 1">else </xsl:if>
      <xsl:text>if (key == sf::Keyboard::Key::</xsl:text>
      <xsl:value-of select="$tables/keys/key[@name = current()/@button]/@sfml" />
      <xsl:value-of select="concat(')&#10;', $indent, '{&#10;')" />
      <xsl:for-each select="*">
        <xsl:choose>
          <xsl:when test="self::trigger">
            <!-- an action of <hop>s: a step at once, if it stays in the window -->
            <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
            <xsl:variable name="dies" select="count($game/objects/object[@name = current()/@object] | $dying) = count($dying)" />
            <xsl:variable name="in" select="concat($indent, '&#9;', substring('&#9;', 1, number($dies)))" />
            <xsl:if test="$dies">
              <xsl:value-of select="concat($indent, '&#9;if (', $name, 'Alive)&#10;', $indent, '&#9;{&#10;')" />
            </xsl:if>
            <xsl:for-each select="key('action', concat(@object, '|', @action))/*">
              <xsl:choose>
                <xsl:when test="self::hop">
                  <xsl:value-of select="concat($in, 'physics::hop(', $name, ', ')" />
                  <xsl:call-template name="direction"><xsl:with-param name="node" select="." /></xsl:call-template>
                  <xsl:text>, windowArea);
</xsl:text>
                </xsl:when>
                <xsl:when test="self::fire">
                  <xsl:call-template name="fire">
                    <xsl:with-param name="shooter" select="$name" />
                    <xsl:with-param name="indent" select="$in" />
                  </xsl:call-template>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:call-template name="common-command"><xsl:with-param name="indent" select="$in" /></xsl:call-template>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
            <xsl:if test="$dies">
              <xsl:value-of select="concat($indent, '&#9;}&#10;')" />
            </xsl:if>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="game-command"><xsl:with-param name="indent" select="concat($indent, '&#9;')" /></xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:value-of select="concat($indent, '}&#10;')" />
    </xsl:for-each>
  </xsl:template>

  <!-- A <fire>: the projectile (the first of a group out of play) put at the
       middle of the shooter's top, its own middle over the shooter's, set off
       at its own velocity; nothing if it is already out. -->
  <xsl:template name="fire">
    <xsl:param name="shooter" />
    <xsl:param name="indent" />
    <xsl:variable name="projectile" select="$things[@name = current()/@object]" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$projectile/self::group">
        <xsl:variable name="velocity">
          <xsl:choose>
            <xsl:when test="count($projectile | $member-velocities) = count($member-velocities)"><xsl:value-of select="concat($name, 'Velocity[i]')" /></xsl:when>
            <xsl:otherwise><xsl:value-of select="concat($name, 'Velocity')" /></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat($indent, '// fire the first of ', @object, ' that is not out already&#10;')" />
        <xsl:value-of select="concat($indent, 'for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;if (!', $name, 'Alive[i])&#10;', $indent, '&#9;{&#10;')" />
        <xsl:call-template name="launch">
          <xsl:with-param name="shooter" select="$shooter" />
          <xsl:with-param name="name" select="concat($name, '[i]')" />
          <xsl:with-param name="velocity" select="$velocity" />
          <xsl:with-param name="alive" select="concat($name, 'Alive[i]')" />
          <xsl:with-param name="projectile" select="$projectile" />
          <xsl:with-param name="indent" select="concat($indent, '&#9;&#9;')" />
        </xsl:call-template>
        <xsl:value-of select="concat($indent, '&#9;&#9;break;&#10;', $indent, '&#9;}&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($indent, '// fire ', @object, ', if it is not out already&#10;')" />
        <xsl:value-of select="concat($indent, 'if (!', $name, 'Alive)&#10;', $indent, '{&#10;')" />
        <xsl:call-template name="launch">
          <xsl:with-param name="shooter" select="$shooter" />
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="velocity" select="concat($name, 'Velocity')" />
          <xsl:with-param name="alive" select="concat($name, 'Alive')" />
          <xsl:with-param name="projectile" select="$projectile" />
          <xsl:with-param name="indent" select="concat($indent, '&#9;')" />
        </xsl:call-template>
        <xsl:value-of select="concat($indent, '}&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="launch">
    <xsl:param name="shooter" />
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="alive" />
    <xsl:param name="projectile" />
    <xsl:param name="indent" />
    <xsl:value-of select="concat($indent, $name, '.setPosition({physics::left(', $shooter, ') + physics::width(', $shooter, ') / 2.0f - physics::width(', $name, ') / 2.0f, physics::top(', $shooter, ')});&#10;')" />
    <xsl:if test="count($projectile | $moving) = count($moving)">
      <xsl:value-of select="concat($indent, $velocity, ' = ')" />
      <xsl:call-template name="vector"><xsl:with-param name="node" select="$projectile/velocity" /></xsl:call-template>
      <xsl:text>;
</xsl:text>
    </xsl:if>
    <xsl:value-of select="concat($indent, $alive, ' = true;&#10;')" />
  </xsl:template>

  <!-- A command about the game rather than one object (on a key, or in a
       condition): screens, the whole game again, a sound, a variable. -->
  <xsl:template name="game-command">
    <xsl:param name="indent" />
    <xsl:choose>
      <xsl:when test="self::push">
        <xsl:value-of select="concat($indent, 'screens.push_back(Screen::')" />
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@state" /></xsl:call-template>
        <xsl:text>);
</xsl:text>
      </xsl:when>
      <xsl:when test="self::pop and @state">
        <xsl:value-of select="concat($indent, 'screens.back() = Screen::')" />
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@state" /></xsl:call-template>
        <xsl:text>;
</xsl:text>
      </xsl:when>
      <xsl:when test="self::pop">
        <xsl:value-of select="concat($indent, 'if (screens.size() &gt; 1)&#10;', $indent, '{&#10;', $indent, '&#9;screens.pop_back();&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:when test="self::reset">
        <xsl:value-of select="concat($indent, 'start();&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="common-command"><xsl:with-param name="indent" select="$indent" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What can run anywhere: a sound, a variable up or down (and the numbers
       shown from it). -->
  <xsl:template name="common-command">
    <xsl:param name="indent" />
    <xsl:param name="self" select="/.." />
    <xsl:param name="index" select="''" />
    <xsl:choose>
      <xsl:when test="self::become">
        <xsl:call-template name="become-command">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="index" select="$index" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::play">
        <xsl:value-of select="$indent" />
        <xsl:call-template name="sound-name"><xsl:with-param name="name" select="@sound" /></xsl:call-template>
        <xsl:text>.play();
</xsl:text>
      </xsl:when>
      <xsl:when test="self::inc or self::dec">
        <xsl:variable name="variable" select="normalize-space(@variable)" />
        <xsl:value-of select="$indent" />
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$variable" /></xsl:call-template>
        <xsl:choose>
          <xsl:when test="self::inc"> += </xsl:when>
          <xsl:otherwise> -= </xsl:otherwise>
        </xsl:choose>
        <xsl:choose>
          <xsl:when test="* or normalize-space(.) != ''"><xsl:call-template name="value-bare" /></xsl:when>
          <xsl:otherwise>1.0f</xsl:otherwise>
        </xsl:choose>
        <xsl:text>;
</xsl:text>
        <xsl:for-each select="$numbers[normalize-space(sprite/text/number) = $variable]">
          <xsl:value-of select="concat($indent, 'show')" />
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>();
</xsl:text>
        </xsl:for-each>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- A screen's frame: the keys held, the objects it shows that do something,
       then its conditions. -->
  <xsl:template name="generate-screen-update">
    <xsl:variable name="state" select="." />
    <xsl:variable name="blocks">
      <xsl:for-each select="inputs/input[count(. | $held-inputs) = count($held-inputs)]">
        <xsl:text>
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::</xsl:text>
        <xsl:value-of select="$tables/keys/key[@name = current()/@button]/@sfml" />
        <xsl:text>))
	{
</xsl:text>
        <xsl:for-each select="trigger">
          <xsl:variable name="object" select="/game/objects/object[@name = current()/@object]" />
          <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
          <xsl:variable name="dies" select="count($object | $dying) = count($dying)" />
          <xsl:if test="$dies">
            <xsl:value-of select="concat('&#9;&#9;if (', $name, 'Alive)&#10;&#9;&#9;{&#10;')" />
          </xsl:if>
          <xsl:for-each select="$object/actions/action[@name = current()/@action]/move">
            <xsl:value-of select="concat('&#9;&#9;', substring('&#9;', 1, number($dies)), $name, '.move(')" />
            <xsl:call-template name="direction"><xsl:with-param name="node" select="." /></xsl:call-template>
            <xsl:text>);
</xsl:text>
          </xsl:for-each>
          <xsl:if test="$dies">
            <xsl:text>		}
</xsl:text>
          </xsl:if>
        </xsl:for-each>
        <xsl:text>	}
</xsl:text>
      </xsl:for-each>
      <xsl:variable name="shown" select="$updating[@name = $state/shows/show/@object]" />
      <xsl:if test="$shown">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$shown">
        <xsl:text>	update</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:for-each>
      <xsl:for-each select="conditions/condition">
        <xsl:call-template name="condition" />
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

  <!-- A condition: when any object it is about has reached the number, its
       commands, and nothing more this frame (the screen may have changed). -->
  <xsl:template name="condition">
    <xsl:choose>
      <xsl:when test="remaining"><xsl:call-template name="remaining-condition" /></xsl:when>
      <xsl:otherwise><xsl:call-template name="variable-condition" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A condition on how many are left: those it is about that are still in
       play (all of them, for what cannot die), no more than the number. -->
  <xsl:template name="remaining-condition">
    <xsl:variable name="condition" select="." />
    <xsl:variable name="about" select="$things[($condition/@object and @name = $condition/@object) or ($condition/@class and @class = $condition/@class)]" />
    <xsl:variable name="limit" select="normalize-space(remaining)" />
    <!-- what cannot die is always there: a number -->
    <xsl:variable name="always">
      <xsl:call-template name="groups-size"><xsl:with-param name="groups" select="$about[self::group][count(. | $dying) != count($dying)]" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="fixed" select="$always + count($about[self::object][count(. | $dying) != count($dying)])" />
    <xsl:variable name="test">
      <xsl:choose>
        <!-- objects alone, none left: none of them in play -->
        <xsl:when test="$limit = '0' and $fixed = 0 and not($about[self::group])">
          <xsl:for-each select="$about">
            <xsl:if test="position() &gt; 1"> &amp;&amp; </xsl:if>
            <xsl:text>!</xsl:text>
            <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
            <xsl:text>Alive</xsl:text>
          </xsl:for-each>
        </xsl:when>
        <xsl:otherwise>
          <xsl:for-each select="$about[count(. | $dying) = count($dying)]">
            <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
            <xsl:if test="position() &gt; 1"> + </xsl:if>
            <xsl:choose>
              <xsl:when test="self::group">
                <xsl:value-of select="concat('std::count(', $name, 'Alive.begin(), ', $name, 'Alive.end(), true)')" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="concat('(', $name, 'Alive ? 1 : 0)')" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:for-each>
          <xsl:if test="$fixed &gt; 0 or not($about[count(. | $dying) = count($dying)])">
            <xsl:if test="$about[count(. | $dying) = count($dying)]"> + </xsl:if>
            <xsl:value-of select="$fixed" />
          </xsl:if>
          <xsl:choose>
            <xsl:when test="$limit = '0'"> == 0</xsl:when>
            <xsl:otherwise><xsl:value-of select="concat(' &lt;= ', $limit)" /></xsl:otherwise>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:text>
	// </xsl:text>
    <xsl:value-of select="concat(@object, @class, ': ')" />
    <xsl:choose>
      <xsl:when test="$limit = '0'">none left</xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('no more than ', $limit, ' left')" /></xsl:otherwise>
    </xsl:choose>
    <xsl:text>
	if (</xsl:text>
    <xsl:value-of select="$test" />
    <xsl:text>)
	{
</xsl:text>
    <xsl:for-each select="*[not(self::remaining)]">
      <xsl:call-template name="game-command"><xsl:with-param name="indent" select="'&#9;&#9;'" /></xsl:call-template>
    </xsl:for-each>
    <xsl:text>		return;
	}
</xsl:text>
  </xsl:template>

  <xsl:template name="variable-condition">
    <xsl:variable name="condition" select="." />
    <xsl:variable name="about" select="$game/objects/object[($condition/@object and @name = $condition/@object) or ($condition/@class and @class = $condition/@class)]
                                                           [variables/variable/@name = $condition/@variable]" />
    <xsl:variable name="test">
      <xsl:choose>
        <xsl:when test="atleast"> &gt;= </xsl:when>
        <xsl:otherwise> &lt;= </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="threshold">
      <xsl:call-template name="value-bare"><xsl:with-param name="node" select="atleast | atmost" /></xsl:call-template>
    </xsl:variable>
    <xsl:text>
	if (</xsl:text>
    <xsl:for-each select="$about">
      <xsl:if test="position() &gt; 1"> || </xsl:if>
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="concat(@name, '.', $condition/@variable)" /></xsl:call-template>
      <xsl:value-of select="concat($test, $threshold)" />
    </xsl:for-each>
    <xsl:text>)
	{
</xsl:text>
    <xsl:for-each select="*[not(self::atleast or self::atmost)]">
      <xsl:call-template name="game-command"><xsl:with-param name="indent" select="'&#9;&#9;'" /></xsl:call-template>
    </xsl:for-each>
    <xsl:text>		return;
	}
</xsl:text>
  </xsl:template>

  <!-- An object's frame: its own move, then its collision rules in the order
       written. A group's: the same for each of its members, in a loop. Blocks
       are written each with a line break before it, and the first one's taken
       off. -->
  <xsl:template name="generate-object-update">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="boolean(self::group)" />
    <xsl:variable name="each-own" select="count(. | $member-velocities) = count($member-velocities)" />
    <xsl:variable name="dies" select="count(. | $dying) = count($dying)" />
    <!-- a block that moves as one: all of it moved first, then its rules, so
         that when one of them turns the block the rest are in step -->
    <xsl:variable name="block" select="count(. | $lockstep) = count($lockstep) and count(. | $moving) = count($moving)" />
    <!-- a group's members counted through when each has a velocity or a flag
         of its own, gone through one by one otherwise -->
    <xsl:variable name="counted" select="$each-own or ($group and ($dies or count(. | $looked) = count($looked)))" />
    <!-- what the rules are written about: the object, or one member -->
    <xsl:variable name="one">
      <xsl:choose>
        <xsl:when test="$counted"><xsl:value-of select="concat($name, '[i]')" /></xsl:when>
        <xsl:when test="$group">one</xsl:when>
        <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="velocity">
      <xsl:value-of select="concat($name, 'Velocity')" />
      <xsl:if test="$each-own">[i]</xsl:if>
    </xsl:variable>
    <xsl:variable name="indent">
      <xsl:text>&#9;</xsl:text>
      <xsl:if test="$group"><xsl:text>&#9;</xsl:text></xsl:if>
    </xsl:variable>
    <!-- whether it is still in play, and how to leave it be when it is not -->
    <xsl:variable name="alive">
      <xsl:if test="$dies">
        <xsl:value-of select="concat($name, 'Alive')" />
        <xsl:if test="$group">[i]</xsl:if>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="out">
      <xsl:choose>
        <xsl:when test="$group">continue</xsl:when>
        <xsl:otherwise>return</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="blocks">
      <xsl:if test="$dies">
        <xsl:value-of select="concat('&#10;', $indent, 'if (!', $alive, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $out, ';&#10;', $indent, '}&#10;')" />
      </xsl:if>
      <xsl:if test="count(. | $moving) = count($moving) and not($block)">
        <xsl:value-of select="concat('&#10;', $indent, $one, '.move(', $velocity, ');&#10;')" />
      </xsl:if>
      <xsl:call-template name="update-rules">
        <xsl:with-param name="name" select="$one" />
        <xsl:with-param name="velocity" select="$velocity" />
        <xsl:with-param name="indent" select="$indent" />
        <xsl:with-param name="alive" select="$alive" />
        <xsl:with-param name="out" select="$out" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:if test="$group">, every one of them</xsl:if>
    <xsl:text>, each frame
void update</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>()
{
</xsl:text>
    <xsl:if test="$block">
      <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
      <xsl:value-of select="concat('&#9;for (', $type, '&amp; one : ', $name, ')&#10;&#9;{&#10;&#9;&#9;one.move(', $name, 'Velocity);&#10;&#9;}&#10;')" />
      <xsl:if test="string($blocks) != ''"><xsl:text>&#10;</xsl:text></xsl:if>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="string($blocks) = ''" />
      <xsl:when test="$counted">
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;')" />
        <xsl:value-of select="substring($blocks, 2)" />
        <xsl:text>	}
</xsl:text>
      </xsl:when>
      <xsl:when test="$group">
        <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
        <xsl:value-of select="concat('&#9;for (', $type, '&amp; one : ', $name, ')&#10;&#9;{&#10;')" />
        <xsl:value-of select="substring($blocks, 2)" />
        <xsl:text>	}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring($blocks, 2)" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- An object's or group's collision rules in the order written: `name`
       the object or one member. -->
  <xsl:template name="update-rules">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="indent" />
    <xsl:param name="alive" />
    <xsl:param name="out" />
    <xsl:for-each select="collisions[normalize-space(enabled) = 'true' or ../@class = 'projectile']/collision[*][count(. | $rules) = count($rules)]">
      <xsl:choose>
        <xsl:when test="@edge">
          <xsl:call-template name="edge-rule">
            <xsl:with-param name="name" select="$name" />
            <xsl:with-param name="velocity" select="$velocity" />
            <xsl:with-param name="indent" select="$indent" />
            <xsl:with-param name="alive" select="$alive" />
            <xsl:with-param name="out" select="$out" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="object-rule">
            <xsl:with-param name="name" select="$name" />
            <xsl:with-param name="velocity" select="$velocity" />
            <xsl:with-param name="indent" select="$indent" />
            <xsl:with-param name="alive" select="$alive" />
            <xsl:with-param name="out" select="$out" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <!-- A screen-edge rule: an if statement for each edge it is about, with
       every rule about that edge in it, in the order written, as the engine
       runs them all for one touch of the edge (Breakout's ball bounces off
       every edge, and dies at the bottom); written at the first rule about it.
       A <wrap /> needs no if: it looks for itself whether the thing has gone
       right off. -->
  <xsl:template name="edge-rule">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="indent" />
    <xsl:param name="alive" />
    <xsl:param name="out" />
    <xsl:variable name="edge" select="@edge" />
    <xsl:variable name="rule" select="." />
    <!-- the last rule: nothing after it to leave out once it has died -->
    <xsl:variable name="last" select="not(following-sibling::collision[*])" />
    <xsl:variable name="self" select="ancestor::*[parent::objects]" />
    <xsl:variable name="moves" select="count($self | $moving) = count($moving)" />
    <xsl:variable name="block" select="count($self | $lockstep) = count($lockstep)" />
    <xsl:variable name="others" select="../collision[*][@edge][not(wrap)][count(. | $rules) = count($rules)]" />
    <xsl:for-each select="document('')//xsl:variable[@name = 'edges']/edge[@name = $edge or @in = $edge or $edge = 'all']">
      <xsl:variable name="side" select="concat('physics::Edge::', @title)" />
      <xsl:variable name="this" select="." />
      <xsl:variable name="about" select="$others[@edge = $this/@name or @edge = $this/@in or @edge = 'all']" />
      <xsl:choose>
        <xsl:when test="$rule/wrap and ($rule/@sprite or $rule/@unless)">
          <xsl:value-of select="concat('&#10;', $indent, '// ', @name, ': wrap')" />
          <xsl:if test="$rule/@sprite"><xsl:value-of select="concat(', while it shows ', $rule/@sprite)" /></xsl:if>
          <xsl:if test="$rule/@unless"><xsl:value-of select="concat(', unless on ', $rule/@unless)" /></xsl:if>
          <xsl:value-of select="concat('&#10;', $indent, 'if (')" />
          <xsl:call-template name="rule-guard">
            <xsl:with-param name="rule" select="$rule" />
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="name" select="$name" />
          </xsl:call-template>
          <xsl:value-of select="concat(')&#10;', $indent, '{&#10;', $indent, '&#9;physics::wrap(', $name, ', ', $velocity, ', ', $side, ', windowArea);&#10;', $indent, '}&#10;')" />
        </xsl:when>
        <xsl:when test="$rule/wrap">
          <xsl:value-of select="concat('&#10;', $indent, '// ', @name, ': wrap&#10;', $indent, 'physics::wrap(', $name, ', ', $velocity, ', ', $side, ', windowArea);&#10;')" />
        </xsl:when>
        <xsl:when test="generate-id($about[1]) = generate-id($rule)">
          <xsl:value-of select="concat('&#10;', $indent, '// ', @name, ':')" />
          <xsl:for-each select="$about/*">
            <xsl:value-of select="concat(' ', local-name())" />
          </xsl:for-each>
          <xsl:value-of select="concat('&#10;', $indent, 'if (physics::past(', $name, ', ', $side, ', windowArea))&#10;', $indent, '{&#10;')" />
          <xsl:for-each select="$about">
            <!-- a rule with sprite= or unless= only while it holds, looked
                 at as the rule comes, as the engine does (guards.xsl) -->
            <xsl:variable name="guard">
              <xsl:call-template name="rule-guard">
                <xsl:with-param name="rule" select="." />
                <xsl:with-param name="self" select="$self" />
                <xsl:with-param name="name" select="$name" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:variable name="at" select="concat($indent, '&#9;', substring('&#9;', 1, number($guard != '')))" />
            <xsl:if test="$guard != ''">
              <xsl:value-of select="concat($indent, '&#9;if (', $guard, ')&#10;', $indent, '&#9;{&#10;')" />
            </xsl:if>
            <xsl:for-each select="*">
              <xsl:choose>
                <xsl:when test="self::reset">
                  <xsl:value-of select="concat($at, 'start')" />
                  <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template>
                  <xsl:text>();
</xsl:text>
                </xsl:when>
                <xsl:when test="self::bounce and $block">
                  <xsl:variable name="group"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template></xsl:variable>
                  <xsl:variable name="type"><xsl:for-each select="$self"><xsl:call-template name="sf-type" /></xsl:for-each></xsl:variable>
                  <xsl:value-of select="concat($at, '// the whole block turns, and every one of them steps back&#10;')" />
                  <xsl:value-of select="concat($at, $group, 'Velocity.x = -', $group, 'Velocity.x;&#10;')" />
                  <xsl:value-of select="concat($at, 'for (', $type, '&amp; each : ', $group, ')&#10;', $at, '{&#10;')" />
                  <xsl:value-of select="concat($at, '&#9;each.move({', $group, 'Velocity.x, 0.0f});&#10;', $at, '}&#10;')" />
                </xsl:when>
                <xsl:when test="self::bounce">
                  <xsl:value-of select="concat($at, 'physics::bounce(', $name, ', ', $velocity, ', ', $side, ', windowArea);&#10;')" />
                </xsl:when>
                <xsl:when test="self::stick and $moves">
                  <xsl:value-of select="concat($at, 'physics::stick(', $name, ', ', $velocity, ', ', $side, ', windowArea);&#10;')" />
                </xsl:when>
                <xsl:when test="self::stick">
                  <xsl:value-of select="concat($at, 'physics::stick(', $name, ', ', $side, ', windowArea);&#10;')" />
                </xsl:when>
                <xsl:when test="self::die">
                  <xsl:value-of select="concat($at, $alive, ' = false;&#10;')" />
                </xsl:when>
                <xsl:otherwise>
                  <xsl:call-template name="common-command">
                    <xsl:with-param name="indent" select="$at" />
                    <xsl:with-param name="self" select="$self" />
                    <xsl:with-param name="index"><xsl:call-template name="index-of"><xsl:with-param name="shape" select="$name" /></xsl:call-template></xsl:with-param>
                  </xsl:call-template>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
            <xsl:if test="$guard != ''">
              <xsl:value-of select="concat($indent, '&#9;}&#10;')" />
            </xsl:if>
          </xsl:for-each>
          <xsl:choose>
            <xsl:when test="not($about/die) or (position() = last() and $last)" />
            <!-- a die held back by a guard may not have happened -->
            <xsl:when test="$about[@sprite or @unless]/die">
              <xsl:value-of select="concat($indent, '&#9;if (!', $alive, ')&#10;', $indent, '&#9;{&#10;', $indent, '&#9;&#9;', $out, ';&#10;', $indent, '&#9;}&#10;')" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat($indent, '&#9;', $out, ';&#10;')" />
            </xsl:otherwise>
          </xsl:choose>
          <xsl:value-of select="concat($indent, '}&#10;')" />
        </xsl:when>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <!-- The four edges, and the edge="..." words that take in each. -->
  <xsl:variable name="edges">
    <edge name="top" title="Top" in="vertical" />
    <edge name="bottom" title="Bottom" in="vertical" />
    <edge name="left" title="Left" in="horizontal" />
    <edge name="right" title="Right" in="horizontal" />
  </xsl:variable>

  <!-- A rule about other objects: an if statement for each it can touch (that
       is in play, and shown on a screen with it); for a group, in a loop over
       its members. When the other has rules about this one too, both happen in
       one touch, as in the engine, written where the one first in the file is
       updated: the other's first, before this one's bounce can take it out of
       the touch or its die out of play. -->
  <xsl:template name="object-rule">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="indent" />
    <xsl:param name="alive" />
    <xsl:param name="out" />
    <xsl:variable name="rule" select="." />
    <!-- the last rule: nothing after it to leave out once it has died -->
    <xsl:variable name="last" select="not(following-sibling::collision[*])" />
    <xsl:variable name="self" select="ancestor::*[parent::objects]" />
    <xsl:variable name="together" select="$states[shows/show/@object = $self/@name]/shows/show/@object" />
    <xsl:variable name="before" select="$self/preceding-sibling::*" />
    <xsl:for-each select="$things[collisions[normalize-space(enabled) = 'true' or ../@class = 'projectile']]
                                 [count(. | $self) != 1]
                                 [@name = $together]
                                 [not($rule/@object) or @name = $rule/@object]
                                 [not($rule/@class) or @class = $rule/@class]">
      <xsl:variable name="thing" select="." />
      <!-- the other's rules about this one -->
      <xsl:variable name="back" select="collisions[normalize-space(enabled) = 'true' or ../@class = 'projectile']/collision[*][not(@edge)]
                                                  [not(@object) or @object = $self/@name]
                                                  [not(@class) or @class = $self/@class]" />
      <!-- written with the other's, when the other comes first in the file -->
      <xsl:if test="not($back and count(. | $before) = count($before))">
        <xsl:call-template name="touch">
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="velocity" select="$velocity" />
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="alive" select="$alive" />
          <xsl:with-param name="out" select="$out" />
          <xsl:with-param name="rule" select="$rule" />
          <xsl:with-param name="self" select="$self" />
          <!-- the other's rules, once: with this one's first rule about it -->
          <xsl:with-param name="back" select="$back[not($rule/preceding-sibling::collision[*][not(@edge)]
                                                         [not(@object) or @object = $thing/@name]
                                                         [not(@class) or @class = $thing/@class])]" />
          <xsl:with-param name="final" select="position() = last() and $last" />
        </xsl:call-template>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- One touch of a rule about other objects, the context node the other. -->
  <xsl:template name="touch">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="indent" />
    <xsl:param name="alive" />
    <xsl:param name="out" />
    <xsl:param name="rule" />
    <xsl:param name="self" />
    <xsl:param name="back" />
    <xsl:param name="final" />
      <xsl:variable name="other-thing" select="." />
      <xsl:variable name="other-name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:variable name="in" select="concat($indent, substring('&#9;', 1, number(boolean(self::group))))" />
      <!-- the other one, if it can die, only while it is in play -->
      <xsl:variable name="other-dies" select="count(. | $dying) = count($dying)" />
      <!-- a group's members counted through when each has a flag or a look of its own -->
      <xsl:variable name="other-counted" select="$other-dies or count(. | $looked) = count($looked)" />
      <xsl:variable name="other">
        <xsl:choose>
          <xsl:when test="self::group and $other-counted"><xsl:value-of select="concat($other-name, '[j]')" /></xsl:when>
          <xsl:when test="self::group">other</xsl:when>
          <xsl:otherwise><xsl:value-of select="$other-name" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="other-alive">
        <xsl:if test="$other-dies">
          <xsl:value-of select="concat($other-name, 'Alive')" />
          <xsl:if test="self::group">[j]</xsl:if>
          <xsl:text> &amp;&amp; </xsl:text>
        </xsl:if>
      </xsl:variable>
      <!-- out of the loop over the other group's members when this one dies
           in it (a member of a group: then on to the next member) -->
      <xsl:variable name="leave">
        <xsl:choose>
          <xsl:when test="self::group and $out = 'continue'">break</xsl:when>
          <xsl:otherwise><xsl:value-of select="$out" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:value-of select="concat('&#10;', $indent, '// ', @name, ':')" />
      <xsl:for-each select="$rule/*">
        <xsl:value-of select="concat(' ', local-name())" />
      </xsl:for-each>
      <xsl:text>&#10;</xsl:text>
      <xsl:choose>
        <xsl:when test="self::group and $other-counted">
          <xsl:value-of select="concat($indent, 'for (std::size_t j = 0; j &lt; ', $other-name, '.size(); ++j)&#10;', $indent, '{&#10;')" />
        </xsl:when>
        <xsl:when test="self::group">
          <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
          <xsl:value-of select="concat($indent, 'for (const ', $type, '&amp; other : ', $other-name, ')&#10;', $indent, '{&#10;')" />
        </xsl:when>
      </xsl:choose>
      <!-- a rule with sprite= or unless= only while it holds (guards.xsl) -->
      <xsl:variable name="own-guard">
        <xsl:call-template name="rule-guard">
          <xsl:with-param name="rule" select="$rule" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="other" select="$other" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat($in, 'if (', $other-alive, 'physics::touching(', $name, ', ', $other, ')', substring(' &amp;&amp; ', 1, 4 * number($own-guard != '')), $own-guard, ')&#10;', $in, '{&#10;')" />
      <xsl:if test="$back">
        <!-- the other's own rules about this one, it being the one that moves or dies -->
        <xsl:variable name="other-velocity">
          <xsl:value-of select="concat($other-name, 'Velocity')" />
          <xsl:if test="count(. | $member-velocities) = count($member-velocities)">[j]</xsl:if>
        </xsl:variable>
        <xsl:variable name="other-alive-name">
          <xsl:if test="$other-dies">
            <xsl:value-of select="concat($other-name, 'Alive')" />
            <xsl:if test="self::group">[j]</xsl:if>
          </xsl:if>
        </xsl:variable>
        <xsl:value-of select="concat($in, '&#9;// ', @name, ', by its own rule:')" />
        <xsl:for-each select="$back/*">
          <xsl:value-of select="concat(' ', local-name())" />
        </xsl:for-each>
        <xsl:text>&#10;</xsl:text>
        <xsl:for-each select="$back">
          <!-- a rule with sprite= or unless= only while it holds for the
               other, looked at as each rule comes, as the engine does -->
          <xsl:variable name="guard">
            <xsl:call-template name="rule-guard">
              <xsl:with-param name="rule" select="." />
              <xsl:with-param name="self" select="$other-thing" />
              <xsl:with-param name="name" select="$other" />
              <xsl:with-param name="other" select="$name" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="at" select="concat($in, '&#9;', substring('&#9;', 1, number($guard != '')))" />
          <xsl:if test="$guard != ''">
            <xsl:value-of select="concat($in, '&#9;if (', $guard, ')&#10;', $in, '&#9;{&#10;')" />
          </xsl:if>
          <xsl:for-each select="*">
            <xsl:call-template name="touch-command">
              <xsl:with-param name="name" select="$other" />
              <xsl:with-param name="velocity" select="$other-velocity" />
              <xsl:with-param name="other" select="$name" />
              <xsl:with-param name="alive" select="$other-alive-name" />
              <xsl:with-param name="self" select="$other-thing" />
              <xsl:with-param name="indent" select="$at" />
            </xsl:call-template>
          </xsl:for-each>
          <xsl:if test="$guard != ''">
            <xsl:value-of select="concat($in, '&#9;}&#10;')" />
          </xsl:if>
        </xsl:for-each>
      </xsl:if>
      <xsl:for-each select="$rule/*">
        <xsl:call-template name="touch-command">
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="velocity" select="$velocity" />
          <xsl:with-param name="other" select="$other" />
          <xsl:with-param name="alive" select="$alive" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="indent" select="concat($in, '&#9;')" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:if test="$rule/die and not($final and not(self::group))">
        <xsl:value-of select="concat($in, '&#9;', $leave, ';&#10;')" />
      </xsl:if>
      <xsl:value-of select="concat($in, '}&#10;')" />
      <xsl:if test="self::group">
        <xsl:value-of select="concat($indent, '}&#10;')" />
        <xsl:if test="$rule/die and $leave = 'break' and not($final)">
          <xsl:value-of select="concat($indent, 'if (!', $alive, ')&#10;', $indent, '{&#10;', $indent, '&#9;continue;&#10;', $indent, '}&#10;')" />
        </xsl:if>
      </xsl:if>
  </xsl:template>

  <!-- What one does when it touches another: `name` (its velocity, its flag
       for being in play) against `other`; `self` is the object or group. -->
  <xsl:template name="touch-command">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="other" />
    <xsl:param name="alive" />
    <xsl:param name="self" />
    <xsl:param name="indent" />
    <xsl:choose>
      <xsl:when test="self::reset">
        <xsl:value-of select="concat($indent, 'start')" />
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:when>
      <xsl:when test="self::bounce">
        <xsl:value-of select="concat($indent, 'physics::bounceOff(', $name, ', ', $velocity, ', ', $other, ');&#10;')" />
      </xsl:when>
      <xsl:when test="self::deflect">
        <xsl:value-of select="concat($indent, 'physics::deflect(', $name, ', ', $velocity, ', ', $other, ', ')" />
        <xsl:call-template name="value-bare" />
        <xsl:text>);
</xsl:text>
      </xsl:when>
      <xsl:when test="self::die">
        <xsl:value-of select="concat($indent, $alive, ' = false;&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="common-command">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="index"><xsl:call-template name="index-of"><xsl:with-param name="shape" select="$name" /></xsl:call-template></xsl:with-param>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
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

  <!-- A member's position or velocity as {x, y}: its own x and y, or else its
       group's. -->
  <xsl:template name="merged-vector">
    <xsl:param name="own" />
    <xsl:param name="shared" />
    <xsl:text>{</xsl:text>
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="($own/x | $shared/x[not($own/x)])[1]" /></xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="($own/y | $shared/y[not($own/y)])[1]" /></xsl:call-template>
    <xsl:text>}</xsl:text>
  </xsl:template>

  <!-- A value SFML wants as a whole number (a text's size): a number as it is,
       anything else worked out and cast. -->
  <xsl:template name="whole-number">
    <xsl:param name="node" />
    <xsl:variable name="text" select="normalize-space($node)" />
    <xsl:choose>
      <xsl:when test="not($node/*) and $text != '' and translate($text, $digits, '') = ''">
        <xsl:value-of select="$text" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>static_cast&lt;unsigned int&gt;(</xsl:text>
        <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$node" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
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

  <!-- How many shapes a group is: its members, or its columns times its rows. -->
  <xsl:template name="group-size">
    <xsl:choose>
      <xsl:when test="columns"><xsl:value-of select="number(normalize-space(columns)) * number(normalize-space(rows))" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="count(member)" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- How many shapes the `groups` are, all together. -->
  <xsl:template name="groups-size">
    <xsl:param name="groups" />
    <xsl:choose>
      <xsl:when test="not($groups)">0</xsl:when>
      <xsl:otherwise>
        <xsl:variable name="first"><xsl:for-each select="$groups[1]"><xsl:call-template name="group-size" /></xsl:for-each></xsl:variable>
        <xsl:variable name="rest"><xsl:call-template name="groups-size"><xsl:with-param name="groups" select="$groups[position() &gt; 1]" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="$first + $rest" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What a drawn sprite is named for: the object or group showing it, or
       a group's member (aliens.2). -->
  <xsl:template name="drawn-name">
    <xsl:choose>
      <xsl:when test="parent::member">
        <xsl:call-template name="cpp-name">
          <xsl:with-param name="name">
            <xsl:value-of select="concat(../../@name, '.')" />
            <xsl:choose>
              <xsl:when test="../@name"><xsl:value-of select="../@name" /></xsl:when>
              <xsl:otherwise><xsl:value-of select="count(../preceding-sibling::member) + 1" /></xsl:otherwise>
            </xsl:choose>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <!-- one of several looks: ballWhole -->
      <xsl:when test="@name and ../sprite[2]">
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="concat(../@name, '.', @name)" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="../@name" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The texture a <sprite> of a picture shows: its image file's, or the one
       it draws itself. -->
  <xsl:template name="sprite-texture">
    <xsl:choose>
      <xsl:when test="image">
        <xsl:call-template name="texture-name"><xsl:with-param name="path" select="image/path" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="drawn-name" />
        <xsl:text>Picture</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Where xgecli puts the picture it draws from an <svg>. -->
  <xsl:template name="drawn-file">
    <xsl:text>assets/drawn/</xsl:text>
    <xsl:call-template name="drawn-name" />
    <xsl:text>.png</xsl:text>
  </xsl:template>

  <!-- A drawn sprite's texture, after its rows or lines written out as they
       are drawn: a bitmap's rows one under another, so it looks like itself. -->
  <xsl:template name="declare-picture">
    <xsl:variable name="name"><xsl:call-template name="drawn-name" /></xsl:variable>
    <xsl:variable name="bitmap" select="bitmap" />
    <xsl:text>
</xsl:text>
    <xsl:choose>
      <!-- the rows of a group's look that another look has already -->
      <xsl:when test="$bitmap and ../@rows and generate-id(..) != generate-id($group-data/look[@rows = current()/../@rows][1])" />
      <xsl:when test="$bitmap">
        <xsl:value-of select="concat('&#10;const std::vector&lt;std::string&gt; ', $name, 'Rows = {')" />
        <xsl:for-each select="$bitmap/row">
          <xsl:if test="position() &gt; 1">,</xsl:if>
          <xsl:text>&#10;&#9;</xsl:text>
          <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="normalize-space(.)" /></xsl:call-template>
        </xsl:for-each>
        <xsl:text>
};</xsl:text>
      </xsl:when>
      <xsl:when test="line">
        <xsl:value-of select="concat('&#10;const std::vector&lt;pictures::Line&gt; ', $name, 'Lines = {')" />
        <xsl:for-each select="line">
          <xsl:if test="position() &gt; 1">,</xsl:if>
          <xsl:text>&#10;&#9;{</xsl:text>
          <xsl:call-template name="vector"><xsl:with-param name="node" select="from" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="vector"><xsl:with-param name="node" select="to" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="color"><xsl:with-param name="name" select="color" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:choose>
            <xsl:when test="thickness"><xsl:call-template name="whole-int"><xsl:with-param name="node" select="thickness" /></xsl:call-template></xsl:when>
            <xsl:otherwise>1</xsl:otherwise>
          </xsl:choose>
          <xsl:text>}</xsl:text>
        </xsl:for-each>
        <xsl:text>
};</xsl:text>
      </xsl:when>
    </xsl:choose>
    <xsl:value-of select="concat('&#10;sf::Texture ', $name, 'Picture;')" />
  </xsl:template>

  <!-- Its texture made, in setup()'s test of what could not be. -->
  <xsl:template name="load-picture">
    <xsl:variable name="name"><xsl:call-template name="drawn-name" /></xsl:variable>
    <xsl:variable name="bitmap" select="bitmap" />
    <xsl:value-of select="concat('!', $name, 'Picture.')" />
    <xsl:choose>
      <xsl:when test="$bitmap">
        <xsl:variable name="rows">
          <xsl:choose>
            <xsl:when test="../@rows"><xsl:for-each select="$group-data/look[@rows = current()/../@rows][1]/sprite"><xsl:call-template name="drawn-name" /></xsl:for-each></xsl:when>
            <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat('loadFromImage(pictures::rows(', $rows, 'Rows, ')" />
        <xsl:choose>
          <xsl:when test="$bitmap/scale"><xsl:call-template name="whole-number"><xsl:with-param name="node" select="$bitmap/scale" /></xsl:call-template></xsl:when>
          <xsl:otherwise>1</xsl:otherwise>
        </xsl:choose>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="color"><xsl:with-param name="name" select="$bitmap/color" /></xsl:call-template>
        <xsl:text>))</xsl:text>
      </xsl:when>
      <xsl:when test="line">
        <xsl:value-of select="concat('loadFromImage(pictures::lines(', $name, 'Lines))')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>loadFromFile(</xsl:text>
        <xsl:variable name="file"><xsl:call-template name="drawn-file" /></xsl:variable>
        <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="$file" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A value C++ wants as an int (a line's thickness): a number as it is,
       anything else worked out and rounded. -->
  <xsl:template name="whole-int">
    <xsl:param name="node" />
    <xsl:variable name="text" select="normalize-space($node)" />
    <xsl:choose>
      <xsl:when test="not($node/*) and $text != '' and translate($text, $digits, '') = ''">
        <xsl:value-of select="$text" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>static_cast&lt;int&gt;(std::lround(</xsl:text>
        <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$node" /></xsl:call-template>
        <xsl:text>))</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The SFML type an object or a group's members are drawn as. -->
  <xsl:template name="sf-type">
    <xsl:variable name="shape" select="(sprite | */sprite)[1]/*" />
    <xsl:choose>
      <xsl:when test="$shape/self::circle">sf::CircleShape</xsl:when>
      <xsl:when test="$shape/self::rectangle">sf::RectangleShape</xsl:when>
      <xsl:when test="$shape/self::text">sf::Text</xsl:when>
      <xsl:otherwise>sf::Sprite</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A picture's texture, named for its file: assets/paddle.jpg is paddleTexture. -->
  <xsl:template name="texture-name">
    <xsl:param name="path" />
    <xsl:call-template name="file-stem"><xsl:with-param name="path" select="normalize-space($path)" /></xsl:call-template>
    <xsl:text>Texture</xsl:text>
  </xsl:template>

  <xsl:template name="file-stem">
    <xsl:param name="path" />
    <xsl:choose>
      <xsl:when test="contains($path, '/')">
        <xsl:call-template name="file-stem"><xsl:with-param name="path" select="substring-after($path, '/')" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="contains($path, '.')">
        <xsl:call-template name="cpp-camel"><xsl:with-param name="name" select="translate(substring-before($path, '.'), ' ', '-')" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="cpp-camel"><xsl:with-param name="name" select="translate($path, ' ', '-')" /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A sound, named for what it is: <sound name="wall"> is wallSound. -->
  <xsl:template name="sound-name">
    <xsl:param name="name" />
    <xsl:call-template name="cpp-camel"><xsl:with-param name="name" select="$name" /></xsl:call-template>
    <xsl:text>Sound</xsl:text>
  </xsl:template>

  <!-- A color name as SFML's own (sf::Color::Red) or by its numbers. -->
  <xsl:template name="color">
    <xsl:param name="name" />
    <xsl:variable name="color" select="$tables/colors/color[@name = normalize-space($name)]" />
    <xsl:choose>
      <!-- none given: white, as the engine draws it -->
      <xsl:when test="normalize-space($name) = ''">sf::Color::White</xsl:when>
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
