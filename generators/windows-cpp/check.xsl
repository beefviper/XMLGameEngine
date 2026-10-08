<?xml version="1.0" encoding="UTF-8"?>
<!-- check.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- What this target can generate so far: all of Pong, Space Race,
     Freeway, Breakout, Depth Charge, Frogger, Astrosmash, Kaboom, Demon
     Attack, Megamania, Asteroids, Combat, Lunar Lander, Frostbite, Air-Sea
     Battle, Donkey Kong and Pitfall!. Screens on a stack, rectangles,
     circles, texts (words or a number) and pictures (from a file, rows of
     text, lines or an SVG); objects that move, bounce, stick, deflect, wrap
     round, ride, turn back, stop, start again and die; groups of them,
     listed or in columns and rows (each member, row, column or cell changing
     what it picks), moving as one block or each its own way; headings that
     turn a picture, thrust, a pull and drag; walking, falling, landing,
     leaping and climbing; touches pixel by pixel, and by speed; things
     hidden until fired, revealed or released; timers of objects, groups and
     screens; keys held to move, and keys pressed to hop, jump, fire, change
     screen, start again, play a sound or count, in sets the screens share;
     conditions on an object's number or on how many are left; and sounds.
     Anything else stops the generator with a message saying what and where,
     rather than writing a program that plays a different game. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="supported" select="concat(
    ' game window width height background fullscreen framerate variables variable',
    ' objects object group member row column cell sprite circle radius rectangle color text content number size image path flip columns rows padding',
    ' line from to thickness bitmap row scale svg hide',
    ' position x y velocity acceleration heading drag facing hidden timers timer every after',
    ' collisions enabled type lockstep collision slower faster bounce stick reset deflect wrap die ride land reverse stop reveal release actions action move hop jump distance seconds leap climb fire turn thrust accelerate',
    ' states keys state shows show inputs input trigger conditions condition atleast atmost remaining',
    ' push pop play inc dec become sounds sound volume note rest',
    ' random equation formula add subtract multiply divide',
    ' augend addend minuend subtrahend multiplicand multiplier dividend divisor ')" />

  <xsl:variable name="waves" select="' square triangle sawtooth sine noise '" />

  <!-- Whether the object or group this node is in moves: a velocity of its own
       that is not 0, 0, or keys that move it. One that never moves never meets
       an edge, so its edge rules are left out. -->
  <xsl:template name="moves">
    <xsl:for-each select="ancestor-or-self::*[parent::objects]">
      <xsl:if test="(velocity | */velocity)[x/* or y/* or number(x) != 0 or number(y) != 0] or actions/action/*">yes</xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- Whether a name is an object's own variable (paddle1.score). -->
  <xsl:template name="is-object-variable">
    <xsl:param name="name" />
    <xsl:if test="/game/objects/object[@name = substring-before($name, '.')]/variables/variable[@name = substring-after($name, '.')]">yes</xsl:if>
  </xsl:template>

  <xsl:template match="*" mode="check">
    <xsl:variable name="tag" select="local-name()" />
    <xsl:variable name="in-rule" select="boolean(parent::collision)" />
    <xsl:variable name="on-key" select="boolean(parent::input)" />
    <xsl:variable name="in-condition" select="boolean(parent::condition)" />
    <xsl:variable name="in-timer" select="boolean(parent::timer)" />
    <!-- an object's or group's timer: one that has something to do it to -->
    <xsl:variable name="in-own-timer" select="boolean(parent::timer/parent::timers/parent::*[parent::objects])" />
    <xsl:choose>
      <xsl:when test="not(contains($supported, concat(' ', $tag, ' ')))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::variable and ../parent::game and .//random">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;random&gt; in a &lt;variable&gt; of the game (give it to an object)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::ride and ancestor::group">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;ride /&gt; in the rules of a &lt;group&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::type and normalize-space(.) != 'pixel' and normalize-space(.) != 'box'">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('the collision &lt;type&gt; ', normalize-space(.))" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::type and normalize-space(.) = 'pixel' and ../..//sprite/image">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'pixel touches of a picture from a file'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::type and normalize-space(.) = 'pixel' and ../../self::object[sprite[2]]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'pixel touches of an object of several looks'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::heading and not(../sprite[1][line or bitmap][not(bitmap/flip)] and count(../sprite) = 1)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;heading&gt; on something that is not one sprite of lines or rows'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::acceleration and parent::group and (normalize-space(../collisions/lockstep) = 'true' or ..//stop)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'an &lt;acceleration&gt; on a &lt;group&gt; in &lt;lockstep&gt;, or that a &lt;stop /&gt; stops'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::turn or self::thrust) and not(ancestor::object/heading)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; on an object with no &lt;heading&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::thrust or self::accelerate) and @burn and not(ancestor::object/variables/variable[@name = normalize-space(current()/@burn)])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, ' burn=&quot;', @burn, '&quot;&gt;, which is not a variable of its object')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::slower or self::faster) and (../../../self::group or /game/objects/group[@name = current()/../@object or @class = current()/../@class])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in a group, or about one')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::release and not($in-rule or $in-own-timer)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;release&gt; outside a &lt;collision&gt; or a &lt;timer&gt; of an object'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::release and not(/game/objects/*[@name = current()/@object][normalize-space(hidden) = 'true' or @class = 'projectile'])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;release object=&quot;', @object, '&quot;&gt;, which is not an object or group that is hidden')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::framerate and (normalize-space(.) = '' or translate(normalize-space(.), $digits, '') != '')">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;framerate&gt; that is not a whole number'" />
        </xsl:call-template>
      </xsl:when>

      <!-- groups: a std::vector of one kind of shape, each member placed and
           moving on its own, with rules that do not need one member put back
           on its own (that is to come) -->
      <xsl:when test="self::group and (variables or actions)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;group&gt; with &lt;variables&gt; or &lt;actions&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::group and member[not(sprite)] and not(sprite)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;member&gt; with no &lt;sprite&gt;, in a group with none'" />
        </xsl:call-template>
      </xsl:when>
      <!-- one kind: circles, rectangles, texts, or pictures (sf::Sprite: an
           image, a bitmap, an svg or lines) -->
      <xsl:when test="(self::group or self::object) and number(boolean((sprite | */sprite)/descendant-or-self::*[self::circle])) + number(boolean((sprite | */sprite)/descendant-or-self::*[self::rectangle]))
                                      + number(boolean((sprite | */sprite)/descendant-or-self::*[self::text])) + number(boolean((sprite | */sprite)/descendant-or-self::*[self::image or self::bitmap or self::svg or self::line])) &gt; 1">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('a &lt;', local-name(), '&gt; whose sprites are not all one kind of shape')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::group and (sprite | */sprite)/text">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;group&gt; of texts'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::lockstep and normalize-space(.) = 'true' and not(parent::collisions/parent::group)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;lockstep&gt; on an object (only on a &lt;group&gt;)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::group and normalize-space(collisions/lockstep) = 'true' and */velocity">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;group&gt; in &lt;lockstep&gt; whose members or cells have velocities of their own'" />
        </xsl:call-template>
      </xsl:when>
      <!-- several sprites are looks (<become>), the group's alone: a part's
           <sprite> changes the group's one look -->
      <xsl:when test="self::group and (*/sprite[2] or (sprite[2] and */sprite))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;member&gt;, &lt;row&gt;, &lt;column&gt; or &lt;cell&gt; with a &lt;sprite&gt; in a group of several looks'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::sprite and parent::*[parent::group] and ../../sprite and string(@name) != string(../../sprite/@name)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('a &lt;sprite&gt; of a &lt;', local-name(..), '&gt; with another name than the sprite of its group (a look of its own)')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::variables and parent::*[self::row or self::column or self::cell]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;variables&gt; of a &lt;', local-name(..), '&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::columns or self::rows) and (* or translate(normalize-space(.), $digits, '') != '' or normalize-space(.) = '' or number(.) &lt; 1)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; that is not a whole number')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::reset and not(@object) and $in-rule and ancestor::group[columns or normalize-space(collisions/lockstep) = 'true']">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;reset /&gt; on the cells of a group in columns and rows, or of a group in &lt;lockstep&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::stick or self::deflect) and $in-rule and ancestor::group">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; on the members of a &lt;group&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::collision and (@object or @class) and not(/game/objects/*[@name = current()/@object or @class = current()/@class])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('a &lt;collision&gt; with ', @object, @class, ', which no object or group is')" />
        </xsl:call-template>
      </xsl:when>

      <!-- what an object does in a collision -->
      <xsl:when test="self::wrap and not($in-rule and ../@edge)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;wrap /&gt; outside a screen-edge &lt;collision&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::wrap and ../*[not(self::wrap)]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;wrap /&gt; with other commands in the same &lt;collision&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::wrap and not(ancestor::*[parent::objects][count(. | $moving) = count($moving) or actions/action/move])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;wrap /&gt; on something with no &lt;velocity&gt; of its own'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::bounce or self::stick or self::deflect) and not($in-rule)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; outside a &lt;collision&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::stick and not(../@edge)) or (self::deflect and ../@edge)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in that kind of &lt;collision&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::bounce or self::deflect) and not(ancestor::*[parent::objects][count(. | $moving) = count($moving)])
                      and not(../@edge and not(ancestor::*[parent::objects]/actions/action/*))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; on an object with no &lt;velocity&gt; of its own')" />
        </xsl:call-template>
      </xsl:when>

      <!-- what an object does to itself: in a collision, or in a timer of its own -->
      <xsl:when test="(self::die or self::reverse or self::stop) and not($in-rule or $in-own-timer)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; outside a &lt;collision&gt; or a &lt;timer&gt; of an object')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::reverse or self::stop) and not(ancestor::*[parent::objects][count(. | $moving) = count($moving)])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, ' /&gt; on something with no &lt;velocity&gt; of its own')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::move or self::fire) and $in-timer and not($in-own-timer)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in a &lt;timer&gt; of a state')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::facing and ../actions/action[move or hop or jump]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;facing&gt; on something keys move (the way it faces changes with them)'" />
        </xsl:call-template>
      </xsl:when>

      <!-- what the game does: in a collision, on a key, in a condition or a timer -->
      <xsl:when test="(self::reset or self::reveal) and @object and not(/game/objects/*[@name = current()/@object][@name = /game/states/state/shows/show/@object or variables/variable])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, ' object=&quot;', @object, '&quot;&gt;, which is not an object or group a screen shows')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::reset or self::reveal or self::play or self::inc or self::dec) and not($in-rule or $on-key or $in-condition or $in-timer or (parent::action and not(self::reveal or self::inc or self::dec or self::reset[@object]) and not(../move)))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; there')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::push or self::pop) and not($on-key or $in-condition or $in-timer)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; outside an &lt;input&gt; or &lt;condition&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::push or (self::pop and @state)) and not(/game/states/state[@name = current()/@state])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, ' state=&quot;', @state, '&quot;&gt;, which is not a &lt;state&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::play and not(/game/sounds/sound[@name = current()/@sound])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;play sound=&quot;', @sound, '&quot;&gt;, which is not a &lt;sound&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::inc or self::dec">
        <xsl:variable name="known">
          <xsl:call-template name="is-object-variable"><xsl:with-param name="name" select="normalize-space(@variable)" /></xsl:call-template>
        </xsl:variable>
        <xsl:if test="$known != 'yes'">
          <xsl:call-template name="refuse">
            <xsl:with-param name="what" select="concat('&lt;', $tag, ' variable=&quot;', @variable, '&quot;&gt; (it counts an object variable, as paddle1.score)')" />
          </xsl:call-template>
        </xsl:if>
      </xsl:when>

      <!-- standing and climbing -->
      <xsl:when test="self::land and not($in-rule and not(../@edge) and not(ancestor::group[normalize-space(collisions/lockstep) = 'true']))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;land /&gt; outside a &lt;collision&gt; with other objects, or in a group in &lt;lockstep&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::leap and not(parent::action and ../../../acceleration[not(y//random) and normalize-space(y) != '' and normalize-space(y) != '0'])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;leap&gt; outside the &lt;action&gt; of an object pulled down by a fixed &lt;acceleration&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::climb and not(parent::action and /game/objects/*[@class = current()/@class] and (@direction = 'up' or @direction = 'down'))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;climb&gt; outside an &lt;action&gt;, not up or down, or of a class nothing is'" />
        </xsl:call-template>
      </xsl:when>
      <!-- something keys walk while it falls, leaps or climbs: a velocity
           across that the keys set -->
      <xsl:when test="self::move and parent::action and ../../../self::object[acceleration or actions/action/leap or actions/action/climb]
                      and (@direction = 'up' or @direction = 'down' or ../../../acceleration[x/* or number(x) != 0])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;move&gt; up or down, or across against a pull across, on something that falls, leaps or climbs'" />
        </xsl:call-template>
      </xsl:when>

      <!-- keys -->
      <xsl:when test="(self::move and not(parent::action or $in-own-timer or ($in-rule and not(ancestor::group)))) or ((self::hop or self::jump) and not(parent::action))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; outside the &lt;action&gt; of an object')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::action and (move or turn or thrust or accelerate or climb) and *[not(self::move or self::turn or self::thrust or self::accelerate or self::climb)]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'an &lt;action&gt; that both moves (held) and does something else (pressed)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::fire and not(parent::action or $in-own-timer)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;fire&gt; outside the &lt;action&gt; or &lt;timer&gt; of an object'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::fire and not(/game/objects/*[@name = current()/@object][@class = 'projectile' or normalize-space(hidden) = 'true'])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;fire object=&quot;', @object, '&quot;&gt;, which is not an object or group that is hidden or of class=&quot;projectile&quot;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::fire and ancestor::*[parent::objects]/facing and not(/game/objects/*[@name = current()/@object]/velocity[x/* or y/* or number(x) != 0 or number(y) != 0])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;fire&gt; from something with a &lt;facing&gt;, of something with no &lt;velocity&gt; (its speed)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::fire and /game/objects/group[@name = current()/@object]/*/velocity">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;fire&gt; of a group whose members or cells have velocities of their own'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::move and parent::action and ancestor::*[parent::objects][(velocity | */velocity)[x/* or y/* or number(x) != 0 or number(y) != 0]]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;move&gt; on an object that has a &lt;velocity&gt; of its own'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::trigger and (not($on-key or (($in-condition or $in-timer) and not(key('action', concat(@object, '|', @action))[move or turn or thrust or accelerate or climb]))) or @class or not(/game/objects/object[@name = current()/@object]/actions/action[@name = current()/@action]))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;trigger&gt; that is not in an &lt;input&gt; (or, of an action not held, a condition or a timer) or does not name an action of an object'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::input and trigger[key('action', concat(@object, '|', @action))[move or turn or thrust or accelerate or climb]]
                      and *[not(self::trigger[key('action', concat(@object, '|', @action))[move or turn or thrust or accelerate or climb]])]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'an &lt;input&gt; that both moves something (held) and does something else (pressed)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::input">
        <xsl:call-template name="check-keys"><xsl:with-param name="rest" select="concat(normalize-space(@button), ' ')" /></xsl:call-template>
      </xsl:when>

      <!-- conditions -->
      <xsl:when test="self::condition and remaining and (@variable or not(@object or @class) or atleast or atmost)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;condition&gt; with &lt;remaining&gt; that is not about object= or class= alone'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::condition and remaining and not(/game/objects/*[@name = current()/@object or (current()/@class and @class = current()/@class)])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('a &lt;condition&gt; on what is left of ', @object, @class, ', which no object or group is')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::condition and remaining and /game/objects/*[@name = current()/@object or (current()/@class and @class = current()/@class)][not(@name = /game/states/state/shows/show/@object)]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('a &lt;condition&gt; on what is left of ', @object, @class, ', some of which no screen shows')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::remaining and (* or translate(normalize-space(.), $digits, '') != '' or normalize-space(.) = '')">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;remaining&gt; that is not a whole number'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::condition and not(remaining) and (not(@variable) or not(@object or @class) or count(atleast | atmost) != 1)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;condition&gt; that is not an object variable (variable= with object= or class=) at least or at most a number, or how many are left'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::condition and not(remaining) and not(/game/objects/object[@name = current()/@object or @class = current()/@class]/variables/variable[@name = current()/@variable])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('a &lt;condition&gt; on ', @variable, ', which no object it names has')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="parent::condition and not(self::atleast or self::atmost or self::remaining or self::push or self::pop or self::reset or self::reveal or self::play or self::inc or self::dec or self::become or self::trigger)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in a &lt;condition&gt;')" />
        </xsl:call-template>
      </xsl:when>

      <!-- looks -->
      <xsl:when test="self::sprite and not(line and count(*) = count(line)) and (count(*) != 1 or not(circle or rectangle or text or image or bitmap or svg))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;sprite&gt; that is not one &lt;circle&gt;, &lt;rectangle&gt;, &lt;text&gt;, &lt;image&gt;, &lt;bitmap&gt; or &lt;svg&gt;, or &lt;line&gt;s'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::text and count(content | number) != 1">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;text&gt; without one &lt;content&gt; or &lt;number&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::number">
        <xsl:variable name="known">
          <xsl:call-template name="is-object-variable"><xsl:with-param name="name" select="normalize-space(.)" /></xsl:call-template>
        </xsl:variable>
        <xsl:if test="$known != 'yes' or *">
          <xsl:call-template name="refuse">
            <xsl:with-param name="what" select="concat('&lt;number&gt;', normalize-space(.), '&lt;/number&gt; (it shows an object variable, as paddle1.score)')" />
          </xsl:call-template>
        </xsl:if>
      </xsl:when>
      <xsl:when test="self::flip and not(normalize-space(.) = 'horizontal' or normalize-space(.) = 'vertical')">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;flip&gt;', normalize-space(.), '&lt;/flip&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::show and not(/game/objects/*[@name = current()/@object])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;show object=&quot;', @object, '&quot;&gt;, which is not an &lt;object&gt; or &lt;group&gt;')" />
        </xsl:call-template>
      </xsl:when>

      <!-- what an <svg> takes is worked out when the program is generated: a
           number, or a game variable that is one -->
      <xsl:when test="(self::x or self::y or self::width or self::height or self::scale) and parent::svg and (* or (string(number(normalize-space(.))) = 'NaN' and string(number(normalize-space(/game/variables/variable[@name = normalize-space(current())][last()]))) = 'NaN'))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in an &lt;svg&gt; that is not a number')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::svg and (x or y or width or height) and not(x and y and width and height) and not(../../parent::group and ../../../sprite/svg[x and y and width and height])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'an &lt;svg&gt; with some of x, y, width and height but not all four'" />
        </xsl:call-template>
      </xsl:when>

      <!-- sounds -->
      <xsl:when test="(self::sound or self::note) and @wave and not(contains($waves, concat(' ', @wave, ' ')))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('the wave ', @wave)" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
    <xsl:apply-templates select="*" mode="check" />
  </xsl:template>

  <!-- Each key of an input's button="..." one SFML has. -->
  <xsl:template name="check-keys">
    <xsl:param name="rest" />
    <xsl:variable name="key" select="substring-before($rest, ' ')" />
    <xsl:if test="$key != ''">
      <xsl:if test="not($tables/keys/key[@name = $key])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('the key ', $key)" />
        </xsl:call-template>
      </xsl:if>
      <xsl:call-template name="check-keys"><xsl:with-param name="rest" select="substring-after($rest, ' ')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- Stops the generator: says what cannot be generated, and where. -->
  <xsl:template name="refuse">
    <xsl:param name="what" />
    <xsl:message terminate="yes">
      <xsl:text>windows-cpp cannot generate </xsl:text>
      <xsl:value-of select="$what" />
      <xsl:text> yet (in </xsl:text>
      <xsl:for-each select="ancestor-or-self::*">
        <xsl:if test="position() &gt; 1"> &gt; </xsl:if>
        <xsl:value-of select="local-name()" />
        <xsl:if test="@name">
          <xsl:value-of select="concat(' ', @name)" />
        </xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:message>
  </xsl:template>

</xsl:stylesheet>
