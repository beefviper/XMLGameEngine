<?xml version="1.0" encoding="UTF-8"?>
<!-- looks.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 7, 2026 -->

<!-- Looks: an object or group with several named <sprite>s
     shows one of them at a time, the first to start with, and <become
     sprite="..." /> shows another (Breakout's top row: whole, then cracked).
     In the program each such thing has an enum of its looks, a variable saying
     which one it shows (a std::vector for a group, one for each member or
     cell), and a function become<Name>() that sets the variable and the look
     itself. start() and a <reset /> show the first look again, as the engine
     does; a <collision sprite="..."> is an if on the variable. A row,
     column, cell or member may change a look (a picture of its own), and
     become<Name>() picks it by the index. An <animation> goes through its
     frames' looks, each interval, by animate<Name>(). -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="looked" select="$things[sprite[2]]" />
  <!-- Those whose looks an <animation> goes through, one each interval. -->
  <xsl:variable name="animated" select="$looked[animation]" />

  <!-- The look it starts with: its animation's first, or its first. -->
  <xsl:template name="first-look">
    <xsl:choose>
      <xsl:when test="animation"><xsl:value-of select="animation/frame[1]/@sprite" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="sprite[1]/@name" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The enum of the looks of the object or group (BallLook). -->
  <xsl:template name="look-type">
    <xsl:param name="thing" select="." />
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$thing/@name" /></xsl:call-template>
    <xsl:text>Look</xsl:text>
  </xsl:template>

  <!-- One look of the object or group as C++ (StrongLook::Cracked). -->
  <xsl:template name="look-value">
    <xsl:param name="thing" />
    <xsl:param name="sprite" />
    <xsl:call-template name="look-type"><xsl:with-param name="thing" select="$thing" /></xsl:call-template>
    <xsl:text>::</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$sprite" /></xsl:call-template>
  </xsl:template>

  <!-- The variable saying which look it shows (strongLook[i], ballLook);
       `index` is the member's or cell's, for a group. -->
  <xsl:template name="look-variable">
    <xsl:param name="thing" />
    <xsl:param name="index" />
    <xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$thing/@name" /></xsl:call-template>
    <xsl:text>Look</xsl:text>
    <xsl:if test="$thing/self::group"><xsl:value-of select="concat('[', $index, ']')" /></xsl:if>
  </xsl:template>

  <!-- The index in a shape written as name[i] (i), or nothing. -->
  <xsl:template name="index-of">
    <xsl:param name="shape" />
    <xsl:value-of select="substring-before(substring-after($shape, '['), ']')" />
  </xsl:template>

  <!-- A call that shows a look: becomeStrong(i, StrongLook::Cracked); -->
  <xsl:template name="become-call">
    <xsl:param name="thing" />
    <xsl:param name="sprite" />
    <xsl:param name="index" />
    <xsl:text>become</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$thing/@name" /></xsl:call-template>
    <xsl:text>(</xsl:text>
    <xsl:if test="$thing/self::group"><xsl:value-of select="concat($index, ', ')" /></xsl:if>
    <xsl:call-template name="look-value">
      <xsl:with-param name="thing" select="$thing" />
      <xsl:with-param name="sprite" select="$sprite" />
    </xsl:call-template>
    <xsl:text>);</xsl:text>
  </xsl:template>

  <!-- A <become>: on `self` (the member `index` of a group), or with object=
       on that object, or on every member of that group. -->
  <xsl:template name="become-command">
    <xsl:param name="indent" />
    <xsl:param name="self" select="/.." />
    <xsl:param name="index" select="''" />
    <xsl:variable name="target" select="$looked[@name = current()/@object] | $self[not(current()/@object)]" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="$target/@name" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$target/self::group and @object">
        <!-- counted with k when it is said by a member, i already -->
        <xsl:variable name="each" select="substring('ik', 1 + number(string($index) != ''), 1)" />
        <xsl:value-of select="concat($indent, 'for (std::size_t ', $each, ' = 0; ', $each, ' &lt; ', $name, '.size(); ++', $each, ')&#10;', $indent, '{&#10;', $indent, '&#9;')" />
        <xsl:call-template name="become-call">
          <xsl:with-param name="thing" select="$target" />
          <xsl:with-param name="sprite" select="@sprite" />
          <xsl:with-param name="index" select="$each" />
        </xsl:call-template>
        <xsl:value-of select="concat('&#10;', $indent, '}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$indent" />
        <xsl:call-template name="become-call">
          <xsl:with-param name="thing" select="$target" />
          <xsl:with-param name="sprite" select="@sprite" />
          <xsl:with-param name="index" select="$index" />
        </xsl:call-template>
        <xsl:text>&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- With an object's or group's own declaration: its enum and the look it
       shows, or each member's or cell's. -->
  <xsl:template name="declare-looks">
    <xsl:variable name="type"><xsl:call-template name="look-type" /></xsl:variable>
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('&#10;enum class ', $type, ' { ')" />
    <xsl:for-each select="sprite">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    </xsl:for-each>
    <xsl:text> };</xsl:text>
    <xsl:choose>
      <xsl:when test="self::group">
        <xsl:variable name="count"><xsl:call-template name="group-size" /></xsl:variable>
        <xsl:value-of select="concat('&#10;std::vector&lt;', $type, '&gt; ', $name, 'Look(', $count, '); // the look each one shows')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#10;', $type, ' ', $name, 'Look = ')" />
        <xsl:call-template name="look-value">
          <xsl:with-param name="thing" select="." />
          <xsl:with-param name="sprite"><xsl:call-template name="first-look" /></xsl:with-param>
        </xsl:call-template>
        <xsl:text>; // the look it shows</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="animation">
      <xsl:variable name="thing" select="." />
      <xsl:value-of select="concat('&#10;const std::vector&lt;', $type, '&gt; ', $name, 'Frames = {')" />
      <xsl:for-each select="animation/frame">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:call-template name="look-value">
          <xsl:with-param name="thing" select="$thing" />
          <xsl:with-param name="sprite" select="@sprite" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:text>}; // its &lt;animation&gt;, a look each interval</xsl:text>
      <xsl:choose>
        <xsl:when test="self::group">
          <xsl:variable name="count"><xsl:call-template name="group-size" /></xsl:variable>
          <xsl:value-of select="concat('&#10;std::vector&lt;std::size_t&gt; ', $name, 'Frame(', $count, '); // where each one is in it')" />
          <xsl:value-of select="concat('&#10;std::vector&lt;int&gt; ', $name, 'Shown(', $count, '); // and for how many frames each has shown that look')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('&#10;std::size_t ', $name, 'Frame = 0; // where it is in it')" />
          <xsl:value-of select="concat('&#10;int ', $name, 'Shown = 0; // and for how many frames it has shown that look')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- Its start: the first look of its animation, from the beginning. -->
  <xsl:template name="start-animation">
    <xsl:param name="indent" />
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="self::group">
        <xsl:value-of select="concat($indent, $name, 'Frame.assign(', $name, '.size(), 0);&#10;', $indent, $name, 'Shown.assign(', $name, '.size(), 0);&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($indent, $name, 'Frame = 0;&#10;', $indent, $name, 'Shown = 0;&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The call in a screen's update, before its timers, as the engine moves
       an animation on first. -->
  <xsl:template name="animate-name">
    <xsl:text>animate</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
  </xsl:template>

  <!-- Its animation, a frame on: each interval it is shown (in play), the
       next look, round and round. -->
  <xsl:template name="define-animate">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="group" select="boolean(self::group)" />
    <xsl:variable name="dies" select="count(. | $dying) = count($dying)" />
    <xsl:variable name="at" select="concat($name, substring('[i]', 1, 3 * number($group)))" />
    <xsl:variable name="in" select="concat('&#9;', substring('&#9;', 1, number($group)))" />
    <xsl:variable name="out" select="substring('continuereturn', 1 + 8 * number(not($group)), 8 - 2 * number(not($group)))" />
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:if test="$group">, every one of them</xsl:if>
    <xsl:text>: its next look each interval it is shown (&lt;animation&gt;)
void </xsl:text>
    <xsl:call-template name="animate-name" />
    <xsl:text>()
{
</xsl:text>
    <xsl:if test="$group">
      <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;')" />
    </xsl:if>
    <xsl:if test="$dies">
      <xsl:value-of select="concat($in, 'if (!', $name, 'Alive', substring('[i]', 1, 3 * number($group)), ')&#10;', $in, '{&#10;', $in, '&#9;', $out, ';&#10;', $in, '}&#10;')" />
    </xsl:if>
    <xsl:value-of select="concat($in, 'if (++', $name, 'Shown', substring('[i]', 1, 3 * number($group)), ' &lt; framesFor(')" />
    <xsl:call-template name="value-bare"><xsl:with-param name="node" select="animation/interval" /></xsl:call-template>
    <xsl:value-of select="concat('))&#10;', $in, '{&#10;', $in, '&#9;', $out, ';&#10;', $in, '}&#10;')" />
    <xsl:variable name="frame" select="concat($name, 'Frame', substring('[i]', 1, 3 * number($group)))" />
    <xsl:value-of select="concat($in, $name, 'Shown', substring('[i]', 1, 3 * number($group)), ' = 0;&#10;')" />
    <xsl:value-of select="concat($in, $frame, ' = (', $frame, ' + 1) % ', $name, 'Frames.size();&#10;')" />
    <xsl:value-of select="concat($in, 'become')" />
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:value-of select="concat('(', substring('i, ', 1, 3 * number($group)), $name, 'Frames[', $frame, ']);&#10;')" />
    <xsl:if test="$group">
      <xsl:text>	}
</xsl:text>
    </xsl:if>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- Its function's first line (void becomeStrong(std::size_t i, StrongLook look)). -->
  <xsl:template name="become-signature">
    <xsl:text>void become</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>(</xsl:text>
    <xsl:if test="self::group">std::size_t i, </xsl:if>
    <xsl:call-template name="look-type" />
    <xsl:text> look)</xsl:text>
  </xsl:template>

  <!-- Its function: the look noted, then shown. -->
  <xsl:template name="define-become">
    <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:variable name="shape">
      <xsl:value-of select="$name" />
      <xsl:if test="self::group">[i]</xsl:if>
    </xsl:variable>
    <xsl:variable name="thing" select="." />
    <xsl:text>
// </xsl:text>
    <xsl:value-of select="@name" />
    <xsl:if test="self::group">, one of them,</xsl:if>
    <xsl:text> shows one of its looks (&lt;become&gt;)
</xsl:text>
    <xsl:call-template name="become-signature" />
    <xsl:value-of select="concat('&#10;{&#10;&#9;', $name, 'Look')" />
    <xsl:if test="self::group">[i]</xsl:if>
    <xsl:text> = look;
	switch (look)
	{
</xsl:text>
    <xsl:variable name="data" select="$group-data[@name = $thing/@name]" />
    <xsl:for-each select="sprite">
      <xsl:variable name="look" select="@name" />
      <!-- this look as the rows, columns, cells or members change it, each
           on the ones it is theirs -->
      <xsl:variable name="others" select="$data/variant-look[@look = $look]/look" />
      <xsl:text>	case </xsl:text>
      <xsl:call-template name="look-value">
        <xsl:with-param name="thing" select="$thing" />
        <xsl:with-param name="sprite" select="@name" />
      </xsl:call-template>
      <xsl:text>:
</xsl:text>
      <xsl:for-each select="$others">
        <xsl:variable name="key" select="@key" />
        <xsl:variable name="set" select="$data/shape[variant[@look = $look]/look/@key = $key]" />
        <xsl:value-of select="concat('&#9;&#9;', substring('else if (', 1 + 5 * number(position() = 1)))" />
        <xsl:variable name="runs" select="$set[not(@i - 1 = $set/@i)]" />
        <xsl:for-each select="$runs">
          <xsl:variable name="from" select="number(@i)" />
          <xsl:variable name="to" select="number($set[number(@i) &gt;= $from][not(@i + 1 = $set/@i)][1]/@i)" />
          <xsl:if test="position() &gt; 1"> || </xsl:if>
          <xsl:choose>
            <xsl:when test="$from = $to"><xsl:value-of select="concat('i == ', $from)" /></xsl:when>
            <xsl:when test="$from = 0"><xsl:value-of select="concat('i &lt;= ', $to)" /></xsl:when>
            <xsl:when test="$to = count($data/shape) - 1"><xsl:value-of select="concat('i &gt;= ', $from)" /></xsl:when>
            <xsl:when test="count($runs) = 1"><xsl:value-of select="concat('i &gt;= ', $from, ' &amp;&amp; i &lt;= ', $to)" /></xsl:when>
            <xsl:otherwise><xsl:value-of select="concat('(i &gt;= ', $from, ' &amp;&amp; i &lt;= ', $to, ')')" /></xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
        <xsl:value-of select="concat(') // ', @comment, '&#10;&#9;&#9;{&#10;')" />
        <xsl:for-each select="sprite">
          <xsl:call-template name="set-look">
            <xsl:with-param name="name" select="$shape" />
            <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:text>		}
</xsl:text>
      </xsl:for-each>
      <xsl:if test="$others">
        <xsl:text>		else
		{
</xsl:text>
      </xsl:if>
      <xsl:call-template name="set-look">
        <xsl:with-param name="name" select="$shape" />
        <xsl:with-param name="indent" select="substring('&#9;&#9;&#9;', 1, 2 + number(boolean($others)))" />
      </xsl:call-template>
      <xsl:if test="$others">
        <xsl:text>		}
</xsl:text>
      </xsl:if>
      <xsl:text>		break;
</xsl:text>
    </xsl:for-each>
    <xsl:text>	}
}
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
