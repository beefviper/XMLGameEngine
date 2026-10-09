<?xml version="1.0" encoding="UTF-8"?>
<!-- paths.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 9, 2026 -->

<!-- Paths: the game's <paths>, flown by what <follow>s them (Galaxian's
     fleet flying in, and each alien's dive). In the program a path is a
     Route (its speed, the place it starts if it has one, and its legs: a step
     by so far, or home to where the thing started), all of them in one table
     indexed by an enum Path. What can follow keeps a Flight (the path it is
     on, the leg, what is left of that leg, whether the leg has begun, and the
     frames to wait first) and where it started the game, and its function
     fly<Name>() moves it on a frame (physics::fly) as the engine's
     applyPaths does, after
     the timers and before anything moves; what a leg does as it begins is
     that function's, as it is the follower's own. A <follow> starts a Flight,
     unless one is under way, and a <die />, a <stop /> or a whole start ends
     it. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="paths" select="/game/paths/path" />
  <!-- The <follow>s things give, of themselves or of others. -->
  <xsl:variable name="follows" select="$things//follow" />
  <!-- Those that follow a path: of their own accord, or sent by another. -->
  <xsl:variable name="followers" select="$things[.//follow[not(@object)] or @name = $things//follow/@object]" />

  <!-- Its enum value (Path::Dive). -->
  <xsl:template name="path-value">
    <xsl:param name="name" />
    <xsl:text>Path::</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$name" /></xsl:call-template>
  </xsl:template>

  <!-- The paths, after the screens: the enum, the table, and the two
       functions every follower uses. -->
  <xsl:template name="generate-paths">
    <xsl:if test="$followers">
      <xsl:text>
// paths: legs flown one after another at a speed (&lt;paths&gt;)
enum class Path { </xsl:text>
      <xsl:for-each select="$paths">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text> };

// in the order of Path
const std::vector&lt;physics::Route&gt; routes = {
</xsl:text>
      <xsl:for-each select="$paths">
        <xsl:value-of select="concat('&#9;// ', @name, '&#10;&#9;{')" />
        <xsl:call-template name="value"><xsl:with-param name="node" select="speed" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:choose>
          <xsl:when test="start">
            <xsl:text>sf::Vector2f</xsl:text>
            <xsl:call-template name="vector"><xsl:with-param name="node" select="start" /></xsl:call-template>
          </xsl:when>
          <xsl:otherwise>std::nullopt</xsl:otherwise>
        </xsl:choose>
        <xsl:text>, {</xsl:text>
        <xsl:for-each select="step | home">
          <xsl:text>&#10;&#9;&#9;</xsl:text>
          <xsl:choose>
            <xsl:when test="self::home">{{}, true}</xsl:when>
            <xsl:otherwise>
              <xsl:text>{</xsl:text>
              <xsl:call-template name="vector"><xsl:with-param name="node" select="." /></xsl:call-template>
              <xsl:text>}</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:if test="position() != last()">,</xsl:if>
          <xsl:if test="self::step/*[not(self::x or self::y)]">
            <xsl:text> // and </xsl:text>
            <xsl:for-each select="*[not(self::x or self::y)]">
              <xsl:if test="position() &gt; 1">, </xsl:if>
              <xsl:value-of select="local-name()" />
            </xsl:for-each>
          </xsl:if>
        </xsl:for-each>
        <xsl:text>}}</xsl:text>
        <xsl:if test="position() != last()">,</xsl:if>
        <xsl:text>&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>};
</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- A follower's declarations, after its shape. -->
  <xsl:template name="declare-flight">
    <xsl:param name="name" />
    <xsl:choose>
      <xsl:when test="self::group">
        <xsl:variable name="cells"><xsl:call-template name="group-size" /></xsl:variable>
        <xsl:value-of select="concat('&#10;std::vector&lt;physics::Flight&lt;Path&gt;&gt; ', $name, 'Flight(', $cells, '); // the path each is on (&lt;follow&gt;)')" />
        <xsl:value-of select="concat('&#10;std::vector&lt;sf::Vector2f&gt; ', $name, 'Home(', $cells, '); // where each started, which a path goes home to')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#10;physics::Flight&lt;Path&gt; ', $name, 'Flight; // the path it is on (&lt;follow&gt;)')" />
        <xsl:value-of select="concat('&#10;sf::Vector2f ', $name, 'Home; // where it started, which a path goes home to')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Its whole start: on no path, and where it is now is home. -->
  <xsl:template name="start-flight">
    <xsl:param name="name" />
    <xsl:choose>
      <xsl:when test="self::group">
        <xsl:value-of select="concat('&#9;', $name, 'Flight.assign(', $name, '.size(), {});&#10;')" />
        <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;&#9;&#9;', $name, 'Home[i] = ', $name, '[i].getPosition();&#10;&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;', $name, 'Flight = {};&#10;&#9;', $name, 'Home = ', $name, '.getPosition();&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="fly-name">
    <xsl:text>fly</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
  </xsl:template>

  <!-- Its function: a frame along the path each is on. -->
  <xsl:template name="define-fly">
    <xsl:variable name="thing" select="." />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="boolean(self::group)" />
    <xsl:variable name="at" select="substring('[i]', 1, 3 * number($group))" />
    <xsl:variable name="in" select="concat('&#9;', substring('&#9;', 1, number($group)))" />
    <xsl:variable name="dies" select="count(. | $dying) = count($dying)" />
    <xsl:variable name="mine" select="$follows[(not(@object) and ancestor::*[parent::objects]/@name = $thing/@name) or @object = $thing/@name]" />
    <xsl:variable name="flown" select="$paths[@name = $mine/@path]" />
    <xsl:variable name="doing" select="$flown/step[*[not(self::x or self::y)]]" />
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:if test="$group">, each of them</xsl:if>
    <xsl:text>: a frame along the path it is on (&lt;follow&gt;)
void </xsl:text>
    <xsl:call-template name="fly-name" />
    <xsl:text>()
{
</xsl:text>
    <xsl:if test="$group">
      <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;')" />
    </xsl:if>
    <xsl:variable name="velocity">
      <xsl:value-of select="concat($name, 'Velocity')" />
      <xsl:if test="$group and count(. | $member-velocities) = count($member-velocities)">[i]</xsl:if>
    </xsl:variable>
    <xsl:variable name="call" select="concat('physics::fly(', $name, $at, ', ', $velocity, ', ', $name, 'Flight', $at, ', ', $name, 'Home', $at, ', routes, ')" />
    <xsl:variable name="inner" select="concat($in, substring('&#9;', 1, number($dies)))" />
    <xsl:if test="$dies">
      <xsl:value-of select="concat($in, 'if (', $name, 'Alive', $at, ')&#10;', $in, '{&#10;')" />
    </xsl:if>
    <xsl:choose>
      <xsl:when test="$doing">
        <xsl:value-of select="concat($inner, $call, '[&amp;](Path path, std::size_t leg)&#10;', $inner, '{&#10;')" />
        <xsl:value-of select="concat($inner, '&#9;// what a leg does as it begins&#10;')" />
        <xsl:for-each select="$doing">
          <xsl:variable name="leg" select="count(preceding-sibling::step | preceding-sibling::home)" />
          <xsl:value-of select="concat($inner, '&#9;', substring('else ', 1, 5 * number(position() &gt; 1)), 'if (path == ')" />
          <xsl:call-template name="path-value"><xsl:with-param name="name" select="../@name" /></xsl:call-template>
          <xsl:value-of select="concat(' &amp;&amp; leg == ', $leg, ')&#10;', $inner, '&#9;{&#10;')" />
          <xsl:for-each select="*[not(self::x or self::y)]">
            <xsl:call-template name="self-command">
              <xsl:with-param name="name" select="concat($name, $at)" />
              <xsl:with-param name="velocity" select="$velocity" />
              <xsl:with-param name="alive" select="concat($name, 'Alive', $at)" />
              <xsl:with-param name="self" select="$thing" />
              <xsl:with-param name="indent" select="concat($inner, '&#9;&#9;')" />
            </xsl:call-template>
          </xsl:for-each>
          <xsl:value-of select="concat($inner, '&#9;}&#10;')" />
        </xsl:for-each>
        <xsl:value-of select="concat($inner, '});&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($inner, $call, '[](Path, std::size_t) {});&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$dies">
      <xsl:value-of select="concat($in, '}&#10;')" />
    </xsl:if>
    <xsl:if test="$group">
      <xsl:text>	}
</xsl:text>
    </xsl:if>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- A <follow> as a command of `self` (`name`, `velocity`): itself, or
       each of what it names in play and on no path, one every <stagger>
       seconds. -->
  <xsl:template name="follow-command">
    <xsl:param name="name" />
    <xsl:param name="velocity" />
    <xsl:param name="indent" />
    <xsl:variable name="path"><xsl:call-template name="path-value"><xsl:with-param name="name" select="@path" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="not(@object)">
        <xsl:variable name="bare" select="substring-before(concat($name, '['), '[')" />
        <xsl:value-of select="concat($indent, 'physics::follow(', $name, ', ', $velocity, ', ', $bare, 'Flight', substring($name, string-length($bare) + 1), ', ', $path, ', 0, routes);&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="target" select="$things[@name = current()/@object]" />
        <xsl:variable name="other"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@object" /></xsl:call-template></xsl:variable>
        <xsl:variable name="dies" select="count($target | $dying) = count($dying)" />
        <xsl:variable name="k">
          <xsl:choose>
            <xsl:when test="contains($name, '[i]')">k</xsl:when>
            <xsl:otherwise>i</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="$target/self::group">
            <xsl:variable name="velocities">
              <xsl:value-of select="concat($other, 'Velocity')" />
              <xsl:if test="count($target | $member-velocities) = count($member-velocities)"><xsl:value-of select="concat('[', $k, ']')" /></xsl:if>
            </xsl:variable>
            <xsl:value-of select="concat($indent, '// ', @object, ' set off along ', @path)" />
            <xsl:if test="stagger">, one after another</xsl:if>
            <xsl:value-of select="concat('&#10;', $indent, '{&#10;')" />
            <xsl:if test="stagger">
              <xsl:value-of select="concat($indent, '&#9;int setOff = 0;&#10;')" />
            </xsl:if>
            <xsl:value-of select="concat($indent, '&#9;for (std::size_t ', $k, ' = 0; ', $k, ' &lt; ', $other, '.size(); ++', $k, ')&#10;', $indent, '&#9;{&#10;')" />
            <xsl:value-of select="concat($indent, '&#9;&#9;if (')" />
            <xsl:if test="$dies"><xsl:value-of select="concat($other, 'Alive[', $k, '] &amp;&amp; ')" /></xsl:if>
            <xsl:value-of select="concat('!', $other, 'Flight[', $k, '].path)&#10;', $indent, '&#9;&#9;{&#10;')" />
            <xsl:value-of select="concat($indent, '&#9;&#9;&#9;physics::follow(', $other, '[', $k, '], ', $velocities, ', ', $other, 'Flight[', $k, '], ', $path, ', ')" />
            <xsl:choose>
              <xsl:when test="stagger">
                <xsl:text>static_cast&lt;int&gt;(std::lround(static_cast&lt;float&gt;(setOff) * </xsl:text>
                <xsl:call-template name="value"><xsl:with-param name="node" select="stagger" /></xsl:call-template>
                <xsl:value-of select="concat(' * static_cast&lt;float&gt;(framerate))), routes);&#10;', $indent, '&#9;&#9;&#9;++setOff;&#10;')" />
              </xsl:when>
              <xsl:otherwise>
                <xsl:text>0, routes);
</xsl:text>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:value-of select="concat($indent, '&#9;&#9;}&#10;', $indent, '&#9;}&#10;', $indent, '}&#10;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$indent" />
            <xsl:if test="$dies"><xsl:value-of select="concat('if (', $other, 'Alive)&#10;', $indent, '{&#10;', $indent, '&#9;')" /></xsl:if>
            <xsl:value-of select="concat('physics::follow(', $other, ', ', $other, 'Velocity, ', $other, 'Flight, ', $path, ', 0, routes);&#10;')" />
            <xsl:if test="$dies"><xsl:value-of select="concat($indent, '}&#10;')" /></xsl:if>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
