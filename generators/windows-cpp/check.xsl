<?xml version="1.0" encoding="UTF-8"?>
<!-- check.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- What this target can generate so far: all of Pong, Space Race and
     Freeway. Screens on a stack, rectangles, circles, texts (words or a number)
     and pictures; objects that move, bounce, stick, deflect, wrap round, start
     again and die; groups of them; keys held to move, and keys pressed to hop,
     change screen, start again, play a sound or count; conditions on an
     object's number; and sounds. Anything else stops the generator with a
     message saying what and where, rather than writing a program that plays a
     different game. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="supported" select="concat(
    ' game window width height background fullscreen framerate variables variable',
    ' objects object group member sprite circle radius rectangle color text content number size image path flip',
    ' position x y velocity',
    ' collisions enabled collision bounce stick reset deflect wrap die actions action move hop',
    ' states state shows show inputs input trigger conditions condition atleast atmost',
    ' push pop play inc dec sounds sound volume note rest',
    ' random equation formula add subtract multiply divide',
    ' augend addend minuend subtrahend multiplicand multiplier dividend divisor ')" />

  <xsl:variable name="waves" select="' square triangle sawtooth sine noise '" />

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
      <xsl:when test="self::object and /game/variables/variable[@name = current()/@name]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('an object named like the game variable ', @name)" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::framerate and (normalize-space(.) = '' or translate(normalize-space(.), $digits, '') != '')">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;framerate&gt; that is not a whole number'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::collision and (@unless or @sprite)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;collision&gt; with unless= or sprite='" />
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
      <xsl:when test="self::group and (sprite | member/sprite)/*[local-name() != local-name((current()/sprite | current()/member/sprite)[1]/*)]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;group&gt; whose members are not all one kind of shape'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::group and (sprite | member/sprite)/text">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;group&gt; of texts'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::bounce or self::stick or self::deflect or self::reset) and $in-rule and ancestor::group">
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
      <xsl:when test="self::wrap and not(ancestor::*[parent::objects][(velocity | member/velocity)[x/* or y/* or number(x) != 0 or number(y) != 0]])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;wrap /&gt; on something with no &lt;velocity&gt; of its own'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::bounce or self::stick or self::deflect or self::die) and not($in-rule)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; outside a &lt;collision&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::stick and not(../@edge)) or (self::deflect and ../@edge)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in that kind of &lt;collision&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::bounce or self::deflect) and not(ancestor::*[parent::objects][(velocity | member/velocity)[x/* or y/* or number(x) != 0 or number(y) != 0]])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; on an object with no &lt;velocity&gt; of its own')" />
        </xsl:call-template>
      </xsl:when>

      <!-- what the game does: in a collision, on a key, or in a condition -->
      <xsl:when test="self::reset and @object">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;reset object=&quot;...&quot;&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::reset or self::play or self::inc or self::dec) and not($in-rule or $on-key or $in-condition)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; there')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="(self::push or self::pop) and not($on-key or $in-condition)">
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

      <!-- keys -->
      <xsl:when test="(self::move or self::hop) and not(parent::action)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; outside the &lt;action&gt; of an object')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::action and move and hop">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'an &lt;action&gt; that both moves (held) and hops (pressed)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::move and ancestor::*[parent::objects][(velocity | member/velocity)[x/* or y/* or number(x) != 0 or number(y) != 0]]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;move&gt; on an object that has a &lt;velocity&gt; of its own'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::trigger and (not($on-key) or @class or not(/game/objects/object[@name = current()/@object]/actions/action[@name = current()/@action]))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;trigger&gt; that is not in an &lt;input&gt; or does not name an action of an object'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::input and trigger[key('action', concat(@object, '|', @action))/move]
                      and *[not(self::trigger[key('action', concat(@object, '|', @action))/move])]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'an &lt;input&gt; that both moves something (held) and does something else (pressed)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::input and (@keys or not($tables/keys/key[@name = current()/@button]))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('the key ', @button, @keys)" />
        </xsl:call-template>
      </xsl:when>

      <!-- conditions -->
      <xsl:when test="self::condition and (not(@variable) or not(@object or @class) or count(atleast | atmost) != 1)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;condition&gt; that is not an object variable (variable= with object= or class=) at least or at most a number'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::condition and not(/game/objects/object[@name = current()/@object or @class = current()/@class]/variables/variable[@name = current()/@variable])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('a &lt;condition&gt; on ', @variable, ', which no object it names has')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="parent::condition and not(self::atleast or self::atmost or self::push or self::pop or self::reset or self::play or self::inc or self::dec)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in a &lt;condition&gt;')" />
        </xsl:call-template>
      </xsl:when>

      <!-- looks -->
      <xsl:when test="self::sprite and (count(*) != 1 or not(circle or rectangle or text or image))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;sprite&gt; that is not one &lt;circle&gt;, &lt;rectangle&gt;, &lt;text&gt; or &lt;image&gt;'" />
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

      <!-- sounds -->
      <xsl:when test="(self::sound or self::note) and @wave and not(contains($waves, concat(' ', @wave, ' ')))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('the wave ', @wave)" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
    <xsl:apply-templates select="*" mode="check" />
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
