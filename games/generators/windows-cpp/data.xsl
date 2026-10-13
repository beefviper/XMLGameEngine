<?xml version="1.0" encoding="UTF-8"?>
<!-- data.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 11, 2026 -->

<!-- The game as the engine's own data: source/<game>.cpp of a generated
     program, a function that returns the GameData (game_data.h) the engine
     would read from the file. Each tag becomes the line of C++ that makes
     what game_xml.cpp makes of it, with the small functions of
     modules/game_parts.h, so the program plays the game with the engine's
     own code. The file has been checked by the engine before this runs
     (generate.cpp), so nothing here checks it again. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="tab" select="'&#9;'" />
  <xsl:variable name="newline" select="'&#10;'" />

  <!-- ===================================================================
       Values
       =================================================================== -->

  <!-- The value $node holds: its text as an expression, or the one value tag
       inside it (readValue). -->
  <xsl:template name="value">
    <xsl:param name="node" />
    <xsl:variable name="tag" select="$node/*[1]" />
    <xsl:choose>
      <xsl:when test="$tag[self::random]">
        <xsl:call-template name="random">
          <xsl:with-param name="node" select="$tag" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$tag[self::equation]">
        <xsl:text>equation({ </xsl:text>
        <xsl:for-each select="$tag/*">
          <xsl:if test="position() &gt; 1">, </xsl:if>
          <xsl:call-template name="equation-step" />
        </xsl:for-each>
        <xsl:text> })</xsl:text>
      </xsl:when>
      <xsl:when test="$tag[self::formula]">
        <xsl:text>formula(</xsl:text>
        <xsl:for-each select="$tag/*[1]">
          <xsl:call-template name="formula-operation" />
        </xsl:for-each>
        <xsl:text>)</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>value(</xsl:text>
        <xsl:call-template name="text">
          <xsl:with-param name="node" select="$node" />
        </xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The value of the child of $node named $name. -->
  <xsl:template name="value-of">
    <xsl:param name="node" />
    <xsl:param name="name" />
    <xsl:choose>
      <xsl:when test="$node/*[local-name() = $name]">
        <xsl:call-template name="value">
          <xsl:with-param name="node" select="$node/*[local-name() = $name][1]" />
        </xsl:call-template>
      </xsl:when>
      <!-- left out, as an object's <velocity> or a half of it may be: 0 -->
      <xsl:otherwise>value("0")</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- An <x>/<y> pair: { x, y }. -->
  <xsl:template name="vector">
    <xsl:param name="node" />
    <xsl:text>{ </xsl:text>
    <xsl:call-template name="value-of">
      <xsl:with-param name="node" select="$node" />
      <xsl:with-param name="name" select="'x'" />
    </xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="value-of">
      <xsl:with-param name="node" select="$node" />
      <xsl:with-param name="name" select="'y'" />
    </xsl:call-template>
    <xsl:text> }</xsl:text>
  </xsl:template>

  <xsl:template name="random">
    <xsl:param name="node" />
    <xsl:text>random(</xsl:text>
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="$node/@min" />
    </xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="$node/@max" />
    </xsl:call-template>
    <xsl:text>)</xsl:text>
  </xsl:template>

  <!-- What an operation's operands are called: the first, then the rest. -->
  <xsl:template name="first-operand">
    <xsl:param name="op" />
    <xsl:choose>
      <xsl:when test="$op = 'add'">augend</xsl:when>
      <xsl:when test="$op = 'subtract'">minuend</xsl:when>
      <xsl:when test="$op = 'multiply'">multiplicand</xsl:when>
      <xsl:when test="$op = 'divide'">dividend</xsl:when>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="other-operand">
    <xsl:param name="op" />
    <xsl:choose>
      <xsl:when test="$op = 'add'">addend</xsl:when>
      <xsl:when test="$op = 'subtract'">subtrahend</xsl:when>
      <xsl:when test="$op = 'multiply'">multiplier</xsl:when>
      <xsl:when test="$op = 'divide'">divisor</xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- A step of an <equation>: <divide name="half" dividend="title.width" divisor="2" />. -->
  <xsl:template name="equation-step">
    <xsl:variable name="first">
      <xsl:call-template name="first-operand">
        <xsl:with-param name="op" select="local-name()" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="other">
      <xsl:call-template name="other-operand">
        <xsl:with-param name="op" select="local-name()" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat('operation(&quot;', local-name(), '&quot;, ')" />
    <xsl:call-template name="text">
      <xsl:with-param name="node" select="@name" />
    </xsl:call-template>
    <xsl:value-of select="concat(', { { &quot;', $first, '&quot;, value(')" />
    <xsl:call-template name="text">
      <xsl:with-param name="node" select="@*[local-name() = $first]" />
    </xsl:call-template>
    <xsl:value-of select="concat(') }, { &quot;', $other, '&quot;, value(')" />
    <xsl:call-template name="text">
      <xsl:with-param name="node" select="@*[local-name() = $other]" />
    </xsl:call-template>
    <xsl:text>) } })</xsl:text>
  </xsl:template>

  <!-- An operation of a <formula>: its operands, each a name, a number, a
       <random> or an operation of its own. -->
  <xsl:template name="formula-operation">
    <xsl:value-of select="concat('operation(&quot;', local-name(), '&quot;, &quot;&quot;, { ')" />
    <xsl:for-each select="*">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:value-of select="concat('{ &quot;', local-name(), '&quot;, ')" />
      <xsl:choose>
        <xsl:when test="random">
          <xsl:call-template name="random">
            <xsl:with-param name="node" select="random" />
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="*">
          <xsl:text>formula(</xsl:text>
          <xsl:for-each select="*[1]">
            <xsl:call-template name="formula-operation" />
          </xsl:for-each>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>value(</xsl:text>
          <xsl:call-template name="text" />
          <xsl:text>)</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:text> }</xsl:text>
    </xsl:for-each>
    <xsl:text> })</xsl:text>
  </xsl:template>

  <!-- ===================================================================
       Commands
       =================================================================== -->

  <!-- One verb tag (readCommand): verb("bounce"), or verb("move", ...) with
       what it says. -->
  <xsl:template name="command">
    <xsl:param name="node" select="." />
    <xsl:variable name="verb" select="local-name($node)" />
    <xsl:variable name="fill">
      <xsl:for-each select="$node/@object | $node/@variable | $node/@state | $node/@action | $node/@direction | $node/@burn | $node/@sound | $node/@sprite | $node/@path | $node/@class">
        <xsl:if test=". != ''">
          <xsl:text> command.</xsl:text>
          <xsl:choose>
            <xsl:when test="local-name() = 'class'">objClass</xsl:when>
            <xsl:otherwise><xsl:value-of select="local-name()" /></xsl:otherwise>
          </xsl:choose>
          <xsl:text> = </xsl:text>
          <xsl:call-template name="string">
            <xsl:with-param name="text" select="." />
          </xsl:call-template>
          <xsl:text>;</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:choose>
        <!-- how many is optional, and one when left out -->
        <xsl:when test="$verb = 'inc' or $verb = 'dec' or $verb = 'reveal' or $verb = 'release'">
          <xsl:text> command.amount = </xsl:text>
          <xsl:choose>
            <xsl:when test="$node/* or normalize-space($node) != ''">
              <xsl:call-template name="value">
                <xsl:with-param name="node" select="$node" />
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>value("1")</xsl:otherwise>
          </xsl:choose>
          <xsl:text>;</xsl:text>
        </xsl:when>
        <xsl:when test="$verb = 'move' or $verb = 'hop' or $verb = 'accelerate' or $verb = 'turn' or $verb = 'thrust' or $verb = 'deflect' or $verb = 'leap' or $verb = 'climb'">
          <xsl:text> command.amount = </xsl:text>
          <xsl:call-template name="value">
            <xsl:with-param name="node" select="$node" />
          </xsl:call-template>
          <xsl:text>;</xsl:text>
        </xsl:when>
        <xsl:when test="$verb = 'jump'">
          <xsl:call-template name="field">
            <xsl:with-param name="name" select="'command.amount'" />
            <xsl:with-param name="node" select="$node/distance" />
          </xsl:call-template>
          <xsl:call-template name="field">
            <xsl:with-param name="name" select="'command.seconds'" />
            <xsl:with-param name="node" select="$node/seconds" />
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$verb = 'chase'">
          <xsl:call-template name="field">
            <xsl:with-param name="name" select="'command.amount'" />
            <xsl:with-param name="node" select="$node/speed" />
          </xsl:call-template>
          <xsl:call-template name="field">
            <xsl:with-param name="name" select="'command.distance'" />
            <xsl:with-param name="node" select="$node/near" />
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$verb = 'follow'">
          <xsl:call-template name="field">
            <xsl:with-param name="name" select="'command.seconds'" />
            <xsl:with-param name="node" select="$node/stagger" />
          </xsl:call-template>
        </xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:value-of select="concat('verb(&quot;', $verb, '&quot;')" />
    <xsl:if test="$fill != ''">
      <xsl:value-of select="concat(', [](RawCommand&amp; command) {', $fill, ' }')" />
    </xsl:if>
    <xsl:text>)</xsl:text>
  </xsl:template>

  <!-- " name = value;" for the first of $node, if there is one. -->
  <xsl:template name="field">
    <xsl:param name="name" />
    <xsl:param name="node" />
    <xsl:if test="$node">
      <xsl:value-of select="concat(' ', $name, ' = ')" />
      <xsl:call-template name="value">
        <xsl:with-param name="node" select="$node[1]" />
      </xsl:call-template>
      <xsl:text>;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The commands $nodes, as a list: on one line if there is one, else a
       line each, at $indent and a tab. -->
  <xsl:template name="commands">
    <xsl:param name="nodes" />
    <xsl:param name="indent" />
    <xsl:choose>
      <xsl:when test="not($nodes)">{}</xsl:when>
      <xsl:when test="count($nodes) = 1">
        <xsl:text>{ </xsl:text>
        <xsl:call-template name="command">
          <xsl:with-param name="node" select="$nodes" />
        </xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>{&#10;</xsl:text>
        <xsl:for-each select="$nodes">
          <xsl:value-of select="concat($indent, $tab)" />
          <xsl:call-template name="command" />
          <xsl:text>,&#10;</xsl:text>
        </xsl:for-each>
        <xsl:value-of select="concat($indent, '}')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ===================================================================
       Sprites
       =================================================================== -->

  <!-- What a <sprite> is: "line" for a drawing of lines, else its shape. -->
  <xsl:template name="sprite-kind">
    <xsl:param name="node" />
    <xsl:choose>
      <xsl:when test="$node/*[1][self::line]">line</xsl:when>
      <xsl:otherwise><xsl:value-of select="local-name($node/*[1])" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A line of a sprite's fill: "look.name = value;" at $indent. -->
  <xsl:template name="look-value">
    <xsl:param name="indent" />
    <xsl:param name="name" />
    <xsl:param name="node" />
    <xsl:if test="$node">
      <xsl:value-of select="concat($indent, 'look.', $name, ' = ')" />
      <xsl:call-template name="value">
        <xsl:with-param name="node" select="$node[1]" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template name="look-text">
    <xsl:param name="indent" />
    <xsl:param name="name" />
    <xsl:param name="node" />
    <xsl:if test="$node">
      <xsl:value-of select="concat($indent, 'look.', $name, ' = ')" />
      <xsl:call-template name="text">
        <xsl:with-param name="node" select="$node[1]" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- A list of the trimmed texts of $nodes: { "a", "b" }. -->
  <xsl:template name="texts">
    <xsl:param name="nodes" />
    <xsl:text>{ </xsl:text>
    <xsl:for-each select="$nodes">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="text" />
    </xsl:for-each>
    <xsl:text> }</xsl:text>
  </xsl:template>

  <!-- What a <sprite> says (readShape, readLine), one statement a line at
       $indent: all of it for a whole sprite, only what it gives for one that
       changes a group's ($change), as changeSprite reads it. -->
  <xsl:template name="sprite-fill">
    <xsl:param name="node" />
    <xsl:param name="indent" />
    <xsl:param name="change" select="false()" />
    <xsl:variable name="kind">
      <xsl:call-template name="sprite-kind">
        <xsl:with-param name="node" select="$node" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="shape" select="$node/*[1]" />
    <xsl:choose>
      <xsl:when test="$kind = 'line'">
        <xsl:value-of select="concat($indent, 'look.lines = {&#10;')" />
        <xsl:for-each select="$node/line">
          <xsl:value-of select="concat($indent, $tab, 'make&lt;RawLine&gt;([](RawLine&amp; line) { line.from = ')" />
          <xsl:call-template name="vector">
            <xsl:with-param name="node" select="from" />
          </xsl:call-template>
          <xsl:text>; line.to = </xsl:text>
          <xsl:call-template name="vector">
            <xsl:with-param name="node" select="to" />
          </xsl:call-template>
          <xsl:text>;</xsl:text>
          <xsl:if test="color">
            <xsl:text> line.color = </xsl:text>
            <xsl:call-template name="text">
              <xsl:with-param name="node" select="color" />
            </xsl:call-template>
            <xsl:text>;</xsl:text>
          </xsl:if>
          <xsl:if test="thickness">
            <xsl:text> line.thickness = </xsl:text>
            <xsl:call-template name="value">
              <xsl:with-param name="node" select="thickness" />
            </xsl:call-template>
            <xsl:text>; line.hasThickness = true;</xsl:text>
          </xsl:if>
          <xsl:text> }),&#10;</xsl:text>
        </xsl:for-each>
        <xsl:value-of select="concat($indent, '};&#10;')" />
      </xsl:when>
      <xsl:when test="$kind = 'circle'">
        <xsl:call-template name="look-value">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'radius'" />
          <xsl:with-param name="node" select="$shape/radius" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$kind = 'rectangle'">
        <xsl:call-template name="look-value">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'width'" />
          <xsl:with-param name="node" select="$shape/width" />
        </xsl:call-template>
        <xsl:call-template name="look-value">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'height'" />
          <xsl:with-param name="node" select="$shape/height" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$kind = 'text'">
        <xsl:choose>
          <xsl:when test="$shape/content">
            <xsl:call-template name="look-text">
              <xsl:with-param name="indent" select="$indent" />
              <xsl:with-param name="name" select="'content'" />
              <xsl:with-param name="node" select="$shape/content" />
            </xsl:call-template>
            <xsl:if test="$change">
              <xsl:value-of select="concat($indent, 'look.textIsNumber = false;&#10;')" />
            </xsl:if>
          </xsl:when>
          <xsl:when test="$shape/number">
            <xsl:value-of select="concat($indent, 'look.textIsNumber = true;&#10;')" />
            <xsl:call-template name="look-value">
              <xsl:with-param name="indent" select="$indent" />
              <xsl:with-param name="name" select="'number'" />
              <xsl:with-param name="node" select="$shape/number" />
            </xsl:call-template>
          </xsl:when>
        </xsl:choose>
        <xsl:call-template name="look-value">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'size'" />
          <xsl:with-param name="node" select="$shape/size" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$kind = 'image'">
        <xsl:call-template name="look-text">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'path'" />
          <xsl:with-param name="node" select="$shape/path" />
        </xsl:call-template>
        <xsl:call-template name="look-text">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'flip'" />
          <xsl:with-param name="node" select="$shape/flip" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$kind = 'bitmap' or $kind = 'svg'">
        <xsl:if test="$shape/row">
          <xsl:value-of select="concat($indent, 'look.bitmapRows = ')" />
          <xsl:call-template name="texts">
            <xsl:with-param name="nodes" select="$shape/row" />
          </xsl:call-template>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
        <xsl:call-template name="look-text">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'path'" />
          <xsl:with-param name="node" select="$shape/path" />
        </xsl:call-template>
        <xsl:if test="$kind = 'svg' and ($shape/x or $shape/y or $shape/width or $shape/height)">
          <xsl:value-of select="concat($indent, 'look.hasSvgRegion = true;&#10;')" />
          <xsl:call-template name="look-value">
            <xsl:with-param name="indent" select="$indent" />
            <xsl:with-param name="name" select="'svgX'" />
            <xsl:with-param name="node" select="$shape/x" />
          </xsl:call-template>
          <xsl:call-template name="look-value">
            <xsl:with-param name="indent" select="$indent" />
            <xsl:with-param name="name" select="'svgY'" />
            <xsl:with-param name="node" select="$shape/y" />
          </xsl:call-template>
          <xsl:call-template name="look-value">
            <xsl:with-param name="indent" select="$indent" />
            <xsl:with-param name="name" select="'svgWidth'" />
            <xsl:with-param name="node" select="$shape/width" />
          </xsl:call-template>
          <xsl:call-template name="look-value">
            <xsl:with-param name="indent" select="$indent" />
            <xsl:with-param name="name" select="'svgHeight'" />
            <xsl:with-param name="node" select="$shape/height" />
          </xsl:call-template>
        </xsl:if>
        <xsl:if test="$shape/scale">
          <xsl:value-of select="concat($indent, 'look.hasScale = true;&#10;')" />
          <xsl:call-template name="look-value">
            <xsl:with-param name="indent" select="$indent" />
            <xsl:with-param name="name" select="'scale'" />
            <xsl:with-param name="node" select="$shape/scale" />
          </xsl:call-template>
        </xsl:if>
        <xsl:if test="$shape/hide">
          <xsl:value-of select="concat($indent, 'look.svgHide = ')" />
          <xsl:call-template name="texts">
            <xsl:with-param name="nodes" select="$shape/hide" />
          </xsl:call-template>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
        <xsl:call-template name="look-text">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="name" select="'flip'" />
          <xsl:with-param name="node" select="$shape/flip" />
        </xsl:call-template>
      </xsl:when>
    </xsl:choose>
    <xsl:if test="$kind != 'image' and $kind != 'svg' and $kind != 'line'">
      <xsl:call-template name="look-text">
        <xsl:with-param name="indent" select="$indent" />
        <xsl:with-param name="name" select="'color'" />
        <xsl:with-param name="node" select="$shape/color" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- The kind, the name and the fill of a <sprite>, as the arguments of
       sprite() and change(): "circle", "name", [](RawSprite& look) { ... },
       the lambda's body at $indent and a tab. -->
  <xsl:template name="sprite-arguments">
    <xsl:param name="node" />
    <xsl:param name="indent" />
    <xsl:param name="change" select="false()" />
    <xsl:text>"</xsl:text>
    <xsl:call-template name="sprite-kind">
      <xsl:with-param name="node" select="$node" />
    </xsl:call-template>
    <xsl:text>", </xsl:text>
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="$node/@name" />
    </xsl:call-template>
    <xsl:value-of select="concat(', [](RawSprite&amp; look)&#10;', $indent, '{&#10;')" />
    <xsl:call-template name="sprite-fill">
      <xsl:with-param name="node" select="$node" />
      <xsl:with-param name="indent" select="concat($indent, $tab)" />
      <xsl:with-param name="change" select="$change" />
    </xsl:call-template>
    <xsl:value-of select="concat($indent, '}')" />
  </xsl:template>

  <!-- The <sprite>s $nodes, as a list, a sprite() each, at $indent. -->
  <xsl:template name="sprites">
    <xsl:param name="nodes" />
    <xsl:param name="indent" />
    <xsl:text>{&#10;</xsl:text>
    <xsl:for-each select="$nodes">
      <xsl:value-of select="concat($indent, $tab, 'sprite(')" />
      <xsl:call-template name="sprite-arguments">
        <xsl:with-param name="node" select="." />
        <xsl:with-param name="indent" select="concat($indent, $tab)" />
      </xsl:call-template>
      <xsl:text>),&#10;</xsl:text>
    </xsl:for-each>
    <xsl:value-of select="concat($indent, '}')" />
  </xsl:template>

  <!-- The names of an <animation>'s frames: { "a", "b" }. -->
  <xsl:template name="frames">
    <xsl:param name="animation" />
    <xsl:text>{ </xsl:text>
    <xsl:for-each select="$animation/frame">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="string">
        <xsl:with-param name="text" select="@sprite" />
      </xsl:call-template>
    </xsl:for-each>
    <xsl:text> }</xsl:text>
  </xsl:template>

  <!-- ===================================================================
       What objects and groups share
       =================================================================== -->

  <xsl:template name="is-true">
    <xsl:param name="node" />
    <xsl:variable name="text" select="normalize-space($node)" />
    <xsl:if test="$text = 'true' or $text = '1'">true</xsl:if>
  </xsl:template>

  <!-- <collisions> (readCollisions), into $to.rawCollisionData, at $indent. -->
  <xsl:template name="collisions">
    <xsl:param name="node" />
    <xsl:param name="to" />
    <xsl:param name="indent" />
    <xsl:variable name="data" select="concat($to, '.rawCollisionData')" />
    <xsl:variable name="enabled">
      <xsl:call-template name="is-true">
        <xsl:with-param name="node" select="$node/enabled" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="lockstep">
      <xsl:call-template name="is-true">
        <xsl:with-param name="node" select="$node/lockstep" />
      </xsl:call-template>
    </xsl:variable>
    <!-- an inner group's <collisions> changes what the outer's set, so it
         says false and box too -->
    <xsl:variable name="nested" select="boolean($node/../parent::group)" />
    <xsl:choose>
      <xsl:when test="$enabled != ''">
        <xsl:value-of select="concat($indent, $data, '.enabled = true;&#10;')" />
      </xsl:when>
      <xsl:when test="$nested">
        <xsl:value-of select="concat($indent, $data, '.enabled = false;&#10;')" />
      </xsl:when>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="$lockstep != ''">
        <xsl:value-of select="concat($indent, $data, '.lockstep = true;&#10;')" />
      </xsl:when>
      <xsl:when test="$nested and $node/lockstep">
        <xsl:value-of select="concat($indent, $data, '.lockstep = false;&#10;')" />
      </xsl:when>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="normalize-space($node/type) = 'pixel'">
        <xsl:value-of select="concat($indent, $data, '.type = CollisionType::Pixel;&#10;')" />
      </xsl:when>
      <xsl:when test="$nested and $node/type">
        <xsl:value-of select="concat($indent, $data, '.type = CollisionType::Box;&#10;')" />
      </xsl:when>
    </xsl:choose>
    <xsl:for-each select="$node/collision">
      <xsl:variable name="commands" select="*[not(self::slower or self::faster)]" />
      <xsl:choose>
        <xsl:when test="@edge != ''">
          <xsl:value-of select="concat($indent, 'edge(', $data, ', &quot;', @edge, '&quot;, ')" />
          <xsl:call-template name="commands">
            <xsl:with-param name="nodes" select="$commands" />
            <xsl:with-param name="indent" select="$indent" />
          </xsl:call-template>
          <xsl:if test="@sprite != '' or @unless != ''">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="string">
              <xsl:with-param name="text" select="@sprite" />
            </xsl:call-template>
            <xsl:if test="@unless != ''">
              <xsl:text>, </xsl:text>
              <xsl:call-template name="string">
                <xsl:with-param name="text" select="@unless" />
              </xsl:call-template>
            </xsl:if>
          </xsl:if>
          <xsl:text>);&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="inner" select="concat($indent, $tab)" />
          <xsl:value-of select="concat($indent, $data, '.basic.push_back(make&lt;RawCollisionRule&gt;([](RawCollisionRule&amp; rule)&#10;', $indent, '{&#10;')" />
          <xsl:for-each select="@class | @object | @unless | @sprite">
            <xsl:if test=". != ''">
              <xsl:value-of select="$inner" />
              <xsl:choose>
                <xsl:when test="local-name() = 'class'">rule.filterClass</xsl:when>
                <xsl:when test="local-name() = 'object'">rule.filterObject</xsl:when>
                <xsl:when test="local-name() = 'unless'">rule.unlessClass</xsl:when>
                <xsl:otherwise>rule.whileSprite</xsl:otherwise>
              </xsl:choose>
              <xsl:text> = </xsl:text>
              <xsl:call-template name="string">
                <xsl:with-param name="text" select="." />
              </xsl:call-template>
              <xsl:text>;&#10;</xsl:text>
            </xsl:if>
          </xsl:for-each>
          <xsl:if test="slower">
            <xsl:value-of select="concat($inner, 'rule.slower = ')" />
            <xsl:call-template name="value">
              <xsl:with-param name="node" select="slower[last()]" />
            </xsl:call-template>
            <xsl:text>;&#10;</xsl:text>
          </xsl:if>
          <xsl:if test="faster">
            <xsl:value-of select="concat($inner, 'rule.faster = ')" />
            <xsl:call-template name="value">
              <xsl:with-param name="node" select="faster[last()]" />
            </xsl:call-template>
            <xsl:text>;&#10;</xsl:text>
          </xsl:if>
          <xsl:value-of select="concat($inner, 'rule.commands = ')" />
          <xsl:call-template name="commands">
            <xsl:with-param name="nodes" select="$commands" />
            <xsl:with-param name="indent" select="$inner" />
          </xsl:call-template>
          <xsl:value-of select="concat(';&#10;', $indent, '}));&#10;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <!-- <timers> (readTimers), each into $list, at $indent. -->
  <xsl:template name="timers">
    <xsl:param name="node" />
    <xsl:param name="list" />
    <xsl:param name="indent" />
    <xsl:for-each select="$node/timer">
      <xsl:value-of select="concat($indent, $list, '.push_back(', local-name(*[1]), '(')" />
      <xsl:call-template name="value">
        <xsl:with-param name="node" select="*[1]" />
      </xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="commands">
        <xsl:with-param name="nodes" select="*[position() &gt; 1]" />
        <xsl:with-param name="indent" select="$indent" />
      </xsl:call-template>
      <xsl:text>));&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- <actions> (readActions), into $to.action, at $indent. -->
  <xsl:template name="actions">
    <xsl:param name="node" />
    <xsl:param name="to" />
    <xsl:param name="indent" />
    <xsl:for-each select="$node/actions/action">
      <xsl:value-of select="concat($indent, $to, '.action[')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text" select="@name" />
      </xsl:call-template>
      <xsl:text>] = </xsl:text>
      <xsl:call-template name="commands">
        <xsl:with-param name="nodes" select="*" />
        <xsl:with-param name="indent" select="$indent" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- <variables> of an object, a group or a part of one, into $to.variable. -->
  <xsl:template name="object-variables">
    <xsl:param name="node" />
    <xsl:param name="to" />
    <xsl:param name="indent" />
    <xsl:for-each select="$node/variables/variable">
      <xsl:value-of select="concat($indent, $to, '.variable[')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text" select="@name" />
      </xsl:call-template>
      <xsl:text>] = </xsl:text>
      <xsl:call-template name="value">
        <xsl:with-param name="node" select="." />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
  </xsl:template>

  <!-- $to.name = value; for $node if there is one. -->
  <xsl:template name="assign-value">
    <xsl:param name="indent" />
    <xsl:param name="to" />
    <xsl:param name="node" />
    <xsl:if test="$node">
      <xsl:value-of select="concat($indent, $to, ' = ')" />
      <xsl:call-template name="value">
        <xsl:with-param name="node" select="$node[1]" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- ===================================================================
       Objects
       =================================================================== -->

  <!-- <object name="..."> (readObject): a function that makes it. -->
  <xsl:template match="object">
    <xsl:variable name="function">
      <xsl:call-template name="function-name">
        <xsl:with-param name="earlier" select="preceding-sibling::object" />
        <xsl:with-param name="suffix" select="'Object'" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="in" select="concat($tab, $tab)" />
    <xsl:text>&#9;// &lt;object name="</xsl:text>
    <xsl:value-of select="@name" />
    <xsl:value-of select="concat('&quot;&gt;&#10;&#9;RawObject ', $function, '()&#10;&#9;{&#10;')" />
    <xsl:value-of select="concat($in, 'RawObject object;&#10;', $in, 'object.name = ')" />
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="@name" />
    </xsl:call-template>
    <xsl:text>;&#10;</xsl:text>
    <xsl:if test="@class != ''">
      <xsl:value-of select="concat($in, 'object.objClass = ')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text" select="@class" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="animation">
        <xsl:value-of select="concat($in, 'animate(object, ')" />
        <xsl:call-template name="sprites">
          <xsl:with-param name="nodes" select="sprite" />
          <xsl:with-param name="indent" select="$in" />
        </xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value">
          <xsl:with-param name="node" select="animation/interval" />
        </xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="frames">
          <xsl:with-param name="animation" select="animation" />
        </xsl:call-template>
        <xsl:text>);&#10;</xsl:text>
      </xsl:when>
      <xsl:when test="count(sprite) = 1">
        <xsl:value-of select="concat($in, 'object.sprite = sprite(')" />
        <xsl:call-template name="sprite-arguments">
          <xsl:with-param name="node" select="sprite" />
          <xsl:with-param name="indent" select="$in" />
        </xsl:call-template>
        <xsl:text>);&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($in, 'looks(object, ')" />
        <xsl:call-template name="sprites">
          <xsl:with-param name="nodes" select="sprite" />
          <xsl:with-param name="indent" select="$in" />
        </xsl:call-template>
        <xsl:text>);&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="concat($in, 'object.rawPosition = ')" />
    <xsl:call-template name="vector">
      <xsl:with-param name="node" select="position" />
    </xsl:call-template>
    <xsl:value-of select="concat(';&#10;', $in, 'object.rawVelocity = ')" />
    <xsl:call-template name="vector">
      <xsl:with-param name="node" select="velocity" />
    </xsl:call-template>
    <xsl:text>;&#10;</xsl:text>
    <xsl:if test="acceleration">
      <xsl:value-of select="concat($in, 'object.hasAcceleration = true;&#10;', $in, 'object.rawAcceleration = ')" />
      <xsl:call-template name="vector">
        <xsl:with-param name="node" select="acceleration" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="heading">
      <xsl:value-of select="concat($in, 'object.hasHeading = true;&#10;')" />
      <xsl:call-template name="assign-value">
        <xsl:with-param name="indent" select="$in" />
        <xsl:with-param name="to" select="'object.rawHeading'" />
        <xsl:with-param name="node" select="heading" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="drag">
      <xsl:value-of select="concat($in, 'object.hasDrag = true;&#10;')" />
      <xsl:call-template name="assign-value">
        <xsl:with-param name="indent" select="$in" />
        <xsl:with-param name="to" select="'object.rawDrag'" />
        <xsl:with-param name="node" select="drag" />
      </xsl:call-template>
    </xsl:if>
    <xsl:call-template name="shared">
      <xsl:with-param name="node" select="." />
      <xsl:with-param name="to" select="'object'" />
      <xsl:with-param name="indent" select="$in" />
    </xsl:call-template>
    <xsl:value-of select="concat($in, 'return object;&#10;&#9;}&#10;&#10;')" />
  </xsl:template>

  <!-- What an <object> and a <group> both have: <facing>, <timers>,
       <hidden>, <collisions>, <actions> and <variables>, into $to. -->
  <xsl:template name="shared">
    <xsl:param name="node" />
    <xsl:param name="to" />
    <xsl:param name="indent" />
    <xsl:if test="$node/facing">
      <xsl:value-of select="concat($indent, $to, '.facing = ')" />
      <xsl:call-template name="text">
        <xsl:with-param name="node" select="$node/facing" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:call-template name="timers">
      <xsl:with-param name="node" select="$node/timers" />
      <xsl:with-param name="list" select="concat($to, '.timers')" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
    <xsl:variable name="hidden">
      <xsl:call-template name="is-true">
        <xsl:with-param name="node" select="$node/hidden[last()]" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$hidden != ''">
        <xsl:value-of select="concat($indent, $to, '.isVisible = false;&#10;')" />
      </xsl:when>
      <!-- an inner group that says it is not hidden, inside one that is -->
      <xsl:when test="$node/parent::group and $node/hidden">
        <xsl:value-of select="concat($indent, $to, '.isVisible = true;&#10;')" />
      </xsl:when>
    </xsl:choose>
    <xsl:call-template name="collisions">
      <xsl:with-param name="node" select="$node/collisions" />
      <xsl:with-param name="to" select="$to" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
    <xsl:call-template name="actions">
      <xsl:with-param name="node" select="$node" />
      <xsl:with-param name="to" select="$to" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
    <xsl:call-template name="object-variables">
      <xsl:with-param name="node" select="$node" />
      <xsl:with-param name="to" select="$to" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
  </xsl:template>

  <!-- ===================================================================
       Groups
       =================================================================== -->

  <!-- <group name="..."> (readGroup): a function that adds its members to
       the game's objects, each made from what the group gives (a Member)
       and changed by its own parts: its <member>, or the <row>s and
       <column>s that pick its cell and the <cell> itself. A group with
       <group>s inside it hands what it gives to a function for each of
       them, which starts from that and changes it; those come first in the
       file, so that the outer group's function can call them. -->
  <xsl:template match="group">
    <xsl:apply-templates select="group" />
    <xsl:variable name="function">
      <xsl:call-template name="group-function" />
    </xsl:variable>
    <xsl:variable name="in" select="concat($tab, $tab)" />
    <xsl:variable name="top" select="boolean(parent::objects)" />
    <xsl:variable name="leaf" select="not(group)" />
    <xsl:variable name="cells" select="boolean(ancestor-or-self::group/columns)" />
    <xsl:text>&#9;// &lt;group name="</xsl:text>
    <xsl:value-of select="@name" />
    <xsl:choose>
      <xsl:when test="$top">
        <xsl:value-of select="concat('&quot;&gt;&#10;&#9;void ', $function, '(std::vector&lt;RawObject&gt;&amp; all)&#10;&#9;{&#10;')" />
        <xsl:value-of select="concat($in, '// what every member has, unless it says otherwise&#10;', $in, 'Member group;&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&quot;&gt;, inside &lt;group name=&quot;', ../@name, '&quot;&gt;&#10;&#9;void ', $function, '(std::vector&lt;RawObject&gt;&amp; all, Member group)&#10;&#9;{&#10;')" />
        <xsl:value-of select="concat($in, '// what the group around it gives, changed by what this one says&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:if test="@class != ''">
      <xsl:value-of select="concat($in, 'group.object.objClass = ')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text" select="@class" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:choose>
      <xsl:when test="$leaf">
        <xsl:value-of select="concat($in, 'group.object.groupName = ')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat($in, 'group.object.outerGroups.push_back(')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="@name" />
    </xsl:call-template>
    <xsl:if test="not($leaf)">)</xsl:if>
    <xsl:text>;&#10;</xsl:text>
    <xsl:choose>
      <xsl:when test="$top and sprite">
        <xsl:value-of select="concat($in, 'group.sprites = ')" />
        <xsl:call-template name="sprites">
          <xsl:with-param name="nodes" select="sprite" />
          <xsl:with-param name="indent" select="$in" />
        </xsl:call-template>
        <xsl:text>;&#10;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <!-- an inner group's sprites change the outer's of their name -->
        <xsl:for-each select="sprite">
          <xsl:value-of select="concat($in, 'change(group, ')" />
          <xsl:call-template name="sprite-arguments">
            <xsl:with-param name="node" select="." />
            <xsl:with-param name="indent" select="$in" />
            <xsl:with-param name="change" select="true()" />
          </xsl:call-template>
          <xsl:text>);&#10;</xsl:text>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:call-template name="part-animation">
      <xsl:with-param name="node" select="animation[last()]" />
      <xsl:with-param name="to" select="'group'" />
      <xsl:with-param name="indent" select="$in" />
    </xsl:call-template>
    <xsl:call-template name="part-vectors">
      <xsl:with-param name="node" select="." />
      <xsl:with-param name="to" select="'group'" />
      <xsl:with-param name="indent" select="$in" />
    </xsl:call-template>
    <!-- a velocity half nobody gives is 0 -->
    <xsl:if test="$top and not(velocity/x)">
      <xsl:value-of select="concat($in, 'group.object.rawVelocity.x = value(&quot;0&quot;);&#10;')" />
    </xsl:if>
    <xsl:if test="$top and not(velocity/y)">
      <xsl:value-of select="concat($in, 'group.object.rawVelocity.y = value(&quot;0&quot;);&#10;')" />
    </xsl:if>
    <xsl:if test="acceleration">
      <xsl:value-of select="concat($in, 'group.object.hasAcceleration = true;&#10;', $in, 'group.object.rawAcceleration = ')" />
      <xsl:call-template name="vector">
        <xsl:with-param name="node" select="acceleration[last()]" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
    <xsl:call-template name="shared">
      <xsl:with-param name="node" select="." />
      <xsl:with-param name="to" select="'group.object'" />
      <xsl:with-param name="indent" select="$in" />
    </xsl:call-template>
    <!-- the gaps a cell takes from the innermost group that gives them -->
    <xsl:if test="$cells and $leaf">
      <xsl:value-of select="concat($in, 'group.object.cell.gapBefore = ')" />
      <xsl:choose>
        <xsl:when test="ancestor-or-self::group/padding/x">
          <xsl:call-template name="value">
            <xsl:with-param name="node" select="(ancestor-or-self::group/padding/x)[last()]" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>value("0")</xsl:otherwise>
      </xsl:choose>
      <xsl:value-of select="concat(';&#10;', $in, 'group.object.cell.gapAbove = ')" />
      <xsl:choose>
        <xsl:when test="ancestor-or-self::group/padding/y">
          <xsl:call-template name="value">
            <xsl:with-param name="node" select="(ancestor-or-self::group/padding/y)[last()]" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>value("0")</xsl:otherwise>
      </xsl:choose>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>

    <!-- the parts that change something, a function each -->
    <xsl:for-each select="member | row | column | cell">
      <xsl:if test="*">
        <xsl:variable name="part">
          <xsl:call-template name="part-name" />
        </xsl:variable>
        <xsl:value-of select="concat('&#10;', $in, '// &lt;', local-name())" />
        <xsl:for-each select="@*">
          <xsl:value-of select="concat(' ', local-name(), '=&quot;', ., '&quot;')" />
        </xsl:for-each>
        <xsl:value-of select="concat('&gt;&#10;', $in, 'const Part ', $part, ' = [](Member&amp; member)&#10;', $in, '{&#10;')" />
        <xsl:call-template name="part-body">
          <xsl:with-param name="indent" select="concat($in, $tab)" />
        </xsl:call-template>
        <xsl:value-of select="concat($in, '};&#10;')" />
      </xsl:if>
    </xsl:for-each>
    <xsl:text>&#10;</xsl:text>

    <xsl:choose>
      <xsl:when test="not($leaf)">
        <xsl:for-each select="group">
          <xsl:value-of select="$in" />
          <xsl:call-template name="group-function" />
          <xsl:text>(all, group);&#10;</xsl:text>
        </xsl:for-each>
      </xsl:when>
      <xsl:when test="$cells">
        <xsl:call-template name="cells">
          <xsl:with-param name="group" select="." />
          <xsl:with-param name="columns" select="number(normalize-space((ancestor-or-self::group/columns)[last()]))" />
          <xsl:with-param name="rows" select="number(normalize-space((ancestor-or-self::group/rows)[last()]))" />
          <xsl:with-param name="row" select="1" />
          <xsl:with-param name="indent" select="$in" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="member">
          <xsl:value-of select="concat($in, 'addMember(all, group, ')" />
          <xsl:call-template name="string">
            <xsl:with-param name="text">
              <xsl:choose>
                <xsl:when test="@name != ''"><xsl:value-of select="@name" /></xsl:when>
                <xsl:otherwise><xsl:value-of select="concat(../@name, '.', position())" /></xsl:otherwise>
              </xsl:choose>
            </xsl:with-param>
          </xsl:call-template>
          <xsl:text>, {</xsl:text>
          <xsl:if test="*">
            <xsl:text> </xsl:text>
            <xsl:call-template name="part-name" />
            <xsl:text> </xsl:text>
          </xsl:if>
          <xsl:text>});&#10;</xsl:text>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&#9;}&#10;&#10;</xsl:text>
  </xsl:template>

  <!-- The function a <group> is written as: its name in camel case and
       Group, told apart from every group before it or around it. -->
  <xsl:template name="group-function">
    <xsl:call-template name="function-name">
      <xsl:with-param name="earlier" select="preceding::group | ancestor::group" />
      <xsl:with-param name="suffix" select="'Group'" />
    </xsl:call-template>
  </xsl:template>

  <!-- What a part of a group is called in the C++: member3, row1, column2, cell1. -->
  <xsl:template name="part-name">
    <xsl:value-of select="concat(local-name(), count(preceding-sibling::*[local-name() = local-name(current())]) + 1)" />
  </xsl:template>

  <!-- An <animation>, into $to's. -->
  <xsl:template name="part-animation">
    <xsl:param name="node" />
    <xsl:param name="to" />
    <xsl:param name="indent" />
    <xsl:if test="$node">
      <xsl:value-of select="concat($indent, $to, '.animated = true;&#10;', $indent, $to, '.interval = ')" />
      <xsl:call-template name="value">
        <xsl:with-param name="node" select="$node/interval" />
      </xsl:call-template>
      <xsl:value-of select="concat(';&#10;', $indent, $to, '.frames = ')" />
      <xsl:call-template name="frames">
        <xsl:with-param name="animation" select="$node" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The halves of a <position> and a <velocity> that $node gives (a group
       gives what its members share, a part only what differs), and of a
       <padding> the gap before (x) and above (y) the cells it picks. -->
  <xsl:template name="part-vectors">
    <xsl:param name="node" />
    <xsl:param name="to" />
    <xsl:param name="indent" />
    <xsl:for-each select="$node/position | $node/velocity | $node/padding[$to != 'group']">
      <xsl:variable name="field">
        <xsl:choose>
          <xsl:when test="self::position">rawPosition</xsl:when>
          <xsl:when test="self::velocity">rawVelocity</xsl:when>
          <xsl:otherwise>cell</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:for-each select="x | y">
        <xsl:variable name="half">
          <xsl:choose>
            <xsl:when test="$field != 'cell'"><xsl:value-of select="local-name()" /></xsl:when>
            <xsl:when test="self::x">gapBefore</xsl:when>
            <xsl:otherwise>gapAbove</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:call-template name="assign-value">
          <xsl:with-param name="indent" select="$indent" />
          <xsl:with-param name="to" select="concat($to, '.object.', $field, '.', $half)" />
          <xsl:with-param name="node" select="." />
        </xsl:call-template>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:template>

  <!-- What a <member>, <row>, <column> or <cell> changes, in the order written. -->
  <xsl:template name="part-body">
    <xsl:param name="indent" />
    <xsl:for-each select="sprite">
      <xsl:value-of select="concat($indent, 'change(member, ')" />
      <xsl:call-template name="sprite-arguments">
        <xsl:with-param name="node" select="." />
        <xsl:with-param name="indent" select="$indent" />
        <xsl:with-param name="change" select="true()" />
      </xsl:call-template>
      <xsl:text>);&#10;</xsl:text>
    </xsl:for-each>
    <xsl:call-template name="part-animation">
      <xsl:with-param name="node" select="animation[last()]" />
      <xsl:with-param name="to" select="'member'" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
    <xsl:call-template name="part-vectors">
      <xsl:with-param name="node" select="." />
      <xsl:with-param name="to" select="'member'" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
    <xsl:call-template name="object-variables">
      <xsl:with-param name="node" select="." />
      <xsl:with-param name="to" select="'member.object'" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
  </xsl:template>

  <!-- Whether a <row number="..."> or <column number="..."> picks $n:
       "yes" if one of its words is $n, or odd or even and $n is. -->
  <xsl:template name="picks">
    <xsl:param name="words" />
    <xsl:param name="n" />
    <xsl:if test="$words != ''">
      <xsl:variable name="word">
        <xsl:choose>
          <xsl:when test="contains($words, ' ')"><xsl:value-of select="substring-before($words, ' ')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$words" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="($word = 'odd' and $n mod 2 = 1) or ($word = 'even' and $n mod 2 = 0) or number($word) = $n">yes</xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="picks">
            <xsl:with-param name="words" select="substring-after($words, ' ')" />
            <xsl:with-param name="n" select="$n" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- The parts of $group that change the cell at $column, $row, "a, b, ":
       the rows (and unless $rowsOnly, the columns) that pick it, in the
       order written, then its <cell>. -->
  <xsl:template name="cell-parts">
    <xsl:param name="group" />
    <xsl:param name="column" />
    <xsl:param name="row" />
    <xsl:param name="rowsOnly" select="false()" />
    <xsl:for-each select="$group/row | $group/column[not($rowsOnly)]">
      <xsl:variable name="picked">
        <xsl:call-template name="picks">
          <xsl:with-param name="words" select="normalize-space(@number)" />
          <xsl:with-param name="n" select="$row * boolean(self::row) + $column * boolean(self::column)" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:if test="$picked = 'yes' and *">
        <xsl:call-template name="part-name" />
        <xsl:text>, </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:if test="not($rowsOnly)">
      <xsl:for-each select="$group/cell[number(@row) = $row and number(@column) = $column][1][*]">
        <xsl:call-template name="part-name" />
        <xsl:text>, </xsl:text>
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <!-- "a, b, " as a list: { a, b }. -->
  <xsl:template name="part-list">
    <xsl:param name="names" />
    <xsl:choose>
      <xsl:when test="$names = ''">{}</xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('{ ', substring($names, 1, string-length($names) - 2), ' }')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Every cell of a group laid out in <columns> and <rows>, from $row on:
       top row first, left to right. -->
  <xsl:template name="cells">
    <xsl:param name="group" />
    <xsl:param name="columns" />
    <xsl:param name="rows" />
    <xsl:param name="row" />
    <xsl:param name="column" select="1" />
    <xsl:param name="indent" />
    <xsl:if test="$row &lt;= $rows">
      <xsl:variable name="cell" select="$group/cell[number(@row) = $row and number(@column) = $column][1]" />
      <xsl:value-of select="concat($indent, 'addCell(all, group, ')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text">
          <xsl:choose>
            <xsl:when test="$cell/@name != ''"><xsl:value-of select="$cell/@name" /></xsl:when>
            <xsl:otherwise><xsl:value-of select="concat($group/@name, '.', $column, '.', $row)" /></xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:variable name="parts">
        <xsl:call-template name="cell-parts">
          <xsl:with-param name="group" select="$group" />
          <xsl:with-param name="column" select="$column" />
          <xsl:with-param name="row" select="$row" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="rowParts">
        <xsl:call-template name="cell-parts">
          <xsl:with-param name="group" select="$group" />
          <xsl:with-param name="column" select="$column" />
          <xsl:with-param name="row" select="$row" />
          <xsl:with-param name="rowsOnly" select="true()" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:call-template name="part-list">
        <xsl:with-param name="names" select="$parts" />
      </xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="part-list">
        <xsl:with-param name="names" select="$rowParts" />
      </xsl:call-template>
      <xsl:value-of select="concat(', ', $column, ', ', $row, ');&#10;')" />
      <xsl:choose>
        <xsl:when test="$column &lt; $columns">
          <xsl:call-template name="cells">
            <xsl:with-param name="group" select="$group" />
            <xsl:with-param name="columns" select="$columns" />
            <xsl:with-param name="rows" select="$rows" />
            <xsl:with-param name="row" select="$row" />
            <xsl:with-param name="column" select="$column + 1" />
            <xsl:with-param name="indent" select="$indent" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="cells">
            <xsl:with-param name="group" select="$group" />
            <xsl:with-param name="columns" select="$columns" />
            <xsl:with-param name="rows" select="$rows" />
            <xsl:with-param name="row" select="$row + 1" />
            <xsl:with-param name="indent" select="$indent" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>
  <!-- ===================================================================
       States
       =================================================================== -->

  <!-- The keys of an <input button="w up">, each given its commands:
       to["w"] = to["up"] = { ... }; at $indent. -->
  <xsl:template name="input">
    <xsl:param name="to" />
    <xsl:param name="indent" />
    <xsl:value-of select="$indent" />
    <xsl:call-template name="buttons">
      <xsl:with-param name="words" select="normalize-space(@button)" />
      <xsl:with-param name="to" select="$to" />
    </xsl:call-template>
    <xsl:call-template name="commands">
      <xsl:with-param name="nodes" select="*" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:call-template>
    <xsl:text>;&#10;</xsl:text>
  </xsl:template>

  <xsl:template name="buttons">
    <xsl:param name="words" />
    <xsl:param name="to" />
    <xsl:if test="$words != ''">
      <xsl:value-of select="concat($to, '[')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text">
          <xsl:choose>
            <xsl:when test="contains($words, ' ')"><xsl:value-of select="substring-before($words, ' ')" /></xsl:when>
            <xsl:otherwise><xsl:value-of select="$words" /></xsl:otherwise>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
      <xsl:text>] = </xsl:text>
      <xsl:call-template name="buttons">
        <xsl:with-param name="words" select="substring-after($words, ' ')" />
        <xsl:with-param name="to" select="$to" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- <keys name="..."> in <states>: a function that gives its keys. -->
  <xsl:template match="keys">
    <xsl:variable name="in" select="concat($tab, $tab)" />
    <xsl:text>&#9;// &lt;keys name="</xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>"&gt;&#10;&#9;Keys </xsl:text>
    <xsl:call-template name="function-name">
      <xsl:with-param name="earlier" select="preceding-sibling::keys" />
      <xsl:with-param name="suffix" select="'Keys'" />
    </xsl:call-template>
    <xsl:value-of select="concat('()&#10;&#9;{&#10;', $in, 'Keys keys;&#10;')" />
    <xsl:for-each select="input">
      <xsl:call-template name="input">
        <xsl:with-param name="to" select="'keys'" />
        <xsl:with-param name="indent" select="$in" />
      </xsl:call-template>
    </xsl:for-each>
    <xsl:value-of select="concat($in, 'return keys;&#10;&#9;}&#10;&#10;')" />
  </xsl:template>

  <!-- A <show> (readShows): its object's name, or a <shows> set's names in
       its place, each followed by ", ". -->
  <xsl:template match="show" mode="shown">
    <xsl:choose>
      <xsl:when test="@set">
        <xsl:variable name="set" select="@set" />
        <xsl:apply-templates select="ancestor::states/shows[@name = $set][1]/show" mode="shown" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="string">
          <xsl:with-param name="text" select="@object" />
        </xsl:call-template>
        <xsl:text>, </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A state's <condition> (readConditions): one pushed onto the state's
       list, or a <conditions> set's in its place, in order. -->
  <xsl:template match="condition" mode="condition">
    <xsl:param name="in" />
    <xsl:param name="inner" />
    <xsl:choose>
      <xsl:when test="@set">
        <xsl:variable name="set" select="@set" />
        <xsl:apply-templates select="ancestor::states/conditions[@name = $set][1]/condition" mode="condition">
          <xsl:with-param name="in" select="$in" />
          <xsl:with-param name="inner" select="$inner" />
        </xsl:apply-templates>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="test" select="*[1]" />
        <xsl:value-of select="concat($in, 'state.conditions.push_back(make&lt;RawCondition&gt;([](RawCondition&amp; condition)&#10;', $in, '{&#10;')" />
        <xsl:for-each select="@class | @object | @variable">
          <xsl:if test=". != ''">
            <xsl:value-of select="$inner" />
            <xsl:choose>
              <xsl:when test="local-name() = 'class'">condition.filterClass</xsl:when>
              <xsl:when test="local-name() = 'object'">condition.filterObject</xsl:when>
              <xsl:otherwise>condition.variableName</xsl:otherwise>
            </xsl:choose>
            <xsl:text> = </xsl:text>
            <xsl:call-template name="string">
              <xsl:with-param name="text" select="." />
            </xsl:call-template>
            <xsl:text>;&#10;</xsl:text>
          </xsl:if>
        </xsl:for-each>
        <xsl:if test="not($test[self::atleast])">
          <xsl:value-of select="concat($inner, 'condition.test = RawCondition::Test::')" />
          <xsl:choose>
            <xsl:when test="$test[self::atmost]">AtMost</xsl:when>
            <xsl:otherwise>Remaining</xsl:otherwise>
          </xsl:choose>
          <xsl:text>;&#10;</xsl:text>
        </xsl:if>
        <xsl:call-template name="assign-value">
          <xsl:with-param name="indent" select="$inner" />
          <xsl:with-param name="to" select="'condition.threshold'" />
          <xsl:with-param name="node" select="$test" />
        </xsl:call-template>
        <xsl:value-of select="concat($inner, 'condition.commands = ')" />
        <xsl:call-template name="commands">
          <xsl:with-param name="nodes" select="*[position() &gt; 1]" />
          <xsl:with-param name="indent" select="$inner" />
        </xsl:call-template>
        <xsl:value-of select="concat(';&#10;', $in, '}));&#10;')" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- <state name="..."> (readState): a function that makes it. -->
  <xsl:template match="state">
    <xsl:variable name="in" select="concat($tab, $tab)" />
    <xsl:variable name="inner" select="concat($in, $tab)" />
    <xsl:text>&#9;// &lt;state name="</xsl:text>
    <xsl:value-of select="@name" />
    <xsl:text>"&gt;&#10;&#9;RawState </xsl:text>
    <xsl:call-template name="function-name">
      <xsl:with-param name="earlier" select="preceding-sibling::state" />
      <xsl:with-param name="suffix" select="'State'" />
    </xsl:call-template>
    <xsl:value-of select="concat('()&#10;&#9;{&#10;', $in, 'RawState state;&#10;', $in, 'state.name = ')" />
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="@name" />
    </xsl:call-template>
    <xsl:value-of select="concat(';&#10;', $in, 'state.show = { ')" />
    <xsl:variable name="shown">
      <xsl:apply-templates select="shows/show" mode="shown" />
    </xsl:variable>
    <xsl:value-of select="substring($shown, 1, string-length($shown) - 2)" />
    <xsl:text> };&#10;</xsl:text>
    <xsl:call-template name="use-keys">
      <xsl:with-param name="words" select="normalize-space(inputs/@keys)" />
      <xsl:with-param name="indent" select="$in" />
    </xsl:call-template>
    <xsl:for-each select="inputs/input">
      <xsl:call-template name="input">
        <xsl:with-param name="to" select="'state.input'" />
        <xsl:with-param name="indent" select="$in" />
      </xsl:call-template>
    </xsl:for-each>
    <xsl:apply-templates select="conditions/condition" mode="condition">
      <xsl:with-param name="in" select="$in" />
      <xsl:with-param name="inner" select="$inner" />
    </xsl:apply-templates>
    <xsl:call-template name="timers">
      <xsl:with-param name="node" select="timers" />
      <xsl:with-param name="list" select="'state.timers'" />
      <xsl:with-param name="indent" select="$in" />
    </xsl:call-template>
    <xsl:value-of select="concat($in, 'return state;&#10;&#9;}&#10;&#10;')" />
  </xsl:template>

  <!-- The <keys> sets a state's <inputs keys="..."> names, each over the one before. -->
  <xsl:template name="use-keys">
    <xsl:param name="words" />
    <xsl:param name="indent" />
    <xsl:if test="$words != ''">
      <xsl:variable name="word">
        <xsl:choose>
          <xsl:when test="contains($words, ' ')"><xsl:value-of select="substring-before($words, ' ')" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="$words" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:for-each select="ancestor::states/keys[@name = $word][1]">
        <xsl:value-of select="concat($indent, 'use(state.input, ')" />
        <xsl:call-template name="function-name">
          <xsl:with-param name="earlier" select="preceding-sibling::keys" />
          <xsl:with-param name="suffix" select="'Keys'" />
        </xsl:call-template>
        <xsl:text>());&#10;</xsl:text>
      </xsl:for-each>
      <xsl:call-template name="use-keys">
        <xsl:with-param name="words" select="substring-after($words, ' ')" />
        <xsl:with-param name="indent" select="$indent" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- ===================================================================
       Sounds and paths
       =================================================================== -->

  <xsl:template name="sounds">
    <xsl:variable name="in" select="concat($tab, $tab)" />
    <xsl:variable name="inner" select="concat($in, $tab)" />
    <xsl:value-of select="concat('&#9;// &lt;sounds&gt;&#10;&#9;std::vector&lt;RawSound&gt; sounds()&#10;&#9;{&#10;', $in, 'std::vector&lt;RawSound&gt; all;&#10;')" />
    <xsl:for-each select="/game/sounds/sound">
      <xsl:value-of select="concat($in, 'all.push_back(make&lt;RawSound&gt;([](RawSound&amp; sound)&#10;', $in, '{&#10;', $inner, 'sound.name = ')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text" select="@name" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
      <xsl:if test="@wave != ''">
        <xsl:value-of select="concat($inner, 'sound.wave = ')" />
        <xsl:call-template name="string">
          <xsl:with-param name="text" select="@wave" />
        </xsl:call-template>
        <xsl:text>;&#10;</xsl:text>
      </xsl:if>
      <xsl:call-template name="assign-value">
        <xsl:with-param name="indent" select="$inner" />
        <xsl:with-param name="to" select="'sound.volume'" />
        <xsl:with-param name="node" select="volume[last()]" />
      </xsl:call-template>
      <xsl:value-of select="concat($inner, 'sound.notes = {&#10;')" />
      <xsl:for-each select="note | rest">
        <xsl:value-of select="concat($inner, $tab)" />
        <xsl:choose>
          <xsl:when test="self::rest">
            <xsl:text>rest(</xsl:text>
            <xsl:call-template name="value">
              <xsl:with-param name="node" select="." />
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>note(</xsl:text>
            <xsl:call-template name="string">
              <xsl:with-param name="text" select="@pitch" />
            </xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="value">
              <xsl:with-param name="node" select="." />
            </xsl:call-template>
            <xsl:if test="@wave != '' or @to != ''">
              <xsl:text>, </xsl:text>
              <xsl:call-template name="string">
                <xsl:with-param name="text" select="@wave" />
              </xsl:call-template>
              <xsl:if test="@to != ''">
                <xsl:text>, </xsl:text>
                <xsl:call-template name="string">
                  <xsl:with-param name="text" select="@to" />
                </xsl:call-template>
              </xsl:if>
            </xsl:if>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>),&#10;</xsl:text>
      </xsl:for-each>
      <xsl:value-of select="concat($inner, '};&#10;', $in, '}));&#10;')" />
    </xsl:for-each>
    <xsl:value-of select="concat($in, 'return all;&#10;&#9;}&#10;&#10;')" />
  </xsl:template>

  <xsl:template name="paths">
    <xsl:variable name="in" select="concat($tab, $tab)" />
    <xsl:variable name="inner" select="concat($in, $tab)" />
    <xsl:value-of select="concat('&#9;// &lt;paths&gt;&#10;&#9;std::vector&lt;RawPath&gt; paths()&#10;&#9;{&#10;', $in, 'std::vector&lt;RawPath&gt; all;&#10;')" />
    <xsl:for-each select="/game/paths/path">
      <xsl:value-of select="concat($in, 'all.push_back(make&lt;RawPath&gt;([](RawPath&amp; path)&#10;', $in, '{&#10;', $inner, 'path.name = ')" />
      <xsl:call-template name="string">
        <xsl:with-param name="text" select="@name" />
      </xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
      <xsl:call-template name="assign-value">
        <xsl:with-param name="indent" select="$inner" />
        <xsl:with-param name="to" select="'path.speed'" />
        <xsl:with-param name="node" select="speed[last()]" />
      </xsl:call-template>
      <xsl:if test="start">
        <xsl:value-of select="concat($inner, 'path.hasStart = true;&#10;')" />
        <xsl:call-template name="assign-value">
          <xsl:with-param name="indent" select="$inner" />
          <xsl:with-param name="to" select="'path.startX'" />
          <xsl:with-param name="node" select="start[last()]/x" />
        </xsl:call-template>
        <xsl:call-template name="assign-value">
          <xsl:with-param name="indent" select="$inner" />
          <xsl:with-param name="to" select="'path.startY'" />
          <xsl:with-param name="node" select="start[last()]/y" />
        </xsl:call-template>
      </xsl:if>
      <xsl:value-of select="concat($inner, 'path.steps = {&#10;')" />
      <xsl:for-each select="step | home">
        <xsl:value-of select="concat($inner, $tab)" />
        <xsl:choose>
          <xsl:when test="self::home">home()</xsl:when>
          <xsl:otherwise>
            <xsl:text>step(</xsl:text>
            <xsl:call-template name="value-of">
              <xsl:with-param name="node" select="." />
              <xsl:with-param name="name" select="'x'" />
            </xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="value-of">
              <xsl:with-param name="node" select="." />
              <xsl:with-param name="name" select="'y'" />
            </xsl:call-template>
            <xsl:if test="*[not(self::x or self::y)]">
              <xsl:text>, </xsl:text>
              <xsl:call-template name="commands">
                <xsl:with-param name="nodes" select="*[not(self::x or self::y)]" />
                <xsl:with-param name="indent" select="concat($inner, $tab)" />
              </xsl:call-template>
            </xsl:if>
            <xsl:text>)</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:text>,&#10;</xsl:text>
      </xsl:for-each>
      <xsl:value-of select="concat($inner, '};&#10;', $in, '}));&#10;')" />
    </xsl:for-each>
    <xsl:value-of select="concat($in, 'return all;&#10;&#9;}&#10;&#10;')" />
  </xsl:template>

  <!-- ===================================================================
       The file
       =================================================================== -->

  <!-- A window's number as a C++ literal: 640 is 640.0f. -->
  <xsl:template name="float">
    <xsl:param name="node" />
    <xsl:variable name="text" select="normalize-space($node)" />
    <xsl:value-of select="$text" />
    <xsl:if test="not(contains($text, '.') or contains($text, 'e') or contains($text, 'E'))">.0</xsl:if>
    <xsl:text>f</xsl:text>
  </xsl:template>

  <!-- source/<game>.cpp: the game, and $function, which returns it. -->
  <xsl:template name="data-file">
    <xsl:variable name="game" select="/game" />
    <xsl:call-template name="header">
      <xsl:with-param name="file" select="concat($name, '.cpp')" />
    </xsl:call-template>
    <xsl:text>//&#10;// </xsl:text>
    <xsl:value-of select="$title" />
    <xsl:text>, as written in </xsl:text>
    <xsl:value-of select="$source" />
    <xsl:text>, written out by xgecli --generate&#10;// windows-cpp (games/generators/windows-cpp/data.xsl) as the engine's own data&#10;// (game_data.h): </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>() is the game file, with no XML left to read, each&#10;// tag the line that makes what the engine makes of it (game_parts.h). Change&#10;// the game file and generate it again rather than this.&#10;&#10;</xsl:text>
    <xsl:text>#include "game_data.h"&#10;#include "game_parts.h"&#10;&#10;#include &lt;vector&gt;&#10;&#10;namespace&#10;{&#10;&#9;using namespace xge;&#10;&#9;using namespace xge::parts;&#10;&#10;</xsl:text>
    <!-- the functions, each followed by a blank line but the last -->
    <xsl:variable name="functions">
      <xsl:if test="$game/sounds/sound">
        <xsl:call-template name="sounds" />
      </xsl:if>
      <xsl:if test="$game/paths/path">
        <xsl:call-template name="paths" />
      </xsl:if>
      <xsl:apply-templates select="$game/objects/object | $game/objects/group" />
      <xsl:text>&#9;// Every object, and every member of a group, in the order of the file.&#10;&#9;std::vector&lt;RawObject&gt; objects()&#10;&#9;{&#10;&#9;&#9;std::vector&lt;RawObject&gt; all;&#10;</xsl:text>
      <xsl:for-each select="$game/objects/*">
        <xsl:text>&#9;&#9;</xsl:text>
        <xsl:choose>
          <xsl:when test="self::group">
            <xsl:call-template name="group-function" />
            <xsl:text>(all);&#10;</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>all.push_back(</xsl:text>
            <xsl:call-template name="function-name">
              <xsl:with-param name="earlier" select="preceding-sibling::object" />
              <xsl:with-param name="suffix" select="'Object'" />
            </xsl:call-template>
            <xsl:text>());&#10;</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:text>&#9;&#9;return all;&#10;&#9;}&#10;&#10;</xsl:text>
      <xsl:apply-templates select="$game/states/keys" />
      <xsl:apply-templates select="$game/states/state" />
    </xsl:variable>
    <xsl:value-of select="substring($functions, 1, string-length($functions) - 1)" />
    <xsl:text>}&#10;&#10;xge::GameData </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>()&#10;{&#10;&#9;xge::GameData game;&#10;&#9;game.window.name = </xsl:text>
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="$game/window/@name" />
    </xsl:call-template>
    <xsl:text>;&#10;&#9;game.window.width = </xsl:text>
    <xsl:call-template name="float">
      <xsl:with-param name="node" select="$game/window/width" />
    </xsl:call-template>
    <xsl:text>;&#10;&#9;game.window.height = </xsl:text>
    <xsl:call-template name="float">
      <xsl:with-param name="node" select="$game/window/height" />
    </xsl:call-template>
    <xsl:text>;&#10;&#9;game.window.background = </xsl:text>
    <xsl:call-template name="text">
      <xsl:with-param name="node" select="$game/window/background" />
    </xsl:call-template>
    <xsl:text>;&#10;&#9;game.window.fullscreen = "</xsl:text>
    <xsl:choose>
      <xsl:when test="normalize-space($game/window/fullscreen) = 'true' or normalize-space($game/window/fullscreen) = '1'">true</xsl:when>
      <xsl:otherwise>false</xsl:otherwise>
    </xsl:choose>
    <xsl:text>";&#10;&#9;game.window.framerate = </xsl:text>
    <xsl:value-of select="normalize-space($game/window/framerate)" />
    <xsl:text>;&#10;</xsl:text>
    <!-- a variable declared twice keeps the later value, in the earlier place -->
    <xsl:variable name="variables" select="$game/variables/variable[not(@name = preceding-sibling::variable/@name)]" />
    <xsl:if test="$variables">
      <xsl:text>&#9;game.variables = {&#10;</xsl:text>
      <xsl:for-each select="$variables">
        <xsl:variable name="named" select="@name" />
        <xsl:text>&#9;&#9;{ </xsl:text>
        <xsl:call-template name="string">
          <xsl:with-param name="text" select="@name" />
        </xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value">
          <xsl:with-param name="node" select="../variable[@name = $named][last()]" />
        </xsl:call-template>
        <xsl:text> },&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>&#9;};&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$game/sounds/sound">
      <xsl:text>&#9;game.sounds = sounds();&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$game/paths/path">
      <xsl:text>&#9;game.paths = paths();&#10;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;game.objects = objects();&#10;&#9;game.states = {</xsl:text>
    <xsl:for-each select="$game/states/state">
      <xsl:if test="position() &gt; 1">,</xsl:if>
      <xsl:text> </xsl:text>
      <xsl:call-template name="function-name">
        <xsl:with-param name="earlier" select="preceding-sibling::state" />
        <xsl:with-param name="suffix" select="'State'" />
      </xsl:call-template>
      <xsl:text>()</xsl:text>
    </xsl:for-each>
    <xsl:text> };&#10;&#9;game.validation = </xsl:text>
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="concat('none (written into the program from ', $source, ')')" />
    </xsl:call-template>
    <xsl:text>;&#10;&#9;return game;&#10;}&#10;</xsl:text>
  </xsl:template>
</xsl:stylesheet>
