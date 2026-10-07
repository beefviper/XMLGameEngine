<?xml version="1.0" encoding="UTF-8"?>
<!-- check.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- What this target can generate so far: one screen of rectangles and circles
     that move, bounce, stick, deflect and start again, moved by held keys.
     Anything else stops the generator with a message saying what and where,
     rather than writing a program that plays a different game. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="supported" select="concat(
    ' game window width height background fullscreen framerate variables variable',
    ' objects object sprite circle radius rectangle color position x y velocity',
    ' collisions enabled collision bounce stick reset deflect actions action move',
    ' states state shows show inputs input trigger',
    ' random equation formula add subtract multiply divide',
    ' augend addend minuend subtrahend multiplicand multiplier dividend divisor ')" />

  <xsl:template match="*" mode="check">
    <xsl:variable name="tag" select="local-name()" />
    <xsl:choose>
      <xsl:when test="not(contains($supported, concat(' ', $tag, ' ')))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::state and preceding-sibling::state">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a second &lt;state&gt; (one screen only, so far)'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::variable and ../parent::game and .//random">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;random&gt; in a &lt;variable&gt;'" />
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
      <!-- a command, where it can stand -->
      <xsl:when test="(self::bounce or self::stick or self::reset) and parent::collision[@edge]">
        <xsl:call-template name="check-moving" />
      </xsl:when>
      <xsl:when test="(self::bounce or self::deflect or self::reset) and parent::collision[not(@edge)]">
        <xsl:call-template name="check-moving" />
      </xsl:when>
      <xsl:when test="(self::bounce or self::stick or self::reset or self::deflect) and not(parent::collision)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; outside a &lt;collision&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::bounce or self::stick or self::reset or self::deflect">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;', $tag, '&gt; in that kind of &lt;collision&gt;')" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::move and not(parent::action)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'&lt;move&gt; outside the &lt;action&gt; of an object'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::move and ancestor::object[velocity/x/* or velocity/y/* or number(velocity/x) != 0 or number(velocity/y) != 0]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;move&gt; on an object that has a &lt;velocity&gt; of its own'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::trigger and (not(parent::input) or @class or not(/game/objects/object[@name = current()/@object]/actions/action[@name = current()/@action]))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;trigger&gt; that is not in an &lt;input&gt; or does not name an action of an object'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::input and *[not(self::trigger)]">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'an &lt;input&gt; that does anything but &lt;trigger&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::input and (@keys or not($tables/keys/key[@name = current()/@button]))">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('the key ', @button, @keys)" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::sprite and count(*) != 1">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="'a &lt;sprite&gt; that is not one &lt;circle&gt; or &lt;rectangle&gt;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="self::show and not(/game/objects/object[@name = current()/@object])">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('&lt;show object=&quot;', @object, '&quot;&gt;, which is not an &lt;object&gt;')" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
    <xsl:apply-templates select="*" mode="check" />
  </xsl:template>

  <!-- bounce, stick, deflect and reset each need something of their object:
       a velocity to turn round, or a place to go back to. Only a velocity is
       missing on an object that never moves on its own. -->
  <xsl:template name="check-moving">
    <xsl:if test="(self::bounce or self::deflect) and not(ancestor::object[velocity/x/* or velocity/y/* or number(velocity/x) != 0 or number(velocity/y) != 0])">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; on an object with no &lt;velocity&gt; of its own')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="self::reset and @object">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="'&lt;reset object=&quot;...&quot;&gt;'" />
      </xsl:call-template>
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
