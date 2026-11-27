<?xml version="1.0" encoding="UTF-8"?>
<!-- become.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <become object sprite /> gives an object that look: computer.look = Computer::Look::Smart; and a thing whose looks differ in size, that size; with from=, only when it shows that look. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: anywhere, naming an object with that look -->
  <xsl:template match="become[@object and @sprite and not(node())]" mode="covers">
    <xsl:variable name="object" select="$everything[@name = current()/@object] | $states[@name = substring-before(current()/@object, '.')]/objects/object[@name = substring-after(current()/@object, '.')]" />
    <xsl:if test="not($object/sprite[@name = current()/@sprite]) or (@from and not($object/sprite[@name = current()/@from]))">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;become&gt; of ', $quote, @object, $quote, ' to a look it has not')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- covered: bare in a grid's rule, or a kind's timer, to a look of its cells -->
  <xsl:template match="group/collisions/collision/collision/become[@sprite and not(@object or @from or node())]" mode="covers">
    <xsl:if test="not(ancestor::group[1]/sprite[@name = current()/@sprite])">
      <xsl:call-template name="gap"><xsl:with-param name="what" select="concat('&lt;become&gt; to ', $quote, @sprite, $quote, ', a look it has not')" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- A grid's cell (a kind's) to one of its looks: one.look = River::Look::Turtlerow3Under;
       how long it has looked so back to nothing, and its size when its
       looks differ in size. -->
  <xsl:template match="become[not(@object)]" mode="touch">
    <xsl:param name="who" />
    <xsl:param name="self" />
    <xsl:variable name="owner" select="ancestor::group[1]" />
    <xsl:choose>
      <xsl:when test="$owner[count(. | $grids) = count($grids) or parent::group[count(. | $grids) = count($grids)]]">
        <xsl:call-template name="cell-becomes"><xsl:with-param name="cell" select="translate($who, '*', '')" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:apply-templates select="." mode="do"><xsl:with-param name="self" select="$self" /></xsl:apply-templates></xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="cell-becomes">
    <xsl:param name="cell" />
    <xsl:variable name="owner" select="ancestor::group[1]" />
    <xsl:variable name="grid" select="$owner[count(. | $grids) = count($grids)] | $owner/parent::group[count(. | $grids) = count($grids)]" />
    <xsl:variable name="kind" select="$owner[count(. | $grid) != count($grid)]" />
    <xsl:variable name="look" select="($kind/sprite | $grid[not($kind)]/sprite)[@name = current()/@sprite]" />
    <xsl:variable name="named">
      <xsl:choose>
        <xsl:when test="$kind"><xsl:call-template name="kind-look"><xsl:with-param name="kind" select="$kind" /><xsl:with-param name="sprite" select="$look" /></xsl:call-template></xsl:when>
        <xsl:otherwise><xsl:value-of select="@sprite" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:value-of select="concat($cell, '.look = ')" />
    <xsl:call-template name="look-name"><xsl:with-param name="object" select="$grid" /><xsl:with-param name="look" select="$named" /></xsl:call-template>
    <xsl:text>;</xsl:text>
    <xsl:if test="count($grid | $timed-grids) = count($timed-grids)">
      <xsl:value-of select="concat('&#10;', $cell, '.lookTime = 0;')" />
    </xsl:if>
    <xsl:if test="not($kind)">
      <xsl:call-template name="cell-look-size"><xsl:with-param name="cell" select="$cell" /><xsl:with-param name="look" select="$look" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template>
      <xsl:call-template name="cell-look-size"><xsl:with-param name="cell" select="$cell" /><xsl:with-param name="look" select="$look" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template>
    </xsl:if>
  </xsl:template>
  <!-- pad.size.y = 32; when the grid's looks differ that way -->
  <xsl:template name="cell-look-size">
    <xsl:param name="cell" />
    <xsl:param name="look" />
    <xsl:param name="axis" />
    <xsl:variable name="size"><xsl:call-template name="look-size"><xsl:with-param name="look" select="$look" /><xsl:with-param name="axis" select="$axis" /></xsl:call-template></xsl:variable>
    <xsl:variable name="differ">
      <xsl:for-each select="$look/../sprite">
        <xsl:variable name="each"><xsl:call-template name="look-size"><xsl:with-param name="look" select="." /><xsl:with-param name="axis" select="$axis" /></xsl:call-template></xsl:variable>
        <xsl:if test="$each != $size">x</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:if test="$differ != ''"><xsl:value-of select="concat('&#10;', $cell, '.size.', $axis, ' = ', $size, ';')" /></xsl:if>
  </xsl:template>

  <xsl:template match="become" mode="do">
    <xsl:variable name="object" select="$everything[@name = current()/@object] | $states[@name = substring-before(current()/@object, '.')]/objects/object[@name = substring-after(current()/@object, '.')]" />
    <xsl:variable name="thing"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:variable name="look" select="$object/sprite[@name = current()/@sprite]" />
    <xsl:variable name="width"><xsl:call-template name="look-size"><xsl:with-param name="look" select="$look" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="height"><xsl:call-template name="look-size"><xsl:with-param name="look" select="$look" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
    <!-- a size that differs between its looks, this look's -->
    <xsl:variable name="widths">
      <xsl:for-each select="$object/sprite">
        <xsl:variable name="each"><xsl:call-template name="look-size"><xsl:with-param name="look" select="." /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
        <xsl:if test="$each != $width">x</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="heights">
      <xsl:for-each select="$object/sprite">
        <xsl:variable name="each"><xsl:call-template name="look-size"><xsl:with-param name="look" select="." /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:variable>
        <xsl:if test="$each != $height">x</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="lines">
      <xsl:value-of select="concat($thing, '.look = ')" />
      <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@sprite" /></xsl:call-template>
      <xsl:text>;</xsl:text>
      <xsl:if test="$widths != ''"><xsl:value-of select="concat('&#10;', $thing, '.size.x = ', $width, ';')" /></xsl:if>
      <xsl:if test="$heights != ''"><xsl:value-of select="concat('&#10;', $thing, '.size.y = ', $height, ';')" /></xsl:if>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="@from">
        <!-- only when it shows that look -->
        <xsl:value-of select="concat('if (', $thing, '.look == ')" />
        <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@from" /></xsl:call-template>
        <xsl:text>)</xsl:text>
        <xsl:choose>
          <xsl:when test="contains($lines, '&#10;')">
            <xsl:text>&#10;{&#10;&#9;</xsl:text>
            <xsl:call-template name="replace"><xsl:with-param name="text" select="$lines" /><xsl:with-param name="from" select="'&#10;'" /><xsl:with-param name="to" select="'&#10;&#9;'" /></xsl:call-template>
            <xsl:text>&#10;}</xsl:text>
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="concat(' { ', $lines, ' }')" /></xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$lines" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A look's width (x) or height (y): its rectangle's, or its picture's. -->
  <xsl:template name="look-size">
    <xsl:param name="look" />
    <xsl:param name="axis" />
    <xsl:variable name="bitmap" select="$pictures[@name = $look/bitmap/@from]" />
    <xsl:choose>
      <xsl:when test="$bitmap">
        <xsl:variable name="scale"><xsl:call-template name="picture-scale"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template></xsl:variable>
        <xsl:choose>
          <xsl:when test="$axis = 'x'"><xsl:value-of select="string-length(normalize-space($bitmap/row[1])) * $scale" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="count($bitmap/row) * $scale" /></xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$look/rectangle">
        <xsl:call-template name="value">
          <xsl:with-param name="node" select="$look/rectangle/*[local-name() = substring('widthheight', 1 + 5 * number($axis = 'y'), 5 + number($axis = 'y'))]" />
          <xsl:with-param name="self" select="$look/.." />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
