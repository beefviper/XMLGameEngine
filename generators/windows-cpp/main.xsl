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
     function. The physics, the pictures it draws and its sound are the
     engine's own code (lib/: geometry, bitmap, sound and the SFML sound
     player), copied unchanged into engine/ only when the game uses them, so it
     plays as the engine does; physics.h beside it (modules/) hands the engine
     SFML's shapes. The few other helpers it needs are in functions.xml. -->
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
  <xsl:variable name="drawn" select="($objects/sprite | $groups[sprite[2]]/sprite | $group-data[not(@looks)]/look/sprite | $group-data/variant-look/look/sprite)[line or bitmap or svg]" />
  <xsl:variable name="drawn-rows" select="$drawn[bitmap]" />
  <!-- the groups' looks that are drawn, which share rows that are the same -->
  <xsl:variable name="drawn-looks" select="$group-data[not(@looks)]/look | $group-data/variant-look/look" />
  <xsl:variable name="drawn-lines" select="$drawn[line]" />
  <xsl:variable name="drawn-svgs" select="$drawn[svg]" />
  <xsl:variable name="loads" select="boolean($texts or $images or $drawn)" />
  <!-- Each picture once, however many objects show it. -->
  <xsl:key name="image-path" match="image/path" use="normalize-space(.)" />
  <xsl:variable name="image-paths" select="$images/sprite/image/path[generate-id() = generate-id(key('image-path', normalize-space(.))[count(ancestor::*[parent::objects] | $images) = count($images)][1])]" />

  <!-- Those with a velocity: one that is not 0, 0 (a group's, or any of its
       members'), or one keys, a pull, a chase or a path give them. -->
  <xsl:variable name="moving" select="$things[(velocity | */velocity)[x/* or y/* or number(x) != 0 or number(y) != 0]] | $pulled | $pushed | $stepped | $things[timers/timer/chase or collisions/collision/chase] | $followers" />

  <!-- Those pulled every frame (<acceleration>: gravity), and those keys push
       (<thrust> along the heading, <accelerate> one way): they have a velocity
       that changes, whatever it starts at. -->
  <xsl:variable name="pulled" select="$things[acceleration[x/* or y/* or number(x) != 0 or number(y) != 0]]" />
  <xsl:variable name="pushed" select="$objects[actions/action[thrust or accelerate]]" />

  <!-- Those with a <heading>: turned by keys, their picture drawn again at
       each whole degree, as the engine does. -->
  <xsl:variable name="headed" select="$objects[heading]" />

  <!-- Those whose touches are tested pixel by pixel (<type>pixel</type>). -->
  <xsl:variable name="pixel-things" select="$things[normalize-space(collisions/type) = 'pixel']" />
  <!-- The pictures of groups of them with more than one, which pixelsOf()
       tells apart by texture. -->
  <xsl:variable name="pixel-looks" select="$group-data[@name = $pixel-things/@name][count(look) &gt; 1]/look/sprite[bitmap or line or svg]" />

  <!-- The groups whose members or cells each have a velocity of their own
       (a member, row, column or cell gives one), kept in a std::vector beside
       the shapes; the rest of the groups that move share one. -->
  <xsl:variable name="member-velocities" select="$groups[*/velocity or velocity//random or timers/timer/chase or collisions/collision/chase] | $groups[count(. | $followers) = count($followers)]
      | $groups[count(. | $moving) = count($moving)][count(. | $resetting | $launched) = count($resetting | $launched)
          or .//reverse or .//stop or collisions/collision[bounce][not(normalize-space(../lockstep) = 'true' and (@edge = 'left' or @edge = 'right' or @edge = 'horizontal'))]]" />

  <!-- Those a rule lends another's velocity (<ride />: a frog on a log). -->
  <xsl:variable name="riders" select="$objects[@name = $rules[ride]/ancestor::object/@name]" />
  <!-- Those that <jump>: in the air a while, touching nothing. -->
  <xsl:variable name="jumpers" select="$objects[actions/action/jump]" />
  <!-- Those keys move (<move>): the keys held set their velocity, as the
       engine's keys do, so they move, meet the edges and touch things like
       anything else that moves. -->
  <xsl:variable name="stepped" select="$objects[actions/action/move]" />
  <!-- Those that <hop>: a step at once, made as the frame's move begins. -->
  <xsl:variable name="hoppers" select="$objects[actions/action/hop]" />
  <!-- Those that hop or jump, which count as moving the frame they do, so
       they meet what they land on even when nothing there moves. -->
  <xsl:variable name="hopping" select="$hoppers | $jumpers" />
  <!-- Those that <leap> up under their own pull, from the ground. -->
  <xsl:variable name="leapers" select="$objects[actions/action/leap]" />
  <!-- Those that <climb> a ladder (a thing of a class) while at one. -->
  <xsl:variable name="climbers" select="$objects[actions/action/climb]" />
  <xsl:variable name="climbs" select="$climbers/actions/action/climb" />
  <xsl:variable name="ladder-classes" select="$climbs[not(@class = preceding::climb[count(. | $climbs) = count($climbs)]/@class)]" />
  <!-- Those that know whether they stand on something this frame (<land />). -->
  <xsl:variable name="standers" select="$leapers | $climbers" />
  <!-- Those that <aim> at the nearest of something: their shots go that way
       from then on, until they start again. -->
  <xsl:variable name="aimers" select="$things[timers/timer/aim or collisions/collision/aim]" />
  <!-- Those that <chase> the nearest of something: a velocity straight at it. -->
  <xsl:variable name="chasers" select="$things[timers/timer/chase or collisions/collision/chase]" />
  <!-- Those whose <facing> changes: with a key that moves, hops or jumps them,
       and as they aim or chase. A fire leaves the way they face now. -->
  <xsl:variable name="facers" select="$things[facing][actions/action[move or hop or jump] or timers/timer/*[self::aim or self::chase] or collisions/collision/*[self::aim or self::chase]]" />
  <!-- What they look for, the nearest one in play found by a function. -->
  <xsl:variable name="sought" select="$things[@name = $things/timers/timer/*[self::aim or self::chase]/@object or @name = $things/collisions/collision/*[self::aim or self::chase]/@object]" />

  <!-- Those that can move in a frame: by a velocity, a hop or a jump, or
       riding something. -->
  <xsl:variable name="movers" select="$moving | $hopping | $riders" />

  <!-- Those that never move (no velocity of their own, no keys): they never
       meet an edge, so their edge rules are left out, as the engine only looks
       at the edges for what is moving. -->
  <xsl:variable name="still" select="$things[count(. | $moving) != count($moving)][not(actions/action/*)]" />

  <!-- Groups that move as one block (<lockstep>): when one of them bounces off
       a side, all of them turn. -->
  <xsl:variable name="lockstep" select="$groups[normalize-space(collisions/lockstep) = 'true']" />

  <!-- What a <fire> or a <release> takes from: in play, its collisions are on,
       whatever <enabled> says, as in the engine. -->
  <xsl:variable name="launched" select="$things[@name = /game//fire/@object or @name = /game//release/@object]" />

  <!-- What can touch and be touched: its collisions on, a projectile, or
       something fired or released. -->
  <xsl:variable name="colliding" select="$things[normalize-space(collisions/enabled) = 'true' or @class = 'projectile'] | $launched" />

  <!-- Those that start out of play (<hidden>, and projectiles), until a
       <fire>, <release> or <reveal> brings them in. -->
  <xsl:variable name="hidden" select="$things[normalize-space(hidden) = 'true' or @class = 'projectile']" />

  <xsl:variable name="rules" select="$colliding/collisions/collision[not(@edge and count(ancestor::*[parent::objects] | $still) = count($still))]" />
  <xsl:variable name="edge-rules" select="$rules[@edge]" />
  <!-- Those that can be taken out of play by a <die />: each has a flag (one
       for each member of a group) saying it is still in play, looked at before
       it is moved, drawn, moved by a key or touched. -->
  <xsl:variable name="dying" select="$things[@name = $rules[die]/ancestor::*[parent::objects]/@name] | $things[timers/timer/die] | $hidden | $launched | $things[@name = /game//reveal/@object]" />

  <!-- Projectiles (class="projectile"): out of play until a <fire> launches
       one, from the middle of the shooter's top at its own velocity. Their
       collisions are on while they fly, whatever <enabled> says, as in the
       engine. -->
  <xsl:variable name="projectiles" select="$things[@class = 'projectile']" />
  <xsl:variable name="object-rules" select="$rules[not(@edge)]" />

  <!-- Those that go back to where they started after a <reset /> (its place
       and velocity, any <random> drawn anew, as in the engine); for a group,
       one member at a time. -->
  <xsl:variable name="resetting" select="$things[count(. | $colliding) = count($colliding)][collisions/collision/reset[not(@object)]] | $things[timers/timer/reset[not(@object)]] | $objects[actions/action/reset[not(@object)]] | $things[@name = /game//reveal/@object or @name = /game//release/@object]" />

  <!-- Those with <timers>, counted down every frame they are shown and in
       play, and the screens with timers of their own. -->
  <xsl:variable name="timed" select="$things[timers/timer]" />
  <xsl:variable name="timed-states" select="$states[timers/timer]" />

  <!-- Those a <reset object="..."> puts back as they started, all of it (its
       variables, place, velocity, whether it is in play, look and timers):
       reset<Name>(), which start() calls too. -->
  <xsl:variable name="reset-whole" select="($things | $keepers)[@name = /game//reset/@object]" />

  <!-- Objects no screen shows whose variables something uses (Megamania's
       energy, shown only by its number): their variables are kept. -->
  <xsl:variable name="keepers" select="/game/objects/object[count(. | $objects) != count($objects)][variables/variable][contains($words, concat(' ', @name, '.'))]" />

  <!-- What a frame does, in the engine's order (Game::updateObjects): what
       changes velocities (a ladder, a pull, drag), the edges of what is
       moving, the move from touch to touch, then what sticks at an edge kept
       in. Each set is of those with something to do in that part. -->
  <xsl:variable name="accelerating" select="$climbers | $pulled | $things[count(. | $moving) = count($moving)][drag]" />
  <xsl:variable name="edged" select="$things[collisions/collision[@edge][*][count(. | $rules) = count($rules)]]" />
  <xsl:variable name="sticking" select="$colliding[collisions/collision[@edge][stick]]" />

  <!-- The pairs of things that may touch, as the engine pairs them
       (Game::canCollide): both can touch, one has a rule about the other, a
       screen shows both, and one of them can move. Each is written first in
       the file first, as the engine has them, and a group with itself is
       left out. Each pair gets its value of the enum Touch (BallBricks). -->
  <xsl:variable name="touches-tree">
    <xsl:for-each select="$colliding">
      <xsl:variable name="a" select="." />
      <xsl:for-each select="following-sibling::*[count(. | $colliding) = count($colliding)]">
        <xsl:variable name="b" select="." />
        <xsl:if test="($a/collisions/collision[*][not(@edge)][not(@object) or @object = $b/@name][not(@class) or @class = $b/@class]
                       or $b/collisions/collision[*][not(@edge)][not(@object) or @object = $a/@name][not(@class) or @class = $a/@class])
                      and $states[shows/show/@object = $a/@name][shows/show/@object = $b/@name]
                      and count(($a | $b)[count(. | $movers) = count($movers)]) &gt; 0">
          <touch a="{$a/@name}" b="{$b/@name}">
            <xsl:attribute name="value">
              <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$a/@name" /></xsl:call-template>
              <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$b/@name" /></xsl:call-template>
            </xsl:attribute>
          </touch>
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="touches" select="exsl:node-set($touches-tree)/touch" />

  <!-- What the keys do: held, a <trigger> of an action of <move>s, <turn>s,
       <thrust>s or <accelerate>s, looked at every frame; pressed, everything
       else (a <hop> too), once for each press. -->
  <xsl:key name="action" match="object/actions/action" use="concat(../../@name, '|', @name)" />
  <xsl:variable name="all-inputs" select="/game/states/keys/input | $states/inputs/input" />
  <xsl:variable name="held-inputs" select="$all-inputs[trigger[key('action', concat(@object, '|', @action))[move or turn or thrust or accelerate or climb]]]" />
  <xsl:variable name="pressed-inputs" select="$all-inputs[* and (not(trigger[key('action', concat(@object, '|', @action))[move or turn or thrust or accelerate or climb]])
      or trigger[key('action', concat(@object, '|', @action))[*[not(self::move or self::turn or self::thrust or self::accelerate or self::climb)] or move[../../../facing]]])]" />

  <!-- Each screen's keys: the <input>s it takes, from the <keys> sets its
       <inputs keys="..."> names (each over the one before) and then its own
       (over those), each with the keys it still holds once the later ones have
       taken theirs, as the engine reads them. -->
  <xsl:variable name="screen-keys-tree">
    <xsl:for-each select="$states">
      <xsl:variable name="state" select="." />
      <xsl:variable name="names" select="concat(' ', normalize-space(inputs/@keys), ' ')" />
      <screen name="{@name}">
        <xsl:for-each select="/game/states/keys[contains($names, concat(' ', @name, ' '))]/input | inputs/input">
          <xsl:variable name="keys">
            <xsl:call-template name="effective-keys">
              <xsl:with-param name="input" select="." />
              <xsl:with-param name="state" select="$state" />
              <xsl:with-param name="names" select="$names" />
              <xsl:with-param name="rest" select="normalize-space(@button)" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:if test="normalize-space($keys) != ''">
            <input id="{generate-id()}" keys="{normalize-space($keys)}">
              <xsl:attribute name="pressed">
                <xsl:choose>
                  <xsl:when test="count(. | $pressed-inputs) = count($pressed-inputs)">yes</xsl:when>
                  <xsl:otherwise>no</xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>
              <xsl:attribute name="held">
                <xsl:choose>
                  <xsl:when test="count(. | $held-inputs) = count($held-inputs)">yes</xsl:when>
                  <xsl:otherwise>no</xsl:otherwise>
                </xsl:choose>
              </xsl:attribute>
            </input>
          </xsl:if>
        </xsl:for-each>
      </screen>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="screen-keys" select="exsl:node-set($screen-keys-tree)/screen" />
  <xsl:variable name="hops" select="boolean($pressed-inputs/trigger[key('action', concat(@object, '|', @action))/hop])" />
  <xsl:variable name="fires" select="boolean($pressed-inputs/trigger[key('action', concat(@object, '|', @action))/fire] or $timed/timers/timer/fire)" />

  <xsl:variable name="sounds" select="/game/sounds/sound" />

  <!-- Every word in the parts of the game that are written out, between
       spaces, to see which names and functions it uses. -->
  <xsl:variable name="words">
    <xsl:text> </xsl:text>
    <xsl:for-each select="$game/window//text() | $game/variables//text() | $game/variables//@* | $things//text() | $things//@* | $states//text() | $states//@* | $game/paths//text()">
      <xsl:value-of select="translate(., '+-*/(),&#9;&#10;&#13;', '          ')" />
      <xsl:text> </xsl:text>
    </xsl:for-each>
  </xsl:variable>

  <!-- The same, without the names of the game's own variables, to see which
       of them are used (an <svg>'s numbers are used when it is generated, not
       by the program). -->
  <xsl:variable name="values-words">
    <xsl:text> </xsl:text>
    <xsl:for-each select="$game/window//text() | $game/variables//text() | $game/sounds//text() | $things//text()[not(ancestor::svg)] | $things//@*[not(local-name() = 'name')] | $states//text() | $states//@* | $game/paths//text()">
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

  <!-- The engine's code it needs: physics for the edges, touches and sizes,
       pictures for what it draws, sound for its sounds. -->
  <xsl:variable name="physics" select="boolean($rules/* or $sizes-read != '' or $movers or $climbers or $fires or $aimers or $chasers)" />
  <xsl:variable name="audio" select="boolean($sounds)" />
  <xsl:variable name="pictures" select="boolean($drawn-rows or $drawn-lines)" />

  <!-- The helper functions the game needs (functions.xml). -->
  <xsl:variable name="used">
    <xsl:text> </xsl:text>
    <xsl:if test="$things//random"> randomBetween </xsl:if>
    <xsl:if test="contains($words, ' sgn ')"> sign </xsl:if>
    <xsl:if test="$timed or $timed-states or $jumpers or $animated"> framesFor </xsl:if>
    <xsl:if test="$pictures"> imageOf </xsl:if>
    <xsl:if test="$sounds//@pitch[not(contains($digits, substring(normalize-space(.), 1, 1)) or starts-with(normalize-space(.), '.'))] or $sounds//@to[not(contains($digits, substring(normalize-space(.), 1, 1)) or starts-with(normalize-space(.), '.'))]"> hertz </xsl:if>
  </xsl:variable>
  <xsl:variable name="used-functions" select="$helpers/function[contains($used, concat(' ', @name, ' '))]" />

  <!-- The standard headers it needs. -->
  <xsl:variable name="headers">
    <xsl:text> optional </xsl:text>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="concat(' ', @uses, ' ')" />
    </xsl:for-each>
    <xsl:if test="contains($words, ' min ') or contains($words, ' max ')"> algorithm </xsl:if>
    <xsl:if test="$headed or $leapers or (//release | //reveal)[* or (normalize-space(.) != '' and string(number(.)) != normalize-space(.))]"> cmath </xsl:if>
    <xsl:if test="contains($words, ' abs ') or contains($words, ' floor ') or contains($words, ' ceil ') or contains($words, ' sqrt ') or contains($words, ' sin ') or contains($words, ' cos ') or contains($words, ' tan ') or contains($words, ' pow ') or contains($words, ' round ')"> cmath </xsl:if>
    <xsl:if test="$numbers or $drawn-rows"> string </xsl:if>
    <xsl:if test="$pictures"> vector </xsl:if>
    <xsl:if test="$screens or $groups or $touches"> vector </xsl:if>
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
    <xsl:call-template name="generate-paths" />
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

  <!-- The engine's code (physics.h and engine/), then SFML, then the standard library. -->
  <xsl:template name="generate-includes">
    <xsl:text>
</xsl:text>
    <xsl:if test="$audio">#include "audio_sfml.h"&#10;</xsl:if>
    <xsl:if test="$pictures">#include "bitmap.h"&#10;</xsl:if>
    <xsl:if test="$physics">#include "physics.h"&#10;</xsl:if>
    <xsl:if test="$physics or $audio or $pictures">
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
    <xsl:if test="$edge-rules/* or $sticking or $hops or $jumpers">const sf::FloatRect windowArea({0.0f, 0.0f}, {windowWidth, windowHeight});&#10;</xsl:if>
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
        <xsl:call-template name="variable-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value-bare" />
        <xsl:text>;</xsl:text>
        <xsl:if test="$game/objects/*[@name = current()/@name]">
          <xsl:value-of select="concat(' // named like the ', local-name($game/objects/*[@name = current()/@name]), ' ', @name)" />
        </xsl:if>
        <xsl:text>&#10;</xsl:text>
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
    <xsl:if test="$timed-states">
      <xsl:text>
// the screens' timers: the frames until each goes off (0: counted again on its next frame)
</xsl:text>
      <xsl:for-each select="$timed-states/timers/timer">
        <xsl:text>int </xsl:text>
        <xsl:call-template name="timer-name" />
        <xsl:text> = 0;
</xsl:text>
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <!-- A timer's count: bomber1Timer2, the second timer of bomber1. -->
  <xsl:template name="timer-name">
    <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="../../@name" /></xsl:call-template>
    <xsl:value-of select="concat('Timer', count(preceding-sibling::timer) + 1)" />
  </xsl:template>

  <!-- An object's timers, each the frames until it goes off (0: counted again
       on its next frame; -1: an <after> that has gone off); one for each
       member of a group. -->
  <xsl:template name="declare-timers">
    <xsl:for-each select="timers/timer">
      <xsl:choose>
        <xsl:when test="../../self::group">
          <xsl:text>&#10;std::vector&lt;int&gt; </xsl:text>
          <xsl:call-template name="timer-name" />
          <xsl:text>; // for each of them, the frames until its timer goes off</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>&#10;int </xsl:text>
          <xsl:call-template name="timer-name" />
          <xsl:text> = 0; // the frames until its timer goes off</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="after"> (once)</xsl:if>
    </xsl:for-each>
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
              <xsl:value-of select="concat('&#10;std::vector&lt;sf::Vector2f&gt; ', $name, 'Velocity')" />
              <xsl:if test="count(. | $resetting) = count($resetting)"><xsl:value-of select="concat('(', $cells, ')')" /></xsl:if>
              <xsl:text>;</xsl:text>
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
      <xsl:if test="count(. | $riders) = count($riders)">
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Riding; // the velocity of what it rides (&lt;ride /&gt;), worked out every frame')" />
      </xsl:if>
      <xsl:if test="count(. | $jumpers) = count($jumpers)">
        <xsl:value-of select="concat('&#10;int ', $name, 'JumpFrames = 0; // the frames left of a &lt;jump&gt; under way: in the air, touching nothing, until 0')" />
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'JumpStep; // how far it goes in each of them')" />
      </xsl:if>
      <xsl:if test="count(. | $pulled) = count($pulled)">
        <xsl:choose>
          <xsl:when test="self::group">
            <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Acceleration; // their pull, one for them all, every frame')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Acceleration; // its pull, every frame (until a &lt;stop /&gt;)')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
      <xsl:if test="count(. | $stepped) = count($stepped)">
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Steps; // the way the keys held move it (&lt;move&gt;), worked out every frame')" />
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Stepped; // the same, as its velocity last took it')" />
      </xsl:if>
      <xsl:if test="count(. | $hoppers) = count($hoppers)">
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Hop; // a &lt;hop&gt; asked for, made as the next move begins')" />
      </xsl:if>
      <xsl:if test="count(. | $hopping) = count($hopping)">
        <xsl:value-of select="concat('&#10;bool ', $name, 'Hopped = false; // it hopped or jumped this frame: moving, whatever its velocity')" />
      </xsl:if>
      <xsl:if test="count(. | $climbers) = count($climbers)">
        <xsl:value-of select="concat('&#10;float ', $name, 'Climb = 0.0f; // up (below 0) or down the keys held climb it, worked out every frame')" />
        <xsl:value-of select="concat('&#10;bool ', $name, 'Climbing = false; // on a ladder: nothing pulls it, nor walks it')" />
      </xsl:if>
      <xsl:if test="count(. | $leapers) = count($leapers)">
        <xsl:value-of select="concat('&#10;bool ', $name, 'Leaping = false; // in a leap: the way across kept until it lands')" />
      </xsl:if>
      <xsl:if test="count(. | $standers) = count($standers)">
        <xsl:value-of select="concat('&#10;bool ', $name, 'Grounded = false; // standing on something, as this frame found')" />
      </xsl:if>
      <xsl:if test="count(. | $facers) = count($facers)">
        <xsl:variable name="facing"><xsl:call-template name="facing-value" /></xsl:variable>
        <xsl:choose>
          <xsl:when test="self::group">
            <xsl:variable name="cells"><xsl:call-template name="group-size" /></xsl:variable>
            <xsl:value-of select="concat('&#10;std::vector&lt;physics::Facing&gt; ', $name, 'Facing(', $cells, ', ', $facing, '); // the way each faces (&lt;facing&gt;), which a &lt;fire&gt; leaves')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('&#10;physics::Facing ', $name, 'Facing = ', $facing, '; // the way it faces (&lt;facing&gt;), which a &lt;fire&gt; leaves')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
      <xsl:if test="count(. | $aimers) = count($aimers)">
        <xsl:choose>
          <xsl:when test="self::group">
            <xsl:variable name="cells"><xsl:call-template name="group-size" /></xsl:variable>
            <xsl:value-of select="concat('&#10;std::vector&lt;sf::Vector2f&gt; ', $name, 'Aim(', $cells, '); // the way the shots of each go (&lt;aim&gt;), one pixel long')" />
            <xsl:value-of select="concat('&#10;std::vector&lt;bool&gt; ', $name, 'Aimed(', $cells, '); // which of them have aimed since they started')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Aim; // the way its shots go (&lt;aim&gt;), one pixel long')" />
            <xsl:value-of select="concat('&#10;bool ', $name, 'Aimed = false; // whether it has aimed since it started')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
      <xsl:if test="count(. | $followers) = count($followers)">
        <xsl:call-template name="declare-flight"><xsl:with-param name="name" select="$name" /></xsl:call-template>
      </xsl:if>
      <xsl:if test="count(. | $headed) = count($headed)">
        <xsl:value-of select="concat('&#10;float ', $name, 'Heading = 0.0f; // degrees clockwise from straight up')" />
        <xsl:value-of select="concat('&#10;int ', $name, 'DrawnAt = -1; // the whole degree its picture is drawn at')" />
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
              <xsl:when test="count(. | $hidden) = count($hidden)">
                <xsl:value-of select="concat('&#10;bool ', $name, 'Alive = false; // hidden: in play once it is brought in, until it dies')" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="concat('&#10;bool ', $name, 'Alive = true; // in play until it dies')" />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
      <xsl:call-template name="declare-timers" />
      <xsl:for-each select="variables/variable">
        <xsl:text>&#10;float </xsl:text>
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="concat(ancestor::object/@name, '.', @name)" /></xsl:call-template>
        <xsl:text> = 0.0f;</xsl:text>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:for-each select="$keepers">
      <xsl:value-of select="concat('&#10;&#10;// ', @name, ': never shown, its variables kept')" />
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
// sounds: made from their notes and played by the engine's own player
xge::SFMLAudio audio;
</xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template name="generate-declarations">
    <xsl:if test="$touches">
      <xsl:text>
// two things that may touch this frame, as the engine pairs them: which two
// (Touch), and which member of each that is a group
enum class Touch { </xsl:text>
      <xsl:for-each select="$touches">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:value-of select="@value" />
      </xsl:for-each>
      <xsl:text> };
struct Pair
{
	Touch touch;
	std::size_t first = 0;
	std::size_t second = 0;
};
</xsl:text>
    </xsl:if>
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
    <xsl:for-each select="$moving-states">
      <xsl:variable name="title"><xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('void move', $title, '();&#10;void advance', $title, '(float fraction);&#10;')" />
    </xsl:for-each>
    <xsl:if test="$touches">
      <xsl:text>std::optional&lt;physics::SweepHit&gt; sweepPair(const Pair&amp; pair, float left);
void touch(const Pair&amp; pair, physics::Edge edge);
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$accelerating">
      <xsl:text>void accelerate</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$edged">
      <xsl:text>void edges</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$sticking">
      <xsl:text>void stick</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$headed">
      <xsl:text>void turn</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$timed">
      <xsl:text>void tick</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$reset-whole">
      <xsl:text>void reset</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$resetting">
      <xsl:text>void start</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:if test="self::group">(std::size_t i);&#10;</xsl:if>
      <xsl:if test="self::object">();&#10;</xsl:if>
    </xsl:for-each>
    <xsl:for-each select="$looked">
      <xsl:call-template name="become-signature" />
      <xsl:text>;
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$animated">
      <xsl:text>void </xsl:text>
      <xsl:call-template name="animate-name" />
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$sought">
      <xsl:call-template name="nearest-signature" />
      <xsl:text>;
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$followers">
      <xsl:text>void </xsl:text>
      <xsl:call-template name="fly-name" />
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$unless-classes">
      <xsl:call-template name="touching-signature" />
      <xsl:text>;
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$ladder-classes">
      <xsl:call-template name="ladder-signature" />
      <xsl:text>;
</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$numbers">
      <xsl:text>void show</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
    <xsl:if test="$pixel-looks">const sf::Image&amp; pixelsOf(const sf::Texture&amp; picture);&#10;</xsl:if>
    <xsl:for-each select="$used-functions">
      <xsl:value-of select="concat(normalize-space(declaration), '&#10;')" />
    </xsl:for-each>
  </xsl:template>

  <!-- The screens with something to do each frame: keys held, objects that
       move or have rules, or conditions. -->
  <xsl:variable name="updating-states" select="$states[@name = $screen-keys[input/@held = 'yes']/@name or shows/show/@object = ($accelerating | $edged | $sticking | $movers | $timed)/@name or conditions/condition or timers/timer]" />
  <!-- The screens that show something that can move: each has its move. -->
  <xsl:variable name="moving-states" select="$states[shows/show/@object = $movers/@name]" />

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
    <xsl:for-each select="$moving-states">
      <xsl:call-template name="define-move" />
      <xsl:call-template name="define-advance" />
    </xsl:for-each>
    <xsl:if test="$touches">
      <xsl:call-template name="define-sweep-pair" />
      <xsl:call-template name="define-touch" />
    </xsl:if>
    <xsl:for-each select="$accelerating">
      <xsl:call-template name="define-accelerate" />
    </xsl:for-each>
    <xsl:for-each select="$edged">
      <xsl:call-template name="define-edges" />
    </xsl:for-each>
    <xsl:for-each select="$sticking">
      <xsl:call-template name="define-stick" />
    </xsl:for-each>
    <xsl:for-each select="$headed">
      <xsl:call-template name="define-turn" />
    </xsl:for-each>
    <xsl:for-each select="$timed">
      <xsl:call-template name="define-tick" />
    </xsl:for-each>
    <xsl:for-each select="$reset-whole">
      <xsl:call-template name="define-reset" />
    </xsl:for-each>
    <xsl:for-each select="$resetting[self::group]">
      <xsl:call-template name="define-member-start" />
    </xsl:for-each>
    <xsl:for-each select="$resetting[self::object]">
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
    <xsl:for-each select="$animated">
      <xsl:call-template name="define-animate" />
    </xsl:for-each>
    <xsl:for-each select="$sought">
      <xsl:call-template name="define-nearest" />
    </xsl:for-each>
    <xsl:for-each select="$followers">
      <xsl:call-template name="define-fly" />
    </xsl:for-each>
    <xsl:for-each select="$unless-classes">
      <xsl:call-template name="define-touching" />
    </xsl:for-each>
    <xsl:for-each select="$ladder-classes">
      <xsl:call-template name="define-ladder" />
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
</xsl:text>
      <!-- an object placed by this number's size (a label beside it) moves with it -->
      <xsl:variable name="number" select="@name" />
      <xsl:for-each select="$objects[@name != $number][position]">
        <xsl:variable name="placed">
          <xsl:for-each select="position//text()">
            <xsl:text> </xsl:text>
            <xsl:value-of select="translate(., '+-*/(),&#9;&#10;&#13;', '          ')" />
            <xsl:text> </xsl:text>
          </xsl:for-each>
        </xsl:variable>
        <xsl:if test="contains($placed, concat(' ', $number, '.width ')) or contains($placed, concat(' ', $number, '.height '))">
          <xsl:variable name="other"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
          <xsl:value-of select="concat('&#9;', $other, '.setPosition(')" />
          <xsl:call-template name="vector"><xsl:with-param name="node" select="position" /></xsl:call-template>
          <xsl:text>);
</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>}
</xsl:text>
    </xsl:for-each>
    <xsl:if test="$pixel-looks">
      <xsl:text>
// The pixels of a picture, for the touches tested pixel by pixel.
const sf::Image&amp; pixelsOf(const sf::Texture&amp; picture)
{
</xsl:text>
      <xsl:for-each select="$pixel-looks">
        <xsl:variable name="name"><xsl:call-template name="drawn-name" /></xsl:variable>
        <xsl:choose>
          <xsl:when test="position() = last()">
            <xsl:value-of select="concat('&#9;return ', $name, 'Pixels;&#10;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('&#9;if (&amp;picture == &amp;', $name, 'Picture)&#10;&#9;{&#10;&#9;&#9;return ', $name, 'Pixels;&#10;&#9;}&#10;')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:text>}
</xsl:text>
    </xsl:if>
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
        <xsl:for-each select="$drawn">
          <xsl:variable name="pixels"><xsl:call-template name="tested-pixels" /></xsl:variable>
          <xsl:if test="$pixels = 'yes'">
            <xsl:variable name="name"><xsl:call-template name="drawn-name" /></xsl:variable>
            <xsl:value-of select="concat('&#9;', $name, 'Pixels = ', $name, 'Picture.copyToImage();&#10;')" />
          </xsl:if>
        </xsl:for-each>
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
	audio.load({</xsl:text>
      <xsl:for-each select="$sounds">
        <xsl:if test="position() &gt; 1">,</xsl:if>
        <xsl:call-template name="make-sound" />
      </xsl:for-each>
      <xsl:text>
	});
</xsl:text>
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

  <!-- A sound as the engine has it (xge::SoundDesc): its name, how loud, and
       its notes, each a wave, a pitch it slides from and to, and a length. -->
  <xsl:template name="make-sound">
    <xsl:variable name="wave" select="@wave" />
    <xsl:text>
		{</xsl:text>
    <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="@name" /></xsl:call-template>
    <xsl:text>, </xsl:text>
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
          <xsl:text>{.rest = true, .seconds = </xsl:text>
          <xsl:call-template name="value-bare" />
          <xsl:text>}</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="noteWave">
            <xsl:choose>
              <xsl:when test="@wave"><xsl:value-of select="@wave" /></xsl:when>
              <xsl:when test="$wave"><xsl:value-of select="$wave" /></xsl:when>
              <xsl:otherwise>square</xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:text>{.wave = xge::Waveform::</xsl:text>
          <xsl:value-of select="concat(translate(substring($noteWave, 1, 1), $lower, $upper), substring($noteWave, 2))" />
          <xsl:text>, .startHz = </xsl:text>
          <xsl:call-template name="pitch"><xsl:with-param name="name" select="@pitch" /></xsl:call-template>
          <xsl:text>, .endHz = </xsl:text>
          <xsl:choose>
            <xsl:when test="@to"><xsl:call-template name="pitch"><xsl:with-param name="name" select="@to" /></xsl:call-template></xsl:when>
            <xsl:otherwise><xsl:call-template name="pitch"><xsl:with-param name="name" select="@pitch" /></xsl:call-template></xsl:otherwise>
          </xsl:choose>
          <xsl:text>, .seconds = </xsl:text>
          <xsl:call-template name="value-bare" />
          <xsl:text>}</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>
		}}</xsl:text>
  </xsl:template>

  <!-- A pitch: hertz as a number, or a note's name (A4) worked out as the engine reads it. -->
  <xsl:template name="pitch">
    <xsl:param name="name" />
    <xsl:choose>
      <xsl:when test="contains($digits, substring($name, 1, 1)) or starts-with($name, '.')">
        <xsl:call-template name="cpp-number"><xsl:with-param name="text" select="normalize-space($name)" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>hertz(</xsl:text>
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
    <xsl:for-each select="$things | $keepers">
      <xsl:if test="position() &gt; 1 or $screens">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="count(. | $reset-whole) = count($reset-whole)">
          <xsl:text>	reset</xsl:text>
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>();
</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="thing-start" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:for-each select="$timed-states/timers/timer">
      <xsl:if test="position() = 1">
        <xsl:text>
	// the screens' timers, counted from the start
</xsl:text>
      </xsl:if>
      <xsl:text>	</xsl:text>
      <xsl:call-template name="timer-name" />
      <xsl:text> = 0;
</xsl:text>
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

  <!-- One object's or group's start: its variables, place and velocity,
       whether it is in play, its look and its timers; for one never shown
       (a keeper), its variables alone. -->
  <xsl:template name="thing-start">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="count(. | $things) != count($things)">
        <xsl:for-each select="variables/variable">
          <xsl:sort select="@name" />
          <xsl:call-template name="set-variable" />
        </xsl:for-each>
      </xsl:when>
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
    <xsl:if test="count(. | $pulled) = count($pulled)">
      <xsl:value-of select="concat('&#9;', $name, 'Acceleration = ')" />
      <xsl:call-template name="vector"><xsl:with-param name="node" select="acceleration" /></xsl:call-template>
      <xsl:text>;
</xsl:text>
    </xsl:if>
    <xsl:if test="count(. | $followers) = count($followers)">
      <xsl:call-template name="start-flight"><xsl:with-param name="name" select="$name" /></xsl:call-template>
    </xsl:if>
    <xsl:if test="count(. | $facers) = count($facers)">
      <xsl:variable name="facing"><xsl:call-template name="facing-value" /></xsl:variable>
      <xsl:choose>
        <xsl:when test="self::group"><xsl:value-of select="concat('&#9;', $name, 'Facing.assign(', $name, '.size(), ', $facing, ');&#10;')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat('&#9;', $name, 'Facing = ', $facing, ';&#10;')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:if test="self::object and count(. | $dying) = count($dying)">
      <xsl:value-of select="concat('&#9;', $name)" />
      <xsl:choose>
        <xsl:when test="@class = 'projectile'">Alive = false; // until it is fired
</xsl:when>
        <xsl:when test="count(. | $hidden) = count($hidden)">Alive = false; // hidden until it is brought in
</xsl:when>
        <xsl:otherwise>Alive = true;
</xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:for-each select="timers/timer">
      <xsl:text>	</xsl:text>
      <xsl:call-template name="timer-name" />
      <xsl:choose>
        <xsl:when test="../../self::group"><xsl:value-of select="concat('.assign(', $name, '.size(), 0);&#10;')" /></xsl:when>
        <xsl:otherwise> = 0;&#10;</xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <!-- A <reset object="...">: what it names put back as it started, all of
       it, and the numbers that show its variables. -->
  <xsl:template name="define-reset">
    <xsl:variable name="owner" select="@name" />
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>: back as it started, all of it (start() and a &lt;reset object="</xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>" /&gt;)
void reset</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>()
{
</xsl:text>
    <xsl:call-template name="thing-start" />
    <xsl:for-each select="$numbers[starts-with(normalize-space(sprite/text/number), concat($owner, '.'))]">
      <xsl:text>	show</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
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
      <xsl:when test="count(. | $resetting) = count($resetting)">
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;&#9;&#9;start')" />
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>(i);
	}
</xsl:text>
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
    <xsl:if test="count(. | $aimers) = count($aimers) and count(. | $resetting) != count($resetting)">
      <xsl:value-of select="concat('&#9;', $name, 'Aim.assign(', $name, '.size(), {});&#10;&#9;', $name, 'Aimed.assign(', $name, '.size(), false);&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $animated) = count($animated)">
      <xsl:call-template name="start-animation"><xsl:with-param name="indent" select="'&#9;'" /></xsl:call-template>
    </xsl:if>
    <xsl:if test="count(. | $looked) = count($looked)">
      <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;&#9;&#9;')" />
      <xsl:call-template name="become-call">
        <xsl:with-param name="thing" select="." />
        <xsl:with-param name="sprite"><xsl:call-template name="first-look" /></xsl:with-param>
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
        <xsl:when test="count(. | $hidden) = count($hidden)">
          <xsl:value-of select="concat('&#9;', $name, 'Alive.assign(', $name, '.size(), false); // hidden until they are brought in&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&#9;', $name, 'Alive.assign(', $name, '.size(), true);&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- A <reset /> of `self`: startBall(); or, for the member `name` of a
       group (boulders[i]), startBoulders(i); -->
  <xsl:template name="start-call">
    <xsl:param name="self" />
    <xsl:param name="name" />
    <xsl:text>start</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template>
    <xsl:text>(</xsl:text>
    <xsl:if test="$self/self::group"><xsl:call-template name="index-of"><xsl:with-param name="shape" select="$name" /></xsl:call-template></xsl:if>
    <xsl:text>);
</xsl:text>
  </xsl:template>

  <!-- A group's start of one member, which a <reset /> calls again: its place
       and its velocity, any <random> drawn anew. What is the same for every
       member is written once; what is not, as a list with one for each. -->
  <xsl:template name="define-member-start">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="." />
    <xsl:variable name="moves" select="count(. | $moving) = count($moving)" />
    <xsl:variable name="body">
      <xsl:call-template name="member-axis">
        <xsl:with-param name="list" select="'xs'" />
        <xsl:with-param name="own" select="member/position/x" />
        <xsl:with-param name="shared" select="position/x" />
      </xsl:call-template>
      <xsl:call-template name="member-axis">
        <xsl:with-param name="list" select="'ys'" />
        <xsl:with-param name="own" select="member/position/y" />
        <xsl:with-param name="shared" select="position/y" />
      </xsl:call-template>
      <xsl:if test="$moves">
        <xsl:call-template name="member-axis">
          <xsl:with-param name="list" select="'speedsX'" />
          <xsl:with-param name="own" select="member/velocity/x" />
          <xsl:with-param name="shared" select="velocity/x" />
        </xsl:call-template>
        <xsl:call-template name="member-axis">
          <xsl:with-param name="list" select="'speedsY'" />
          <xsl:with-param name="own" select="member/velocity/y" />
          <xsl:with-param name="shared" select="velocity/y" />
        </xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>, one of them: where it starts, and starts again after a &lt;reset /&gt; (any &lt;random&gt; drawn anew)
void start</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>(std::size_t i)
{
</xsl:text>
    <xsl:for-each select="exsl:node-set($body)/list">
      <xsl:value-of select="concat('&#9;const float ', @name, '[] = {', ., '};&#10;')" />
    </xsl:for-each>
    <xsl:variable name="axes" select="exsl:node-set($body)/axis" />
    <xsl:value-of select="concat('&#9;', $name, '[i].setPosition({', $axes[1], ', ', $axes[2], '});&#10;')" />
    <xsl:if test="$moves">
      <xsl:value-of select="concat('&#9;', $name, 'Velocity[i] = {', $axes[3], ', ', $axes[4], '};&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $aimers) = count($aimers)">
      <xsl:value-of select="concat('&#9;', $name, 'Aimed[i] = false;&#10;')" />
    </xsl:if>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- One axis of a member's start: <axis> the value (the group's, the same
       for all of them), or, when a member gives its own, <list> the value of
       each and <axis> the one for member i. -->
  <xsl:template name="member-axis">
    <xsl:param name="list" />
    <xsl:param name="own" />
    <xsl:param name="shared" />
    <xsl:choose>
      <xsl:when test="$own">
        <list name="{$list}">
          <xsl:for-each select="member">
            <xsl:if test="position() &gt; 1">, </xsl:if>
            <xsl:variable name="axis" select="local-name($shared | $own[1])" />
            <xsl:call-template name="value-bare"><xsl:with-param name="node" select="(*[local-name() = local-name($own[1]/..)]/*[local-name() = $axis] | $shared[not(current()/*[local-name() = local-name($own[1]/..)]/*[local-name() = $axis])])[1]" /></xsl:call-template>
          </xsl:for-each>
        </list>
        <axis><xsl:value-of select="concat($list, '[i]')" /></axis>
      </xsl:when>
      <xsl:otherwise>
        <axis><xsl:call-template name="value-bare"><xsl:with-param name="node" select="$shared" /></xsl:call-template></axis>
      </xsl:otherwise>
    </xsl:choose>
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
    <xsl:choose>
      <xsl:when test="count(. | $stepped) = count($stepped)">
        <xsl:value-of select="concat('&#9;if (', $name, 'Stepped == sf::Vector2f{}) // while a key moves it, it keeps going that way&#10;&#9;{&#10;&#9;&#9;', $name, 'Velocity = ')" />
        <xsl:call-template name="vector"><xsl:with-param name="node" select="velocity" /></xsl:call-template>
        <xsl:text>;
	}
</xsl:text>
      </xsl:when>
      <xsl:when test="count(. | $moving) = count($moving)">
        <xsl:value-of select="concat('&#9;', $name, 'Velocity = ')" />
        <xsl:call-template name="vector"><xsl:with-param name="node" select="velocity" /></xsl:call-template>
        <xsl:text>;
</xsl:text>
      </xsl:when>
    </xsl:choose>
    <xsl:if test="count(. | $hoppers) = count($hoppers)">
      <xsl:value-of select="concat('&#9;', $name, 'Hop = {};&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $riders) = count($riders)">
      <xsl:value-of select="concat('&#9;', $name, 'Riding = {};&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $jumpers) = count($jumpers)">
      <xsl:value-of select="concat('&#9;', $name, 'JumpFrames = 0;&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $climbers) = count($climbers)">
      <xsl:value-of select="concat('&#9;', $name, 'Climbing = false;&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $leapers) = count($leapers)">
      <xsl:value-of select="concat('&#9;', $name, 'Leaping = false;&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $standers) = count($standers)">
      <xsl:value-of select="concat('&#9;', $name, 'Grounded = false;&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $headed) = count($headed)">
      <xsl:value-of select="concat('&#9;', $name, 'Heading = ')" />
      <xsl:call-template name="value-bare"><xsl:with-param name="node" select="heading" /></xsl:call-template>
      <xsl:text>;
	turn</xsl:text>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:if>
    <xsl:if test="count(. | $aimers) = count($aimers)">
      <xsl:value-of select="concat('&#9;', $name, 'Aimed = false;&#10;')" />
    </xsl:if>
    <xsl:if test="count(. | $animated) = count($animated)">
      <xsl:call-template name="start-animation"><xsl:with-param name="indent" select="'&#9;'" /></xsl:call-template>
    </xsl:if>
    <xsl:if test="count(. | $looked) = count($looked)">
      <xsl:text>	</xsl:text>
      <xsl:call-template name="become-call">
        <xsl:with-param name="thing" select="." />
        <xsl:with-param name="sprite"><xsl:call-template name="first-look" /></xsl:with-param>
      </xsl:call-template>
      <xsl:text>
</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The keys of `rest` (an <input>'s button="...") that `input` still holds
       on `state`: those no <input> after it takes, in a later set or the
       state's own, or later in its own list. -->
  <xsl:template name="effective-keys">
    <xsl:param name="input" />
    <xsl:param name="state" />
    <xsl:param name="names" />
    <xsl:param name="rest" />
    <xsl:if test="$rest != ''">
      <xsl:variable name="key">
        <xsl:choose>
          <xsl:when test="contains($rest, ' ')"><xsl:value-of select="substring-before($rest, ' ')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$rest" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="word" select="concat(' ', $key, ' ')" />
      <xsl:variable name="at" select="string-length(substring-before($names, concat(' ', $input/../@name, ' ')))" />
      <xsl:variable name="later" select="$input/following-sibling::input[contains(concat(' ', normalize-space(@button), ' '), $word)]
          | $state/inputs/input[not($input/parent::inputs)][contains(concat(' ', normalize-space(@button), ' '), $word)]
          | /game/states/keys[not($input/parent::inputs)][contains($names, concat(' ', @name, ' '))][string-length(substring-before($names, concat(' ', @name, ' '))) &gt; $at]/input[contains(concat(' ', normalize-space(@button), ' '), $word)]" />
      <xsl:if test="not($later)"><xsl:value-of select="$word" /></xsl:if>
      <xsl:call-template name="effective-keys">
        <xsl:with-param name="input" select="$input" />
        <xsl:with-param name="state" select="$state" />
        <xsl:with-param name="names" select="$names" />
        <xsl:with-param name="rest" select="substring-after($rest, ' ')" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- The test for an input's keys on `state`: key == sf::Keyboard::Key::A
       || ..., or, held, sf::Keyboard::isKeyPressed(...) || .... -->
  <xsl:template name="key-test">
    <xsl:param name="state" />
    <xsl:param name="held" select="false()" />
    <xsl:variable name="keys" select="concat($screen-keys[@name = $state/@name]/input[@id = generate-id(current())]/@keys, ' ')" />
    <xsl:call-template name="key-tests">
      <xsl:with-param name="rest" select="$keys" />
      <xsl:with-param name="held" select="$held" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="key-tests">
    <xsl:param name="rest" />
    <xsl:param name="held" />
    <xsl:variable name="key" select="substring-before($rest, ' ')" />
    <xsl:if test="$key != ''">
      <xsl:variable name="sfml" select="$tables/keys/key[@name = $key]/@sfml" />
      <xsl:choose>
        <xsl:when test="$held"><xsl:value-of select="concat('sf::Keyboard::isKeyPressed(sf::Keyboard::Key::', $sfml, ')')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat('key == sf::Keyboard::Key::', $sfml)" /></xsl:otherwise>
      </xsl:choose>
      <xsl:if test="normalize-space(substring-after($rest, ' ')) != ''"> || </xsl:if>
      <xsl:call-template name="key-tests">
        <xsl:with-param name="rest" select="substring-after($rest, ' ')" />
        <xsl:with-param name="held" select="$held" />
      </xsl:call-template>
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
        <xsl:for-each select="$states[@name = $screen-keys[input[@pressed = 'yes']]/@name]">
          <xsl:text>	case Screen::</xsl:text>
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>:
</xsl:text>
          <xsl:call-template name="pressed-keys"><xsl:with-param name="indent" select="'&#9;&#9;'" /></xsl:call-template>
          <xsl:text>		break;
</xsl:text>
        </xsl:for-each>
        <xsl:if test="$states[not(@name = $screen-keys[input[@pressed = 'yes']]/@name)]">
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
    <xsl:variable name="state" select="." />
    <xsl:for-each select="$pressed-inputs[generate-id() = $screen-keys[@name = $state/@name]/input/@id]">
      <xsl:value-of select="$indent" />
      <xsl:if test="position() &gt; 1">else </xsl:if>
      <xsl:text>if (</xsl:text>
      <xsl:call-template name="key-test"><xsl:with-param name="state" select="$state" /></xsl:call-template>
      <xsl:value-of select="concat(')&#10;', $indent, '{&#10;')" />
      <xsl:for-each select="*">
        <xsl:choose>
          <xsl:when test="self::trigger">
            <xsl:call-template name="trigger-action"><xsl:with-param name="indent" select="$indent" /></xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="game-command"><xsl:with-param name="indent" select="concat($indent, '&#9;')" /></xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:value-of select="concat($indent, '}&#10;')" />
    </xsl:for-each>
  </xsl:template>

  <!-- A <trigger> of an object's action that is pressed, on a key or in a
       condition or a timer (`indent` one less than its commands): what it
       does. A <hop> is asked for, and made as the frame's move begins, if it
       stays in the window; a <jump> is
       begun, if it lands in the window and it is not in the air already; a
       <reset /> puts the object back as it started. -->
  <xsl:template name="trigger-action">
    <xsl:param name="indent" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <xsl:variable name="dies" select="count($game/objects/object[@name = current()/@object] | $dying) = count($dying)" />
    <xsl:variable name="in" select="concat($indent, '&#9;', substring('&#9;', 1, number($dies)))" />
    <xsl:if test="$dies">
      <xsl:value-of select="concat($indent, '&#9;if (', $name, 'Alive)&#10;', $indent, '&#9;{&#10;')" />
    </xsl:if>
    <xsl:for-each select="key('action', concat(@object, '|', @action))/*">
      <xsl:variable name="owner" select="ancestor::object[1]" />
      <xsl:choose>
        <xsl:when test="self::move or self::turn or self::thrust or self::accelerate or self::climb">
          <!-- held, in the screen's update; a move pressed turns it -->
          <xsl:if test="self::move">
            <xsl:call-template name="turned-by-key"><xsl:with-param name="owner" select="$owner" /><xsl:with-param name="name" select="$name" /><xsl:with-param name="indent" select="$in" /></xsl:call-template>
          </xsl:if>
        </xsl:when>
        <xsl:when test="self::hop">
          <xsl:value-of select="concat($in, $name, 'Hop = ')" />
          <xsl:call-template name="direction"><xsl:with-param name="node" select="." /></xsl:call-template>
          <xsl:text>; // made as the move begins, if it stays on the screen
</xsl:text>
          <xsl:call-template name="turned-by-key"><xsl:with-param name="owner" select="$owner" /><xsl:with-param name="name" select="$name" /><xsl:with-param name="indent" select="$in" /></xsl:call-template>
        </xsl:when>
        <xsl:when test="self::jump">
          <xsl:value-of select="concat($in, 'physics::jump(', $name, ', ')" />
          <xsl:call-template name="direction">
            <xsl:with-param name="node" select="." />
            <xsl:with-param name="amount" select="distance" />
          </xsl:call-template>
          <xsl:text>, framesFor(</xsl:text>
          <xsl:choose>
            <xsl:when test="seconds"><xsl:call-template name="value-bare"><xsl:with-param name="node" select="seconds" /></xsl:call-template></xsl:when>
            <xsl:otherwise>0.3f</xsl:otherwise>
          </xsl:choose>
          <xsl:value-of select="concat('), ', $name, 'JumpStep, ', $name, 'JumpFrames, windowArea);&#10;')" />
          <xsl:call-template name="turned-by-key"><xsl:with-param name="owner" select="$owner" /><xsl:with-param name="name" select="$name" /><xsl:with-param name="indent" select="$in" /></xsl:call-template>
        </xsl:when>
        <xsl:when test="self::leap">
          <xsl:variable name="climbs" select="count(ancestor::object[1] | $climbers) = count($climbers)" />
          <xsl:value-of select="concat($in, '// up off the ground, as fast as its pull takes ', normalize-space(.), ' to stop&#10;')" />
          <xsl:value-of select="concat($in, 'if (', $name, 'Grounded')" />
          <xsl:if test="$climbs"><xsl:value-of select="concat(' &amp;&amp; !', $name, 'Climbing')" /></xsl:if>
          <xsl:value-of select="concat(')&#10;', $in, '{&#10;')" />
          <xsl:value-of select="concat($in, '&#9;', $name, 'Velocity.y = -std::sqrt(2.0f * ', $name, 'Acceleration.y * ')" />
          <xsl:call-template name="value" />
          <xsl:value-of select="concat(');&#10;', $in, '&#9;', $name, 'Grounded = false;&#10;', $in, '&#9;', $name, 'Leaping = true;&#10;', $in, '}&#10;')" />
        </xsl:when>
        <xsl:when test="self::fire">
          <xsl:call-template name="fire">
            <xsl:with-param name="shooter" select="$name" />
            <xsl:with-param name="from" select="ancestor::object[1]" />
            <xsl:with-param name="indent" select="$in" />
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="self::reset[not(@object)]">
          <xsl:value-of select="$in" />
          <xsl:call-template name="start-call"><xsl:with-param name="self" select="ancestor::object[1]" /><xsl:with-param name="name" select="$name" /></xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="common-command">
            <xsl:with-param name="indent" select="$in" />
            <xsl:with-param name="self" select="$owner" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:if test="$dies">
      <xsl:value-of select="concat($indent, '&#9;}&#10;')" />
    </xsl:if>
  </xsl:template>

  <!-- A key that moves, hops or jumps something: it faces that way, and an
       aim it had is forgotten. -->
  <xsl:template name="turned-by-key">
    <xsl:param name="owner" />
    <xsl:param name="name" />
    <xsl:param name="indent" />
    <xsl:if test="count($owner | $facers) = count($facers)">
      <xsl:value-of select="concat($indent, $name, 'Facing = physics::Facing::', translate(substring(@direction, 1, 1), 'udlr', 'UDLR'), substring(@direction, 2), ';&#10;')" />
    </xsl:if>
    <xsl:if test="count($owner | $aimers) = count($aimers)">
      <xsl:value-of select="concat($indent, $name, 'Aimed = false;&#10;')" />
    </xsl:if>
  </xsl:template>

  <!-- The way something faces at the start, as C++ (physics::Facing::Left). -->
  <xsl:template name="facing-value">
    <xsl:variable name="facing" select="normalize-space(facing)" />
    <xsl:value-of select="concat('physics::Facing::', translate(substring($facing, 1, 1), 'udlr', 'UDLR'), substring($facing, 2))" />
  </xsl:template>

  <!-- A <fire>: the projectile (the first of a group out of play) put at the
       middle of the shooter's top, its own middle over the shooter's, set off
       at its own velocity; or, from a shooter with a <facing>, at the middle
       of the side it faces, just clear of it, that way at its own speed.
       Nothing if it is already out. `from` is the shooter's object or group. -->
  <xsl:template name="fire">
    <xsl:param name="shooter" />
    <xsl:param name="from" select="/.." />
    <xsl:param name="indent" />
    <xsl:variable name="projectile" select="$things[@name = current()/@object]" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <!-- the pool's count: i, or k when the shooter is a member of a group -->
    <xsl:variable name="k">
      <xsl:choose>
        <xsl:when test="contains($shooter, '[')">k</xsl:when>
        <xsl:otherwise>i</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$projectile/self::group">
        <xsl:variable name="velocity">
          <xsl:choose>
            <xsl:when test="count($projectile | $member-velocities) = count($member-velocities)"><xsl:value-of select="concat($name, 'Velocity[', $k, ']')" /></xsl:when>
            <xsl:otherwise><xsl:value-of select="concat($name, 'Velocity')" /></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat($indent, '// fire the first of ', @object, ' that is not out already&#10;')" />
        <xsl:value-of select="concat($indent, 'for (std::size_t ', $k, ' = 0; ', $k, ' &lt; ', $name, '.size(); ++', $k, ')&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;if (!', $name, 'Alive[', $k, '])&#10;', $indent, '&#9;{&#10;')" />
        <xsl:call-template name="launch">
          <xsl:with-param name="shooter" select="$shooter" />
          <xsl:with-param name="from" select="$from" />
          <xsl:with-param name="name" select="concat($name, '[', $k, ']')" />
          <xsl:with-param name="velocity" select="$velocity" />
          <xsl:with-param name="alive" select="concat($name, 'Alive[', $k, ']')" />
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
          <xsl:with-param name="from" select="$from" />
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
    <xsl:param name="from" />
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="alive" />
    <xsl:param name="projectile" />
    <xsl:param name="indent" />
    <xsl:variable name="facing" select="normalize-space($from/facing)" />
    <xsl:variable name="aims" select="count($from | $aimers) = count($aimers)" />
    <xsl:variable name="in" select="concat($indent, substring('&#9;', 1, number($aims)))" />
    <xsl:if test="$aims">
      <xsl:variable name="bare" select="substring-before(concat($shooter, '['), '[')" />
      <xsl:variable name="at" select="substring($shooter, string-length($bare) + 1)" />
      <xsl:value-of select="concat($indent, 'if (', $bare, 'Aimed', $at, ')&#10;', $indent, '{&#10;')" />
      <xsl:value-of select="concat($indent, '&#9;physics::fireAlong(', $shooter, ', ', $name, ', ', $velocity, ', ', $bare, 'Aim', $at, ', sf::Vector2f')" />
      <xsl:call-template name="vector"><xsl:with-param name="node" select="$projectile/velocity" /></xsl:call-template>
      <xsl:value-of select="concat('.length());&#10;', $indent, '}&#10;', $indent, 'else&#10;', $indent, '{&#10;')" />
    </xsl:if>
    <xsl:choose>
      <xsl:when test="$from/heading">
        <xsl:value-of select="concat($in, 'physics::fireAhead(', $shooter, ', ', $name, ', ', $velocity, ', ', $shooter, 'Heading, sf::Vector2f')" />
        <xsl:call-template name="vector"><xsl:with-param name="node" select="$projectile/velocity" /></xsl:call-template>
        <xsl:text>.length());
</xsl:text>
      </xsl:when>
      <xsl:when test="$facing != ''">
        <xsl:value-of select="concat($in, 'physics::fireFrom(', $shooter, ', ', $name, ', ', $velocity, ', ')" />
        <xsl:choose>
          <xsl:when test="count($from | $facers) = count($facers)">
            <xsl:variable name="bare" select="substring-before(concat($shooter, '['), '[')" />
            <xsl:value-of select="concat($bare, 'Facing', substring($shooter, string-length($bare) + 1))" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('physics::Facing::', translate(substring($facing, 1, 1), 'udlr', 'UDLR'), substring($facing, 2))" />
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>, sf::Vector2f</xsl:text>
        <xsl:call-template name="vector"><xsl:with-param name="node" select="$projectile/velocity" /></xsl:call-template>
        <xsl:text>.length());
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($in, $name, '.setPosition({physics::left(', $shooter, ') + physics::width(', $shooter, ') / 2.0f - physics::width(', $name, ') / 2.0f, physics::top(', $shooter, ')});&#10;')" />
        <xsl:if test="count($projectile | $moving) = count($moving)">
          <xsl:value-of select="concat($in, $velocity, ' = ')" />
          <xsl:call-template name="vector"><xsl:with-param name="node" select="$projectile/velocity" /></xsl:call-template>
          <xsl:text>;
</xsl:text>
        </xsl:if>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$aims">
      <xsl:value-of select="concat($indent, '}&#10;')" />
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
      <xsl:when test="self::reset[not(@object)]">
        <xsl:value-of select="concat($indent, 'start();&#10;')" />
      </xsl:when>
      <xsl:when test="self::trigger">
        <xsl:call-template name="trigger-action"><xsl:with-param name="indent" select="substring($indent, 2)" /></xsl:call-template>
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
      <xsl:when test="self::reset">
        <xsl:value-of select="concat($indent, 'reset')" />
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@object" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:when>
      <xsl:when test="self::reveal">
        <xsl:call-template name="reveal"><xsl:with-param name="indent" select="$indent" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="self::play">
        <xsl:value-of select="concat($indent, 'audio.play(')" />
        <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="@sound" /></xsl:call-template>
        <xsl:text>);
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

  <!-- What a key held does each frame to the object `name`: a <move> adds to
       its steps (the velocity the keys give it), a <turn> turns its heading,
       a <thrust> along its heading and an <accelerate> one way go into its
       velocity, burning a unit of its variable burn=
       if it says one, and doing nothing with none left. -->
  <xsl:template name="held-command">
    <xsl:param name="name" />
    <xsl:param name="indent" />
    <xsl:variable name="change">
      <xsl:choose>
        <xsl:when test="self::move">
          <xsl:value-of select="concat($name, 'Steps.')" />
          <xsl:choose>
            <xsl:when test="@direction = 'up' or @direction = 'down'">y</xsl:when>
            <xsl:otherwise>x</xsl:otherwise>
          </xsl:choose>
          <xsl:choose>
            <xsl:when test="@direction = 'left' or @direction = 'up'"> -= </xsl:when>
            <xsl:otherwise> += </xsl:otherwise>
          </xsl:choose>
          <xsl:call-template name="value-bare" />
          <xsl:text>;</xsl:text>
        </xsl:when>
        <xsl:when test="self::climb">
          <xsl:value-of select="concat($name, 'Climb')" />
          <xsl:choose>
            <xsl:when test="@direction = 'up'"> -= </xsl:when>
            <xsl:otherwise> += </xsl:otherwise>
          </xsl:choose>
          <xsl:call-template name="value-bare" />
          <xsl:text>;</xsl:text>
        </xsl:when>
        <xsl:when test="self::turn">
          <xsl:value-of select="concat($name, 'Heading')" />
          <xsl:choose>
            <xsl:when test="@direction = 'left'"> -= </xsl:when>
            <xsl:otherwise> += </xsl:otherwise>
          </xsl:choose>
          <xsl:call-template name="value-bare" />
          <xsl:value-of select="concat(';&#10;', $indent, 'turn')" />
          <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="../../../@name" /></xsl:call-template>
          <xsl:text>();</xsl:text>
        </xsl:when>
        <xsl:when test="self::thrust">
          <xsl:value-of select="concat($name, 'Velocity += physics::ahead(', $name, 'Heading) * ')" />
          <xsl:call-template name="value" />
          <xsl:text>;</xsl:text>
        </xsl:when>
        <xsl:when test="self::accelerate">
          <xsl:value-of select="concat($name, 'Velocity.')" />
          <xsl:choose>
            <xsl:when test="@direction = 'up' or @direction = 'down'">y</xsl:when>
            <xsl:otherwise>x</xsl:otherwise>
          </xsl:choose>
          <xsl:choose>
            <xsl:when test="@direction = 'up' or @direction = 'left'"> -= </xsl:when>
            <xsl:otherwise> += </xsl:otherwise>
          </xsl:choose>
          <xsl:call-template name="value-bare" />
          <xsl:text>;</xsl:text>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="normalize-space(@burn) != ''">
        <xsl:variable name="fuel">
          <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="concat(../../../@name, '.', normalize-space(@burn))" /></xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat($indent, '// burning ', normalize-space(@burn), ', while there is any&#10;')" />
        <xsl:value-of select="concat($indent, 'if (', $fuel, ' &gt; 0.0f)&#10;', $indent, '{&#10;', $indent, '&#9;', $fuel, ' -= 1.0f;&#10;')" />
        <xsl:call-template name="show-numbers">
          <xsl:with-param name="variable" select="concat(../../../@name, '.', normalize-space(@burn))" />
          <xsl:with-param name="indent" select="concat($indent, '&#9;')" />
        </xsl:call-template>
        <xsl:value-of select="concat($indent, '&#9;', $change, '&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($indent, $change, '&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The numbers that show `variable` (paddle1.score), shown again. -->
  <xsl:template name="show-numbers">
    <xsl:param name="variable" />
    <xsl:param name="indent" />
    <xsl:for-each select="$numbers[normalize-space(sprite/text/number) = $variable]">
      <xsl:value-of select="concat($indent, 'show')" />
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>();
</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- A <reveal>: the first of those it names that are out of play (as many
       as it says, one if it does not) back in, where they started and at
       their own velocity. -->
  <xsl:template name="reveal">
    <xsl:param name="indent" />
    <xsl:variable name="target" select="$things[@name = current()/@object]" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <xsl:variable name="title"><xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$target/self::group">
        <xsl:variable name="count">
          <xsl:choose>
            <xsl:when test="not(*) and normalize-space(.) = ''">1</xsl:when>
            <xsl:when test="not(*) and string(number(.)) = normalize-space(.) and floor(number(.)) = number(.)"><xsl:value-of select="number(.)" /></xsl:when>
            <xsl:otherwise>
              <xsl:text>static_cast&lt;std::size_t&gt;(std::lround(</xsl:text>
              <xsl:call-template name="value-bare" />
              <xsl:text>))</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat($indent, '// bring back ', $count, ' of ', @object, ' out of play&#10;')" />
        <xsl:value-of select="concat($indent, 'for (std::size_t k = 0, brought = 0; k &lt; ', $name, '.size() &amp;&amp; brought &lt; ', $count, '; ++k)&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;if (!', $name, 'Alive[k])&#10;', $indent, '&#9;{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;&#9;start', $title, '(k);&#10;', $indent, '&#9;&#9;', $name, 'Alive[k] = true;&#10;', $indent, '&#9;&#9;++brought;&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;}&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($indent, '// bring back ', @object, ', if it is out of play&#10;')" />
        <xsl:value-of select="concat($indent, 'if (!', $name, 'Alive)&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;start', $title, '();&#10;', $indent, '&#9;', $name, 'Alive = true;&#10;', $indent, '}&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A <release>: the first of those it names that are out of play (as many
       as it says, one if it does not) brought back in at their own start's
       velocity, each centred on `from`. -->
  <xsl:template name="release">
    <xsl:param name="from" />
    <xsl:param name="indent" />
    <xsl:variable name="target" select="$things[@name = current()/@object]" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <xsl:variable name="title"><xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$target/self::group">
        <xsl:variable name="count">
          <xsl:choose>
            <xsl:when test="not(*) and normalize-space(.) = ''">1</xsl:when>
            <xsl:when test="not(*) and string(number(.)) = normalize-space(.) and floor(number(.)) = number(.)"><xsl:value-of select="number(.)" /></xsl:when>
            <xsl:otherwise>
              <xsl:text>static_cast&lt;std::size_t&gt;(std::lround(</xsl:text>
              <xsl:call-template name="value-bare" />
              <xsl:text>))</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat($indent, '// release ', $count, ' of ', @object, ' from where it is&#10;')" />
        <xsl:value-of select="concat($indent, 'for (std::size_t k = 0, released = 0; k &lt; ', $name, '.size() &amp;&amp; released &lt; ', $count, '; ++k)&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;if (!', $name, 'Alive[k])&#10;', $indent, '&#9;{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;&#9;start', $title, '(k);&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;&#9;', $name, '[k].setPosition(', $from, '.getGlobalBounds().getCenter() - ', $name, '[k].getGlobalBounds().size / 2.0f);&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;&#9;', $name, 'Alive[k] = true;&#10;', $indent, '&#9;&#9;++released;&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;}&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($indent, '// release ', @object, ' from where it is, if it is out of play&#10;')" />
        <xsl:value-of select="concat($indent, 'if (!', $name, 'Alive)&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;start', $title, '();&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;', $name, '.setPosition(', $from, '.getGlobalBounds().getCenter() - ', $name, '.getGlobalBounds().size / 2.0f);&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;', $name, 'Alive = true;&#10;', $indent, '}&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What an object does to itself (in a timer, or in a rule after what is
       only for rules): `name` the object or one member, with its velocity and
       its flag for being in play; `self` the object or group. Anything else is
       about the game. -->
  <xsl:template name="self-command">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="alive" />
    <xsl:param name="self" />
    <xsl:param name="indent" />
    <xsl:choose>
      <xsl:when test="self::reset[not(@object)]">
        <xsl:value-of select="$indent" />
        <xsl:call-template name="start-call"><xsl:with-param name="self" select="$self" /><xsl:with-param name="name" select="$name" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="self::die">
        <xsl:value-of select="concat($indent, $alive, ' = false;&#10;')" />
        <xsl:if test="count($self | $followers) = count($followers)">
          <xsl:value-of select="concat($indent, substring-before($alive, 'Alive'), 'Flight', substring-after($alive, 'Alive'), ' = {}; // and off its path&#10;')" />
        </xsl:if>
      </xsl:when>
      <xsl:when test="self::follow">
        <xsl:call-template name="follow-command">
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="velocity" select="$velocity" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::reverse">
        <xsl:value-of select="concat($indent, $velocity, ' = -', $velocity, ';&#10;')" />
      </xsl:when>
      <xsl:when test="self::stop">
        <xsl:value-of select="concat($indent, $velocity, ' = {};&#10;')" />
        <xsl:if test="count($self | $followers) = count($followers)">
          <xsl:value-of select="concat($indent, substring-before(concat($velocity, '['), 'Velocity'), 'Flight', substring-after($velocity, 'Velocity'), ' = {}; // and off its path&#10;')" />
        </xsl:if>
        <xsl:if test="count($self | $pulled) = count($pulled)">
          <xsl:value-of select="concat($indent, substring-before(concat($velocity, '['), 'Velocity'), 'Acceleration = {}; // and its pull, until it is reset&#10;')" />
        </xsl:if>
      </xsl:when>
      <xsl:when test="self::release">
        <xsl:call-template name="release">
          <xsl:with-param name="from" select="$name" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::aim">
        <xsl:variable name="bare" select="substring-before(concat($name, '['), '[')" />
        <xsl:variable name="at" select="substring($name, string-length($bare) + 1)" />
        <xsl:value-of select="concat($indent, '// aim at the nearest ', @object, ' in play&#10;')" />
        <xsl:value-of select="concat($indent, 'if (const std::optional&lt;sf::Vector2f&gt; way = physics::aimAt(', $name, ', ')" />
        <xsl:call-template name="nearest-call"><xsl:with-param name="from" select="$name" /></xsl:call-template>
        <xsl:value-of select="concat('))&#10;', $indent, '{&#10;')" />
        <xsl:value-of select="concat($indent, '&#9;', $bare, 'Aim', $at, ' = *way;&#10;', $indent, '&#9;', $bare, 'Aimed', $at, ' = true;&#10;')" />
        <xsl:if test="count($self | $facers) = count($facers)">
          <xsl:value-of select="concat($indent, '&#9;', $bare, 'Facing', $at, ' = physics::facingOf(*way);&#10;')" />
        </xsl:if>
        <xsl:value-of select="concat($indent, '}&#10;')" />
      </xsl:when>
      <xsl:when test="self::chase">
        <xsl:variable name="faces" select="count($self | $facers) = count($facers)" />
        <xsl:value-of select="$indent" />
        <xsl:if test="$faces">if (const std::optional&lt;sf::Vector2f&gt; way = </xsl:if>
        <xsl:value-of select="concat('physics::chase(', $name, ', ', $velocity, ', ')" />
        <xsl:call-template name="nearest-call"><xsl:with-param name="from" select="$name" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="speed" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:choose>
          <xsl:when test="near"><xsl:call-template name="value"><xsl:with-param name="node" select="near" /></xsl:call-template></xsl:when>
          <xsl:otherwise>0.0f</xsl:otherwise>
        </xsl:choose>
        <xsl:choose>
          <xsl:when test="$faces">
            <xsl:variable name="bare" select="substring-before(concat($name, '['), '[')" />
            <xsl:value-of select="concat(')) // the nearest ', @object, ' in play&#10;', $indent, '{&#10;', $indent, '&#9;', $bare, 'Facing', substring($name, string-length($bare) + 1), ' = physics::facingOf(*way);&#10;', $indent, '}&#10;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('); // the nearest ', @object, ' in play&#10;')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="self::move and count($self | $lockstep) = count($lockstep)">
        <xsl:variable name="group"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template></xsl:variable>
        <xsl:variable name="type"><xsl:for-each select="$self"><xsl:call-template name="sf-type" /></xsl:for-each></xsl:variable>
        <xsl:value-of select="concat($indent, '// the whole block moves&#10;', $indent, 'for (', $type, '&amp; each : ', $group, ')&#10;', $indent, '{&#10;', $indent, '&#9;each.move(')" />
        <xsl:call-template name="direction"><xsl:with-param name="node" select="." /></xsl:call-template>
        <xsl:value-of select="concat(');&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:when test="self::move">
        <xsl:value-of select="concat($indent, $name, '.move(')" />
        <xsl:call-template name="direction"><xsl:with-param name="node" select="." /></xsl:call-template>
        <xsl:text>);
</xsl:text>
      </xsl:when>
      <xsl:when test="self::fire">
        <xsl:call-template name="fire">
          <xsl:with-param name="shooter" select="$name" />
          <xsl:with-param name="from" select="$self" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::push or self::pop">
        <xsl:call-template name="game-command"><xsl:with-param name="indent" select="$indent" /></xsl:call-template>
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

  <!-- A <timer>: counted down each frame, worked out again when it is 0, and
       its commands when it gets to 0 (an <after> once: then -1, done). For an
       object's, `name` the object or member i, and so on as self-command;
       for a screen's, none. `out` leaves what is left of the frame after a
       command that takes it out of play or changes the screen. -->
  <xsl:template name="timer">
    <xsl:param name="indent" />
    <xsl:param name="name" select="''" />
    <xsl:param name="velocity" select="''" />
    <xsl:param name="alive" select="''" />
    <xsl:param name="self" select="/.." />
    <xsl:param name="out" select="''" />
    <xsl:variable name="count">
      <xsl:call-template name="timer-name" />
      <xsl:if test="../../self::group">[i]</xsl:if>
    </xsl:variable>
    <xsl:value-of select="$indent" />
    <xsl:if test="not($self)">// this screen's timer </xsl:if>
    <xsl:if test="$self">// timer </xsl:if>
    <xsl:value-of select="count(preceding-sibling::timer) + 1" />
    <xsl:if test="after">, once</xsl:if>
    <xsl:text>:</xsl:text>
    <xsl:for-each select="*[not(self::every or self::after)]">
      <xsl:value-of select="concat(' ', local-name())" />
    </xsl:for-each>
    <xsl:value-of select="concat('&#10;', $indent, 'if (', $count, ' == 0)&#10;', $indent, '{&#10;', $indent, '&#9;', $count, ' = framesFor(')" />
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="every | after" /></xsl:call-template>
    <xsl:value-of select="concat(');&#10;', $indent, '}&#10;', $indent, 'if (')" />
    <xsl:if test="after"><xsl:value-of select="concat($count, ' &gt; 0 &amp;&amp; ')" /></xsl:if>
    <xsl:value-of select="concat('--', $count, ' == 0)&#10;', $indent, '{&#10;')" />
    <xsl:if test="after">
      <xsl:value-of select="concat($indent, '&#9;', $count, ' = -1;&#10;')" />
    </xsl:if>
    <xsl:for-each select="*[not(self::every or self::after)]">
      <xsl:choose>
        <xsl:when test="$self">
          <xsl:call-template name="self-command">
            <xsl:with-param name="name" select="$name" />
            <xsl:with-param name="velocity" select="$velocity" />
            <xsl:with-param name="alive" select="$alive" />
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="indent" select="concat($indent, '&#9;')" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="game-command"><xsl:with-param name="indent" select="concat($indent, '&#9;')" /></xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:if test="$out != ''">
      <xsl:value-of select="concat($indent, '&#9;', $out, ';&#10;')" />
    </xsl:if>
    <xsl:value-of select="concat($indent, '}&#10;')" />
  </xsl:template>

  <!-- An object with a heading: the heading kept from 0 up to 360, and its
       picture drawn again at it when the whole degree changes. -->
  <xsl:template name="define-turn">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="heading" select="concat($name, 'Heading')" />
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>: its heading kept from 0 up to 360, and its picture drawn again at it
// when the whole degree changes, as the engine draws it
void turn</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>()
{
</xsl:text>
    <xsl:value-of select="concat('&#9;', $heading, ' = std::fmod(', $heading, ', 360.0f);&#10;')" />
    <xsl:value-of select="concat('&#9;if (', $heading, ' &lt; 0.0f)&#10;&#9;{&#10;&#9;&#9;', $heading, ' += 360.0f;&#10;&#9;}&#10;')" />
    <xsl:value-of select="concat('&#9;const int degrees = xge::wholeDegrees(', $heading, ');&#10;')" />
    <xsl:value-of select="concat('&#9;if (degrees != ', $name, 'DrawnAt)&#10;&#9;{&#10;&#9;&#9;', $name, 'DrawnAt = degrees;&#10;')" />
    <xsl:text>		const sf::Image picture = </xsl:text>
    <xsl:for-each select="sprite">
      <xsl:choose>
        <xsl:when test="line">
          <xsl:value-of select="concat('imageOf(xge::rasterizeTurned(', $name, 'Lines, static_cast&lt;float&gt;(degrees)));&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>imageOf(xge::turnBitmap(</xsl:text>
          <xsl:call-template name="rows-picture"><xsl:with-param name="rows" select="$name" /></xsl:call-template>
          <xsl:text>, static_cast&lt;float&gt;(degrees)));&#10;</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:value-of select="concat('&#9;&#9;', $name, 'Picture.update(picture);&#10;')" />
    <xsl:if test="count(. | $pixel-things) = count($pixel-things)">
      <xsl:value-of select="concat('&#9;&#9;', $name, 'Pixels = picture;&#10;')" />
    </xsl:if>
    <xsl:text>	}
}
</xsl:text>
  </xsl:template>

  <!-- An object's timers, each frame it is shown and in play; a group's, for
       each of them. -->
  <xsl:template name="define-tick">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="boolean(self::group)" />
    <xsl:variable name="self" select="." />
    <xsl:variable name="dies" select="count(. | $dying) = count($dying)" />
    <xsl:variable name="one">
      <xsl:value-of select="$name" />
      <xsl:if test="$group">[i]</xsl:if>
    </xsl:variable>
    <xsl:variable name="velocity">
      <xsl:if test="count(. | $moving) = count($moving)">
        <xsl:value-of select="concat($name, 'Velocity')" />
        <xsl:if test="count(. | $member-velocities) = count($member-velocities)">[i]</xsl:if>
      </xsl:if>
    </xsl:variable>
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
    <xsl:variable name="indent">
      <xsl:text>&#9;</xsl:text>
      <xsl:if test="$group"><xsl:text>&#9;</xsl:text></xsl:if>
    </xsl:variable>
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:if test="$group">, every one of them</xsl:if>
    <xsl:text>: its timers, each frame it is shown</xsl:text>
    <xsl:if test="$dies"> and in play</xsl:if>
    <xsl:text>
void tick</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>()
{
</xsl:text>
    <xsl:if test="$group">
      <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;')" />
    </xsl:if>
    <xsl:if test="$dies">
      <xsl:value-of select="concat($indent, 'if (!', $alive, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $out, ';&#10;', $indent, '}&#10;')" />
    </xsl:if>
    <xsl:for-each select="timers/timer">
      <xsl:if test="position() &gt; 1 or $dies"><xsl:text>&#10;</xsl:text></xsl:if>
      <xsl:call-template name="timer">
        <xsl:with-param name="indent" select="$indent" />
        <xsl:with-param name="name" select="$one" />
        <xsl:with-param name="velocity" select="$velocity" />
        <xsl:with-param name="alive" select="$alive" />
        <xsl:with-param name="self" select="$self" />
        <xsl:with-param name="out" select="substring($out, 1, string-length($out) * number(boolean(die) and position() != last()))" />
      </xsl:call-template>
    </xsl:for-each>
    <xsl:if test="$group">
      <xsl:text>	}
</xsl:text>
    </xsl:if>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- A screen's frame: the keys held, the objects it shows that do something,
       then its conditions. -->
  <xsl:template name="generate-screen-update">
    <xsl:variable name="state" select="." />
    <xsl:variable name="blocks">
      <!-- each action once a frame, however many of its keys are held, as
           the engine holds an action rather than a key -->
      <xsl:variable name="held-here" select="$held-inputs[generate-id() = $screen-keys[@name = $state/@name]/input/@id]" />
      <xsl:variable name="walking" select="($stepped | $climbers)[@name = $held-here/trigger/@object]" />
      <xsl:if test="$walking">
        <xsl:text>&#10;</xsl:text>
      </xsl:if>
      <xsl:for-each select="$walking">
        <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
        <xsl:if test="count(. | $stepped) = count($stepped)">
          <xsl:value-of select="concat('&#9;', $name, 'Steps = {}; // and the keys held below&#10;')" />
        </xsl:if>
        <xsl:if test="count(. | $climbers) = count($climbers)">
          <xsl:value-of select="concat('&#9;', $name, 'Climb = 0.0f;&#10;')" />
        </xsl:if>
      </xsl:for-each>
      <xsl:for-each select="$held-here/trigger">
        <xsl:variable name="trigger" select="." />
        <xsl:variable name="keyed" select="$held-here[trigger[@object = $trigger/@object][@action = $trigger/@action]]" />
        <xsl:if test="count($keyed[1] | ..) = 1 and not(preceding-sibling::trigger[@object = $trigger/@object][@action = $trigger/@action])">
        <xsl:text>
	if (</xsl:text>
        <xsl:for-each select="$keyed">
          <xsl:if test="position() &gt; 1"> || </xsl:if>
          <xsl:call-template name="key-test">
            <xsl:with-param name="state" select="$state" />
            <xsl:with-param name="held" select="true()" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:text>)
	{
</xsl:text>
          <xsl:variable name="object" select="/game/objects/object[@name = current()/@object]" />
          <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
          <xsl:variable name="dies" select="count($object | $dying) = count($dying)" />
          <xsl:if test="$dies">
            <xsl:value-of select="concat('&#9;&#9;if (', $name, 'Alive)&#10;&#9;&#9;{&#10;')" />
          </xsl:if>
          <xsl:for-each select="$object/actions/action[@name = current()/@action]/*">
            <xsl:call-template name="held-command">
              <xsl:with-param name="name" select="$name" />
              <xsl:with-param name="indent" select="concat('&#9;&#9;', substring('&#9;', 1, number($dies)))" />
            </xsl:call-template>
          </xsl:for-each>
          <xsl:if test="$dies">
            <xsl:text>		}
</xsl:text>
          </xsl:if>
        <xsl:text>	}
</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:variable name="steering" select="$stepped[@name = $held-here/trigger/@object]" />
      <!-- shown here without its keys: let go, as the engine lets every key
           go when the screen changes -->
      <xsl:variable name="let-go" select="$stepped[@name = $state/shows/show/@object][not(@name = $held-here/trigger/@object)]" />
      <xsl:if test="$steering">
        <xsl:text>
	// a key pressed or let go sets that way of its velocity, as in the engine
</xsl:text>
      </xsl:if>
      <xsl:if test="$let-go and not($steering)">
        <xsl:text>
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$steering | $let-go">
        <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
        <xsl:variable name="steps">
          <xsl:choose>
            <xsl:when test="count(. | $steering) = count($steering)"><xsl:value-of select="concat($name, 'Steps')" /></xsl:when>
            <xsl:otherwise>{}</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;physics::steer(', $name, 'Velocity, ', $steps, ', ', $name, 'Stepped')" />
        <xsl:if test="count(. | $leapers) = count($leapers)"><xsl:value-of select="concat(', !', $name, 'Leaping')" /></xsl:if>
        <xsl:text>);</xsl:text>
        <xsl:if test="count(. | $let-go) = count($let-go)"> // its keys do nothing here: let go</xsl:if>
        <xsl:text>&#10;</xsl:text>
      </xsl:for-each>
      <xsl:variable name="animating" select="$animated[@name = $state/shows/show/@object]" />
      <xsl:if test="$animating">
        <xsl:text>
	// the animations a frame on
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$animating">
        <xsl:text>	</xsl:text>
        <xsl:call-template name="animate-name" />
        <xsl:text>();
</xsl:text>
      </xsl:for-each>
      <xsl:variable name="ticking" select="$timed[@name = $state/shows/show/@object]" />
      <xsl:if test="$ticking">
        <xsl:text>
	// the timers first, before anything moves
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$ticking">
        <xsl:text>	tick</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:for-each>
      <xsl:for-each select="timers/timer">
        <xsl:text>
</xsl:text>
        <xsl:call-template name="timer">
          <xsl:with-param name="indent" select="'&#9;'" />
          <xsl:with-param name="out" select="substring('return', 1, 6 * number(boolean(push or pop or reset[not(@object)])))" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:variable name="flying" select="$followers[@name = $state/shows/show/@object]" />
      <xsl:if test="$flying">
        <xsl:text>
	// the paths, a frame on, before anything moves
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$flying">
        <xsl:text>	</xsl:text>
        <xsl:call-template name="fly-name" />
        <xsl:text>();
</xsl:text>
      </xsl:for-each>
      <xsl:variable name="shows" select="$state/shows/show/@object" />
      <xsl:variable name="pushing" select="$accelerating[@name = $shows]" />
      <xsl:if test="$pushing">
        <xsl:text>
	// what changes how they move (ladders, pulls, drag), before anything moves
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$pushing">
        <xsl:text>	accelerate</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:for-each>
      <xsl:variable name="meeting" select="$edged[@name = $shows]" />
      <xsl:if test="$meeting">
        <xsl:text>
	// the edges of the screen, for what is moving, before it moves
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$meeting">
        <xsl:text>	edges</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:for-each>
      <xsl:if test="count(. | $moving-states) = count($moving-states)">
        <xsl:text>
	// everything moves, from touch to touch
	move</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>();
</xsl:text>
      </xsl:if>
      <xsl:variable name="stuck" select="$sticking[@name = $shows]" />
      <xsl:if test="$stuck">
        <xsl:text>
	// what sticks at an edge kept in, after the move
</xsl:text>
      </xsl:if>
      <xsl:for-each select="$stuck">
        <xsl:text>	stick</xsl:text>
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

  <!-- ===================================================================== -->
  <!-- The frame, in the engine's order (Game::updateObjects)                 -->
  <!-- ===================================================================== -->

  <!-- The context thing, or member `index` of it when it is a group. -->
  <xsl:template name="one-of">
    <xsl:param name="index" />
    <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:if test="self::group"><xsl:value-of select="concat('[', $index, ']')" /></xsl:if>
  </xsl:template>

  <!-- Its velocity (member `index`'s, when each has its own), or nothing
       when it has none. -->
  <xsl:template name="velocity-of">
    <xsl:param name="index" />
    <xsl:if test="count(. | $moving) = count($moving)">
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>Velocity</xsl:text>
      <xsl:if test="count(. | $member-velocities) = count($member-velocities)"><xsl:value-of select="concat('[', $index, ']')" /></xsl:if>
    </xsl:if>
  </xsl:template>

  <!-- How far it goes in a frame, as the engine's motionOf: its velocity and
       the velocity of what it rides. -->
  <xsl:template name="motion-of">
    <xsl:param name="index" />
    <xsl:variable name="velocity"><xsl:call-template name="velocity-of"><xsl:with-param name="index" select="$index" /></xsl:call-template></xsl:variable>
    <xsl:variable name="riding">
      <xsl:if test="count(. | $riders) = count($riders)">
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>Riding</xsl:text>
      </xsl:if>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$velocity != '' and $riding != ''"><xsl:value-of select="concat('(', $velocity, ' + ', $riding, ')')" /></xsl:when>
      <xsl:when test="$velocity != ''"><xsl:value-of select="$velocity" /></xsl:when>
      <xsl:when test="$riding != ''"><xsl:value-of select="$riding" /></xsl:when>
      <xsl:otherwise>sf::Vector2f{}</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Its flag for being in play (member `index`'s), or nothing when it
       cannot be taken out of play. -->
  <xsl:template name="alive-of">
    <xsl:param name="index" />
    <xsl:if test="count(. | $dying) = count($dying)">
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>Alive</xsl:text>
      <xsl:if test="self::group"><xsl:value-of select="concat('[', $index, ']')" /></xsl:if>
    </xsl:if>
  </xsl:template>

  <!-- Whether it can touch anything now, as a C++ condition (in play, and
       not in the middle of a jump), or nothing when it always can. -->
  <xsl:template name="touchable">
    <xsl:param name="index" />
    <xsl:variable name="alive"><xsl:call-template name="alive-of"><xsl:with-param name="index" select="$index" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="$alive" />
    <xsl:if test="count(. | $jumpers) = count($jumpers)">
      <xsl:if test="$alive != ''"> &amp;&amp; </xsl:if>
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>JumpFrames == 0</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The opposite: why it cannot touch anything now, or nothing. -->
  <xsl:template name="untouchable">
    <xsl:param name="index" />
    <xsl:variable name="alive"><xsl:call-template name="alive-of"><xsl:with-param name="index" select="$index" /></xsl:call-template></xsl:variable>
    <xsl:if test="$alive != ''"><xsl:value-of select="concat('!', $alive)" /></xsl:if>
    <xsl:if test="count(. | $jumpers) = count($jumpers)">
      <xsl:if test="$alive != ''"> || </xsl:if>
      <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>JumpFrames != 0</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- A function that does `body` (a line break before each block, the
       first taken off) to the thing, or to each member of a group in turn,
       `out` (return, or continue) leaving one. The first line of `body` is
       about `one`, the object or member i. -->
  <xsl:template name="each-one">
    <xsl:param name="function" />
    <xsl:param name="comment" />
    <xsl:param name="body" />
    <xsl:value-of select="concat('&#10;// ', @name, $comment, '&#10;void ', $function)" />
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>()
{
</xsl:text>
    <xsl:choose>
      <xsl:when test="self::group">
        <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;', substring($body, 2), '&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring($body, 2)" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- What changes how a thing moves this frame, before anything moves, as
       the engine's Game::applyClimbing and Game::applyAcceleration: at a
       ladder, on to it, up or down it, or off it at an end; off one, its pull
       and then its drag. A group's members each in turn; a velocity they
       share is changed once. -->
  <xsl:template name="define-accelerate">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="shared" select="self::group and count(. | $member-velocities) != count($member-velocities)" />
    <xsl:variable name="indent" select="concat('&#9;', substring('&#9;', 1, number(self::group and not($shared))))" />
    <xsl:variable name="one"><xsl:call-template name="one-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="velocity"><xsl:call-template name="velocity-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="alive"><xsl:call-template name="alive-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="out">
      <xsl:choose>
        <xsl:when test="self::group">continue</xsl:when>
        <xsl:otherwise>return</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="climber" select="count(. | $climbers) = count($climbers)" />
    <xsl:variable name="pushes">
      <xsl:if test="count(. | $pulled) = count($pulled)">
        <xsl:value-of select="concat($velocity, ' += ', $name, 'Acceleration;&#10;')" />
      </xsl:if>
      <xsl:if test="drag">
        <xsl:value-of select="concat($velocity, ' *= 1.0f - ')" />
        <xsl:call-template name="value"><xsl:with-param name="node" select="drag" /></xsl:call-template>
        <xsl:text>; // drag: what is left of its speed after this frame&#10;</xsl:text>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="body">
      <xsl:if test="$alive != '' and not($shared)">
        <xsl:value-of select="concat('&#10;', $indent, 'if (!', $alive, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $out, ';&#10;', $indent, '}&#10;')" />
      </xsl:if>
      <xsl:if test="$climber">
        <xsl:value-of select="concat('&#10;', $indent, '// at a ladder: on to it from the ground, up or down it, or off it at an end&#10;', $indent)" />
        <xsl:variable name="call">
          <xsl:value-of select="concat('physics::climb(', $one, ', ', $velocity, ', ', $name, 'Climb, ', $name, 'Climbing, ', $name, 'Grounded, ')" />
          <xsl:choose>
            <xsl:when test="count(. | $stepped) = count($stepped)"><xsl:value-of select="concat($name, 'Steps.x')" /></xsl:when>
            <xsl:otherwise>0.0f</xsl:otherwise>
          </xsl:choose>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="ladder-name"><xsl:with-param name="class" select="actions/action/climb/@class" /></xsl:call-template>
          <xsl:value-of select="concat('(', $one, '))')" />
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="count(. | $leapers) = count($leapers)">
            <xsl:value-of select="concat('if (', $call, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $name, 'Leaping = false; // on to a ladder, out of a leap&#10;', $indent, '}&#10;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($call, ';&#10;')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
      <xsl:if test="string($pushes) != ''">
        <xsl:text>&#10;</xsl:text>
        <xsl:choose>
          <xsl:when test="$climber">
            <xsl:value-of select="concat($indent, 'if (!', $name, 'Climbing) // on a ladder nothing pulls it&#10;', $indent, '{&#10;')" />
            <xsl:call-template name="indent-lines"><xsl:with-param name="text" select="$pushes" /><xsl:with-param name="indent" select="concat($indent, '&#9;')" /></xsl:call-template>
            <xsl:value-of select="concat($indent, '}&#10;')" />
          </xsl:when>
          <xsl:when test="$shared">
            <xsl:value-of select="concat($indent, '// one velocity for them all&#10;')" />
            <xsl:call-template name="indent-lines"><xsl:with-param name="text" select="$pushes" /><xsl:with-param name="indent" select="$indent" /></xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="indent-lines"><xsl:with-param name="text" select="$pushes" /><xsl:with-param name="indent" select="$indent" /></xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:if>
    </xsl:variable>
    <xsl:value-of select="concat('&#10;// ', @name, ', what changes how it moves this frame, before anything moves&#10;void accelerate')" />
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>()
{
</xsl:text>
    <xsl:choose>
      <xsl:when test="self::group and not($shared)">
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;', substring($body, 2), '&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="substring($body, 2)" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- Each line of `text` with `indent` before it. -->
  <xsl:template name="indent-lines">
    <xsl:param name="text" />
    <xsl:param name="indent" />
    <xsl:if test="$text != ''">
      <xsl:value-of select="concat($indent, substring-before($text, '&#10;'), '&#10;')" />
      <xsl:call-template name="indent-lines">
        <xsl:with-param name="text" select="substring-after($text, '&#10;')" />
        <xsl:with-param name="indent" select="$indent" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- A thing at the edges of the screen, before it moves and only while it
       is moving, as the engine's Game::checkEdge: the top, the bottom, the
       left and the right, each it is past with every rule about it in the
       order written (Breakout's ball bounces off every edge, and dies at the
       bottom), a rule with sprite= or unless= looked at as it comes. Nothing
       stops at a <die />: the engine runs the rest. -->
  <xsl:template name="define-edges">
    <xsl:variable name="self" select="." />
    <xsl:variable name="indent" select="concat('&#9;', substring('&#9;', 1, number(boolean(self::group))))" />
    <xsl:variable name="one"><xsl:call-template name="one-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="velocity"><xsl:call-template name="velocity-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="motion"><xsl:call-template name="motion-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="alive"><xsl:call-template name="alive-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="out">
      <xsl:choose>
        <xsl:when test="self::group">continue</xsl:when>
        <xsl:otherwise>return</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="rules-here" select="collisions/collision[@edge][*][count(. | $rules) = count($rules)]" />
    <xsl:variable name="body">
      <xsl:value-of select="concat('&#10;', $indent, 'if (')" />
      <xsl:if test="$alive != ''"><xsl:value-of select="concat('!', $alive, ' || ')" /></xsl:if>
      <xsl:value-of select="concat('!physics::moving(', $motion, '))&#10;', $indent, '{&#10;', $indent, '&#9;', $out, ';&#10;', $indent, '}&#10;')" />
      <xsl:for-each select="document('')//xsl:variable[@name = 'edges']/edge">
        <xsl:variable name="this" select="." />
        <xsl:variable name="side" select="concat('physics::Edge::', @title)" />
        <xsl:variable name="about" select="$rules-here[@edge = $this/@name or @edge = $this/@in or @edge = 'all']" />
        <xsl:if test="$about">
          <xsl:value-of select="concat('&#10;', $indent, '// ', @name, ':')" />
          <xsl:for-each select="$about/*">
            <xsl:value-of select="concat(' ', local-name())" />
          </xsl:for-each>
          <xsl:value-of select="concat('&#10;', $indent, 'if (physics::past(', $one, ', ', $side, ', windowArea))&#10;', $indent, '{&#10;')" />
          <xsl:for-each select="$about">
            <xsl:variable name="guard">
              <xsl:call-template name="rule-guard">
                <xsl:with-param name="rule" select="." />
                <xsl:with-param name="self" select="$self" />
                <xsl:with-param name="name" select="$one" />
              </xsl:call-template>
            </xsl:variable>
            <xsl:variable name="at" select="concat($indent, '&#9;', substring('&#9;', 1, number($guard != '')))" />
            <xsl:if test="$guard != ''">
              <xsl:value-of select="concat($indent, '&#9;if (', $guard, ')&#10;', $indent, '&#9;{&#10;')" />
            </xsl:if>
            <xsl:for-each select="*">
              <xsl:call-template name="edge-command">
                <xsl:with-param name="self" select="$self" />
                <xsl:with-param name="name" select="$one" />
                <xsl:with-param name="velocity" select="$velocity" />
                <xsl:with-param name="motion" select="$motion" />
                <xsl:with-param name="alive" select="$alive" />
                <xsl:with-param name="side" select="$side" />
                <xsl:with-param name="indent" select="$at" />
              </xsl:call-template>
            </xsl:for-each>
            <xsl:if test="$guard != ''">
              <xsl:value-of select="concat($indent, '&#9;}&#10;')" />
            </xsl:if>
          </xsl:for-each>
          <xsl:value-of select="concat($indent, '}&#10;')" />
        </xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:call-template name="each-one">
      <xsl:with-param name="function" select="'edges'" />
      <xsl:with-param name="comment">
        <xsl:if test="self::group">, every one of them</xsl:if>
        <xsl:text>, at the edges of the screen before it moves</xsl:text>
      </xsl:with-param>
      <xsl:with-param name="body" select="$body" />
    </xsl:call-template>
  </xsl:template>

  <!-- The four edges, in the order the engine looks at them, and the
       edge="..." words that take in each. -->
  <xsl:variable name="edges">
    <edge name="top" title="Top" in="vertical" />
    <edge name="bottom" title="Bottom" in="vertical" />
    <edge name="left" title="Left" in="horizontal" />
    <edge name="right" title="Right" in="horizontal" />
  </xsl:variable>

  <!-- What one does at an edge it is past (`side`), as the engine's
       CommandExecutor::executeScreenEdgeCollision: a <bounce /> heads it
       away (a block in lockstep at a side: all of it turns, and steps back),
       a <stick /> keeps it in, a <wrap /> takes it round once it has gone. -->
  <xsl:template name="edge-command">
    <xsl:param name="self" />
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="motion" />
    <xsl:param name="alive" />
    <xsl:param name="side" />
    <xsl:param name="indent" />
    <xsl:choose>
      <xsl:when test="self::bounce and count($self | $lockstep) = count($lockstep) and ($side = 'physics::Edge::Left' or $side = 'physics::Edge::Right')">
        <xsl:variable name="group"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat($indent, '// the whole block turns, and every one of them steps back&#10;')" />
        <xsl:choose>
          <xsl:when test="count($self | $member-velocities) = count($member-velocities)">
            <xsl:value-of select="concat($indent, 'for (std::size_t k = 0; k &lt; ', $group, '.size(); ++k)&#10;', $indent, '{&#10;')" />
            <xsl:value-of select="concat($indent, '&#9;', $group, 'Velocity[k].x = -', $group, 'Velocity[k].x;&#10;')" />
            <xsl:value-of select="concat($indent, '&#9;', $group, '[k].move({', $group, 'Velocity[k].x, 0.0f});&#10;', $indent, '}&#10;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="type"><xsl:for-each select="$self"><xsl:call-template name="sf-type" /></xsl:for-each></xsl:variable>
            <xsl:value-of select="concat($indent, $group, 'Velocity.x = -', $group, 'Velocity.x;&#10;')" />
            <xsl:value-of select="concat($indent, 'for (', $type, '&amp; each : ', $group, ')&#10;', $indent, '{&#10;')" />
            <xsl:value-of select="concat($indent, '&#9;each.move({', $group, 'Velocity.x, 0.0f});&#10;', $indent, '}&#10;')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="self::bounce">
        <xsl:if test="$velocity != ''">
          <xsl:value-of select="concat($indent, 'physics::bounce(', $velocity, ', ', $side, ');&#10;')" />
        </xsl:if>
      </xsl:when>
      <xsl:when test="self::stick and $velocity != ''">
        <xsl:value-of select="concat($indent, 'physics::stick(', $name, ', ', $velocity, ', ', $side, ', windowArea);&#10;')" />
      </xsl:when>
      <xsl:when test="self::stick">
        <xsl:value-of select="concat($indent, 'physics::stick(', $name, ', ', $side, ', windowArea);&#10;')" />
      </xsl:when>
      <xsl:when test="self::wrap">
        <xsl:value-of select="concat($indent, 'physics::wrap(', $name, ', ', $motion, ', ', $side, ', windowArea);&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="self-command">
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="velocity" select="$velocity" />
          <xsl:with-param name="alive" select="$alive" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What sticks at an edge kept in after the move, moving or not, as the
       engine's Game::keepStuckObjectInBounds: past an edge it has a <stick />
       for (in a rule whose sprite= or unless= holds), back inside. -->
  <xsl:template name="define-stick">
    <xsl:variable name="self" select="." />
    <xsl:variable name="indent" select="concat('&#9;', substring('&#9;', 1, number(boolean(self::group))))" />
    <xsl:variable name="one"><xsl:call-template name="one-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="velocity"><xsl:call-template name="velocity-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="alive"><xsl:call-template name="alive-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="out">
      <xsl:choose>
        <xsl:when test="self::group">continue</xsl:when>
        <xsl:otherwise>return</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="sticks" select="collisions/collision[@edge][stick]" />
    <xsl:variable name="body">
      <xsl:if test="$alive != ''">
        <xsl:value-of select="concat('&#10;', $indent, 'if (!', $alive, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $out, ';&#10;', $indent, '}&#10;')" />
      </xsl:if>
      <xsl:for-each select="document('')//xsl:variable[@name = 'edges']/edge">
        <xsl:variable name="this" select="." />
        <xsl:variable name="side" select="concat('physics::Edge::', @title)" />
        <xsl:variable name="about" select="$sticks[@edge = $this/@name or @edge = $this/@in or @edge = 'all']" />
        <xsl:if test="$about">
          <!-- held back only while every rule with a stick about it is -->
          <xsl:variable name="guards">
            <xsl:for-each select="$about">
              <xsl:variable name="guard">
                <xsl:call-template name="rule-guard">
                  <xsl:with-param name="rule" select="." />
                  <xsl:with-param name="self" select="$self" />
                  <xsl:with-param name="name" select="$one" />
                </xsl:call-template>
              </xsl:variable>
              <xsl:choose>
                <xsl:when test="$guard = ''">|always</xsl:when>
                <xsl:otherwise><xsl:value-of select="concat('|', $guard)" /></xsl:otherwise>
              </xsl:choose>
            </xsl:for-each>
          </xsl:variable>
          <xsl:variable name="held" select="not(contains(concat($guards, '|'), '|always|'))" />
          <xsl:value-of select="concat('&#10;', $indent, 'if (physics::past(', $one, ', ', $side, ', windowArea)')" />
          <xsl:if test="$held">
            <xsl:text> &amp;&amp; (</xsl:text>
            <xsl:call-template name="join-guards"><xsl:with-param name="text" select="substring($guards, 2)" /></xsl:call-template>
            <xsl:text>)</xsl:text>
          </xsl:if>
          <xsl:value-of select="concat(') // ', @name, '&#10;', $indent, '{&#10;', $indent, '&#9;physics::stick(', $one, ', ', substring(concat($velocity, ', '), 1, string-length($velocity) + 2 * number($velocity != '')), $side, ', windowArea);&#10;', $indent, '}&#10;')" />
        </xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:call-template name="each-one">
      <xsl:with-param name="function" select="'stick'" />
      <xsl:with-param name="comment">
        <xsl:if test="self::group">, every one of them</xsl:if>
        <xsl:text>, kept in at the edges it sticks at, after the move</xsl:text>
      </xsl:with-param>
      <xsl:with-param name="body" select="$body" />
    </xsl:call-template>
  </xsl:template>

  <!-- Guards written a|b|c, as (a) || (b) || (c). -->
  <xsl:template name="join-guards">
    <xsl:param name="text" />
    <xsl:choose>
      <xsl:when test="contains($text, '|')">
        <xsl:value-of select="concat('(', substring-before($text, '|'), ') || ')" />
        <xsl:call-template name="join-guards"><xsl:with-param name="text" select="substring-after($text, '|')" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('(', $text, ')')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A screen's move, as the engine's Game::moveObjects: the hops and jumps
       first, and what each rides or stands on worked out again
       (Game::applyHops); then every pair that may touch, in the engine's
       order, and the frame played from touch to touch (physics::playFrame). -->
  <xsl:template name="define-move">
    <xsl:variable name="shows" select="shows/show/@object" />
    <xsl:variable name="shown" select="$things[@name = $shows]" />
    <xsl:variable name="title"><xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="here" select="$touches[@a = $shows][@b = $shows]" />
    <xsl:value-of select="concat('&#10;// ', @name, ': everything shown moves this frame, from touch to touch, as in the engine&#10;void move', $title, '()&#10;{&#10;')" />
    <xsl:variable name="starting" select="$shown[count(. | $hopping | $riders | $standers) = count($hopping | $riders | $standers)]" />
    <xsl:if test="$starting">
      <xsl:text>	// the hops and jumps first, and what each rides or stands on worked out again
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$starting">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:variable name="alive"><xsl:call-template name="alive-of" /></xsl:variable>
      <xsl:if test="count(. | $hopping) = count($hopping)">
        <xsl:value-of select="concat('&#9;', $name, 'Hopped = false;&#10;')" />
      </xsl:if>
      <xsl:if test="count(. | $riders) = count($riders)">
        <xsl:value-of select="concat('&#9;', $name, 'Riding = {};&#10;')" />
      </xsl:if>
      <xsl:if test="count(. | $standers) = count($standers)">
        <xsl:value-of select="concat('&#9;', $name, 'Grounded = false;&#10;')" />
      </xsl:if>
      <xsl:if test="count(. | $jumpers) = count($jumpers)">
        <xsl:value-of select="concat('&#9;if (', $name, 'JumpFrames &gt; 0) // a jump under way: its next step, if that stays on the screen&#10;&#9;{&#10;')" />
        <xsl:value-of select="concat('&#9;&#9;physics::jumping(', $name, ', ', $name, 'JumpStep, ', $name, 'JumpFrames, windowArea);&#10;&#9;&#9;', $name, 'Hopped = true;&#10;&#9;}&#10;')" />
      </xsl:if>
      <xsl:if test="count(. | $hoppers) = count($hoppers)">
        <xsl:value-of select="concat('&#9;if (', $name, 'Hop != sf::Vector2f{}) // a hop asked for: made, if it stays on the screen&#10;&#9;{&#10;')" />
        <xsl:value-of select="concat('&#9;&#9;const sf::Vector2f hop = ', $name, 'Hop;&#10;&#9;&#9;', $name, 'Hop = {};&#10;')" />
        <xsl:choose>
          <xsl:when test="$alive != ''">
            <xsl:value-of select="concat('&#9;&#9;if (', $alive, ')&#10;&#9;&#9;{&#10;&#9;&#9;&#9;physics::hop(', $name, ', hop, windowArea);&#10;&#9;&#9;&#9;', $name, 'Hopped = true;&#10;&#9;&#9;}&#10;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('&#9;&#9;physics::hop(', $name, ', hop, windowArea);&#10;&#9;&#9;', $name, 'Hopped = true;&#10;')" />
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>	}
</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:choose>
      <xsl:when test="$here">
        <xsl:if test="$starting"><xsl:text>&#10;</xsl:text></xsl:if>
        <xsl:text>	// every two that may touch, in the engine's order
	std::vector&lt;Pair&gt; pairs;
</xsl:text>
        <xsl:for-each select="$shown[@name = $here/@a]">
          <xsl:call-template name="pairs-of">
            <xsl:with-param name="here" select="$here[@a = current()/@name]" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:value-of select="concat('&#10;&#9;physics::playFrame(pairs, sweepPair, advance', $title, ', touch);&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="$starting"><xsl:text>&#10;</xsl:text></xsl:if>
        <xsl:value-of select="concat('&#9;advance', $title, '(1.0f); // nothing here touches anything&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- The pairs the context thing (the first of each) makes with the things
       of `here`, each of its members with each of theirs, as long as both can
       touch now. -->
  <xsl:template name="pairs-of">
    <xsl:param name="here" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="boolean(self::group)" />
    <xsl:variable name="out"><xsl:call-template name="untouchable"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="can"><xsl:call-template name="touchable"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="indent" select="concat('&#9;', substring('&#9;', 1, number($group or $out != '')))" />
    <xsl:variable name="first">
      <xsl:choose>
        <xsl:when test="$group">i</xsl:when>
        <xsl:otherwise>0</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="inner">
      <xsl:if test="$group and $out != ''">
        <xsl:value-of select="concat($indent, 'if (', $out, ')&#10;', $indent, '{&#10;', $indent, '&#9;continue;&#10;', $indent, '}&#10;')" />
      </xsl:if>
      <xsl:for-each select="$here">
        <xsl:variable name="touch" select="." />
        <xsl:for-each select="$things[@name = $touch/@b]">
          <xsl:variable name="other"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
          <xsl:variable name="other-out"><xsl:call-template name="touchable"><xsl:with-param name="index" select="'j'" /></xsl:call-template></xsl:variable>
          <xsl:variable name="push">
            <xsl:value-of select="concat('pairs.push_back({Touch::', $touch/@value)" />
            <xsl:choose>
              <xsl:when test="self::group"><xsl:value-of select="concat(', ', $first, ', j')" /></xsl:when>
              <xsl:when test="$group">, i</xsl:when>
            </xsl:choose>
            <xsl:text>});</xsl:text>
          </xsl:variable>
          <xsl:choose>
            <xsl:when test="self::group and $other-out != ''">
              <xsl:value-of select="concat($indent, 'for (std::size_t j = 0; j &lt; ', $other, '.size(); ++j)&#10;', $indent, '{&#10;')" />
              <xsl:value-of select="concat($indent, '&#9;if (', $other-out, ')&#10;', $indent, '&#9;{&#10;', $indent, '&#9;&#9;', $push, '&#10;', $indent, '&#9;}&#10;', $indent, '}&#10;')" />
            </xsl:when>
            <xsl:when test="self::group">
              <xsl:value-of select="concat($indent, 'for (std::size_t j = 0; j &lt; ', $other, '.size(); ++j)&#10;', $indent, '{&#10;', $indent, '&#9;', $push, '&#10;', $indent, '}&#10;')" />
            </xsl:when>
            <xsl:when test="$other-out != ''">
              <xsl:value-of select="concat($indent, 'if (', $other-out, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $push, '&#10;', $indent, '}&#10;')" />
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="concat($indent, $push, '&#10;')" />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </xsl:for-each>
    </xsl:variable>
    <xsl:value-of select="concat('&#9;// ', @name, ', and what it may touch&#10;')" />
    <xsl:choose>
      <xsl:when test="$group">
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;', $inner, '&#9;}&#10;')" />
      </xsl:when>
      <xsl:when test="$out != ''">
        <xsl:value-of select="concat('&#9;if (', $can, ')&#10;&#9;{&#10;', $inner, '&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$inner" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What a screen shows moved that far through the frame, by its velocity
       and what it rides, as the engine advances everything in play. -->
  <xsl:template name="define-advance">
    <xsl:variable name="shows" select="shows/show/@object" />
    <xsl:variable name="title"><xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="going" select="$things[@name = $shows][count(. | $moving | $riders) = count($moving | $riders)]" />
    <xsl:value-of select="concat('&#10;// ', @name, ': what is shown moved `fraction` of the way through the frame, by its velocity and what it rides&#10;void advance', $title, '(float fraction)&#10;{&#10;')" />
    <xsl:if test="not($going)">
      <xsl:text>	static_cast&lt;void&gt;(fraction); // nothing here has a velocity
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$going">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:variable name="one"><xsl:call-template name="one-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
      <xsl:variable name="motion"><xsl:call-template name="motion-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
      <xsl:variable name="alive"><xsl:call-template name="alive-of"><xsl:with-param name="index" select="'i'" /></xsl:call-template></xsl:variable>
      <xsl:variable name="indent" select="concat('&#9;', substring('&#9;', 1, number(boolean(self::group))))" />
      <xsl:variable name="move" select="concat($one, '.move(', $motion, ' * fraction);')" />
      <xsl:if test="self::group">
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;')" />
      </xsl:if>
      <xsl:choose>
        <xsl:when test="$alive != ''">
          <xsl:value-of select="concat($indent, 'if (', $alive, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $move, '&#10;', $indent, '}&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat($indent, $move, '&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="self::group">
        <xsl:text>	}
</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- When the two of a pair first touch through what is left of the frame,
       as the engine's CollisionDetector::sweep (physics::sweep): none once
       either is out of play or in the air, or while neither moves. -->
  <xsl:template name="define-sweep-pair">
    <xsl:text>
// When the two of a pair first touch, through what is left of the frame
// (`left`), as the engine sweeps them; none once one is out of play
std::optional&lt;physics::SweepHit&gt; sweepPair(const Pair&amp; pair, float left)
{
	switch (pair.touch)
	{
</xsl:text>
    <xsl:for-each select="$touches">
      <xsl:variable name="touch" select="." />
      <xsl:variable name="a" select="$things[@name = $touch/@a]" />
      <xsl:variable name="b" select="$things[@name = $touch/@b]" />
      <xsl:variable name="a-out"><xsl:for-each select="$a"><xsl:call-template name="untouchable"><xsl:with-param name="index" select="'pair.first'" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="b-out"><xsl:for-each select="$b"><xsl:call-template name="untouchable"><xsl:with-param name="index" select="'pair.second'" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="out" select="concat($a-out, substring(' || ', 1, 4 * number($a-out != '' and $b-out != '')), $b-out)" />
      <xsl:variable name="hopped">
        <xsl:for-each select="($a | $b)[count(. | $hopping) = count($hopping)]">
          <xsl:if test="position() &gt; 1"> || </xsl:if>
          <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          <xsl:text>Hopped</xsl:text>
        </xsl:for-each>
      </xsl:variable>
      <xsl:variable name="first"><xsl:for-each select="$a"><xsl:call-template name="one-of"><xsl:with-param name="index" select="'pair.first'" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="second"><xsl:for-each select="$b"><xsl:call-template name="one-of"><xsl:with-param name="index" select="'pair.second'" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="first-motion"><xsl:for-each select="$a"><xsl:call-template name="motion-of"><xsl:with-param name="index" select="'pair.first'" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="second-motion"><xsl:for-each select="$b"><xsl:call-template name="motion-of"><xsl:with-param name="index" select="'pair.second'" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:value-of select="concat('&#9;case Touch::', @value, ':&#10;')" />
      <xsl:if test="$out != ''">
        <xsl:value-of select="concat('&#9;&#9;if (', $out, ')&#10;&#9;&#9;{&#10;&#9;&#9;&#9;return std::nullopt;&#10;&#9;&#9;}&#10;')" />
      </xsl:if>
      <xsl:choose>
        <xsl:when test="($a | $b)[count(. | $pixel-things) = count($pixel-things)]">
          <xsl:value-of select="concat('&#9;&#9;return physics::sweepPixels(', $first, ', ')" />
          <xsl:for-each select="$a"><xsl:call-template name="pixels"><xsl:with-param name="shape" select="$first" /></xsl:call-template></xsl:for-each>
          <xsl:value-of select="concat(', ', $first-motion, ', ', $second, ', ')" />
          <xsl:for-each select="$b"><xsl:call-template name="pixels"><xsl:with-param name="shape" select="$second" /></xsl:call-template></xsl:for-each>
          <xsl:value-of select="concat(', ', $second-motion, ', left')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&#9;&#9;return physics::sweep(', $first, ', ', $first-motion, ', ', $second, ', ', $second-motion, ', left')" />
        </xsl:otherwise>
      </xsl:choose>
      <xsl:if test="$hopped != ''"><xsl:value-of select="concat(', ', $hopped)" /></xsl:if>
      <xsl:text>);
</xsl:text>
    </xsl:for-each>
    <xsl:text>	}
	return std::nullopt;
}
</xsl:text>
  </xsl:template>

  <!-- What the two of a pair do as they touch, as the engine's
       Game::applyObjectCollision: the first's rules about the second, then the
       second's about the first, every one that holds, whatever an earlier one
       did. -->
  <xsl:template name="define-touch">
    <xsl:text>
// What the two of a pair do as they touch, as the engine's rules run: the
// first's rules about the second, then the second's about the first. `edge`
// is the second's edge that was touched; the first touched with the opposite.
void touch(const Pair&amp; pair, [[maybe_unused]] physics::Edge edge)
{
	switch (pair.touch)
	{
</xsl:text>
    <xsl:for-each select="$touches">
      <xsl:variable name="touch" select="." />
      <xsl:variable name="a" select="$things[@name = $touch/@a]" />
      <xsl:variable name="b" select="$things[@name = $touch/@b]" />
      <xsl:value-of select="concat('&#9;case Touch::', @value, ':&#10;&#9;{&#10;')" />
      <xsl:call-template name="touch-side">
        <xsl:with-param name="self" select="$a" />
        <xsl:with-param name="other" select="$b" />
        <xsl:with-param name="self-index" select="'pair.first'" />
        <xsl:with-param name="other-index" select="'pair.second'" />
        <xsl:with-param name="edge" select="'physics::opposite(edge)'" />
      </xsl:call-template>
      <xsl:call-template name="touch-side">
        <xsl:with-param name="self" select="$b" />
        <xsl:with-param name="other" select="$a" />
        <xsl:with-param name="self-index" select="'pair.second'" />
        <xsl:with-param name="other-index" select="'pair.first'" />
        <xsl:with-param name="edge" select="'edge'" />
      </xsl:call-template>
      <xsl:text>		break;
	}
</xsl:text>
    </xsl:for-each>
    <xsl:text>	}
}
</xsl:text>
  </xsl:template>

  <!-- One side of a touch: `self`'s rules about `other`, in the order
       written, each while its sprite=, unless=, <slower> and <faster> hold
       (guards.xsl), how fast it is going taken once, before the first, as in
       the engine; `edge` its own edge that touched. -->
  <xsl:template name="touch-side">
    <xsl:param name="self" />
    <xsl:param name="other" />
    <xsl:param name="self-index" />
    <xsl:param name="other-index" />
    <xsl:param name="edge" />
    <xsl:variable name="about" select="$self/collisions/collision[*][not(@edge)][not(@object) or @object = $other/@name][not(@class) or @class = $other/@class]" />
    <xsl:if test="$about">
      <xsl:variable name="name"><xsl:for-each select="$self"><xsl:call-template name="one-of"><xsl:with-param name="index" select="$self-index" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="velocity"><xsl:for-each select="$self"><xsl:call-template name="velocity-of"><xsl:with-param name="index" select="$self-index" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="motion"><xsl:for-each select="$self"><xsl:call-template name="motion-of"><xsl:with-param name="index" select="$self-index" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="alive"><xsl:for-each select="$self"><xsl:call-template name="alive-of"><xsl:with-param name="index" select="$self-index" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="other-name"><xsl:for-each select="$other"><xsl:call-template name="one-of"><xsl:with-param name="index" select="$other-index" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="other-velocity"><xsl:for-each select="$other"><xsl:call-template name="velocity-of"><xsl:with-param name="index" select="$other-index" /></xsl:call-template></xsl:for-each></xsl:variable>
      <xsl:variable name="speed">
        <xsl:if test="$about[slower or faster] and $motion != 'sf::Vector2f{}'">
          <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template>
          <xsl:text>Speed</xsl:text>
        </xsl:if>
      </xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;// ', $self/@name, ', by its rules about ', $other/@name, ':')" />
      <xsl:for-each select="$about/*[not(self::slower or self::faster)]">
        <xsl:value-of select="concat(' ', local-name())" />
      </xsl:for-each>
      <xsl:text>&#10;</xsl:text>
      <xsl:if test="$speed != ''">
        <xsl:value-of select="concat('&#9;&#9;const float ', $speed, ' = ', $motion, '.length(); // as it touches, before a rule changes it&#10;')" />
      </xsl:if>
      <xsl:for-each select="$about">
        <xsl:variable name="guard">
          <xsl:call-template name="rule-guard">
            <xsl:with-param name="rule" select="." />
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="name" select="$name" />
            <xsl:with-param name="other" select="$other-name" />
            <xsl:with-param name="velocity" select="$velocity" />
            <xsl:with-param name="speed" select="string($speed)" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="at" select="concat('&#9;&#9;', substring('&#9;', 1, number($guard != '')))" />
        <xsl:if test="$guard != ''">
          <xsl:value-of select="concat('&#9;&#9;if (', $guard, ')&#10;&#9;&#9;{&#10;')" />
        </xsl:if>
        <xsl:for-each select="*[not(self::slower or self::faster)]">
          <xsl:call-template name="touch-command">
            <xsl:with-param name="name" select="$name" />
            <xsl:with-param name="velocity" select="$velocity" />
            <xsl:with-param name="other" select="$other-name" />
            <xsl:with-param name="other-velocity" select="$other-velocity" />
            <xsl:with-param name="alive" select="$alive" />
            <xsl:with-param name="self" select="$self" />
            <xsl:with-param name="edge" select="$edge" />
            <xsl:with-param name="indent" select="$at" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:if test="$guard != ''">
          <xsl:text>		}
</xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <!-- The pixels a touch tests for `shape`, of the context thing: its
       picture's, or nullptr for its whole box (or circle). -->
  <xsl:template name="pixels">
    <xsl:param name="shape" />
    <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
    <xsl:variable name="looks" select="$group-data[@name = current()/@name]/look" />
    <xsl:choose>
      <xsl:when test="count(. | $pixel-things) != count($pixel-things) or $type != 'sf::Sprite'">nullptr</xsl:when>
      <xsl:when test="self::object">
        <xsl:for-each select="sprite[1]">
          <xsl:text>&amp;</xsl:text>
          <xsl:call-template name="drawn-name" />
          <xsl:text>Pixels</xsl:text>
        </xsl:for-each>
      </xsl:when>
      <xsl:when test="count($looks) = 1">
        <xsl:for-each select="$looks/sprite">
          <xsl:text>&amp;</xsl:text>
          <xsl:call-template name="drawn-name" />
          <xsl:text>Pixels</xsl:text>
        </xsl:for-each>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&amp;pixelsOf(', $shape, '.getTexture())')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- What one does when it touches another, as the engine's
       CommandExecutor::executeObjectCollision: `name` (its velocity, its flag
       for being in play) against `other`, `edge` its own edge that touched;
       `self` is the object or group. -->
  <xsl:template name="touch-command">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="other" />
    <xsl:param name="other-velocity" select="''" />
    <xsl:param name="alive" />
    <xsl:param name="self" />
    <xsl:param name="edge" />
    <xsl:param name="indent" />
    <xsl:choose>
      <xsl:when test="self::bounce">
        <xsl:if test="$velocity != ''">
          <xsl:value-of select="concat($indent, 'physics::bounce(', $velocity, ', ', $edge, ');&#10;')" />
        </xsl:if>
      </xsl:when>
      <xsl:when test="self::deflect">
        <xsl:if test="$velocity != ''">
          <xsl:value-of select="concat($indent, 'physics::deflect(', $name, ', ', $velocity, ', ', $other, ', ', $edge, ', ')" />
          <xsl:call-template name="value-bare" />
          <xsl:text>);
</xsl:text>
        </xsl:if>
      </xsl:when>
      <xsl:when test="self::land and $velocity != ''">
        <xsl:variable name="bare"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template></xsl:variable>
        <xsl:variable name="test">
          <xsl:if test="count($self | $climbers) = count($climbers)">
            <xsl:value-of select="concat('!', $bare, 'Climbing &amp;&amp; ')" />
          </xsl:if>
          <xsl:value-of select="concat('physics::land(', $name, ', ', $velocity, ', ', $other, ', ', $edge, ')')" />
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="count($self | $standers) = count($standers)">
            <xsl:value-of select="concat($indent, 'if (', $test, ')&#10;', $indent, '{&#10;', $indent, '&#9;', $bare, 'Grounded = true;&#10;')" />
            <xsl:if test="count($self | $leapers) = count($leapers)">
              <xsl:value-of select="concat($indent, '&#9;if (', $bare, 'Leaping) // a leap ends: the keys held decide the way across again&#10;', $indent, '&#9;{&#10;', $indent, '&#9;&#9;', $bare, 'Leaping = false;&#10;')" />
              <xsl:if test="count($self | $stepped) = count($stepped)">
                <xsl:value-of select="concat($indent, '&#9;&#9;', $velocity, '.x = ', $bare, 'Steps.x;&#10;')" />
              </xsl:if>
              <xsl:value-of select="concat($indent, '&#9;}&#10;')" />
            </xsl:if>
            <xsl:value-of select="concat($indent, '}&#10;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat($indent, $test, ';&#10;')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="self::land" />
      <xsl:when test="self::ride">
        <xsl:value-of select="$indent" />
        <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$self/@name" /></xsl:call-template>
        <xsl:text>Riding = </xsl:text>
        <xsl:choose>
          <xsl:when test="$other-velocity != ''"><xsl:value-of select="$other-velocity" /></xsl:when>
          <xsl:otherwise>{}</xsl:otherwise>
        </xsl:choose>
        <xsl:text>;
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="self-command">
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="velocity" select="$velocity" />
          <xsl:with-param name="alive" select="$alive" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Ladders                                                                -->
  <!-- ===================================================================== -->

  <!-- The function that finds the ladder of a class a climber is at
       (ladderAt). -->
  <xsl:template name="ladder-name">
    <xsl:param name="class" />
    <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$class" /></xsl:call-template>
    <xsl:text>At</xsl:text>
  </xsl:template>

  <xsl:template name="ladder-signature">
    <xsl:text>template &lt;typename Shape&gt;
std::optional&lt;sf::FloatRect&gt; </xsl:text>
    <xsl:call-template name="ladder-name"><xsl:with-param name="class" select="@class" /></xsl:call-template>
    <xsl:text>(const Shape&amp; one)</xsl:text>
  </xsl:template>

  <!-- It goes through everything of the class that is in play and shown
       where `one` is, as the engine's applyClimbing looks only at what the
       screen shows, and gives the first that `one` is at. -->
  <!-- Aim and chase: the middle of the nearest one in play of what they look
       for, worked out when they look (nearestCities(missiles1[i])). -->
  <xsl:template name="nearest-signature">
    <xsl:if test="self::group">
      <xsl:text>template &lt;typename Shape&gt;
</xsl:text>
    </xsl:if>
    <xsl:text>std::optional&lt;sf::Vector2f&gt; nearest</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:if test="self::group">(const Shape&amp; one)</xsl:if>
    <xsl:if test="self::object">()</xsl:if>
  </xsl:template>

  <xsl:template name="nearest-call">
    <xsl:param name="from" />
    <xsl:variable name="target" select="$things[@name = current()/@object]" />
    <xsl:text>nearest</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@object" /></xsl:call-template>
    <xsl:if test="$target/self::group"><xsl:value-of select="concat('(', $from, ')')" /></xsl:if>
    <xsl:if test="$target/self::object">()</xsl:if>
  </xsl:template>

  <xsl:template name="define-nearest">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="target" select="@name" />
    <xsl:variable name="askers" select="$things[timers/timer/*[self::aim or self::chase]/@object = $target or collisions/collision/*[self::aim or self::chase]/@object = $target]" />
    <xsl:variable name="with" select="$states[shows/show/@object = $askers/@name]" />
    <xsl:variable name="dies" select="count(. | $dying) = count($dying)" />
    <xsl:text>
// </xsl:text>
    <xsl:choose>
      <xsl:when test="self::group">the middle of the nearest of </xsl:when>
      <xsl:otherwise>the middle of </xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="@name" />
    <xsl:text> in play</xsl:text>
    <xsl:if test="self::group"> to `one`</xsl:if>
    <xsl:text>, if any (&lt;aim&gt;, &lt;chase&gt;)
</xsl:text>
    <xsl:call-template name="nearest-signature" />
    <xsl:text>
{
</xsl:text>
    <!-- the screens showing one that looks but not this -->
    <xsl:if test="$with[not(shows/show/@object = $target)]">
      <xsl:text>	if (</xsl:text>
      <xsl:for-each select="$with[not(shows/show/@object = $target)]">
        <xsl:if test="position() &gt; 1"> || </xsl:if>
        <xsl:text>screens.back() == Screen::</xsl:text>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text>)
	{
		return std::nullopt; // not shown on this screen
	}
</xsl:text>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="self::group">
        <xsl:text>	const sf::Vector2f here = one.getGlobalBounds().getCenter();
	std::optional&lt;sf::Vector2f&gt; nearest;
</xsl:text>
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;')" />
        <xsl:if test="$dies">
          <xsl:value-of select="concat('&#9;&#9;if (!', $name, 'Alive[i])&#10;&#9;&#9;{&#10;&#9;&#9;&#9;continue;&#10;&#9;&#9;}&#10;')" />
        </xsl:if>
        <xsl:value-of select="concat('&#9;&#9;const sf::Vector2f there = ', $name, '[i].getGlobalBounds().getCenter();&#10;')" />
        <xsl:text>		if (!nearest || (there - here).length() &lt; (*nearest - here).length())
		{
			nearest = there;
		}
	}
	return nearest;
</xsl:text>
      </xsl:when>
      <xsl:when test="$dies">
        <xsl:value-of select="concat('&#9;if (!', $name, 'Alive)&#10;&#9;{&#10;&#9;&#9;return std::nullopt;&#10;&#9;}&#10;&#9;return ', $name, '.getGlobalBounds().getCenter();&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;return ', $name, '.getGlobalBounds().getCenter();&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <xsl:template name="define-ladder">
    <xsl:variable name="class" select="@class" />
    <xsl:variable name="askers" select="$climbers[actions/action/climb/@class = $class]" />
    <xsl:variable name="with" select="$states[shows/show/@object = $askers/@name]" />
    <xsl:variable name="ladders" select="$things[@class = $class][@name = $with/shows/show/@object]" />
    <xsl:text>
// the ladder of class </xsl:text>
    <xsl:value-of select="$class" />
    <xsl:text> that `one` is at, if any (&lt;climb&gt;): its middle over it, its feet between the ends
</xsl:text>
    <xsl:call-template name="ladder-signature" />
    <xsl:text>
{
</xsl:text>
    <xsl:if test="not($ladders)">
      <xsl:text>	// nothing of that class is shown with it
	static_cast&lt;void&gt;(one);
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$ladders">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:variable name="dies" select="count(. | $dying) = count($dying)" />
      <!-- the screens showing a climber but not this -->
      <xsl:variable name="screen-test">
        <xsl:if test="$with[not(shows/show/@object = current()/@name)]">
          <xsl:variable name="on" select="$with[shows/show/@object = current()/@name]" />
          <xsl:if test="count($on) &gt; 1">(</xsl:if>
          <xsl:for-each select="$on">
            <xsl:if test="position() &gt; 1"> || </xsl:if>
            <xsl:text>screens.back() == Screen::</xsl:text>
            <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
          </xsl:for-each>
          <xsl:if test="count($on) &gt; 1">)</xsl:if>
          <xsl:text> &amp;&amp; </xsl:text>
        </xsl:if>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="self::group and $dies">
          <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;&#9;&#9;if (', $screen-test, $name, 'Alive[i] &amp;&amp; ')" />
          <xsl:value-of select="concat('physics::atLadder(one, ', $name, '[i]))&#10;&#9;&#9;{&#10;&#9;&#9;&#9;return ', $name, '[i].getGlobalBounds();&#10;&#9;&#9;}&#10;&#9;}&#10;')" />
        </xsl:when>
        <xsl:when test="self::group">
          <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
          <xsl:value-of select="concat('&#9;for (const ', $type, '&amp; each : ', $name, ')&#10;&#9;{&#10;&#9;&#9;if (', $screen-test)" />
          <xsl:value-of select="'physics::atLadder(one, each))&#10;&#9;&#9;{&#10;&#9;&#9;&#9;return each.getGlobalBounds();&#10;&#9;&#9;}&#10;&#9;}&#10;'" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&#9;if (', $screen-test)" />
          <xsl:if test="$dies"><xsl:value-of select="concat($name, 'Alive &amp;&amp; ')" /></xsl:if>
          <xsl:value-of select="concat('physics::atLadder(one, ', $name, '))&#10;&#9;{&#10;&#9;&#9;return ', $name, '.getGlobalBounds();&#10;&#9;}&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>	return std::nullopt;
}
</xsl:text>
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
    <xsl:param name="amount" select="$node" />
    <xsl:variable name="direction" select="$node/@direction" />
    <xsl:choose>
      <xsl:when test="$direction = 'up' or $direction = 'left'">
        <xsl:if test="$direction = 'up'">{0.0f, </xsl:if>
        <xsl:if test="$direction = 'left'">{</xsl:if>
        <xsl:text>-</xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$amount" /></xsl:call-template>
        <xsl:if test="$direction = 'up'">}</xsl:if>
        <xsl:if test="$direction = 'left'">, 0.0f}</xsl:if>
      </xsl:when>
      <xsl:when test="$direction = 'down'">
        <xsl:text>{0.0f, </xsl:text>
        <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$amount" /></xsl:call-template>
        <xsl:text>}</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>{</xsl:text>
        <xsl:call-template name="value-bare"><xsl:with-param name="node" select="$amount" /></xsl:call-template>
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
      <xsl:when test="$bitmap and ../@rows and generate-id(..) != generate-id($drawn-looks[@rows = current()/../@rows][1])" />
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
        <xsl:value-of select="concat('&#10;const std::vector&lt;xge::LineSegment&gt; ', $name, 'Lines = {')" />
        <xsl:for-each select="line">
          <xsl:if test="position() &gt; 1">,</xsl:if>
          <xsl:text>&#10;&#9;{</xsl:text>
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="from/x" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="from/y" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="to/x" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value-bare"><xsl:with-param name="node" select="to/y" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="engine-color"><xsl:with-param name="name" select="color" /></xsl:call-template>
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
    <xsl:variable name="pixels"><xsl:call-template name="tested-pixels" /></xsl:variable>
    <xsl:if test="$pixels = 'yes'">
      <xsl:value-of select="concat('&#10;sf::Image ', $name, 'Pixels; // what its touches test, pixel by pixel')" />
    </xsl:if>
  </xsl:template>

  <!-- yes when a drawn sprite (or a group's look) is the picture of something
       whose touches are tested pixel by pixel. -->
  <xsl:template name="tested-pixels">
    <xsl:variable name="owner">
      <xsl:choose>
        <xsl:when test="parent::look"><xsl:value-of select="../../@name" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="ancestor::*[parent::objects]/@name" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$pixel-things[@name = $owner]">yes</xsl:if>
  </xsl:template>

  <!-- Its texture made, in setup()'s test of what could not be. -->
  <xsl:template name="load-picture">
    <xsl:variable name="name"><xsl:call-template name="drawn-name" /></xsl:variable>
    <xsl:variable name="bitmap" select="bitmap" />
    <!-- the picture of something with a heading: a square it turns in -->
    <xsl:variable name="turns" select="boolean(parent::object[count(. | $headed) = count($headed)])" />
    <xsl:value-of select="concat('!', $name, 'Picture.')" />
    <xsl:choose>
      <xsl:when test="$bitmap">
        <xsl:variable name="rows">
          <xsl:choose>
            <xsl:when test="../@rows"><xsl:for-each select="$drawn-looks[@rows = current()/../@rows][1]/sprite"><xsl:call-template name="drawn-name" /></xsl:for-each></xsl:when>
            <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:text>loadFromImage(imageOf(</xsl:text>
        <xsl:if test="$turns">xge::turnBitmap(</xsl:if>
        <xsl:call-template name="rows-picture"><xsl:with-param name="rows" select="$rows" /></xsl:call-template>
        <xsl:if test="$turns">, 0.0f)</xsl:if>
        <xsl:text>))</xsl:text>
      </xsl:when>
      <xsl:when test="line and $turns">
        <xsl:value-of select="concat('loadFromImage(imageOf(xge::rasterizeTurned(', $name, 'Lines, 0.0f)))')" />
      </xsl:when>
      <xsl:when test="line">
        <xsl:value-of select="concat('loadFromImage(imageOf(xge::rasterizeLines(', $name, 'Lines)))')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>loadFromFile(</xsl:text>
        <xsl:variable name="file"><xsl:call-template name="drawn-file" /></xsl:variable>
        <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="$file" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A <bitmap>'s picture: xge::rasterizeRows(playerRows, 3, {255, 255, 255, 255}). -->
  <xsl:template name="rows-picture">
    <xsl:param name="rows" />
    <xsl:value-of select="concat('xge::rasterizeRows(', $rows, 'Rows, ')" />
    <xsl:choose>
      <xsl:when test="bitmap/scale"><xsl:call-template name="whole-number"><xsl:with-param name="node" select="bitmap/scale" /></xsl:call-template></xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="engine-color"><xsl:with-param name="name" select="bitmap/color" /></xsl:call-template>
    <xsl:text>)</xsl:text>
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

  <!-- A color name as the engine's xge::Color, by its numbers: {255, 0, 0, 255}. -->
  <xsl:template name="engine-color">
    <xsl:param name="name" />
    <xsl:variable name="color" select="$tables/colors/color[@name = normalize-space($name)]" />
    <xsl:choose>
      <!-- none given: white, as the engine draws it -->
      <xsl:when test="normalize-space($name) = ''">{255, 255, 255, 255}</xsl:when>
      <xsl:when test="$color">
        <xsl:value-of select="concat('{', $color/@rgba, '}')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">windows-cpp: there is no color named "<xsl:value-of select="normalize-space($name)" />"</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
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
