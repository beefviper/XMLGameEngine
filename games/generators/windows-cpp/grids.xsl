<?xml version="1.0" encoding="UTF-8"?>
<!-- grids.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- A grid whose cells meet (kinds.xsl) is a list of them in the game:
     std::vector<Brick> wall. A group in it is one kind of its cells, a look
     of the cell's record, with the place, numbers and sound it gives them.
     Its start builds the list, a kind at a time; on meeting a thing, the
     loop over the list runs the thing's rules, then the cell's, and a cell
     that dies is erased. -->
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

  <!-- { 48, 28 }: the size of a kind's picture. -->
  <xsl:template name="kind-size-of">
    <xsl:variable name="named"><xsl:call-template name="kind-picture" /></xsl:variable>
    <xsl:variable name="bitmap" select="$pictures[@name = $named] | /game/pictures/bitmap[@name = $named]" />
    <xsl:if test="$bitmap">
      <xsl:variable name="scale"><xsl:call-template name="picture-scale"><xsl:with-param name="bitmap" select="$bitmap[1]" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('{ ', string-length(normalize-space($bitmap[1]/row[1])) * $scale, ', ', count($bitmap[1]/row) * $scale, ' }')" />
    </xsl:if>
  </xsl:template>

  <!-- Whether its kinds each have a sound of their own when they meet. -->
  <xsl:template name="kind-sound">
    <xsl:param name="grid" select="." />
    <xsl:if test="$grid/group/collisions/collision/play">yes</xsl:if>
  </xsl:template>

  <!-- struct Brick : Thing { its look, the numbers and the sound its kind
       gives it }. -->
  <xsl:template name="grid-record">
    <xsl:variable name="grid" select="." />
    <xsl:variable name="kinds" select="group" />
    <xsl:variable name="sound"><xsl:call-template name="kind-sound" /></xsl:variable>
    <xsl:variable name="own" select="variables/variable" />
    <xsl:if test="count($kinds) &gt; 1 or $kinds/variables/variable or $sound = 'yes' or $own">
      <xsl:text>&#10;&#9;struct </xsl:text>
      <xsl:call-template name="record-name" />
      <xsl:text> : Thing&#10;&#9;{&#10;</xsl:text>
      <xsl:if test="count($kinds) &gt; 1">
        <xsl:text>&#9;&#9;enum class Look { </xsl:text>
        <xsl:for-each select="$kinds">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        </xsl:for-each>
        <xsl:text> };&#10;&#9;&#9;Look look{ Look::</xsl:text>
        <xsl:call-template name="type-word"><xsl:with-param name="name" select="$kinds[1]/@name" /></xsl:call-template>
        <xsl:text> };&#10;</xsl:text>
      </xsl:if>
      <xsl:for-each select="$kinds[1]/variables/variable | $own">
        <xsl:text>&#9;&#9;</xsl:text>
        <xsl:call-template name="grid-variable-type"><xsl:with-param name="grid" select="$grid" /></xsl:call-template>
        <xsl:text> </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text>{ </xsl:text>
        <xsl:choose>
          <xsl:when test="parent::variables/parent::group[count(. | $kinds) = count($kinds)]">0</xsl:when>
          <xsl:otherwise><xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template></xsl:otherwise>
        </xsl:choose>
        <xsl:text> };&#10;</xsl:text>
      </xsl:for-each>
      <xsl:if test="$sound = 'yes'">
        <xsl:text>&#9;&#9;Sound sound{ </xsl:text>
        <xsl:call-template name="sound-name"><xsl:with-param name="sound" select="$kinds[1]/collisions/collision/play/@sound" /></xsl:call-template>
        <xsl:text> };&#10;</xsl:text>
      </xsl:if>
      <xsl:text>&#9;};&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- int when every kind gives a whole number, else float. -->
  <xsl:template name="grid-variable-type">
    <xsl:param name="grid" />
    <xsl:variable name="named" select="@name" />
    <xsl:variable name="all" select="$grid/group/variables/variable[@name = $named] | $grid/variables/variable[@name = $named]" />
    <xsl:choose>
      <xsl:when test="not($all[* or floor(number(.)) != number(.)])">int</xsl:when>
      <xsl:otherwise>float</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The type of a grid's cells: its record, or Thing. -->
  <xsl:template name="cell-type">
    <xsl:param name="grid" select="." />
    <xsl:choose>
      <xsl:when test="count($grid/group) &gt; 1 or $grid/group/variables/variable or $grid/group/collisions/collision/play or $grid/variables/variable">
        <xsl:call-template name="record-name"><xsl:with-param name="object" select="$grid" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise>Thing</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- addBricks: the function that builds one kind's rows. -->
  <xsl:template name="add-name">
    <xsl:param name="grid" select="." />
    <xsl:text>add</xsl:text>
    <xsl:call-template name="type-word">
      <xsl:with-param name="name">
        <xsl:call-template name="record-name"><xsl:with-param name="object" select="$grid" /></xsl:call-template>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:text>s</xsl:text>
  </xsl:template>

  <!-- void addBricks(Brick::Look look, float y, int worth, Sound sound) -->
  <xsl:template name="add-parameters">
    <xsl:variable name="grid" select="." />
    <xsl:variable name="axis"><xsl:call-template name="kind-axis" /></xsl:variable>
    <xsl:variable name="sound"><xsl:call-template name="kind-sound" /></xsl:variable>
    <xsl:variable name="own-size"><xsl:call-template name="kind-size" /></xsl:variable>
    <xsl:variable name="speed"><xsl:call-template name="kind-speed" /></xsl:variable>
    <xsl:variable name="count"><xsl:call-template name="kind-count" /></xsl:variable>
    <xsl:variable name="gap"><xsl:call-template name="kind-gap" /></xsl:variable>
    <xsl:variable name="parts">
      <xsl:if test="count(group) &gt; 1">
        <p><xsl:call-template name="record-name" /><xsl:text>::Look look</xsl:text></p>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="$axis = 'xy'"><p>sf::Vector2f first</p></xsl:when>
        <xsl:when test="$axis != ''"><p><xsl:value-of select="concat('float ', $axis)" /></p></xsl:when>
      </xsl:choose>
      <xsl:if test="$own-size = 'yes'"><p>sf::Vector2f size</p></xsl:if>
      <xsl:if test="$speed = 'yes'"><p>float speed</p></xsl:if>
      <xsl:if test="$count = 'yes'"><p>int count</p></xsl:if>
      <xsl:if test="$gap = 'yes'"><p>float gap</p></xsl:if>
      <xsl:for-each select="group[1]/variables/variable">
        <p>
          <xsl:call-template name="grid-variable-type"><xsl:with-param name="grid" select="$grid" /></xsl:call-template>
          <xsl:text> </xsl:text>
          <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        </p>
      </xsl:for-each>
      <xsl:if test="$sound = 'yes'"><p>Sound sound</p></xsl:if>
    </xsl:variable>
    <xsl:for-each select="exsl:node-set($parts)/p">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:value-of select="." />
    </xsl:for-each>
  </xsl:template>

  <!-- startWall(): the list emptied, then each kind's rows added, or the
       cells built here when it has no kinds. -->
  <xsl:template name="grid-start-function">
    <xsl:variable name="grid" select="." />
    <xsl:variable name="list"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="axis"><xsl:call-template name="kind-axis" /></xsl:variable>
    <xsl:variable name="sound"><xsl:call-template name="kind-sound" /></xsl:variable>
    <xsl:variable name="add"><xsl:call-template name="add-name" /></xsl:variable>
    <xsl:variable name="own-size"><xsl:call-template name="kind-size" /></xsl:variable>
    <xsl:variable name="speed"><xsl:call-template name="kind-speed" /></xsl:variable>
    <xsl:variable name="count"><xsl:call-template name="kind-count" /></xsl:variable>
    <xsl:variable name="gap"><xsl:call-template name="kind-gap" /></xsl:variable>
    <xsl:text>&#10;</xsl:text>
    <xsl:if test="@name = $reset-names">
      <xsl:value-of select="concat('&#9;// Where the ', @name, ' starts, and starts again.&#10;')" />
    </xsl:if>
    <xsl:value-of select="concat('&#9;void ', $class, '::')" />
    <xsl:call-template name="start-name" />
    <xsl:value-of select="concat('()&#10;&#9;{&#10;&#9;&#9;', $list, '.clear();&#10;')" />
    <xsl:choose>
      <xsl:when test="group">
        <xsl:for-each select="group">
          <xsl:value-of select="concat('&#9;&#9;', $add, '(')" />
          <xsl:variable name="arguments">
            <xsl:if test="count($grid/group) &gt; 1">
              <a><xsl:call-template name="look-name"><xsl:with-param name="object" select="$grid" /><xsl:with-param name="look" select="@name" /></xsl:call-template></a>
            </xsl:if>
            <xsl:choose>
              <xsl:when test="$axis = 'xy'">
                <a>
                  <xsl:text>{ </xsl:text>
                  <xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /></xsl:call-template>
                  <xsl:text>, </xsl:text>
                  <xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /></xsl:call-template>
                  <xsl:text> }</xsl:text>
                </a>
              </xsl:when>
              <xsl:when test="$axis != ''">
                <a><xsl:call-template name="value"><xsl:with-param name="node" select="position/*[local-name() = $axis]" /></xsl:call-template></a>
              </xsl:when>
            </xsl:choose>
            <xsl:if test="$own-size = 'yes'"><a><xsl:call-template name="kind-size-of" /></a></xsl:if>
            <xsl:if test="$speed = 'yes'">
              <a><xsl:call-template name="value"><xsl:with-param name="node" select="(velocity/x | ../velocity/x)[last()]" /></xsl:call-template></a>
            </xsl:if>
            <xsl:if test="$count = 'yes'"><a><xsl:value-of select="normalize-space((columns | ../columns)[last()])" /></a></xsl:if>
            <xsl:if test="$gap = 'yes'">
              <a><xsl:call-template name="value"><xsl:with-param name="node" select="(padding/x | ../padding/x)[last()]" /></xsl:call-template></a>
            </xsl:if>
            <xsl:variable name="kind" select="." />
            <xsl:for-each select="$grid/group[1]/variables/variable">
              <a><xsl:call-template name="value"><xsl:with-param name="node" select="$kind/variables/variable[@name = current()/@name]" /></xsl:call-template></a>
            </xsl:for-each>
            <xsl:if test="$sound = 'yes'">
              <a><xsl:call-template name="sound-name"><xsl:with-param name="sound" select="collisions/collision/play/@sound" /></xsl:call-template></a>
            </xsl:if>
          </xsl:variable>
          <xsl:for-each select="exsl:node-set($arguments)/a">
            <xsl:if test="position() &gt; 1">, </xsl:if>
            <xsl:value-of select="." />
          </xsl:for-each>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>&#9;}&#10;</xsl:text>
        <xsl:text>&#10;&#9;// </xsl:text>
        <xsl:choose>
          <xsl:when test="$count = 'yes'">count</xsl:when>
          <xsl:otherwise><xsl:value-of select="concat(normalize-space(rows), ' rows of ', normalize-space(columns))" /></xsl:otherwise>
        </xsl:choose>
        <xsl:text> </xsl:text>
        <xsl:choose>
          <xsl:when test="@class"><xsl:value-of select="@class" /></xsl:when>
          <xsl:otherwise><xsl:call-template name="singular"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:otherwise>
        </xsl:choose>
        <xsl:text>s</xsl:text>
        <xsl:if test="count(group) &gt; 1"> of one look</xsl:if>
        <xsl:if test="$axis = 'y'">, the first row's top at y</xsl:if>
        <xsl:if test="$axis = 'x'">, the first column's left at x</xsl:if>
        <xsl:if test="$axis = 'xy'">, the first at first</xsl:if>
        <xsl:if test="$gap = 'yes'">, gap apart</xsl:if>
        <xsl:if test="$speed = 'yes'">, going at speed</xsl:if>
        <xsl:value-of select="concat('.&#10;&#9;void ', $class, '::', $add, '(')" />
        <xsl:call-template name="add-parameters" />
        <xsl:text>)&#10;&#9;{&#10;</xsl:text>
        <xsl:call-template name="cell-loops"><xsl:with-param name="axis" select="$axis" /></xsl:call-template>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="cell-loops" />
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The size, then a cell for each row and column, pushed onto the list. -->
  <xsl:template name="cell-loops">
    <xsl:param name="axis" select="''" />
    <xsl:variable name="grid" select="." />
    <xsl:variable name="list"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="rows" select="number(concat('0', normalize-space(rows)))" />
    <xsl:variable name="columns" select="number(concat('0', normalize-space(columns)))" />
    <xsl:variable name="sound"><xsl:call-template name="kind-sound" /></xsl:variable>
    <xsl:variable name="type"><xsl:call-template name="cell-type" /></xsl:variable>
    <xsl:variable name="row-speeds" select="row[number][velocity/x]" />
    <xsl:variable name="own-size"><xsl:call-template name="kind-size" /></xsl:variable>
    <xsl:variable name="speed"><xsl:call-template name="kind-speed" /></xsl:variable>
    <xsl:variable name="count"><xsl:call-template name="kind-count" /></xsl:variable>
    <xsl:variable name="gap"><xsl:call-template name="kind-gap" /></xsl:variable>
    <xsl:if test="$own-size != 'yes'">
      <xsl:text>&#9;&#9;const sf::Vector2f size</xsl:text>
      <xsl:call-template name="size" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$row-speeds">
      <!-- each row's way across, row 1 first -->
      <xsl:text>&#9;&#9;constexpr float rowSpeeds[] = { </xsl:text>
      <xsl:call-template name="row-speeds"><xsl:with-param name="rows" select="$rows" /></xsl:call-template>
      <xsl:text> };&#10;</xsl:text>
    </xsl:if>
    <xsl:variable name="x">
      <xsl:call-template name="cell-place"><xsl:with-param name="along" select="'x'" /><xsl:with-param name="given" select="contains($axis, 'x')" /><xsl:with-param name="first" select="$axis = 'xy'" /><xsl:with-param name="count" select="$columns + number($count = 'yes') * 2" /><xsl:with-param name="index" select="'column'" /><xsl:with-param name="gap" select="$gap = 'yes'" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="y">
      <xsl:call-template name="cell-place"><xsl:with-param name="along" select="'y'" /><xsl:with-param name="given" select="contains($axis, 'y')" /><xsl:with-param name="first" select="$axis = 'xy'" /><xsl:with-param name="count" select="$rows" /><xsl:with-param name="index" select="'row'" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="parts">
      <xsl:if test="count(group) &gt; 1">, look</xsl:if>
      <xsl:for-each select="group[1]/variables/variable">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      </xsl:for-each>
      <xsl:if test="$sound = 'yes'">, sound</xsl:if>
    </xsl:variable>
    <xsl:variable name="velocity">
      <xsl:choose>
        <xsl:when test="$row-speeds">{ rowSpeeds[row], 0 }</xsl:when>
        <xsl:when test="$speed = 'yes'">{ speed, 0 }</xsl:when>
        <xsl:otherwise>{}</xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="push">
      <xsl:value-of select="concat('const sf::Vector2f position{ ', $x, ', ', $y, ' };&#10;')" />
      <xsl:choose>
        <xsl:when test="$type = 'Thing'"><xsl:value-of select="concat($list, '.push_back({ position, ', $velocity, ', size });')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat($list, '.push_back({ { position, ', $velocity, ', size }', $parts, ' });')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="lines">
      <xsl:choose>
        <xsl:when test="$count = 'yes' and $rows &lt;= 1">
          <line>for (int column = 0; column &lt; count; ++column)</line>
          <line>{</line>
          <line depth="1"><xsl:value-of select="$push" /></line>
          <line>}</line>
        </xsl:when>
        <xsl:when test="$rows &gt; 1 and $columns &gt; 1">
          <line><xsl:value-of select="concat('for (int row = 0; row &lt; ', $rows, '; ++row)')" /></line>
          <line>{</line>
          <line depth="1"><xsl:value-of select="concat('for (int column = 0; column &lt; ', $columns, '; ++column)')" /></line>
          <line depth="1">{</line>
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
          <line><xsl:value-of select="concat('for (int ', $along, ' = 0; ', $along, ' &lt; ', $rows + $columns - 1, '; ++', $along, ')')" /></line>
          <line>{</line>
          <line depth="1"><xsl:value-of select="$push" /></line>
          <line>}</line>
        </xsl:when>
        <xsl:otherwise><line><xsl:value-of select="$push" /></line></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="print-lines">
      <xsl:with-param name="lines" select="exsl:node-set($lines)/line" />
      <xsl:with-param name="indent" select="'&#9;&#9;'" />
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
      <xsl:value-of select="concat('static_cast&lt;float&gt;(', $index, ') * ')" />
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

  <!-- On meeting: a loop over the list with the thing's rules for the
       grid's cells, then the cells' own on the thing, a kind's sound, and
       the erasing when a cell dies. $rules are the thing's. -->
  <xsl:template name="grid-meeting">
    <xsl:param name="grid" />
    <xsl:param name="thing" />
    <xsl:param name="object" />
    <xsl:param name="rules" />
    <xsl:variable name="list"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$grid" /></xsl:call-template></xsl:variable>
    <xsl:variable name="cell"><xsl:call-template name="cell-name"><xsl:with-param name="grid" select="$grid" /></xsl:call-template></xsl:variable>
    <xsl:variable name="own" select="$grid/collisions/collision[@object = $object/@name]" />
    <xsl:variable name="dies" select="boolean($own)" />
    <xsl:variable name="met">
      <xsl:if test="$dies">*</xsl:if>
      <xsl:value-of select="$cell" />
    </xsl:variable>
    <xsl:variable name="body">
      <xsl:for-each select="$rules">
        <xsl:call-template name="rule-lines">
          <xsl:with-param name="who" select="$thing" />
          <xsl:with-param name="self" select="$object" />
          <xsl:with-param name="other" select="$met" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:for-each select="$own">
        <xsl:call-template name="rule-lines">
          <xsl:with-param name="who" select="concat('*', $cell)" />
          <xsl:with-param name="self" select="$grid" />
          <xsl:with-param name="other" select="$thing" />
          <xsl:with-param name="skip" select="die" />
        </xsl:call-template>
      </xsl:for-each>
      <xsl:if test="$grid/group/collisions/collision[@object = $object/@name]/play">
        <line><xsl:value-of select="concat('sounds.push_back(', $cell, '-&gt;sound);')" /></line>
      </xsl:if>
      <xsl:for-each select="$own/die">
        <line><xsl:value-of select="concat($cell, ' = ', $list, '.erase(', $cell, ');')" /></line>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="lines" select="exsl:node-set($body)/line" />
    <xsl:choose>
      <xsl:when test="$dies">
        <xsl:value-of select="concat('&#9;&#9;for (auto ', $cell, ' = ', $list, '.begin(); ', $cell, ' != ', $list, '.end();)&#10;&#9;&#9;{&#10;')" />
        <xsl:call-template name="write-block">
          <xsl:with-param name="header">
            <xsl:text>if (</xsl:text>
            <xsl:call-template name="touch-test"><xsl:with-param name="object" select="$object" /><xsl:with-param name="thing" select="$thing" /><xsl:with-param name="other" select="concat('*', $cell)" /></xsl:call-template>
            <xsl:text>)</xsl:text>
          </xsl:with-param>
          <xsl:with-param name="lines" select="$lines" />
          <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
          <xsl:with-param name="otherwise">
            <xsl:if test="$own/die">
              <xsl:value-of select="concat('++', $cell, ';')" />
            </xsl:if>
          </xsl:with-param>
        </xsl:call-template>
        <xsl:text>&#9;&#9;}&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="type"><xsl:call-template name="cell-type"><xsl:with-param name="grid" select="$grid" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat('&#9;&#9;for (const ', $type, '&amp; ', $cell, ' : ', $list, ')&#10;&#9;&#9;{&#10;')" />
        <xsl:call-template name="write-block">
          <xsl:with-param name="header">
            <xsl:text>if (</xsl:text>
            <xsl:call-template name="touch-test"><xsl:with-param name="object" select="$object" /><xsl:with-param name="thing" select="$thing" /><xsl:with-param name="other" select="$cell" /></xsl:call-template>
            <xsl:text>)</xsl:text>
          </xsl:with-param>
          <xsl:with-param name="lines" select="$lines" />
          <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
        </xsl:call-template>
        <xsl:text>&#9;&#9;}&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
</xsl:stylesheet>
