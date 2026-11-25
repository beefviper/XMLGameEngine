<?xml version="1.0" encoding="UTF-8"?>
<!-- wrap.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 25, 2026 -->

<!-- <wrap /> at an edge: once right off it, back in from the other side, a
     window and its own size along, so things spaced along a lane keep
     their spacing. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: bare, in a rule at an edge -->
  <xsl:template match="collision[@edge]/wrap[not(@* or node())]" mode="covers" />

  <!-- The whole test, not only the edge's: it waits until it is off. -->
  <xsl:template match="wrap" mode="edge">
    <xsl:param name="of" />
    <xsl:param name="edge" />
    <xsl:choose>
      <xsl:when test="$edge = 'left'"><xsl:value-of select="concat('if (', $of, 'velocity.x &lt; 0 &amp;&amp; ', $of, 'position.x + ', $of, 'size.x &lt;= 0) { ', $of, 'position.x += windowWidth + ', $of, 'size.x; }')" /></xsl:when>
      <xsl:when test="$edge = 'right'"><xsl:value-of select="concat('if (', $of, 'velocity.x &gt; 0 &amp;&amp; ', $of, 'position.x &gt;= windowWidth) { ', $of, 'position.x -= windowWidth + ', $of, 'size.x; }')" /></xsl:when>
      <xsl:when test="$edge = 'top'"><xsl:value-of select="concat('if (', $of, 'velocity.y &lt; 0 &amp;&amp; ', $of, 'position.y + ', $of, 'size.y &lt;= 0) { ', $of, 'position.y += windowHeight + ', $of, 'size.y; }')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('if (', $of, 'velocity.y &gt; 0 &amp;&amp; ', $of, 'position.y &gt;= windowHeight) { ', $of, 'position.y -= windowHeight + ', $of, 'size.y; }')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
