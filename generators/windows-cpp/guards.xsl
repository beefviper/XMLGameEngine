<?xml version="1.0" encoding="UTF-8"?>
<!-- guards.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 7, 2026 -->

<!-- Guards: what holds a rule back, as a C++ condition. A rule with
     sprite="..." runs only while its object shows that look (looks.xsl), and
     one with unless="class" is passed over while its object is touching
     anything of that class, other than itself and the one it touches
     (Frogger's water costs a life, unless the frog is also on a log). Both
     are looked at as the rule comes, as the engine does. Each class named by
     an unless= has a function, touching<Class>(one, other), that goes
     through everything of that class that can be touched. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- The rules with unless=, one for each class they name. -->
  <xsl:variable name="unless-rules" select="$rules[@unless]" />
  <xsl:variable name="unless-classes" select="$unless-rules[not(@unless = preceding::collision[@unless][count(. | $unless-rules) = count($unless-rules)]/@unless)]" />

  <!-- The function's name for a class (touchingLogs). -->
  <xsl:template name="touching-name">
    <xsl:param name="class" />
    <xsl:text>touching</xsl:text>
    <xsl:call-template name="cpp-title"><xsl:with-param name="name" select="$class" /></xsl:call-template>
  </xsl:template>

  <!-- What holds `rule` back, as a condition on `name` (the object, or one
       member) touching `other` (empty for an edge), or nothing: the look it
       must show, and the class it must not be touching. -->
  <xsl:template name="rule-guard">
    <xsl:param name="rule" />
    <xsl:param name="self" />
    <xsl:param name="name" />
    <xsl:param name="other" select="''" />
    <xsl:if test="$rule/@sprite">
      <xsl:call-template name="look-variable">
        <xsl:with-param name="thing" select="$self" />
        <xsl:with-param name="index"><xsl:call-template name="index-of"><xsl:with-param name="shape" select="$name" /></xsl:call-template></xsl:with-param>
      </xsl:call-template>
      <xsl:text> == </xsl:text>
      <xsl:call-template name="look-value">
        <xsl:with-param name="thing" select="$self" />
        <xsl:with-param name="sprite" select="$rule/@sprite" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="$rule/@sprite and $rule/@unless"> &amp;&amp; </xsl:if>
    <xsl:if test="$rule/@unless">
      <xsl:text>!</xsl:text>
      <xsl:call-template name="touching-name"><xsl:with-param name="class" select="$rule/@unless" /></xsl:call-template>
      <xsl:value-of select="concat('(', $name, ', ')" />
      <xsl:choose>
        <xsl:when test="$other = ''">nullptr</xsl:when>
        <xsl:otherwise><xsl:value-of select="concat('&amp;', $other)" /></xsl:otherwise>
      </xsl:choose>
      <xsl:text>)</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- Its first line, for the declarations. -->
  <xsl:template name="touching-signature">
    <xsl:text>template &lt;typename Shape&gt;
bool </xsl:text>
    <xsl:call-template name="touching-name"><xsl:with-param name="class" select="@unless" /></xsl:call-template>
    <xsl:text>(const Shape&amp; one, const void* other)</xsl:text>
  </xsl:template>

  <!-- The function for the class of this rule's unless=: everything of that
       class that can be touched (in play, shown with something that asks). -->
  <xsl:template name="define-touching">
    <xsl:variable name="class" select="@unless" />
    <xsl:variable name="askers" select="$unless-rules[@unless = $class]/ancestor::*[parent::objects]" />
    <xsl:variable name="together" select="$states[shows/show/@object = $askers/@name]/shows/show/@object" />
    <xsl:text>
// whether `one` is touching anything of class </xsl:text>
    <xsl:value-of select="$class" />
    <xsl:text>, other than itself and `other` (a rule's unless="</xsl:text>
    <xsl:value-of select="$class" />
    <xsl:text>")
</xsl:text>
    <xsl:call-template name="touching-signature" />
    <xsl:variable name="touchable" select="$colliding[@class = $class][@name = $together]" />
    <xsl:if test="not($touchable)">
      <xsl:text>
{
	// nothing of that class is shown with it
	static_cast&lt;void&gt;(one);
	static_cast&lt;void&gt;(other);
</xsl:text>
    </xsl:if>
    <xsl:if test="$touchable">
    <xsl:text>
{
	const auto counts = [&amp;](const auto&amp; each)
	{
		return static_cast&lt;const void*&gt;(&amp;each) != static_cast&lt;const void*&gt;(&amp;one) &amp;&amp; static_cast&lt;const void*&gt;(&amp;each) != other &amp;&amp; physics::touching(one, each);
	};
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$touchable">
      <xsl:variable name="name"><xsl:call-template name="cpp-name"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:variable name="dies" select="count(. | $dying) = count($dying)" />
      <xsl:choose>
        <xsl:when test="self::group and $dies">
          <xsl:value-of select="concat('&#9;for (std::size_t i = 0; i &lt; ', $name, '.size(); ++i)&#10;&#9;{&#10;&#9;&#9;if (', $name, 'Alive[i] &amp;&amp; counts(', $name, '[i]))&#10;&#9;&#9;{&#10;&#9;&#9;&#9;return true;&#10;&#9;&#9;}&#10;&#9;}&#10;')" />
        </xsl:when>
        <xsl:when test="self::group">
          <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
          <xsl:value-of select="concat('&#9;for (const ', $type, '&amp; each : ', $name, ')&#10;&#9;{&#10;&#9;&#9;if (counts(each))&#10;&#9;&#9;{&#10;&#9;&#9;&#9;return true;&#10;&#9;&#9;}&#10;&#9;}&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="'&#9;if ('" />
          <xsl:if test="$dies"><xsl:value-of select="concat($name, 'Alive &amp;&amp; ')" /></xsl:if>
          <xsl:value-of select="concat('counts(', $name, '))&#10;&#9;{&#10;&#9;&#9;return true;&#10;&#9;}&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>	return false;
}
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
