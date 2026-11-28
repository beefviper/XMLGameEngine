<?xml version="1.0" encoding="UTF-8"?>
<!-- cells.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 28, 2026 -->

<!-- A map of cells (Snake's field): its rows of letters kept as they are
     drawn, each kind of piece a list of Spots (a column and a row) read from
     them, and a piece that goes on by itself a record of its own (Head),
     stepping a cell every so often. A step into a taken cell is a touch:
     both sides' rules run, and it is refused unless they took what was
     there away. A chain's links follow its head. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <!-- Spot, and each map's size, place and rows. -->
  <xsl:template name="map-constants">
    <xsl:text>
	// A cell of a map: its column and row, from 0 at the top left.
	struct Spot
	{
		int column{ 0 };
		int row{ 0 };

		bool operator==(const Spot&amp;) const = default;
	};
</xsl:text>
    <xsl:for-each select="$cell-maps">
      <xsl:variable name="map"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#10;&#9;// The ', @name, ', a cell a letter: ')" />
      <xsl:for-each select="group | object">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:value-of select="concat(@char, ' ', @name)" />
      </xsl:for-each>
      <xsl:text>.&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;constexpr int ', $map, 'Columns = ', string-length(normalize-space(row[1])), ';&#10;')" />
      <xsl:value-of select="concat('&#9;constexpr int ', $map, 'Rows = ', count(row), ';&#10;')" />
      <xsl:value-of select="concat('&#9;constexpr sf::Vector2f ', $map, 'Tile{ ')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="tile/width" /></xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="tile/height" /></xsl:call-template>
      <xsl:text> }; // a cell's size&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;constexpr sf::Vector2f ', $map, 'Position{ ')" />
      <xsl:call-template name="axis-value"><xsl:with-param name="node" select="position/x" /></xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="axis-value"><xsl:with-param name="node" select="position/y" /></xsl:call-template>
      <xsl:text> }; // its top left&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;inline const std::vector&lt;std::string&gt; ', $map, 'Map{&#10;')" />
      <xsl:for-each select="row">
        <xsl:value-of select="concat('&#9;&#9;&quot;', normalize-space(.), '&quot;,&#10;')" />
      </xsl:for-each>
      <xsl:text>&#9;};&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- A piece that goes: struct Head. -->
  <xsl:template name="goer-record">
    <xsl:value-of select="concat('&#10;&#9;// The ', @name, ': its cell, and the way it goes on by itself, a cell every so often.&#10;&#9;struct ')" />
    <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
    <xsl:text>
	{
		Spot spot;
		Spot way; // a cell up, down, left or right
		bool going{ false };
		float every{ 0 }; // seconds a step
		float wait{ 0 }; // until the next step
</xsl:text>
    <xsl:if test="collisions/collision/grow">
      <xsl:text>&#9;&#9;int growing{ 0 }; // links still to come at the tail&#10;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;};&#10;</xsl:text>
  </xsl:template>

  <!-- The class's lists and pieces of each map. -->
  <xsl:template name="map-members">
    <xsl:for-each select="$cell-maps/group | $cell-maps/object">
      <xsl:text>&#9;&#9;</xsl:text>
      <xsl:choose>
        <xsl:when test="self::group">std::vector&lt;Spot&gt;</xsl:when>
        <xsl:when test="count(. | $goers) = count($goers)"><xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:when>
        <xsl:otherwise>Spot</xsl:otherwise>
      </xsl:choose>
      <xsl:text> </xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- go, step, a chain's test and a free cell: their names. -->
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

  <!-- Their declarations, private. -->
  <xsl:template name="map-declarations">
    <xsl:for-each select="$cell-maps">
      <xsl:text>&#9;&#9;void </xsl:text>
      <xsl:call-template name="map-start-name" />
      <xsl:text>();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$goers">
      <xsl:variable name="type"><xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
      <xsl:text>&#9;&#9;void </xsl:text>
      <xsl:call-template name="go-name" />
      <xsl:text>(Spot way, float speed);&#10;&#9;&#9;void </xsl:text>
      <xsl:call-template name="step-name" />
      <xsl:text>();&#10;</xsl:text>
      <xsl:if test="chain">
        <xsl:text>&#9;&#9;bool </xsl:text>
        <xsl:call-template name="chain-test-name" />
        <xsl:text>(Spot spot) const;&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:if test="$free-spawns">
      <xsl:text>&#9;&#9;std::optional&lt;Spot&gt; freeSpot(Spot stepping);&#10;</xsl:text>
    </xsl:if>
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
    <xsl:value-of select="concat('&#10;&#9;// The ', @name, ' as its map draws it, a cell a letter.&#10;&#9;void ', $class, '::')" />
    <xsl:call-template name="map-start-name" />
    <xsl:text>()&#10;&#9;{&#10;</xsl:text>
    <xsl:for-each select="group | object">
      <xsl:text>&#9;&#9;</xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:choose>
        <xsl:when test="self::group">.clear();&#10;</xsl:when>
        <xsl:otherwise> = {};&#10;</xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:value-of select="concat('&#9;&#9;for (std::size_t row = 0; row &lt; ', $map, 'Map.size(); ++row)&#10;&#9;&#9;{&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;&#9;for (std::size_t column = 0; column &lt; ', $map, 'Map[row].size(); ++column)&#10;&#9;&#9;&#9;{&#10;')" />
    <xsl:text>&#9;&#9;&#9;&#9;const Spot spot{ static_cast&lt;int&gt;(column), static_cast&lt;int&gt;(row) };&#10;</xsl:text>
    <xsl:value-of select="concat('&#9;&#9;&#9;&#9;switch (', $map, 'Map[row][column])&#10;&#9;&#9;&#9;&#9;{&#10;')" />
    <xsl:for-each select="group | object">
      <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;&#9;&#9;case ', $quote, @char, $quote, ': ')" />
      <xsl:choose>
        <xsl:when test="self::group"><xsl:value-of select="concat($named, '.push_back(spot); break;&#10;')" /></xsl:when>
        <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat($named, '.spot = spot; break;&#10;')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat($named, ' = spot; break;&#10;')" /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>&#9;&#9;&#9;&#9;default: break;&#10;&#9;&#9;&#9;&#9;}&#10;&#9;&#9;&#9;}&#10;&#9;&#9;}&#10;</xsl:text>
    <xsl:for-each select="object[chain]">
      <xsl:variable name="head"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="../group[@name = current()/chain/@object]" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#10;&#9;&#9;// the ', $links, ' in order from the ', $head, ', each link the one beside the last&#10;')" />
      <xsl:text>&#9;&#9;std::vector&lt;Spot&gt; links;&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;&#9;for (Spot last = ', $head, '.spot;;)&#10;&#9;&#9;{&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;&#9;const auto next = std::find_if(', $links, '.begin(), ', $links, '.end(), [&amp;](Spot link) { return std::abs(link.column - last.column) + std::abs(link.row - last.row) == 1; });&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;&#9;if (next == ', $links, '.end()) { break; }&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;&#9;last = *next;&#10;&#9;&#9;&#9;links.push_back(last);&#10;&#9;&#9;&#9;', $links, '.erase(next);&#10;&#9;&#9;}&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;', $links, ' = links;&#10;')" />
    </xsl:for-each>
    <xsl:text>&#9;}&#10;</xsl:text>
  </xsl:template>

  <!-- go(): off on its way, `speed` cells a second. -->
  <xsl:template name="go-function">
    <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#10;&#9;// The ', $goer, ' off on its way, `speed` cells a second')" />
    <xsl:if test="chain">, never straight back into its <xsl:value-of select="chain/@object" /></xsl:if>
    <xsl:value-of select="concat('.&#10;&#9;void ', $class, '::')" />
    <xsl:call-template name="go-name" />
    <xsl:text>(Spot way, float speed)&#10;&#9;{&#10;</xsl:text>
    <xsl:if test="chain">
      <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="../group[@name = current()/chain/@object]" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;if (!', $links, '.empty() &amp;&amp; ', $links, '.front() == Spot{ ', $goer, '.spot.column + way.column, ', $goer, '.spot.row + way.row }) { return; }&#10;')" />
    </xsl:if>
    <xsl:value-of select="concat('&#9;&#9;', $goer, '.every = 1 / speed;&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;if (!', $goer, '.going || ', $goer, '.wait &gt; ', $goer, '.every) { ', $goer, '.wait = ', $goer, '.every; }&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;', $goer, '.going = true;&#10;&#9;&#9;', $goer, '.way = way;&#10;&#9;}&#10;')" />
  </xsl:template>

  <!-- stepHead(): a cell on its way, a touch for each kind of piece it
       steps into; refused unless the touch took the piece away. -->
  <xsl:template name="step-function">
    <xsl:variable name="goer-node" select="." />
    <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="map" select=".." />
    <xsl:variable name="map-name"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$map" /></xsl:call-template></xsl:variable>
    <xsl:variable name="chained" select="$map/group[@name = current()/chain/@object]" />
    <xsl:value-of select="concat('&#10;&#9;// The ', $goer, ' a cell on its way. A step into another piece is a touch, and is&#10;&#9;// refused unless the touch takes that piece away.&#10;&#9;void ', $class, '::')" />
    <xsl:call-template name="step-name" />
    <xsl:text>()&#10;&#9;{&#10;</xsl:text>
    <xsl:value-of select="concat('&#9;&#9;const Spot next{ ', $goer, '.spot.column + ', $goer, '.way.column, ', $goer, '.spot.row + ', $goer, '.way.row };&#10;')" />
    <xsl:variable name="walled" select="not(contains(normalize-space($map/row[1]), '.') or contains(normalize-space($map/row[last()]), '.'))
        and not($map/row[substring(normalize-space(.), 1, 1) = '.' or substring(normalize-space(.), string-length(normalize-space(.))) = '.'])" />
    <xsl:if test="not($walled)">
      <xsl:value-of select="concat('&#9;&#9;if (next.column &lt; 0 || next.row &lt; 0 || next.column &gt;= ', $map-name, 'Columns || next.row &gt;= ', $map-name, 'Rows) { return; }&#10;')" />
    </xsl:if>
    <!-- the lists and pieces nobody's rules name: a plain refusal -->
    <xsl:variable name="quiet" select="($map/group | $map/object[count(. | $goer-node) != 1])
        [not(@class = $goer-node/collisions/collision/@class) and not(collisions/collision/@class = $goer-node/@class)]" />
    <xsl:if test="$quiet">
      <xsl:text>&#9;&#9;if (</xsl:text>
      <xsl:for-each select="$quiet">
        <xsl:if test="position() &gt; 1"> || </xsl:if>
        <xsl:call-template name="spot-test"><xsl:with-param name="goer" select="$goer-node" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text>) { return; }&#10;</xsl:text>
    </xsl:if>
    <xsl:for-each select="($map/group | $map/object[count(. | $goer-node) != 1])[count(. | $quiet) != count($quiet)]">
      <xsl:variable name="met" select="." />
      <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
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
          <xsl:choose>
            <xsl:when test="$taken"><xsl:value-of select="concat('if (const auto met = std::find(', $named, '.begin(), ', $named, '.end(), next); met != ', $named, '.end())')" /></xsl:when>
            <xsl:otherwise>
              <xsl:text>if (</xsl:text>
              <xsl:call-template name="spot-test"><xsl:with-param name="goer" select="$goer-node" /></xsl:call-template>
              <xsl:text>)</xsl:text>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
        <xsl:with-param name="lines" select="exsl:node-set($body)/line" />
        <xsl:with-param name="indent" select="'&#9;&#9;'" />
      </xsl:call-template>
    </xsl:for-each>
    <xsl:choose>
      <xsl:when test="$chained">
        <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$chained" /></xsl:call-template></xsl:variable>
        <xsl:value-of select="concat('&#10;&#9;&#9;// the ', $links, ' after it, each link into the cell of the one ahead&#10;')" />
        <xsl:value-of select="concat('&#9;&#9;', $links, '.insert(', $links, '.begin(), ', $goer, '.spot);&#10;')" />
        <xsl:choose>
          <xsl:when test="collisions/collision/grow">
            <xsl:value-of select="concat('&#9;&#9;if (', $goer, '.growing &gt; 0) { --', $goer, '.growing; }&#10;&#9;&#9;else { ', $links, '.pop_back(); }&#10;')" />
          </xsl:when>
          <xsl:otherwise><xsl:value-of select="concat('&#9;&#9;', $links, '.pop_back();&#10;')" /></xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise><xsl:text>&#10;</xsl:text></xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="concat('&#9;&#9;', $goer, '.spot = next;&#10;&#9;}&#10;')" />
  </xsl:template>

  <!-- Whether the cell stepped into holds this list or piece. -->
  <xsl:template name="spot-test">
    <xsl:param name="goer" />
    <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:choose>
      <xsl:when test="@name = $goer/chain/@object"><xsl:call-template name="chain-test-name"><xsl:with-param name="goer" select="$goer" /></xsl:call-template><xsl:text>(next)</xsl:text></xsl:when>
      <xsl:when test="self::group"><xsl:value-of select="concat('contains(', $named, ', next)')" /></xsl:when>
      <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat('next == ', $named, '.spot')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('next == ', $named)" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- inBody(): the links but the tail, which moves on as the head comes
       unless it grows. -->
  <xsl:template name="chain-test-function">
    <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="links"><xsl:call-template name="object-name"><xsl:with-param name="object" select="../group[@name = current()/chain/@object]" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('&#10;&#9;// Whether the ', $links, ' is in that cell; its tail moves on as the ', $goer, ' comes')" />
    <xsl:if test="collisions/collision/grow">, unless it grows</xsl:if>
    <xsl:value-of select="concat('.&#10;&#9;bool ', $class, '::')" />
    <xsl:call-template name="chain-test-name" />
    <xsl:text>(Spot spot) const&#10;&#9;{&#10;</xsl:text>
    <xsl:text>&#9;&#9;const auto end = </xsl:text>
    <xsl:if test="collisions/collision/grow"><xsl:value-of select="concat($goer, '.growing &gt; 0 || ')" /></xsl:if>
    <xsl:value-of select="concat($links, '.empty() ? ', $links, '.end() : ', $links, '.end() - 1;&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;return std::find(', $links, '.begin(), end, spot) != end;&#10;&#9;}&#10;')" />
  </xsl:template>

  <!-- freeSpot(): a cell at random that nothing is in. -->
  <xsl:template name="free-spot-function">
    <xsl:variable name="map" select="$cell-maps[1]" />
    <xsl:variable name="map-name"><xsl:call-template name="object-name"><xsl:with-param name="object" select="$map" /></xsl:call-template></xsl:variable>
    <xsl:value-of select="concat('&#10;&#9;// A cell of the ', $map/@name, ' at random that nothing is in, nor stepping into; none when it is full.&#10;')" />
    <xsl:value-of select="concat('&#9;std::optional&lt;Spot&gt; ', $class, '::freeSpot(Spot stepping)&#10;&#9;{&#10;&#9;&#9;std::vector&lt;Spot&gt; free;&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;for (int row = 0; row &lt; ', $map-name, 'Rows; ++row)&#10;&#9;&#9;{&#10;')" />
    <xsl:value-of select="concat('&#9;&#9;&#9;for (int column = 0; column &lt; ', $map-name, 'Columns; ++column)&#10;&#9;&#9;&#9;{&#10;')" />
    <xsl:text>&#9;&#9;&#9;&#9;const Spot spot{ column, row };&#10;&#9;&#9;&#9;&#9;if (spot == stepping</xsl:text>
    <xsl:for-each select="$map/group | $map/object">
      <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:choose>
        <xsl:when test="self::group"><xsl:value-of select="concat(' || contains(', $named, ', spot)')" /></xsl:when>
        <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat(' || spot == ', $named, '.spot')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="concat(' || spot == ', $named)" /></xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>) { continue; }&#10;&#9;&#9;&#9;&#9;free.push_back(spot);&#10;&#9;&#9;&#9;}&#10;&#9;&#9;}&#10;</xsl:text>
    <xsl:text>&#9;&#9;if (free.empty()) { return std::nullopt; }&#10;</xsl:text>
    <xsl:text>&#9;&#9;return free[std::uniform_int_distribution&lt;std::size_t&gt;(0, free.size() - 1)(random)];&#10;&#9;}&#10;</xsl:text>
  </xsl:template>

  <!-- In frame(): each piece that goes, as often as its time owes a step. -->
  <xsl:template name="map-steps">
    <xsl:param name="after" select="true()" />
    <xsl:for-each select="$goers">
      <xsl:variable name="goer"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:if test="$after or position() &gt; 1"><xsl:text>&#10;</xsl:text></xsl:if>
      <xsl:value-of select="concat('&#9;&#9;// the ', $goer, ' goes on, a cell every so often&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;if (', $goer, '.going)&#10;&#9;&#9;{&#10;&#9;&#9;&#9;', $goer, '.wait -= elapsed;&#10;')" />
      <xsl:value-of select="concat('&#9;&#9;&#9;while (', $goer, '.wait &lt;= 0)&#10;&#9;&#9;&#9;{&#10;&#9;&#9;&#9;&#9;', $goer, '.wait += ', $goer, '.every;&#10;&#9;&#9;&#9;&#9;')" />
      <xsl:call-template name="step-name" />
      <xsl:text>();&#10;&#9;&#9;&#9;}&#10;&#9;&#9;}&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- A verb on stepping into a cell is written as anywhere else, but
       for those that take or add pieces. -->
  <xsl:template match="*" mode="cell">
    <xsl:param name="self" />
    <xsl:apply-templates select="." mode="do"><xsl:with-param name="self" select="$self" /></xsl:apply-templates>
  </xsl:template>

  <!-- The drawing of a map's pieces, each in the middle of its cell. -->
  <xsl:template name="map-draw-lines">
    <xsl:variable name="map"><xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:variable>
    <xsl:for-each select="group | object">
      <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:variable name="spot">
        <xsl:choose>
          <xsl:when test="self::group">spot</xsl:when>
          <xsl:when test="count(. | $goers) = count($goers)"><xsl:value-of select="concat('game.', $named, '.spot')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="concat('game.', $named)" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="shape">
        <xsl:choose>
          <xsl:when test="sprite/circle">
            <xsl:value-of select="concat(substring-before($box, 'box('), 'disc(on', $map, '(', $spot, ', {}), ')" />
            <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/circle/radius" /></xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(sprite/circle/color)" /></xsl:call-template>
            <xsl:text>);</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:variable name="size">
              <xsl:text>{ </xsl:text>
              <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/rectangle/width" /></xsl:call-template>
              <xsl:text>, </xsl:text>
              <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/rectangle/height" /></xsl:call-template>
              <xsl:text> }</xsl:text>
            </xsl:variable>
            <xsl:value-of select="concat($box, 'on', $map, '(', $spot, ', ', $size, '), ', $size, ', ')" />
            <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(sprite/rectangle/color)" /></xsl:call-template>
            <xsl:text>);</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <line>
        <xsl:choose>
          <xsl:when test="self::group"><xsl:value-of select="concat('for (const Spot&amp; spot : game.', $named, ') { ', $shape, ' }')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$shape" /></xsl:otherwise>
        </xsl:choose>
      </line>
    </xsl:for-each>
  </xsl:template>

  <!-- onField(): where something that big sits, in the middle of its cell. -->
  <xsl:template name="map-place-functions">
    <xsl:for-each select="$cell-maps">
      <xsl:variable name="map"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#10;&#9;// Where something that big sits on the ', @name, ': in the middle of its cell.&#10;&#9;sf::Vector2f on')" />
      <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:text>(Spot spot, sf::Vector2f size)&#10;&#9;{&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;&#9;return ', $map, 'Position + sf::Vector2f{ (static_cast&lt;float&gt;(spot.column) + 0.5f) * ', $map, 'Tile.x, (static_cast&lt;float&gt;(spot.row) + 0.5f) * ', $map, 'Tile.y } - size / 2.0f;&#10;&#9;}&#10;')" />
    </xsl:for-each>
  </xsl:template>
</xsl:stylesheet>
