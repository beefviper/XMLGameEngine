<?xml version="1.0" encoding="UTF-8"?>
<!-- cells.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 28, 2026 -->

<!-- A map of cells (Snake's field), as windows-cpp's cells.xsl writes it,
     in JavaScript: its rows of letters kept as they are drawn, each kind of
     piece an array of spots ({ column, row }) read from them, and a piece
     that goes on by itself an object of its own stepping a cell every so
     often. A step into a taken cell is a touch: both sides' rules run, and
     it is refused unless they took what was there away. A chain's links
     follow its head. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <!-- Each map's size, place and rows. -->
  <xsl:template name="map-constants">
    <xsl:for-each select="$cell-maps">
      <xsl:variable name="map"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#10;// The ', @name, ', a cell a letter: ')" />
      <xsl:for-each select="group | object">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:value-of select="concat(@char, ' ', @name)" />
      </xsl:for-each>
      <xsl:text>.&#10;</xsl:text>
      <xsl:value-of select="concat('const ', $map, 'Columns = ', string-length(normalize-space(row[1])), ';&#10;')" />
      <xsl:value-of select="concat('const ', $map, 'Rows = ', count(row), ';&#10;')" />
      <xsl:value-of select="concat('const ', $map, 'Tile = { x: ')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="tile/width" /></xsl:call-template>
      <xsl:text>, y: </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="tile/height" /></xsl:call-template>
      <xsl:text> }; // a cell's size&#10;</xsl:text>
      <xsl:value-of select="concat('const ', $map, 'Position = { x: ')" />
      <xsl:call-template name="axis-value"><xsl:with-param name="node" select="position/x" /></xsl:call-template>
      <xsl:text>, y: </xsl:text>
      <xsl:call-template name="axis-value"><xsl:with-param name="node" select="position/y" /></xsl:call-template>
      <xsl:text> }; // its top left&#10;</xsl:text>
      <xsl:value-of select="concat('const ', $map, 'Map = [&#10;')" />
      <xsl:for-each select="row">
        <xsl:value-of select="concat('&#9;&quot;', normalize-space(.), '&quot;,&#10;')" />
      </xsl:for-each>
      <xsl:text>];&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- The helpers a map's rules use. -->
  <xsl:template name="map-helpers">
    <part>// Whether two spots are the same cell, and whether a list holds that cell.
function same(a, b) { return a.column === b.column &amp;&amp; a.row === b.row; }
function contains(spots, spot) { return spots.some((one) => same(one, spot)); }
</part>
  </xsl:template>

  <!-- The lists and pieces of each map, as they are before it starts. -->
  <xsl:template name="map-lets">
    <xsl:for-each select="$cell-maps/group | $cell-maps/object">
      <xsl:text>let </xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:choose>
        <xsl:when test="self::group"> = [];&#10;</xsl:when>
        <xsl:when test="count(. | $goers) = count($goers)"><xsl:text> = </xsl:text><xsl:call-template name="goer-start" /><xsl:text>;&#10;</xsl:text></xsl:when>
        <xsl:otherwise> = { column: 0, row: 0 };&#10;</xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:for-each select="$cell-maps">
      <xsl:call-template name="map-start-name" />
      <xsl:text>();&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- A piece that goes, before it is placed: its cell, the way it goes on
       by itself (a cell up, down, left or right), every so many seconds. -->
  <xsl:template name="goer-start">
    <xsl:text>{ spot: { column: 0, row: 0 }, way: { column: 0, row: 0 }, going: false, every: 0, wait: 0</xsl:text>
    <xsl:if test="collisions/collision/grow">, growing: 0</xsl:if>
    <xsl:text> }</xsl:text>
  </xsl:template>

  <xsl:template name="go-name">
    <xsl:param name="goer" select="." />
    <xsl:text>go</xsl:text>
    <xsl:if test="count($goers) &gt; 1"><xsl:call-template name="type-word"><xsl:with-param name="name" select="$goer/@name" /></xsl:call-template></xsl:if>
  </xsl:template>
  <xsl:template name="step-name">
    <xsl:text>step</xsl:text>
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
  </xsl:template>
  <xsl:template name="chain-test-name">
    <xsl:param name="goer" select="." />
    <xsl:text>in</xsl:text>
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="$goer/chain/@object" /></xsl:call-template>
  </xsl:template>
  <xsl:template name="map-start-name">
    <xsl:text>start</xsl:text>
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
  </xsl:template>

  <!-- The functions. -->
  <xsl:template name="map-functions">
    <xsl:for-each select="$cell-maps">
      <xsl:call-template name="map-start-function" />
    </xsl:for-each>
    <xsl:for-each select="$goers">
      <xsl:call-template name="go-function" />
      <xsl:call-template name="step-function" />
      <xsl:if test="chain"><xsl:call-template name="chain-test-function" /></xsl:if>
    </xsl:for-each>
    <xsl:if test="$free-spawns">
      <xsl:call-template name="free-spot-function" />
    </xsl:if>
  </xsl:template>

  <!-- startField(): every piece where its letter is. -->
  <xsl:template name="map-start-function">
    <xsl:variable name="map"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#10;// The ', @name, ' as its map draws it, a cell a letter.&#10;function ')" />
    <xsl:call-template name="map-start-name" />
    <xsl:text>() {&#10;</xsl:text>
    <xsl:for-each select="group | object">
      <xsl:text>&#9;</xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:choose>
        <xsl:when test="self::group"> = [];&#10;</xsl:when>
        <xsl:when test="count(. | $goers) = count($goers)"><xsl:text> = </xsl:text><xsl:call-template name="goer-start" /><xsl:text>;&#10;</xsl:text></xsl:when>
        <xsl:otherwise> = { column: 0, row: 0 };&#10;</xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:value-of select="concat('&#9;for (let row = 0; row &lt; ', $map, 'Map.length; ++row) {&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;for (let column = 0; column &lt; ', $map, 'Map[row].length; ++column) {&#10;')" />
    <xsl:text>&#9;&#9;&#9;const spot = { column, row };&#10;</xsl:text>
    <xsl:value-of select="concat('&#9;&#9;&#9;switch (', $map, 'Map[row][column]) {&#10;')" />
    <xsl:for-each select="group | object">
      <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;&#9;case &quot;', @char, '&quot;: ')" />
      <xsl:choose>
        <xsl:when test="self::group"><xsl:value-of select="concat($named, '.push(spot); break;&#10;')" /></xsl:when>
        <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat($named, '.spot = spot; break;&#10;')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat($named, ' = spot; break;&#10;')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>&#9;&#9;&#9;}&#10;&#9;&#9;}&#10;&#9;}&#10;</xsl:text>
    <xsl:for-each select="object[chain]">
      <xsl:variable name="head"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="../group[@name = current()/chain/@object]" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#10;&#9;// the ', $links, ' in order from the ', $head, ', each link the one beside the last&#10;')" />
      <xsl:text>&#9;const links = [];&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;for (let last = ', $head, '.spot; ;) {&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;const next = ', $links, '.findIndex((link) =&gt; Math.abs(link.column - last.column) + Math.abs(link.row - last.row) === 1);&#10;')" />
      <xsl:text>&#9;&#9;if (next &lt; 0) { break; }&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;&#9;last = ', $links, '[next];&#10;&#9;&#9;links.push(last);&#10;&#9;&#9;', $links, '.splice(next, 1);&#10;&#9;}&#10;')" />
      <xsl:value-of select="concat('&#9;', $links, ' = links;&#10;')" />
    </xsl:for-each>
    <xsl:text>}&#10;</xsl:text>
  </xsl:template>

  <!-- go(): off on its way, `speed` cells a second. -->
  <xsl:template name="go-function">
    <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#10;// The ', $goer, ' off on its way, speed cells a second')" />
    <xsl:if test="chain">, never straight back into its <xsl:value-of select="chain/@object" /></xsl:if>
    <xsl:text>.&#10;function </xsl:text>
    <xsl:call-template name="go-name" />
    <xsl:text>(way, speed) {&#10;</xsl:text>
    <xsl:if test="chain">
      <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="../group[@name = current()/chain/@object]" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#9;if (', $links, '.length &gt; 0 &amp;&amp; same(', $links, '[0], { column: ', $goer, '.spot.column + way.column, row: ', $goer, '.spot.row + way.row })) { return; }&#10;')" />
    </xsl:if>
    <xsl:value-of select="concat('&#9;', $goer, '.every = 1 / speed;&#10;')" />
    <xsl:value-of select="concat('&#9;if (!', $goer, '.going || ', $goer, '.wait &gt; ', $goer, '.every) { ', $goer, '.wait = ', $goer, '.every; }&#10;')" />
    <xsl:value-of select="concat('&#9;', $goer, '.going = true;&#10;&#9;', $goer, '.way = way;&#10;}&#10;')" />
  </xsl:template>

  <!-- stepHead(): a cell on its way, a touch for each kind of piece it
       steps into; refused unless the touch took the piece away. -->
  <xsl:template name="step-function">
    <xsl:variable name="goer-node" select="." />
    <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="map" select=".." />
    <xsl:variable name="map-name"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$map" /></xsl:call-template></xsl:variable>
    <xsl:variable name="chained" select="$map/group[@name = current()/chain/@object]" />
    <xsl:value-of select="concat('&#10;// The ', $goer, ' a cell on its way. A step into another piece is a touch, and is&#10;// refused unless the touch takes that piece away.&#10;function ')" />
    <xsl:call-template name="step-name" />
    <xsl:text>() {&#10;</xsl:text>
    <xsl:value-of select="concat('&#9;const next = { column: ', $goer, '.spot.column + ', $goer, '.way.column, row: ', $goer, '.spot.row + ', $goer, '.way.row };&#10;')" />
    <xsl:variable name="walled" select="not(contains(normalize-space($map/row[1]), '.') or contains(normalize-space($map/row[last()]), '.'))
        and not($map/row[substring(normalize-space(.), 1, 1) = '.' or substring(normalize-space(.), string-length(normalize-space(.))) = '.'])" />
    <xsl:if test="not($walled)">
      <xsl:value-of select="concat('&#9;if (next.column &lt; 0 || next.row &lt; 0 || next.column &gt;= ', $map-name, 'Columns || next.row &gt;= ', $map-name, 'Rows) { return; }&#10;')" />
    </xsl:if>
    <xsl:variable name="quiet" select="($map/group | $map/object[count(. | $goer-node) != 1])
        [not(@class = $goer-node/collisions/collision/@class) and not(collisions/collision/@class = $goer-node/@class)]" />
    <xsl:if test="$quiet">
      <xsl:text>&#9;if (</xsl:text>
      <xsl:for-each select="$quiet">
        <xsl:if test="position() &gt; 1"> || </xsl:if>
        <xsl:call-template name="spot-test"><xsl:with-param name="goer" select="$goer-node" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text>) { return; }&#10;</xsl:text>
    </xsl:if>
    <xsl:for-each select="($map/group | $map/object[count(. | $goer-node) != 1])[count(. | $quiet) != count($quiet)]">
      <xsl:variable name="met" select="." />
      <xsl:variable name="taken" select="boolean(collisions/collision[@class = $goer-node/@class]/despawn[not(@* or node())])" />
      <xsl:variable name="body">
        <xsl:for-each select="$goer-node/collisions/collision[@class = current()/@class]/*">
          <line><xsl:apply-templates select="." mode="cell"><xsl:with-param name="self" select="$goer-node" /></xsl:apply-templates></line>
        </xsl:for-each>
        <xsl:for-each select="collisions/collision[@class = $goer-node/@class]/*">
          <line><xsl:apply-templates select="." mode="cell"><xsl:with-param name="self" select="$met" /></xsl:apply-templates></line>
        </xsl:for-each>
        <xsl:if test="not($taken)"><line>return;</line></xsl:if>
      </xsl:variable>
      <xsl:call-template name="write-block">
        <xsl:with-param name="header">
          <xsl:text>if (</xsl:text>
          <xsl:call-template name="spot-test"><xsl:with-param name="goer" select="$goer-node" /></xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:with-param>
        <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
        <xsl:with-param name="indent" select="'&#9;'" />
      </xsl:call-template>
    </xsl:for-each>
    <xsl:choose>
      <xsl:when test="$chained">
        <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$chained" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat('&#10;&#9;// the ', $links, ' after it, each link into the cell of the one ahead&#10;')" />
        <xsl:value-of select="concat('&#9;', $links, '.unshift(', $goer, '.spot);&#10;')" />
        <xsl:choose>
          <xsl:when test="collisions/collision/grow">
            <xsl:value-of select="concat('&#9;if (', $goer, '.growing &gt; 0) { ', $goer, '.growing -= 1; }&#10;&#9;else { ', $links, '.pop(); }&#10;')" />
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="concat('&#9;', $links, '.pop();&#10;')" /></xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise><xsl:text>&#10;</xsl:text></xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="concat('&#9;', $goer, '.spot = next;&#10;}&#10;')" />
  </xsl:template>

  <!-- Whether the cell stepped into holds this list or piece. -->
  <xsl:template name="spot-test">
    <xsl:param name="goer" />
    <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:choose>
      <xsl:when test="@name = $goer/chain/@object"><xsl:call-template name="chain-test-name"><xsl:with-param name="goer" select="$goer" /></xsl:call-template><xsl:text>(next)</xsl:text></xsl:when>
      <xsl:when test="self::group"><xsl:value-of select="concat('contains(', $named, ', next)')" /></xsl:when>
      <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat('same(next, ', $named, '.spot)')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('same(next, ', $named, ')')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- inBody(): the links but the tail, which moves on as the head comes
       unless it grows. -->
  <xsl:template name="chain-test-function">
    <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="../group[@name = current()/chain/@object]" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('&#10;// Whether the ', $links, ' is in that cell; its tail moves on as the ', $goer, ' comes')" />
    <xsl:if test="collisions/collision/grow">, unless it grows</xsl:if>
    <xsl:text>.&#10;function </xsl:text>
    <xsl:call-template name="chain-test-name" />
    <xsl:text>(spot) {&#10;</xsl:text>
    <xsl:choose>
      <xsl:when test="collisions/collision/grow">
        <xsl:value-of select="concat('&#9;const end = ', $goer, '.growing &gt; 0 ? ', $links, '.length : ', $links, '.length - 1;&#10;')" />
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('&#9;const end = ', $links, '.length - 1;&#10;')" /></xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="concat('&#9;return contains(', $links, '.slice(0, end), spot);&#10;}&#10;')" />
  </xsl:template>

  <!-- freeSpot(): a cell at random that nothing is in. -->
  <xsl:template name="free-spot-function">
    <xsl:variable name="map" select="$cell-maps[1]" />
    <xsl:variable name="map-name"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$map" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('&#10;// A cell of the ', $map/@name, ' at random that nothing is in, nor stepping into; null when it is full.&#10;')" />
    <xsl:text>function freeSpot(stepping) {&#10;&#9;const free = [];&#10;</xsl:text>
    <xsl:value-of select="concat('&#9;for (let row = 0; row &lt; ', $map-name, 'Rows; ++row) {&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;for (let column = 0; column &lt; ', $map-name, 'Columns; ++column) {&#10;')" />
    <xsl:text>&#9;&#9;&#9;const spot = { column, row };&#10;&#9;&#9;&#9;if (same(spot, stepping)</xsl:text>
    <xsl:for-each select="$map/group | $map/object">
      <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:choose>
        <xsl:when test="self::group"><xsl:value-of select="concat(' || contains(', $named, ', spot)')" /></xsl:when>
        <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat(' || same(spot, ', $named, '.spot)')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat(' || same(spot, ', $named, ')')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>) { continue; }&#10;&#9;&#9;&#9;free.push(spot);&#10;&#9;&#9;}&#10;&#9;}&#10;</xsl:text>
    <xsl:text>&#9;if (free.length === 0) { return null; }&#10;</xsl:text>
    <xsl:text>&#9;return free[Math.floor(Math.random() * free.length)];&#10;}&#10;</xsl:text>
  </xsl:template>

  <!-- In frame(): each piece that goes, as often as its time owes a step. -->
  <xsl:template name="map-steps">
    <xsl:param name="after" select="true()" />
    <xsl:for-each select="$goers">
      <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:if test="$after or position() &gt; 1"><xsl:text>&#10;</xsl:text></xsl:if>
      <xsl:value-of select="concat('&#9;// the ', $goer, ' goes on, a cell every so often&#10;')" />
      <xsl:value-of select="concat('&#9;if (', $goer, '.going) {&#10;&#9;&#9;', $goer, '.wait -= elapsed;&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;while (', $goer, '.wait &lt;= 0) {&#10;&#9;&#9;&#9;', $goer, '.wait += ', $goer, '.every;&#10;&#9;&#9;&#9;')" />
      <xsl:call-template name="step-name" />
      <xsl:text>();&#10;&#9;&#9;}&#10;&#9;}&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- A verb on stepping into a cell is written as anywhere else, but
       for those that take or add pieces. -->
  <xsl:template match="*" mode="cell">
    <xsl:param name="self" />
    <xsl:apply-templates select="." mode="do"><xsl:with-param name="self" select="$self" /></xsl:apply-templates>
  </xsl:template>

  <!-- The drawing helpers: a box or a disc in the middle of a cell. -->
  <xsl:template name="map-draw-helpers">
    <xsl:for-each select="$cell-maps">
      <xsl:variable name="map"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:variable name="type"><xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#10;// A box that big in the middle of a cell of the ', @name, '.&#10;function boxOn', $type, '(spot, width, height, color) {&#10;')" />
      <xsl:value-of select="concat('&#9;box(', $map, 'Position.x + (spot.column + 0.5) * ', $map, 'Tile.x - width / 2, ', $map, 'Position.y + (spot.row + 0.5) * ', $map, 'Tile.y - height / 2, width, height, color);&#10;}&#10;')" />
      <xsl:if test=".//circle">
        <xsl:value-of select="concat('&#10;// A disc in the middle of a cell of the ', @name, '.&#10;function discOn', $type, '(spot, radius, color) {&#10;&#9;context.fillStyle = color;&#10;&#9;context.beginPath();&#10;')" />
        <xsl:value-of select="concat('&#9;context.arc(', $map, 'Position.x + (spot.column + 0.5) * ', $map, 'Tile.x, ', $map, 'Position.y + (spot.row + 0.5) * ', $map, 'Tile.y, radius, 0, 2 * Math.PI);&#10;&#9;context.fill();&#10;}&#10;')" />
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- The drawing of a map's pieces, each in the middle of its cell. -->
  <xsl:template name="map-draw-lines">
    <xsl:variable name="type"><xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:for-each select="group | object">
      <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:variable name="spot">
        <xsl:choose>
          <xsl:when test="self::group">spot</xsl:when>
          <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat($named, '.spot')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$named" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="shape">
        <xsl:choose>
          <xsl:when test="sprite/circle">
            <xsl:value-of select="concat('discOn', $type, '(', $spot, ', ')" />
            <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/circle/radius" /></xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(sprite/circle/color)" /></xsl:call-template>
            <xsl:text>);</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('boxOn', $type, '(', $spot, ', ')" />
            <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/rectangle/width" /></xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/rectangle/height" /></xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(sprite/rectangle/color)" /></xsl:call-template>
            <xsl:text>);</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <line>
        <xsl:choose>
          <xsl:when test="self::group"><xsl:value-of select="concat('for (const spot of ', $named, ') { ', $shape, ' }')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$shape" /></xsl:otherwise>
        </xsl:choose>
      </line>
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
