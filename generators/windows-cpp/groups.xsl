<?xml version="1.0" encoding="UTF-8"?>
<!-- groups.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 7, 2026 -->

<!-- A group, shape by shape, as the engine makes it (game_xml.cpp, readGroup):
     its members in the order written, or its cells the top row first and left
     to right, each with what the group says changed by its member, or by the
     rows, columns and cell that pick it, in that order. That is worked out
     once, here, into $group-data; main.xsl writes the program from it: every
     look once (a loop over all of them for the group's own, then the cells or
     members a change picks), each row of cells where its looks put it, and
     the velocities.

     $group-data is one <group name columns rows base-look base-velocity> for
     each group shown, holding:
       <base>     the group's own <look> (when it has a sprite) and <velocity>
       <shape>    one for each member or cell: i (its place in the vector), row
                  and column, look (its look's key), velocity (its velocity's
                  key), gap (what its <padding> before it says), and for a
                  cell, slot (its row's look) and center (yes when its look is
                  another size than its row's)
       <line>     one for each row of cells: row, slot (the row's look: the
                  rows' changes alone) and above (the gap above it)
       <look>     each look once, in the order first shown: key, size (what
                  its size depends on), name (what its picture is named for),
                  comment, rows (a bitmap's, to share them), and the <sprite>
     A field of a look or a velocity carries said: its text, or for a value
     tag the node it came from, so two looks are the same when every field
     says the same. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    exclude-result-prefixes="exsl">

  <xsl:variable name="group-tree">
    <xsl:for-each select="$groups">
      <xsl:call-template name="group-data" />
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="group-data" select="exsl:node-set($group-tree)/group" />

  <!-- One group's data (above). -->
  <xsl:template name="group-data">
    <xsl:variable name="group" select="." />
    <xsl:variable name="first-tree">
      <base>
        <xsl:call-template name="resolve-look">
          <xsl:with-param name="group" select="$group" />
          <xsl:with-param name="parts" select="/.." />
        </xsl:call-template>
        <xsl:call-template name="resolve-vector">
          <xsl:with-param name="group" select="$group" />
          <xsl:with-param name="parts" select="/.." />
          <xsl:with-param name="tag" select="'velocity'" />
        </xsl:call-template>
      </base>
      <xsl:choose>
        <xsl:when test="columns">
          <xsl:call-template name="cell-rows">
            <xsl:with-param name="group" select="$group" />
            <xsl:with-param name="row" select="1" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:for-each select="member">
            <xsl:call-template name="shape-data">
              <xsl:with-param name="group" select="$group" />
              <xsl:with-param name="parts" select="." />
              <xsl:with-param name="index" select="position() - 1" />
            </xsl:call-template>
          </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="first" select="exsl:node-set($first-tree)" />

    <group name="{@name}" base-look="{$first/base/look/@key}" base-velocity="{$first/base/velocity/@key}">
      <xsl:attribute name="columns">
        <xsl:choose>
          <xsl:when test="columns"><xsl:value-of select="number(normalize-space(columns))" /></xsl:when>
          <xsl:otherwise>0</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:attribute name="rows">
        <xsl:choose>
          <xsl:when test="rows"><xsl:value-of select="number(normalize-space(rows))" /></xsl:when>
          <xsl:otherwise>0</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:copy-of select="$first/base" />
      <xsl:for-each select="$first/shape">
        <xsl:if test="not(look)">
          <xsl:for-each select="$group">
            <xsl:call-template name="refuse">
              <xsl:with-param name="what" select="'a member or cell with no &lt;sprite&gt;, in a group with none'" />
            </xsl:call-template>
          </xsl:for-each>
        </xsl:if>
        <xsl:variable name="row" select="@row" />
        <xsl:variable name="slot" select="$first/line[@row = $row]/look" />
        <xsl:copy>
          <xsl:copy-of select="@*" />
          <xsl:if test="$slot">
            <xsl:attribute name="slot"><xsl:value-of select="$slot/@key" /></xsl:attribute>
            <xsl:attribute name="center">
              <xsl:call-template name="other-size">
                <xsl:with-param name="look" select="look" />
                <xsl:with-param name="than" select="$slot" />
              </xsl:call-template>
            </xsl:attribute>
          </xsl:if>
          <xsl:copy-of select="*" />
        </xsl:copy>
      </xsl:for-each>
      <xsl:copy-of select="$first/line" />
      <xsl:for-each select="($first/base | $first/shape | $first/line)/look">
        <xsl:variable name="key" select="@key" />
        <xsl:if test="not(preceding::look[@key = $key])">
          <xsl:copy-of select="." />
        </xsl:if>
      </xsl:for-each>
    </group>
  </xsl:template>

  <!-- The rows of a group in columns and rows, from `row` on: each row's line,
       then its cells. -->
  <xsl:template name="cell-rows">
    <xsl:param name="group" />
    <xsl:param name="row" />
    <xsl:if test="$row &lt;= number(normalize-space($group/rows))">
      <xsl:variable name="rows" select="$group/row[contains(concat(' ', normalize-space(@number), ' '), concat(' ', $row, ' '))
          or ($row mod 2 = 1 and contains(concat(' ', normalize-space(@number), ' '), ' odd '))
          or ($row mod 2 = 0 and contains(concat(' ', normalize-space(@number), ' '), ' even '))]" />
      <xsl:variable name="above" select="($group/padding/y | $rows/padding/y)[last()][$row &gt; 1]" />
      <line row="{$row}">
        <xsl:attribute name="above"><xsl:call-template name="said"><xsl:with-param name="node" select="$above" /></xsl:call-template></xsl:attribute>
        <xsl:variable name="look-tree">
          <xsl:call-template name="resolve-look">
            <xsl:with-param name="group" select="$group" />
            <xsl:with-param name="parts" select="$rows" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:attribute name="slot"><xsl:value-of select="exsl:node-set($look-tree)/look/@key" /></xsl:attribute>
        <xsl:attribute name="size"><xsl:value-of select="exsl:node-set($look-tree)/look/@size" /></xsl:attribute>
        <xsl:copy-of select="$look-tree" />
        <xsl:if test="$above"><above><xsl:copy-of select="$above" /></above></xsl:if>
      </line>
      <xsl:call-template name="cell-columns">
        <xsl:with-param name="group" select="$group" />
        <xsl:with-param name="row" select="$row" />
        <xsl:with-param name="rows" select="$rows" />
        <xsl:with-param name="column" select="1" />
      </xsl:call-template>
      <xsl:call-template name="cell-rows">
        <xsl:with-param name="group" select="$group" />
        <xsl:with-param name="row" select="$row + 1" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- The cells of one row, from `column` on. -->
  <xsl:template name="cell-columns">
    <xsl:param name="group" />
    <xsl:param name="row" />
    <xsl:param name="rows" />
    <xsl:param name="column" />
    <xsl:variable name="count" select="number(normalize-space($group/columns))" />
    <xsl:if test="$column &lt;= $count">
      <xsl:variable name="columns" select="$group/column[contains(concat(' ', normalize-space(@number), ' '), concat(' ', $column, ' '))
          or ($column mod 2 = 1 and contains(concat(' ', normalize-space(@number), ' '), ' odd '))
          or ($column mod 2 = 0 and contains(concat(' ', normalize-space(@number), ' '), ' even '))]" />
      <xsl:variable name="cell" select="$group/cell[normalize-space(@row) = $row and normalize-space(@column) = $column]" />
      <xsl:call-template name="shape-data">
        <xsl:with-param name="group" select="$group" />
        <xsl:with-param name="parts" select="$rows | $columns | $cell" />
        <xsl:with-param name="index" select="($row - 1) * $count + $column - 1" />
        <xsl:with-param name="row" select="$row" />
        <xsl:with-param name="column" select="$column" />
        <xsl:with-param name="gap" select="($group/padding/x | $rows/padding/x | $columns/padding/x)[last()][$column &gt; 1]" />
      </xsl:call-template>
      <xsl:call-template name="cell-columns">
        <xsl:with-param name="group" select="$group" />
        <xsl:with-param name="row" select="$row" />
        <xsl:with-param name="rows" select="$rows" />
        <xsl:with-param name="column" select="$column + 1" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- One member or cell: the group changed by `parts`, in the order written. -->
  <xsl:template name="shape-data">
    <xsl:param name="group" />
    <xsl:param name="parts" />
    <xsl:param name="index" />
    <xsl:param name="row" select="0" />
    <xsl:param name="column" select="0" />
    <xsl:param name="gap" select="/.." />
    <xsl:variable name="look-tree">
      <xsl:call-template name="resolve-look">
        <xsl:with-param name="group" select="$group" />
        <xsl:with-param name="parts" select="$parts" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="velocity-tree">
      <xsl:call-template name="resolve-vector">
        <xsl:with-param name="group" select="$group" />
        <xsl:with-param name="parts" select="$parts" />
        <xsl:with-param name="tag" select="'velocity'" />
      </xsl:call-template>
    </xsl:variable>
    <shape i="{$index}" row="{$row}" column="{$column}" look="{exsl:node-set($look-tree)/look/@key}" velocity="{exsl:node-set($velocity-tree)/velocity/@key}">
      <xsl:attribute name="gap"><xsl:call-template name="said"><xsl:with-param name="node" select="$gap" /></xsl:call-template></xsl:attribute>
      <xsl:copy-of select="$look-tree" />
      <xsl:copy-of select="$velocity-tree" />
      <xsl:if test="$gap"><gap><xsl:copy-of select="$gap" /></gap></xsl:if>
    </shape>
  </xsl:template>

  <!-- The look the group's sprite becomes with the sprites of `parts`, each
       in turn: one of the same shape changes only what it gives (a <color>),
       and one of another shape, or of lines, is a look of its own. Nothing
       when neither the group nor its parts have a sprite. -->
  <xsl:template name="resolve-look">
    <xsl:param name="group" />
    <xsl:param name="parts" />
    <xsl:variable name="all" select="$group/sprite | $parts/sprite" />
    <xsl:if test="$all">
      <xsl:variable name="kind" select="local-name($all[last()]/*[1])" />
      <!-- the last sprite that starts a look of its own, then those that change it -->
      <xsl:variable name="other" select="$all[local-name(*[1]) != $kind][last()]" />
      <xsl:variable name="chain" select="$all[$kind != 'line' or position() = last()][not($other) or preceding::sprite[generate-id() = generate-id($other)]]" />
      <xsl:variable name="sprite-tree">
        <sprite>
          <xsl:choose>
            <xsl:when test="$kind = 'line'">
              <xsl:for-each select="$chain/line">
                <line said="{generate-id()}"><xsl:copy-of select="node()" /></line>
              </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>
              <xsl:element name="{$kind}">
                <!-- each field from the last that gives it -->
                <xsl:for-each select="$chain/*/*">
                  <xsl:sort select="local-name()" />
                  <xsl:variable name="field" select="local-name()" />
                  <xsl:variable name="in" select="generate-id(..)" />
                  <xsl:if test="not($chain/*[preceding::*[generate-id() = $in]][*[local-name() = $field]])">
                    <xsl:element name="{$field}">
                      <xsl:attribute name="said"><xsl:call-template name="said"><xsl:with-param name="node" select="." /></xsl:call-template></xsl:attribute>
                      <xsl:copy-of select="node()" />
                    </xsl:element>
                  </xsl:if>
                </xsl:for-each>
              </xsl:element>
            </xsl:otherwise>
          </xsl:choose>
        </sprite>
      </xsl:variable>
      <xsl:variable name="sprite" select="exsl:node-set($sprite-tree)/sprite" />
      <xsl:variable name="from" select="$chain/parent::*[not(self::group)]" />
      <look>
        <xsl:attribute name="key">
          <xsl:value-of select="$kind" />
          <xsl:for-each select="$sprite/*/*[$kind != 'line'] | $sprite/line">
            <xsl:value-of select="concat(' ', local-name(), '=', @said)" />
          </xsl:for-each>
        </xsl:attribute>
        <!-- what its size depends on: a circle's radius, a rectangle's width
             and height, or all of a picture -->
        <xsl:attribute name="size">
          <xsl:choose>
            <xsl:when test="$kind = 'circle'"><xsl:value-of select="concat('circle ', $sprite/circle/radius/@said)" /></xsl:when>
            <xsl:when test="$kind = 'rectangle'"><xsl:value-of select="concat('rectangle ', $sprite/rectangle/width/@said, ' ', $sprite/rectangle/height/@said)" /></xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$kind" />
              <xsl:for-each select="$sprite/*/*[$kind != 'line'] | $sprite/line">
                <xsl:value-of select="concat(' ', local-name(), '=', @said)" />
              </xsl:for-each>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:if test="$kind = 'bitmap'">
          <xsl:attribute name="rows">
            <xsl:for-each select="$sprite/bitmap/row"><xsl:value-of select="concat(@said, '|')" /></xsl:for-each>
          </xsl:attribute>
        </xsl:if>
        <xsl:attribute name="name">
          <xsl:value-of select="$group/@name" />
          <xsl:for-each select="$from">
            <xsl:text>.</xsl:text>
            <xsl:call-template name="part-name" />
          </xsl:for-each>
        </xsl:attribute>
        <xsl:attribute name="comment">
          <xsl:choose>
            <xsl:when test="$from/self::member">
              <xsl:value-of select="concat($group/@name, '.')" />
              <xsl:for-each select="$from"><xsl:call-template name="part-name" /></xsl:for-each>
              <xsl:text>, a look of its own</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="$group/@name" />
              <xsl:for-each select="$from">
                <xsl:text>, </xsl:text>
                <xsl:call-template name="part-title" />
              </xsl:for-each>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:attribute>
        <xsl:copy-of select="$sprite" />
      </look>
    </xsl:if>
  </xsl:template>

  <!-- The group's <position> or <velocity> (`tag`) as `parts` change it: an x
       and a y each from the last that gives it. -->
  <xsl:template name="resolve-vector">
    <xsl:param name="group" />
    <xsl:param name="parts" />
    <xsl:param name="tag" />
    <xsl:variable name="all" select="$group/*[local-name() = $tag] | $parts/*[local-name() = $tag]" />
    <xsl:variable name="x-said"><xsl:call-template name="said"><xsl:with-param name="node" select="($all/x)[last()]" /></xsl:call-template></xsl:variable>
    <xsl:variable name="y-said"><xsl:call-template name="said"><xsl:with-param name="node" select="($all/y)[last()]" /></xsl:call-template></xsl:variable>
    <xsl:element name="{$tag}">
      <xsl:attribute name="key"><xsl:value-of select="concat($x-said, ', ', $y-said)" /></xsl:attribute>
      <xsl:attribute name="comment">
        <xsl:value-of select="$group/@name" />
        <xsl:for-each select="$parts[*[local-name() = $tag]]">
          <xsl:text>, </xsl:text>
          <xsl:call-template name="part-title" />
        </xsl:for-each>
      </xsl:attribute>
      <xsl:copy-of select="($all/x)[last()] | ($all/y)[last()]" />
    </xsl:element>
  </xsl:template>

  <!-- What a field or a value says, to tell two apart: its text, or the
       node a value tag is (two <random>s are never the same). -->
  <xsl:template name="said">
    <xsl:param name="node" />
    <xsl:choose>
      <xsl:when test="not($node)" />
      <xsl:when test="$node/*"><xsl:value-of select="generate-id($node)" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="normalize-space($node)" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A part as part of a name (row.2, column.3, column.3.row.2, 3 for the
       third member), the dots taken out by cpp-name. -->
  <xsl:template name="part-name">
    <xsl:choose>
      <xsl:when test="self::member and @name"><xsl:value-of select="@name" /></xsl:when>
      <xsl:when test="self::member"><xsl:value-of select="count(preceding-sibling::member) + 1" /></xsl:when>
      <xsl:when test="self::cell and @name"><xsl:value-of select="@name" /></xsl:when>
      <xsl:when test="self::cell"><xsl:value-of select="concat('column.', normalize-space(@column), '.row.', normalize-space(@row))" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat(local-name(), '.', translate(normalize-space(@number), ' ', '.'))" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A part as a comment says it: row 2, rows 2, 3, every even row, column 3, row 2. -->
  <xsl:template name="part-title">
    <xsl:variable name="number" select="normalize-space(@number)" />
    <xsl:choose>
      <xsl:when test="self::member and @name"><xsl:value-of select="@name" /></xsl:when>
      <xsl:when test="self::member"><xsl:value-of select="concat('member ', count(preceding-sibling::member) + 1)" /></xsl:when>
      <xsl:when test="self::cell and @name"><xsl:value-of select="@name" /></xsl:when>
      <xsl:when test="self::cell"><xsl:value-of select="concat('column ', normalize-space(@column), ', row ', normalize-space(@row))" /></xsl:when>
      <xsl:when test="$number = 'odd' or $number = 'even'"><xsl:value-of select="concat('every ', $number, ' ', local-name())" /></xsl:when>
      <xsl:when test="contains($number, ' ')">
        <xsl:value-of select="concat(local-name(), 's ')" />
        <xsl:call-template name="replace">
          <xsl:with-param name="text" select="$number" />
          <xsl:with-param name="from" select="' '" />
          <xsl:with-param name="to" select="', '" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="concat(local-name(), ' ', $number)" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- yes when `look` is another size than `than` (its row's). -->
  <xsl:template name="other-size">
    <xsl:param name="look" />
    <xsl:param name="than" />
    <xsl:if test="$look/@size != $than/@size">yes</xsl:if>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- What main.xsl writes from it                                           -->
  <!-- ===================================================================== -->

  <!-- In setup(): the group's own look on every one of them, in a loop, then
       each other look on the members or cells that have it. Of the same
       shape, only what is not the group's is set (a row's color). -->
  <xsl:template name="group-looks">
    <xsl:param name="name" />
    <xsl:variable name="data" select="$group-data[@name = current()/@name]" />
    <xsl:variable name="base" select="$data/look[@key = $data/@base-look]" />
    <xsl:if test="$base">
      <xsl:variable name="type"><xsl:call-template name="sf-type" /></xsl:variable>
      <xsl:value-of select="concat('&#9;for (', $type, '&amp; one : ', $name, ')&#10;&#9;{&#10;')" />
      <xsl:for-each select="$base/sprite">
        <xsl:call-template name="set-look">
          <xsl:with-param name="name" select="'one'" />
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:text>	}
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$data/look[@key != string($data/@base-look)]">
      <xsl:variable name="key" select="@key" />
      <xsl:variable name="set" select="$data/shape[@look = $key]" />
      <xsl:if test="$set">
        <xsl:if test="$base or $data/@columns != 0">
          <xsl:value-of select="concat('&#9;// ', @comment, '&#10;')" />
        </xsl:if>
        <xsl:call-template name="for-shapes">
          <xsl:with-param name="set" select="$set" />
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="body">
            <xsl:for-each select="sprite">
              <xsl:call-template name="set-look">
                <xsl:with-param name="name" select="'@'" />
                <xsl:with-param name="indent" select="''" />
                <xsl:with-param name="base" select="$base/sprite" />
              </xsl:call-template>
            </xsl:for-each>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- In start(): a group in columns and rows. Each cell is a slot the size of
       its row's look, the cells of a row left to right and the rows down,
       each with the padding before it; a cell of another size sits in the
       middle of its slot. When every row is alike, two loops; otherwise row
       by row. Then the velocities: the group's for all of them, and each
       other on the cells that have it. -->
  <xsl:template name="cells-start">
    <xsl:param name="name" />
    <xsl:variable name="data" select="$group-data[@name = current()/@name]" />
    <xsl:variable name="columns" select="number($data/@columns)" />
    <xsl:variable name="rows" select="number($data/@rows)" />
    <xsl:variable name="lines" select="$data/line" />
    <xsl:variable name="x"><xsl:call-template name="value-bare"><xsl:with-param name="node" select="position/x" /></xsl:call-template></xsl:variable>
    <xsl:variable name="y"><xsl:call-template name="value-bare"><xsl:with-param name="node" select="position/y" /></xsl:call-template></xsl:variable>
    <xsl:variable name="alike" select="not($lines[@size != $lines[1]/@size]) and not($lines[@row &gt; 1][@above != $lines[@row &gt; 1][1]/@above])
        and not($data/shape[@column &gt; 1][@gap != $data/shape[@column &gt; 1][1]/@gap])" />
    <xsl:choose>
      <xsl:when test="$alike">
        <xsl:variable name="slot" select="$data/look[@key = $lines[1]/@slot]" />
        <xsl:variable name="step-x">
          <xsl:call-template name="grid-step">
            <xsl:with-param name="size"><xsl:call-template name="look-size"><xsl:with-param name="look" select="$slot" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:with-param>
            <xsl:with-param name="padding" select="$data/shape[@column &gt; 1][1]/gap/*" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="step-y">
          <xsl:call-template name="grid-step">
            <xsl:with-param name="size"><xsl:call-template name="look-size"><xsl:with-param name="look" select="$slot" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template></xsl:with-param>
            <xsl:with-param name="padding" select="$lines[@row &gt; 1][1]/above/*" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="index">
          <xsl:choose>
            <xsl:when test="$columns &gt; 1 and $rows &gt; 1"><xsl:value-of select="concat('row * ', $columns, ' + column')" /></xsl:when>
            <xsl:when test="$columns &gt; 1">column</xsl:when>
            <xsl:when test="$rows &gt; 1">row</xsl:when>
            <xsl:otherwise>0</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:variable name="at-x">
          <xsl:value-of select="$x" />
          <xsl:if test="$columns &gt; 1"><xsl:value-of select="concat(' + static_cast&lt;float&gt;(column) * ', $step-x)" /></xsl:if>
        </xsl:variable>
        <xsl:variable name="at-y">
          <xsl:value-of select="$y" />
          <xsl:if test="$rows &gt; 1"><xsl:value-of select="concat(' + static_cast&lt;float&gt;(row) * ', $step-y)" /></xsl:if>
        </xsl:variable>
        <xsl:variable name="in">
          <xsl:text>&#9;</xsl:text>
          <xsl:if test="$columns &gt; 1"><xsl:text>&#9;</xsl:text></xsl:if>
          <xsl:if test="$rows &gt; 1"><xsl:text>&#9;</xsl:text></xsl:if>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;// ', @name, ': ', $columns, ' columns by ', $rows, ' rows&#10;')" />
        <xsl:if test="$rows &gt; 1">
          <xsl:value-of select="concat('&#9;for (std::size_t row = 0; row &lt; ', $rows, '; ++row)&#10;&#9;{&#10;')" />
        </xsl:if>
        <xsl:if test="$columns &gt; 1">
          <xsl:variable name="out" select="substring('&#9;&#9;', 1, 1 + number($rows &gt; 1))" />
          <xsl:value-of select="concat($out, 'for (std::size_t column = 0; column &lt; ', $columns, '; ++column)&#10;', $out, '{&#10;')" />
        </xsl:if>
        <xsl:value-of select="concat($in, $name, '[', $index, '].setPosition({', $at-x, ', ', $at-y, '});&#10;')" />
        <xsl:if test="$columns &gt; 1">
          <xsl:value-of select="concat(substring('&#9;&#9;', 1, 1 + number($rows &gt; 1)), '}&#10;')" />
        </xsl:if>
        <xsl:if test="$rows &gt; 1">
          <xsl:text>	}
</xsl:text>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="rows-start">
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="data" select="$data" />
          <xsl:with-param name="x" select="$x" />
          <xsl:with-param name="y" select="$y" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>

    <!-- a cell of another size than its row's, in the middle of its slot -->
    <xsl:for-each select="$data/shape[@center = 'yes']">
      <xsl:variable name="look" select="@look" />
      <xsl:variable name="slot" select="@slot" />
      <xsl:if test="not(preceding-sibling::shape[@center = 'yes'][@look = $look][@slot = $slot])">
        <xsl:variable name="mine" select="$data/look[@key = $look]" />
        <xsl:variable name="its" select="$data/look[@key = $slot]" />
        <xsl:value-of select="concat('&#9;// ', $mine/@comment, ': in the middle of its place&#10;')" />
        <xsl:call-template name="for-shapes">
          <xsl:with-param name="set" select="$data/shape[@center = 'yes'][@look = $look][@slot = $slot]" />
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="body">
            <xsl:text>@.move({(</xsl:text>
            <xsl:call-template name="look-size"><xsl:with-param name="look" select="$its" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template>
            <xsl:text> - </xsl:text>
            <xsl:call-template name="look-size"><xsl:with-param name="look" select="$mine" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template>
            <xsl:text>) / 2.0f, (</xsl:text>
            <xsl:call-template name="look-size"><xsl:with-param name="look" select="$its" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template>
            <xsl:text> - </xsl:text>
            <xsl:call-template name="look-size"><xsl:with-param name="look" select="$mine" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template>
            <xsl:text>) / 2.0f});&#10;</xsl:text>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:if>
    </xsl:for-each>

    <xsl:choose>
      <xsl:when test="count(. | $member-velocities) = count($member-velocities)">
        <xsl:value-of select="concat('&#9;', $name, 'Velocity.assign(', count($data/shape), ', ')" />
        <xsl:call-template name="merged-vector">
          <xsl:with-param name="own" select="$data/base/velocity" />
          <xsl:with-param name="shared" select="/.." />
        </xsl:call-template>
        <xsl:text>);
</xsl:text>
        <xsl:for-each select="$data/shape[@velocity != $data/@base-velocity]">
          <xsl:variable name="key" select="@velocity" />
          <xsl:if test="not(preceding-sibling::shape[@velocity = $key])">
            <xsl:value-of select="concat('&#9;// ', velocity/@comment, '&#10;')" />
            <xsl:variable name="vector">
              <xsl:call-template name="merged-vector">
                <xsl:with-param name="own" select="velocity" />
                <xsl:with-param name="shared" select="/.." />
              </xsl:call-template>
            </xsl:variable>
            <xsl:call-template name="for-shapes">
              <xsl:with-param name="set" select="$data/shape[@velocity = $key]" />
              <xsl:with-param name="name" select="concat($name, 'Velocity')" />
              <xsl:with-param name="body" select="concat('@ = ', $vector, ';&#10;')" />
            </xsl:call-template>
          </xsl:if>
        </xsl:for-each>
      </xsl:when>
      <xsl:when test="count(. | $moving) = count($moving)">
        <xsl:value-of select="concat('&#9;', $name, 'Velocity = ')" />
        <xsl:call-template name="vector"><xsl:with-param name="node" select="velocity" /></xsl:call-template>
        <xsl:text>;
</xsl:text>
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- Rows that are not alike, one at a time, a running top (nameTop) the
       height of each row and the gap below it on from the last; within a row,
       a loop when its gaps are alike, or a running left (nameLeft). -->
  <xsl:template name="rows-start">
    <xsl:param name="name" />
    <xsl:param name="data" />
    <xsl:param name="x" />
    <xsl:param name="y" />
    <xsl:variable name="columns" select="number($data/@columns)" />
    <xsl:variable name="rows" select="number($data/@rows)" />
    <xsl:variable name="top" select="concat($name, 'Top')" />
    <xsl:variable name="left" select="concat($name, 'Left')" />
    <xsl:value-of select="concat('&#9;// ', @name, ': ', $columns, ' columns by ', $rows, ' rows, each row as big as its look&#10;')" />
    <xsl:value-of select="concat('&#9;float ', $top, ' = ', $y, ';&#10;')" />
    <!-- the rows whose gaps are not alike, written a cell at a time -->
    <xsl:variable name="uneven">
      <xsl:if test="$columns &gt; 1">
        <xsl:for-each select="$data/line">
          <xsl:variable name="row" select="number(@row)" />
          <xsl:variable name="cells" select="$data/shape[@row = $row][@column &gt; 1]" />
          <xsl:if test="$cells[@gap != $cells[1]/@gap]"><xsl:value-of select="concat(' ', $row, ' ')" /></xsl:if>
        </xsl:for-each>
      </xsl:if>
    </xsl:variable>
    <xsl:for-each select="$data/line">
      <xsl:variable name="row" select="number(@row)" />
      <xsl:variable name="cells" select="$data/shape[@row = $row]" />
      <xsl:variable name="slot" select="$data/look[@key = current()/@slot]" />
      <xsl:variable name="from" select="($row - 1) * $columns" />
      <xsl:variable name="width"><xsl:call-template name="look-size"><xsl:with-param name="look" select="$slot" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template></xsl:variable>
      <xsl:if test="$rows &gt; 1">
        <xsl:value-of select="concat('&#9;// row ', $row, '&#10;')" />
      </xsl:if>
      <xsl:choose>
        <xsl:when test="$columns = 1">
          <xsl:value-of select="concat('&#9;', $name, '[', $from, '].setPosition({', $x, ', ', $top, '});&#10;')" />
        </xsl:when>
        <xsl:when test="not(contains($uneven, concat(' ', $row, ' ')))">
          <xsl:variable name="step">
            <xsl:call-template name="grid-step">
              <xsl:with-param name="size" select="$width" />
              <xsl:with-param name="padding" select="$cells[@column &gt; 1][1]/gap/*" />
            </xsl:call-template>
          </xsl:variable>
          <xsl:variable name="index">
            <xsl:if test="$from &gt; 0"><xsl:value-of select="concat($from, ' + ')" /></xsl:if>
            <xsl:text>column</xsl:text>
          </xsl:variable>
          <xsl:value-of select="concat('&#9;for (std::size_t column = 0; column &lt; ', $columns, '; ++column)&#10;&#9;{&#10;')" />
          <xsl:value-of select="concat('&#9;&#9;', $name, '[', $index, '].setPosition({', $x, ' + static_cast&lt;float&gt;(column) * ', $step, ', ', $top, '});&#10;&#9;}&#10;')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>&#9;</xsl:text>
          <xsl:if test="starts-with($uneven, concat(' ', $row, ' '))">float </xsl:if>
          <xsl:value-of select="concat($left, ' = ', $x, ';&#10;')" />
          <xsl:for-each select="$cells">
            <xsl:if test="@column &gt; 1">
              <xsl:value-of select="concat('&#9;', $left, ' += ', $width)" />
              <xsl:if test="gap/*">
                <xsl:text> + </xsl:text>
                <xsl:call-template name="value"><xsl:with-param name="node" select="gap/*" /></xsl:call-template>
              </xsl:if>
              <xsl:text>;
</xsl:text>
            </xsl:if>
            <xsl:value-of select="concat('&#9;', $name, '[', @i, '].setPosition({', $left, ', ', $top, '});&#10;')" />
          </xsl:for-each>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:variable name="next" select="following-sibling::line[1]" />
      <xsl:if test="$next">
        <xsl:value-of select="concat('&#9;', $top, ' += ')" />
        <xsl:call-template name="look-size"><xsl:with-param name="look" select="$slot" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template>
        <xsl:if test="$next/above/*">
          <xsl:text> + </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="$next/above/*" /></xsl:call-template>
        </xsl:if>
        <xsl:text>;
</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- How wide (axis x) or tall (y) a look is, as C++. -->
  <xsl:template name="look-size">
    <xsl:param name="look" />
    <xsl:param name="axis" />
    <xsl:variable name="shape" select="$look/sprite/*[1]" />
    <xsl:choose>
      <xsl:when test="$shape/self::circle">
        <xsl:text>2.0f * </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$shape/radius" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$shape/self::rectangle and $axis = 'x'">
        <xsl:call-template name="value"><xsl:with-param name="node" select="$shape/width" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$shape/self::rectangle">
        <xsl:call-template name="value"><xsl:with-param name="node" select="$shape/height" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>static_cast&lt;float&gt;(</xsl:text>
        <xsl:for-each select="$look/sprite"><xsl:call-template name="sprite-texture" /></xsl:for-each>
        <xsl:value-of select="concat('.getSize().', $axis, ')')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- `body` (lines, @ for the shape) for each of `set`, shapes of one
       group's data, in `name` (the shapes, or their velocities): once for one;
       a loop over a run of them; over whole rows or whole columns; or over a
       list of them (unsigned, so a std::size_t takes them without a warning). -->
  <xsl:template name="for-shapes">
    <xsl:param name="set" />
    <xsl:param name="name" />
    <xsl:param name="body" />
    <xsl:variable name="data" select="$set[1]/.." />
    <xsl:variable name="columns" select="number($data/@columns)" />
    <xsl:variable name="rows" select="number($data/@rows)" />
    <xsl:variable name="first" select="number($set[1]/@i)" />
    <xsl:variable name="last" select="number($set[last()]/@i)" />
    <xsl:variable name="part-rows">
      <xsl:for-each select="$set">
        <xsl:variable name="row" select="@row" />
        <xsl:if test="count($set[@row = $row]) != $columns">x</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="part-columns">
      <xsl:for-each select="$set">
        <xsl:variable name="column" select="@column" />
        <xsl:if test="count($set[@column = $column]) != $rows">x</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="count($set) = 1">
        <xsl:call-template name="body-lines">
          <xsl:with-param name="body" select="$body" />
          <xsl:with-param name="shape" select="concat($name, '[', $first, ']')" />
          <xsl:with-param name="indent" select="'&#9;'" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$last - $first + 1 = count($set)">
        <xsl:value-of select="concat('&#9;for (std::size_t i = ', $first, '; i &lt; ', $last + 1, '; ++i)&#10;&#9;{&#10;')" />
        <xsl:call-template name="body-lines">
          <xsl:with-param name="body" select="$body" />
          <xsl:with-param name="shape" select="concat($name, '[i]')" />
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
        <xsl:text>	}
</xsl:text>
      </xsl:when>
      <xsl:when test="$columns &gt; 0 and $part-rows = ''">
        <xsl:text>	for (std::size_t row : {</xsl:text>
        <xsl:for-each select="$set[@column = 1]">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:value-of select="concat(@row - 1, 'u')" />
        </xsl:for-each>
        <xsl:value-of select="concat('})&#10;&#9;{&#10;&#9;&#9;for (std::size_t column = 0; column &lt; ', $columns, '; ++column)&#10;&#9;&#9;{&#10;')" />
        <xsl:call-template name="body-lines">
          <xsl:with-param name="body" select="$body" />
          <xsl:with-param name="shape" select="concat($name, '[row * ', $columns, ' + column]')" />
          <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
        </xsl:call-template>
        <xsl:text>		}
	}
</xsl:text>
      </xsl:when>
      <xsl:when test="$columns &gt; 0 and $part-columns = ''">
        <xsl:variable name="picked" select="$set[@row = 1]" />
        <xsl:value-of select="concat('&#9;for (std::size_t row = 0; row &lt; ', $rows, '; ++row)&#10;&#9;{&#10;')" />
        <xsl:choose>
          <xsl:when test="count($picked) = 1">
            <xsl:call-template name="body-lines">
              <xsl:with-param name="body" select="$body" />
              <xsl:with-param name="shape" select="concat($name, '[row * ', $columns, ' + ', $picked/@column - 1, ']')" />
              <xsl:with-param name="indent" select="'&#9;&#9;'" />
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>		for (std::size_t column : {</xsl:text>
            <xsl:for-each select="$picked">
              <xsl:if test="position() &gt; 1">, </xsl:if>
              <xsl:value-of select="concat(@column - 1, 'u')" />
            </xsl:for-each>
            <xsl:text>})
		{
</xsl:text>
            <xsl:call-template name="body-lines">
              <xsl:with-param name="body" select="$body" />
              <xsl:with-param name="shape" select="concat($name, '[row * ', $columns, ' + column]')" />
              <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
            </xsl:call-template>
            <xsl:text>		}
</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>	}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>	for (std::size_t i : {</xsl:text>
        <xsl:for-each select="$set">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:value-of select="concat(@i, 'u')" />
        </xsl:for-each>
        <xsl:text>})
	{
</xsl:text>
        <xsl:call-template name="body-lines">
          <xsl:with-param name="body" select="$body" />
          <xsl:with-param name="shape" select="concat($name, '[i]')" />
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
        <xsl:text>	}
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Each line of `body` indented, its @s the shape. -->
  <xsl:template name="body-lines">
    <xsl:param name="body" />
    <xsl:param name="shape" />
    <xsl:param name="indent" />
    <xsl:if test="contains($body, '&#10;')">
      <xsl:value-of select="$indent" />
      <xsl:call-template name="replace">
        <xsl:with-param name="text" select="substring-before($body, '&#10;')" />
        <xsl:with-param name="from" select="'@'" />
        <xsl:with-param name="to" select="$shape" />
      </xsl:call-template>
      <xsl:text>&#10;</xsl:text>
      <xsl:call-template name="body-lines">
        <xsl:with-param name="body" select="substring-after($body, '&#10;')" />
        <xsl:with-param name="shape" select="$shape" />
        <xsl:with-param name="indent" select="$indent" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- `text` with every `from` in it made `to`. -->
  <xsl:template name="replace">
    <xsl:param name="text" />
    <xsl:param name="from" />
    <xsl:param name="to" />
    <xsl:choose>
      <xsl:when test="contains($text, $from)">
        <xsl:value-of select="concat(substring-before($text, $from), $to)" />
        <xsl:call-template name="replace">
          <xsl:with-param name="text" select="substring-after($text, $from)" />
          <xsl:with-param name="from" select="$from" />
          <xsl:with-param name="to" select="$to" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$text" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
