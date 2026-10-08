<?xml version="1.0" encoding="UTF-8"?>
<!-- looks.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 7, 2026 -->

<!-- Looks: an object or group with several named <sprite>s and no animation
     shows one of them at a time, the first to start with, and <become
     sprite="..." /> shows another (Breakout's top row: whole, then cracked).
     In the program each such thing has an enum of its looks, a variable saying
     which one it shows (a std::vector for a group, one for each member or
     cell), and a function become<Name>() that sets the variable and the look
     itself. start() and a <reset /> show the first look again, as the engine
     does; a <collision sprite="..."> is an if on the variable. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="looked" select="$things[sprite[2]]" />

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
          <xsl:with-param name="sprite" select="sprite[1]/@name" />
        </xsl:call-template>
        <xsl:text>; // the look it shows</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
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
    <xsl:for-each select="sprite">
      <xsl:text>	case </xsl:text>
      <xsl:call-template name="look-value">
        <xsl:with-param name="thing" select="$thing" />
        <xsl:with-param name="sprite" select="@name" />
      </xsl:call-template>
      <xsl:text>:
</xsl:text>
      <xsl:call-template name="set-look">
        <xsl:with-param name="name" select="$shape" />
        <xsl:with-param name="indent" select="'&#9;&#9;'" />
      </xsl:call-template>
      <xsl:text>		break;
</xsl:text>
    </xsl:for-each>
    <xsl:text>	}
}
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
