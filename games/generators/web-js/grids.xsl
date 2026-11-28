<?xml version="1.0" encoding="UTF-8"?>
<!-- grids.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- A grid whose cells meet (kinds.xsl) is an array of them in the page:
     let wall = startWall(). A group in it is one kind of its cells, with the
     look, place, numbers and sound it gives them. Its start builds the
     array, a kind at a time; on meeting a thing, the loop over the array
     runs the thing's rules, then the cell's, and a cell that dies is taken
     out. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <!-- The axis the kinds' places give (x or y, or xy for both), when they
       give one. -->
  <xsl:template name="kind-axis">
    <xsl:param name="grid" select="." />
    <xsl:choose>
      <xsl:when test="$grid/group/position/x and $grid/group/position/y">xy</xsl:when>
      <xsl:when test="$grid/group/position/y">y</xsl:when>
      <xsl:when test="$grid/group/position/x">x</xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- { x: 48, y: 28 }: the size of a kind's picture. -->
  <xsl:template name="kind-size-of">
    <xsl:variable name="named"><xsl:call-template name="kind-picture" /></xsl:variable>
    <xsl:variable name="bitmap" select="/game/pictures/bitmap[@name = $named]" />
    <xsl:if test="$bitmap">
      <xsl:variable name="scale"><xsl:call-template name="kind-scale" /></xsl:variable>
      <xsl:value-of select="concat('{ x: ', string-length(normalize-space($bitmap[1]/row[1])) * $scale, ', y: ', count($bitmap[1]/row) * $scale, ' }')" />
    </xsl:if>
  </xsl:template>

  <!-- Whether its kinds each have a sound of their own when they meet. -->
  <xsl:template name="kind-sound">
    <xsl:param name="grid" select="." />
    <xsl:if test="$grid/group/collisions/collision/play">yes</xsl:if>
  </xsl:template>

  <!-- addBricks: the function that builds one kind's rows. -->
  <xsl:template name="add-name">
    <xsl:param name="grid" select="." />
    <xsl:variable name="word"><xsl:call-template name="cell-word"><xsl:with-param name="grid" select="$grid" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('add', translate(substring($word, 1, 1), $lower, $upper), substring($word, 2), 's')" />
  </xsl:template>

  <!-- What a kind gives each of its cells, by name: look, y, worth, sound. -->
  <xsl:template name="kind-parts">
    <xsl:variable name="axis"><xsl:call-template name="kind-axis" /></xsl:variable>
    <xsl:variable name="sound"><xsl:call-template name="kind-sound" /></xsl:variable>
    <xsl:variable name="own-size"><xsl:call-template name="kind-size" /></xsl:variable>
    <xsl:variable name="speed"><xsl:call-template name="kind-speed" /></xsl:variable>
    <xsl:variable name="count"><xsl:call-template name="kind-count" /></xsl:variable>
    <xsl:variable name="gap"><xsl:call-template name="kind-gap" /></xsl:variable>
    <xsl:if test="group and count(. | $looking-grids) = count($looking-grids)"><p>look</p></xsl:if>
    <xsl:choose>
      <xsl:when test="$axis = 'xy'"><p given="yes">first</p></xsl:when>
      <xsl:when test="$axis != ''"><p given="yes"><xsl:value-of select="$axis" /></p></xsl:when>
    </xsl:choose>
    <xsl:if test="$own-size = 'yes'"><p given="yes">size</p></xsl:if>
    <xsl:if test="$speed = 'yes'"><p given="yes">speed</p></xsl:if>
    <xsl:if test="$count = 'yes'"><p given="yes">count</p></xsl:if>
    <xsl:if test="$gap = 'yes'"><p given="yes">gap</p></xsl:if>
    <xsl:for-each select="group[1]/variables/variable">
      <p><xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template></p>
    </xsl:for-each>
    <xsl:if test="$sound = 'yes'"><p>sound</p></xsl:if>
  </xsl:template>

  <!-- startWall(): each kind's rows added to a new array, or the cells built
       here when it has no kinds; then addBricks(), which builds one kind's. -->
  <xsl:template name="grid-start-function">
    <xsl:variable name="grid" select="." />
    <xsl:variable name="axis"><xsl:call-template name="kind-axis" /></xsl:variable>
    <xsl:variable name="sound"><xsl:call-template name="kind-sound" /></xsl:variable>
    <xsl:variable name="add"><xsl:call-template name="add-name" /></xsl:variable>
    <xsl:variable name="cells"><xsl:call-template name="cells-name" /></xsl:variable>
    <xsl:variable name="parts"><xsl:call-template name="kind-parts" /></xsl:variable>
    <xsl:variable name="own-size"><xsl:call-template name="kind-size" /></xsl:variable>
    <xsl:variable name="speed"><xsl:call-template name="kind-speed" /></xsl:variable>
    <xsl:variable name="count"><xsl:call-template name="kind-count" /></xsl:variable>
    <xsl:variable name="gap"><xsl:call-template name="kind-gap" /></xsl:variable>
    <xsl:text>&#10;</xsl:text>
    <xsl:if test="@name = $reset-names">
      <xsl:value-of select="concat('// The ', @name, ' as it starts, and starts again.&#10;')" />
    </xsl:if>
    <xsl:text>function </xsl:text>
    <xsl:call-template name="start-name" />
    <xsl:value-of select="concat('() {&#10;&#9;const ', $cells, ' = [];&#10;')" />
    <xsl:choose>
      <xsl:when test="group">
        <xsl:for-each select="group">
          <xsl:value-of select="concat('&#9;', $add, '(', $cells)" />
          <xsl:if test="count($grid | $looking-grids) = count($looking-grids)">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="look-name"><xsl:with-param name="object" select="$grid" /><xsl:with-param name="look"><xsl:call-template name="kind-look" /></xsl:with-param></xsl:call-template>
          </xsl:if>
          <xsl:choose>
            <xsl:when test="$axis = 'xy'">
              <xsl:text>, { x: </xsl:text>
              <xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /></xsl:call-template>
              <xsl:text>, y: </xsl:text>
              <xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /></xsl:call-template>
              <xsl:text> }</xsl:text>
            </xsl:when>
            <xsl:when test="$axis != ''">
              <xsl:text>, </xsl:text>
              <xsl:call-template name="value"><xsl:with-param name="node" select="position/*[local-name() = $axis]" /></xsl:call-template>
            </xsl:when>
          </xsl:choose>
          <xsl:if test="$own-size = 'yes'">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="kind-size-of" />
          </xsl:if>
          <xsl:if test="$speed = 'yes'">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="value"><xsl:with-param name="node" select="(velocity/x | ../velocity/x)[last()]" /></xsl:call-template>
          </xsl:if>
          <xsl:if test="$count = 'yes'"><xsl:value-of select="concat(', ', normalize-space((columns | ../columns)[last()]))" /></xsl:if>
          <xsl:if test="$gap = 'yes'">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="value"><xsl:with-param name="node" select="(padding/x | ../padding/x)[last()]" /></xsl:call-template>
          </xsl:if>
          <xsl:variable name="kind" select="." />
          <xsl:for-each select="$grid/group[1]/variables/variable">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="value"><xsl:with-param name="node" select="$kind/variables/variable[@name = current()/@name]" /></xsl:call-template>
          </xsl:for-each>
          <xsl:if test="$sound = 'yes'">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="sound-name"><xsl:with-param name="sound" select="collisions/collision/play/@sound" /></xsl:call-template>
          </xsl:if>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:value-of select="concat('&#9;return ', $cells, ';&#10;}&#10;')" />
        <xsl:text>&#10;// </xsl:text>
        <xsl:choose>
          <xsl:when test="$count = 'yes'">count</xsl:when>
          <xsl:otherwise><xsl:value-of select="concat(normalize-space(rows), ' rows of ', normalize-space(columns))" /></xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="concat(' ', $cells)" />
        <xsl:if test="count(. | $looking-grids) = count($looking-grids)"> of one look</xsl:if>
        <xsl:if test="$axis = 'y'">, the first row's top at y</xsl:if>
        <xsl:if test="$axis = 'x'">, the first column's left at x</xsl:if>
        <xsl:if test="$axis = 'xy'">, the first at first</xsl:if>
        <xsl:if test="$gap = 'yes'">, gap apart</xsl:if>
        <xsl:if test="$speed = 'yes'">, going at speed</xsl:if>
        <xsl:value-of select="concat('.&#10;function ', $add, '(', $cells)" />
        <xsl:for-each select="exsl:node-set($parts)/p">
          <xsl:value-of select="concat(', ', .)" />
        </xsl:for-each>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:call-template name="cell-loops"><xsl:with-param name="axis" select="$axis" /></xsl:call-template>
        <xsl:text>}&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="cell-loops" />
        <xsl:value-of select="concat('&#9;return ', $cells, ';&#10;}&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The size, then a cell for each row and column, pushed onto the array. -->
  <!-- "diamond", "ring", ... or 40, 25, ...: each row's look (what =
       'look') or number, by the row that names its number, the grid's own
       where none does. -->
  <xsl:template name="row-values">
    <xsl:param name="rows" />
    <xsl:param name="what" />
    <xsl:param name="at" select="1" />
    <xsl:variable name="row" select="row[number[normalize-space(.) = $at]]" />
    <xsl:if test="$at &gt; 1">, </xsl:if>
    <xsl:choose>
      <xsl:when test="$what = 'look'">
        <xsl:variable name="look"><xsl:call-template name="march-look"><xsl:with-param name="from" select="$row[sprite] | self::*[not($row/sprite)]" /></xsl:call-template></xsl:variable>
        <xsl:call-template name="look-name"><xsl:with-param name="object" select="." /><xsl:with-param name="look" select="$look" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="value"><xsl:with-param name="node" select="($row/variables/variable[@name = $what] | variables/variable[@name = $what][not($row/variables/variable[@name = $what])])[1]" /><xsl:with-param name="self" select="." /></xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$at &lt; $rows">
      <xsl:call-template name="row-values"><xsl:with-param name="rows" select="$rows" /><xsl:with-param name="what" select="$what" /><xsl:with-param name="at" select="$at + 1" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="cell-loops">
    <xsl:param name="axis" select="''" />
    <xsl:variable name="cells"><xsl:call-template name="cells-name" /></xsl:variable>
    <xsl:variable name="rows" select="number(concat('0', normalize-space(rows)))" />
    <xsl:variable name="columns" select="number(concat('0', normalize-space(columns)))" />
    <xsl:variable name="parts"><xsl:call-template name="kind-parts" /></xsl:variable>
    <xsl:variable name="row-speeds" select="row[number][velocity/x]" />
    <xsl:variable name="own-size"><xsl:call-template name="kind-size" /></xsl:variable>
    <xsl:variable name="speed"><xsl:call-template name="kind-speed" /></xsl:variable>
    <xsl:variable name="count"><xsl:call-template name="kind-count" /></xsl:variable>
    <xsl:variable name="gap"><xsl:call-template name="kind-gap" /></xsl:variable>
    <xsl:if test="$own-size != 'yes'">
      <xsl:text>&#9;const size = </xsl:text>
      <xsl:call-template name="size" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$row-speeds">
      <!-- each row's way across, row 1 first -->
      <xsl:text>&#9;const rowSpeeds = [</xsl:text>
      <xsl:call-template name="row-speeds"><xsl:with-param name="rows" select="$rows" /></xsl:call-template>
      <xsl:text>];&#10;</xsl:text>
    </xsl:if>
    <xsl:variable name="grid" select="." />
    <xsl:variable name="row-looks" select="boolean(row[number][sprite]) and count(. | $marching) = count($marching)" />
    <xsl:variable name="row-numbers" select="variables/variable[@name = $grid/row[number]/variables/variable/@name]" />
    <xsl:if test="$row-looks or $row-numbers">
      <xsl:text>&#9;// each row's </xsl:text>
      <xsl:if test="$row-looks">look</xsl:if>
      <xsl:for-each select="$row-numbers">
        <xsl:choose>
          <xsl:when test="position() = last() and ($row-looks or position() &gt; 1)"> and </xsl:when>
          <xsl:when test="$row-looks or position() &gt; 1">, </xsl:when>
        </xsl:choose>
        <xsl:value-of select="@name" />
      </xsl:for-each>
      <xsl:text>, row 1 first&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$row-looks">
      <xsl:text>&#9;const rowLooks = [</xsl:text>
      <xsl:call-template name="row-values"><xsl:with-param name="rows" select="$rows" /><xsl:with-param name="what" select="'look'" /></xsl:call-template>
      <xsl:text>];&#10;</xsl:text>
    </xsl:if>
    <xsl:for-each select="$row-numbers">
      <xsl:text>&#9;const </xsl:text>
      <xsl:call-template name="camel"><xsl:with-param name="name" select="concat('row ', @name, 's')" /></xsl:call-template>
      <xsl:text> = [</xsl:text>
      <xsl:variable name="what" select="string(@name)" />
      <xsl:for-each select="$grid"><xsl:call-template name="row-values"><xsl:with-param name="rows" select="$rows" /><xsl:with-param name="what" select="$what" /></xsl:call-template></xsl:for-each>
      <xsl:text>];&#10;</xsl:text>
    </xsl:for-each>
    <xsl:variable name="x">
      <xsl:call-template name="cell-place"><xsl:with-param name="along" select="'x'" /><xsl:with-param name="given" select="contains($axis, 'x')" /><xsl:with-param name="first" select="$axis = 'xy'" /><xsl:with-param name="count" select="$columns + number($count = 'yes') * 2" /><xsl:with-param name="index" select="'column'" /><xsl:with-param name="gap" select="$gap = 'yes'" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="y">
      <xsl:call-template name="cell-place"><xsl:with-param name="along" select="'y'" /><xsl:with-param name="given" select="contains($axis, 'y')" /><xsl:with-param name="first" select="$axis = 'xy'" /><xsl:with-param name="count" select="$rows" /><xsl:with-param name="index" select="'row'" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="velocity">
      <xsl:choose>
        <xsl:when test="$row-speeds">{ x: rowSpeeds[row], y: 0 }</xsl:when>
        <xsl:when test="$speed = 'yes'">{ x: speed, y: 0 }</xsl:when>
        <xsl:when test="velocity"><xsl:call-template name="pair"><xsl:with-param name="of" select="velocity" /><xsl:with-param name="self" select="$grid" /></xsl:call-template></xsl:when>
        <xsl:otherwise>{ x: 0, y: 0 }</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="push">
      <xsl:value-of select="concat($cells, '.push({ position: { x: ', $x, ', y: ', $y, ' }, velocity: ', $velocity, ', size: { ...size }')" />
      <xsl:for-each select="exsl:node-set($parts)/p[not(@given)]">
        <xsl:value-of select="concat(', ', .)" />
      </xsl:for-each>
      <xsl:choose>
        <xsl:when test="$row-looks">, look: rowLooks[row]</xsl:when>
        <xsl:when test="not(group) and count(. | $looking-grids) = count($looking-grids)">
          <xsl:variable name="named"><xsl:call-template name="grid-looks" /></xsl:variable>
          <xsl:text>, look: </xsl:text>
          <xsl:call-template name="look-name"><xsl:with-param name="object" select="." /><xsl:with-param name="look" select="exsl:node-set($named)/look[1]/@name" /></xsl:call-template>
        </xsl:when>
      </xsl:choose>
      <xsl:if test="count(. | $timed-grids) = count($timed-grids)">, lookTime: 0</xsl:if>
      <xsl:for-each select="variables/variable[$row-looks or $row-numbers or $grid/collisions/collision/hole or $grid/timers/timer]">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>: </xsl:text>
        <xsl:choose>
          <xsl:when test="count(. | $row-numbers) = count($row-numbers)">
            <xsl:call-template name="camel"><xsl:with-param name="name" select="concat('row ', @name, 's')" /></xsl:call-template>
            <xsl:text>[row]</xsl:text>
          </xsl:when>
          <xsl:otherwise><xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$grid" /></xsl:call-template></xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:if test="collisions/collision/hole">
        <xsl:text>, picture: [...</xsl:text>
        <xsl:call-template name="picture-name"><xsl:with-param name="bitmap" select="$pictures[@name = $grid/sprite/bitmap/@from]" /></xsl:call-template>
        <xsl:text>]</xsl:text>
      </xsl:if>
      <xsl:for-each select="timers/timer">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="timer-name" />
        <xsl:text>: </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="every" /><xsl:with-param name="self" select="$grid" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text> });</xsl:text>
    </xsl:variable>
    <xsl:variable name="lines">
      <xsl:choose>
        <xsl:when test="$count = 'yes' and $rows &lt;= 1">
          <line>for (let column = 0; column &lt; count; ++column) {</line>
          <line depth="1"><xsl:value-of select="$push" /></line>
          <line>}</line>
        </xsl:when>
        <xsl:when test="$rows &gt; 1 and $columns &gt; 1">
          <line><xsl:value-of select="concat('for (let row = 0; row &lt; ', $rows, '; ++row) {')" /></line>
          <line depth="1"><xsl:value-of select="concat('for (let column = 0; column &lt; ', $columns, '; ++column) {')" /></line>
          <line depth="2"><xsl:value-of select="$push" /></line>
          <line depth="1">}</line>
          <line>}</line>
        </xsl:when>
        <xsl:when test="$rows &gt; 1 or $columns &gt; 1">
          <xsl:variable name="along">
            <xsl:choose>
              <xsl:when test="$rows &gt; 1">row</xsl:when>
              <xsl:otherwise>column</xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <line><xsl:value-of select="concat('for (let ', $along, ' = 0; ', $along, ' &lt; ', $rows + $columns - 1, '; ++', $along, ') {')" /></line>
          <line depth="1"><xsl:value-of select="$push" /></line>
          <line>}</line>
        </xsl:when>
        <xsl:otherwise><line><xsl:value-of select="$push" /></line></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($lines)/line" />
      <xsl:with-param name="indent" select="'&#9;'" />
    </xsl:call-template>
  </xsl:template>

  <!-- 90, -150, ...: the way across each row of so many is given, by the
     row that names its number, 0 where none does. -->
  <xsl:template name="row-speeds">
    <xsl:param name="rows" />
    <xsl:param name="at" select="1" />
    <xsl:variable name="row" select="row[number[normalize-space(.) = $at]]" />
    <xsl:if test="$at &gt; 1">, </xsl:if>
    <xsl:choose>
      <xsl:when test="$row/velocity/x"><xsl:call-template name="value"><xsl:with-param name="node" select="$row/velocity/x" /></xsl:call-template></xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
    <xsl:if test="$at &lt; $rows">
      <xsl:call-template name="row-speeds"><xsl:with-param name="rows" select="$rows" /><xsl:with-param name="at" select="$at + 1" /></xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- One axis of a cell's place: the grid's (or the kind's, given as a
       parameter), then a cell's size and padding for each before it. -->
  <xsl:template name="cell-place">
    <xsl:param name="along" />
    <xsl:param name="given" />
    <xsl:param name="count" />
    <xsl:param name="index" />
    <xsl:param name="first" select="false()" />
    <xsl:param name="gap" select="false()" />
    <xsl:variable name="start" select="position/*[local-name() = $along][* or normalize-space(.) != '0']" />
    <xsl:choose>
      <xsl:when test="$given and $first"><xsl:value-of select="concat('first.', $along)" /></xsl:when>
      <xsl:when test="$given"><xsl:value-of select="$along" /></xsl:when>
      <xsl:when test="$start">
        <xsl:call-template name="value"><xsl:with-param name="node" select="position/*[local-name() = $along]" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="$along" /><xsl:with-param name="wrap" select="1" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="$count &lt;= 1">0</xsl:when>
    </xsl:choose>
    <xsl:if test="$count &gt; 1">
      <xsl:if test="$given or $start"> + </xsl:if>
      <xsl:value-of select="concat($index, ' * ')" />
      <xsl:choose>
        <xsl:when test="$gap"><xsl:value-of select="concat('(size.', $along, ' + gap)')" /></xsl:when>
        <xsl:when test="padding/*[local-name() = $along]">
          <xsl:value-of select="concat('(size.', $along, ' + ')" />
          <xsl:call-template name="value"><xsl:with-param name="node" select="padding/*[local-name() = $along]" /><xsl:with-param name="wrap" select="1" /></xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise><xsl:value-of select="concat('size.', $along)" /></xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- A rule of a cell's by its look, the context: if (turtle.look ===
       "turtlerow3-under") { drown(); }, as one line of lines. -->
  <xsl:template name="look-rule">
    <xsl:param name="cell" />
    <xsl:param name="grid" />
    <xsl:param name="other" />
    <xsl:variable name="kind" select="ancestor::group[1][count(. | $grid) != count($grid)]" />
    <xsl:variable name="named">
      <xsl:choose>
        <xsl:when test="$kind"><xsl:call-template name="kind-look"><xsl:with-param name="kind" select="$kind" /><xsl:with-param name="sprite" select="$kind/sprite[@name = current()/@sprite]" /></xsl:call-template></xsl:when>
        <xsl:otherwise><xsl:value-of select="@sprite" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="body">
      <xsl:for-each select="*">
        <line>
          <xsl:apply-templates select="." mode="touch">
            <xsl:with-param name="who" select="$cell" />
            <xsl:with-param name="self" select="$grid" />
            <xsl:with-param name="other" select="$other" />
          </xsl:apply-templates>
        </line>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="header">
      <xsl:value-of select="concat('if (', $cell, '.look === ')" />
      <xsl:call-template name="look-name"><xsl:with-param name="object" select="$grid" /><xsl:with-param name="look" select="$named" /></xsl:call-template>
      <xsl:text>)</xsl:text>
    </xsl:variable>
    <xsl:variable name="block">
      <xsl:call-template name="block-lines">
        <xsl:with-param name="header" select="$header" />
        <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($block)/line">
      <xsl:variable name="tabs" select="substring('&#9;&#9;&#9;&#9;&#9;&#9;', 1, number(concat('0', @depth)))" />
      <xsl:if test="position() &gt; 1"><xsl:text>&#10;</xsl:text></xsl:if>
      <xsl:value-of select="$tabs" />
      <xsl:call-template name="replace"><xsl:with-param name="text" select="string(.)" /><xsl:with-param name="from" select="'&#10;'" /><xsl:with-param name="to" select="concat('&#10;', $tabs)" /></xsl:call-template>
    </xsl:for-each>
  </xsl:template>

  <!-- A grid's rule by look on a thing, nested in its rule on it. -->
  <xsl:template match="group/collisions/collision/collision[@sprite]" mode="touch">
    <xsl:param name="who" />
    <xsl:param name="other" />
    <xsl:call-template name="look-rule">
      <xsl:with-param name="cell" select="$who" />
      <xsl:with-param name="grid" select="ancestor::group[1]" />
      <xsl:with-param name="other" select="$other" />
    </xsl:call-template>
  </xsl:template>

  <!-- On meeting: a loop over the array with the thing's rules for the
       grid's cells, then the cells' own on the thing, a kind's sound, and
       the taking out when a cell dies (over a copy, so the loop sees each
       cell once). $rules are the thing's. -->
  <xsl:template name="grid-meeting">
    <xsl:param name="grid" />
    <xsl:param name="thing" />
    <xsl:param name="object" />
    <xsl:param name="rules" />
    <xsl:variable name="list"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$grid" /></xsl:call-template></xsl:variable>
    <xsl:variable name="cell"><xsl:call-template name="cell-name"><xsl:with-param name="grid" select="$grid" /></xsl:call-template></xsl:variable>
    <xsl:variable name="own" select="$grid/collisions/collision[@object = $object/@name]" />
    <xsl:variable name="body">
      <xsl:for-each select="$rules">
        <xsl:call-template name="rule-lines">
          <xsl:with-param name="who" select="$thing" />
          <xsl:with-param name="self" select="$object" />
          <xsl:with-param name="other" select="$cell" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:for-each select="$own">
        <xsl:call-template name="rule-lines">
          <xsl:with-param name="who" select="$cell" />
          <xsl:with-param name="self" select="$grid" />
          <xsl:with-param name="other" select="$thing" />
          <xsl:with-param name="skip" select="die" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:for-each select="$grid/group/collisions/collision[@object = $object/@name][@sprite]">
        <line>
          <xsl:call-template name="look-rule">
            <xsl:with-param name="cell" select="$cell" />
            <xsl:with-param name="grid" select="$grid" />
            <xsl:with-param name="other" select="$thing" />
          </xsl:call-template>
        </line>
      </xsl:for-each>
      <xsl:if test="$grid/group/collisions/collision[@object = $object/@name][not(@sprite)]/play">
        <line><xsl:value-of select="concat('play(', $cell, '.sound);')" /></line>
      </xsl:if>
      <xsl:for-each select="$own/die">
        <line><xsl:value-of select="concat($list, '.splice(', $list, '.indexOf(', $cell, '), 1);')" /></line>
      </xsl:for-each>
    </xsl:variable>
    <xsl:value-of select="concat('&#9;for (const ', $cell, ' of ')" />
    <xsl:choose>
      <xsl:when test="$own/die"><xsl:value-of select="concat('[...', $list, ']')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$list" /></xsl:otherwise>
    </xsl:choose>
    <xsl:text>) {&#10;</xsl:text>
    <xsl:call-template name="write-block">
      <xsl:with-param name="header">
        <xsl:text>if (</xsl:text>
        <xsl:call-template name="touch-test"><xsl:with-param name="object" select="$object" /><xsl:with-param name="thing" select="$thing" /><xsl:with-param name="other" select="$cell" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:with-param>
      <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
      <xsl:with-param name="indent" select="'&#9;&#9;'" />
    </xsl:call-template>
    <xsl:text>&#9;}&#10;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
