<?xml version="1.0" encoding="UTF-8"?>
<!-- game.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- The game's own C++, in namespace game: its window, values, objects, sounds,
     states, and every rule written out as plain statements. Each verb becomes
     what the engine does for it (lib/source/command_executor.cpp), with the names
     of the game's objects in place of a search by name. A tag this target cannot
     generate yet stops the generator with a message naming it. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="tables" select="document('tables.xml')/tables" />

  <xsl:variable name="objects" select="/game/objects/object" />
  <xsl:variable name="states" select="/game/states/state" />
  <xsl:variable name="edges" select="'top bottom left right'" />

  <!-- ===================================================================== -->
  <!-- What this target can generate                                          -->
  <!-- ===================================================================== -->

  <xsl:variable name="supported" select="concat(
    ' game window width height background fullscreen framerate variables variable',
    ' sounds sound note rest volume',
    ' objects object sprite text content number size color circle radius rectangle image path flip',
    ' position x y velocity hidden collisions enabled type collision',
    ' bounce stick reset die stop reverse move inc dec play deflect',
    ' actions action states state shows show inputs input push pop trigger',
    ' conditions condition atleast atmost remaining',
    ' random equation formula add subtract multiply divide',
    ' augend addend minuend subtrahend multiplicand multiplier dividend divisor ')" />

  <xsl:template match="*" mode="check">
    <xsl:if test="not(contains($supported, concat(' ', local-name(), ' ')))">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt;')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="self::collision and (@unless or @sprite)">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="'a &lt;collision&gt; with unless= or sprite='" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="self::type and normalize-space(.) != 'box'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="'a pixel &lt;type&gt;'" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="self::inputs and @keys">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="'&lt;inputs keys&gt; (key sets)'" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="self::show and not($objects[@name = current()/@object])">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;show object=&quot;', @object, '&quot;&gt;, which is not an &lt;object&gt;')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:apply-templates select="*" mode="check" />
  </xsl:template>

  <!-- Stops the generator: says what cannot be generated, and where. -->
  <xsl:template name="refuse">
    <xsl:param name="what" />
    <xsl:message terminate="yes">
      <xsl:text>windows-cpp-full cannot generate </xsl:text>
      <xsl:value-of select="$what" />
      <xsl:text> yet (in </xsl:text>
      <xsl:for-each select="ancestor-or-self::*">
        <xsl:if test="position() &gt; 1"> &gt; </xsl:if>
        <xsl:value-of select="local-name()" />
        <xsl:if test="@name">
          <xsl:value-of select="concat(' ', @name)" />
        </xsl:if>
      </xsl:for-each>
      <xsl:text>)</xsl:text>
    </xsl:message>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Small helpers                                                          -->
  <!-- ===================================================================== -->

  <xsl:template name="object-name">
    <xsl:param name="name" select="@name" />
    <xsl:call-template name="cpp-name">
      <xsl:with-param name="prefix" select="'o_'" />
      <xsl:with-param name="name" select="$name" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="state-name">
    <xsl:param name="name" select="@name" />
    <xsl:call-template name="cpp-name">
      <xsl:with-param name="prefix" select="'s_'" />
      <xsl:with-param name="name" select="$name" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="sound-name">
    <xsl:param name="name" select="@name" />
    <xsl:call-template name="cpp-name">
      <xsl:with-param name="prefix" select="'snd_'" />
      <xsl:with-param name="name" select="$name" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="color">
    <xsl:param name="name" />
    <xsl:variable name="color" select="$tables/colors/color[@name = normalize-space($name)]" />
    <xsl:if test="not($color)">
      <xsl:message terminate="yes">windows-cpp-full: there is no color named "<xsl:value-of select="$name" />"</xsl:message>
    </xsl:if>
    <xsl:value-of select="concat('sf::Color(', $color/@rgba, ')')" />
  </xsl:template>

  <xsl:template name="edge-name">
    <xsl:param name="edge" />
    <xsl:value-of select="concat('Edge::', translate(substring($edge, 1, 1), 'tblr', 'TBLR'), substring($edge, 2))" />
  </xsl:template>

  <xsl:template name="direction-name">
    <xsl:param name="direction" />
    <xsl:variable name="d" select="normalize-space($direction)" />
    <xsl:value-of select="concat('Direction::', translate(substring($d, 1, 1), 'udlr', 'UDLR'), substring($d, 2))" />
  </xsl:template>

  <!-- A variable a command names (owner.variable), as its C++ name; the owner
       must be an object that has it. -->
  <xsl:template name="object-variable">
    <xsl:param name="name" select="@variable" />
    <xsl:variable name="owner" select="substring-before($name, '.')" />
    <xsl:variable name="variable" select="substring-after($name, '.')" />
    <xsl:if test="not($objects[@name = $owner]/variables/variable[@name = $variable])">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('variable=&quot;', $name, '&quot;, which is not a variable of an object,')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:call-template name="cpp-name">
      <xsl:with-param name="prefix" select="'v_'" />
      <xsl:with-param name="name" select="$name" />
    </xsl:call-template>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Commands                                                               -->
  <!-- ===================================================================== -->

  <!-- One command as C++ statements. $self is the object running it (its C++
       name), $other the one it touched, $edge the C++ edge it touched, and
       $indent the tabs before each line. $context says where it is written:
       edge, touch, input, condition or action. In an input, most commands only
       run on the press ($pressed is the C++ to test), and in an action a held
       move stops when the key is let go. -->
  <xsl:template match="*" mode="command">
    <xsl:param name="context" />
    <xsl:call-template name="refuse">
      <xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; in ', $context, ' rules')" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="line">
    <xsl:param name="indent" />
    <xsl:param name="code" />
    <xsl:param name="guard" select="''" />
    <xsl:value-of select="$indent" />
    <xsl:if test="$guard != ''">
      <xsl:value-of select="concat('if (', $guard, ') { ')" />
    </xsl:if>
    <xsl:value-of select="$code" />
    <xsl:if test="$guard != ''"> }</xsl:if>
    <xsl:text>&#10;</xsl:text>
  </xsl:template>

  <!-- Whether the context only runs a one-off command on the press. -->
  <xsl:template name="press-guard">
    <xsl:param name="context" />
    <xsl:if test="$context = 'input' or $context = 'action'">pressed</xsl:if>
  </xsl:template>

  <xsl:template match="bounce" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="edge" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'edge' and $context != 'touch'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;bounce&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat('bounceOff(', $self, ', ', $edge, ');')" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="stick" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="edge" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'edge'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;stick&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat('stick(', $self, ', ', $edge, ');')" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="deflect" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="other" />
    <xsl:param name="edge" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'touch'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;deflect&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="angle">
      <xsl:call-template name="value" />
    </xsl:variable>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat('deflect(', $self, ', ', $other, ', ', $edge, ', ', $angle, ');')" />
    </xsl:call-template>
  </xsl:template>

  <!-- A bare <reset />: in a rule or an action the object goes back where it
       started; on a key or in a condition the whole game starts over. With
       object=, that object is put back as it started, variables and all. -->
  <xsl:template match="reset" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="indent" />
    <xsl:variable name="guard">
      <xsl:call-template name="press-guard"><xsl:with-param name="context" select="$context" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="code">
      <xsl:choose>
        <xsl:when test="@object">
          <xsl:if test="not($objects[@name = current()/@object])">
            <xsl:call-template name="refuse">
              <xsl:with-param name="what" select="concat('&lt;reset object=&quot;', @object, '&quot;&gt;, which is not an &lt;object&gt;,')" />
            </xsl:call-template>
          </xsl:if>
          <xsl:text>reset_</xsl:text>
          <xsl:call-template name="object-name"><xsl:with-param name="name" select="@object" /></xsl:call-template>
          <xsl:text>();</xsl:text>
        </xsl:when>
        <xsl:when test="$context = 'input' or $context = 'condition'">resetAll();</xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat($self, '.position = ', $self, '.positionOriginal;')" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="$code" />
      <xsl:with-param name="guard" select="$guard" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="die" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'edge' and $context != 'touch'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;die&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat($self, '.collides = false; ', $self, '.visible = false;')" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="stop" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'edge' and $context != 'touch'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;stop&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat($self, '.velocity = {}; ', $self, '.held = {};')" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="reverse" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'edge' and $context != 'touch'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;reverse&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat($self, '.velocity = -', $self, '.velocity;')" />
    </xsl:call-template>
  </xsl:template>

  <!-- <move>: held in an action (the velocity, while the key is down), a step
       of its position in a rule. -->
  <xsl:template match="move" mode="command">
    <xsl:param name="context" />
    <xsl:param name="self" />
    <xsl:param name="indent" />
    <xsl:variable name="step">
      <xsl:call-template name="value" />
    </xsl:variable>
    <xsl:variable name="direction">
      <xsl:call-template name="direction-name"><xsl:with-param name="direction" select="@direction" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="code">
      <xsl:choose>
        <xsl:when test="$context = 'action'">
          <xsl:value-of select="concat('setHeldMove(', $self, ', ', $direction, ', pressed ? ', $step, ' : 0.0f);')" />
        </xsl:when>
        <xsl:when test="$context = 'edge' or $context = 'touch'">
          <xsl:value-of select="$self" />
          <xsl:choose>
            <xsl:when test="@direction = 'up'">.position.y -= </xsl:when>
            <xsl:when test="@direction = 'down'">.position.y += </xsl:when>
            <xsl:when test="@direction = 'left'">.position.x -= </xsl:when>
            <xsl:otherwise>.position.x += </xsl:otherwise>
          </xsl:choose>
          <xsl:value-of select="concat($step, ';')" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="refuse">
            <xsl:with-param name="what" select="concat('&lt;move&gt; in ', $context, ' rules')" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="$code" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="inc | dec" mode="command">
    <xsl:param name="context" />
    <xsl:param name="indent" />
    <xsl:if test="$context = 'action'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;', local-name(), '&gt; in an action')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="guard">
      <xsl:call-template name="press-guard"><xsl:with-param name="context" select="$context" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="variable">
      <xsl:call-template name="object-variable" />
    </xsl:variable>
    <xsl:variable name="amount">
      <xsl:choose>
        <xsl:when test="normalize-space(.) = '' and not(*)">1.0f</xsl:when>
        <xsl:otherwise><xsl:call-template name="value" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="operator">
      <xsl:choose>
        <xsl:when test="self::inc"> += </xsl:when>
        <xsl:otherwise> -= </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat($variable, $operator, $amount, ';')" />
      <xsl:with-param name="guard" select="$guard" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="play" mode="command">
    <xsl:param name="context" />
    <xsl:param name="indent" />
    <xsl:if test="not(/game/sounds/sound[@name = current()/@sound])">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;play sound=&quot;', @sound, '&quot;&gt;, which is not a &lt;sound&gt;,')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="guard">
      <xsl:call-template name="press-guard"><xsl:with-param name="context" select="$context" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="sound">
      <xsl:call-template name="sound-name"><xsl:with-param name="name" select="@sound" /></xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat('play(', $sound, ');')" />
      <xsl:with-param name="guard" select="$guard" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="push" mode="command">
    <xsl:param name="context" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'input' and $context != 'condition'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;push&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="not($states[@name = current()/@state])">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;push state=&quot;', @state, '&quot;&gt;, which is not a &lt;state&gt;,')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="guard">
      <xsl:call-template name="press-guard"><xsl:with-param name="context" select="$context" /></xsl:call-template>
    </xsl:variable>
    <xsl:variable name="state">
      <xsl:call-template name="state-name"><xsl:with-param name="name" select="@state" /></xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="concat('push(', $state, ');')" />
      <xsl:with-param name="guard" select="$guard" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template match="pop" mode="command">
    <xsl:param name="context" />
    <xsl:param name="indent" />
    <xsl:if test="$context != 'input' and $context != 'condition'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;pop&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="guard">
      <xsl:call-template name="press-guard"><xsl:with-param name="context" select="$context" /></xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="line">
      <xsl:with-param name="indent" select="$indent" />
      <xsl:with-param name="code" select="'pop();'" />
      <xsl:with-param name="guard" select="$guard" />
    </xsl:call-template>
  </xsl:template>

  <!-- <trigger>: the commands of another object's action, run as that object. -->
  <xsl:template match="trigger" mode="command">
    <xsl:param name="context" />
    <xsl:param name="indent" />
    <xsl:variable name="target" select="$objects[@name = current()/@object]" />
    <xsl:variable name="action" select="$target/actions/action[@name = current()/@action]" />
    <xsl:if test="not($action)">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;trigger object=&quot;', @object, '&quot; action=&quot;', @action, '&quot;&gt;, which names no action,')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:if test="$context != 'input'">
      <xsl:call-template name="refuse">
        <xsl:with-param name="what" select="concat('&lt;trigger&gt; in ', $context, ' rules')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="self">
      <xsl:call-template name="object-name"><xsl:with-param name="name" select="@object" /></xsl:call-template>
    </xsl:variable>
    <xsl:apply-templates select="$action/*" mode="command">
      <xsl:with-param name="context" select="'action'" />
      <xsl:with-param name="self" select="$self" />
      <xsl:with-param name="indent" select="$indent" />
    </xsl:apply-templates>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- The game                                                               -->
  <!-- ===================================================================== -->

  <xsl:template name="game">
    <xsl:apply-templates select="/game" mode="check" />

    <xsl:text>
// The game, from </xsl:text>
    <xsl:value-of select="$source" />
    <xsl:text>: its window, values, objects, sounds, states and rules.
namespace game
{
	using namespace xge;

	// &lt;window&gt;
	constexpr unsigned width = </xsl:text>
    <xsl:value-of select="normalize-space(/game/window/width)" />
    <xsl:text>;
	constexpr unsigned height = </xsl:text>
    <xsl:value-of select="normalize-space(/game/window/height)" />
    <xsl:text>;
	constexpr unsigned framerate = </xsl:text>
    <xsl:value-of select="normalize-space(/game/window/framerate)" />
    <xsl:text>;
	constexpr bool fullscreen = </xsl:text>
    <xsl:choose>
      <xsl:when test="normalize-space(/game/window/fullscreen) = 'true'">true</xsl:when>
      <xsl:otherwise>false</xsl:otherwise>
    </xsl:choose>
    <xsl:text>;
	const char* const title = </xsl:text>
    <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="/game/window/@name" /></xsl:call-template>
    <xsl:text>;
	const sf::Color background = </xsl:text>
    <xsl:call-template name="color"><xsl:with-param name="name" select="/game/window/background" /></xsl:call-template>
    <xsl:text>;

	// What an expression can name: the window, the game's &lt;variables&gt;, and
	// every object's size and own &lt;variables&gt; (each with its starting value).
	const float v_window_top = 0.0f;
	const float v_window_bottom = static_cast&lt;float&gt;(height);
	const float v_window_left = 0.0f;
	const float v_window_right = static_cast&lt;float&gt;(width);
	const float v_window_width_center = static_cast&lt;float&gt;(width) / 2.0f;
	const float v_window_height_center = static_cast&lt;float&gt;(height) / 2.0f;
</xsl:text>
    <xsl:for-each select="/game/variables/variable">
      <xsl:text>	float </xsl:text>
      <xsl:call-template name="cpp-name">
        <xsl:with-param name="prefix" select="'v_'" />
        <xsl:with-param name="name" select="@name" />
      </xsl:call-template>
      <xsl:text> = 0.0f;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$objects">
      <xsl:variable name="object" select="." />
      <xsl:if test="not(variables/variable[@name = 'width'])">
        <xsl:value-of select="concat('&#9;float v_', translate(@name, '.-', '__'), '_width = 0.0f;&#10;')" />
      </xsl:if>
      <xsl:if test="not(variables/variable[@name = 'height'])">
        <xsl:value-of select="concat('&#9;float v_', translate(@name, '.-', '__'), '_height = 0.0f;&#10;')" />
      </xsl:if>
      <xsl:for-each select="variables/variable">
        <xsl:variable name="name">
          <xsl:call-template name="cpp-name">
            <xsl:with-param name="prefix" select="'v_'" />
            <xsl:with-param name="name" select="concat($object/@name, '.', @name)" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;float ', $name, ' = 0.0f;&#10;&#9;float ', $name, '_start = 0.0f;&#10;')" />
      </xsl:for-each>
    </xsl:for-each>

    <xsl:text>
	// &lt;objects&gt;, in the order they are drawn
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:text>	Object </xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>
	const std::vector&lt;Object*&gt; objects{</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:if test="position() &gt; 1">,</xsl:if>
      <xsl:text> &amp;</xsl:text>
      <xsl:call-template name="object-name" />
    </xsl:for-each>
    <xsl:text> };
</xsl:text>

    <xsl:call-template name="sounds" />
    <xsl:call-template name="states" />
    <xsl:call-template name="resets" />
    <xsl:call-template name="edges" />
    <xsl:call-template name="touches" />
    <xsl:call-template name="conditions" />
    <xsl:call-template name="inputs" />
    <xsl:call-template name="setup" />

    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Sounds                                                                 -->
  <!-- ===================================================================== -->

  <xsl:template name="sounds">
    <xsl:text>
	// &lt;sounds&gt;: each asked for at most once a frame, and played once the
	// frame's keys, rules and conditions have all run.
	enum SoundId : std::size_t
	{
</xsl:text>
    <xsl:for-each select="/game/sounds/sound">
      <xsl:text>		</xsl:text>
      <xsl:call-template name="sound-name" />
      <xsl:text>,&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>		soundCount
	};

</xsl:text>
    <xsl:choose>
      <xsl:when test="/game/sounds/sound">
        <xsl:text>	std::array&lt;Voice, soundCount&gt; voices;
	std::array&lt;bool, soundCount&gt; soundAskedFor{};

	void play(SoundId sound)
	{
		soundAskedFor[sound] = true;
	}

	void loadSounds()
	{
</xsl:text>
        <xsl:for-each select="/game/sounds/sound">
          <xsl:variable name="sound" select="." />
          <xsl:text>		voices[</xsl:text>
          <xsl:call-template name="sound-name" />
          <xsl:text>].load(SoundDesc{ </xsl:text>
          <xsl:choose>
            <xsl:when test="volume"><xsl:call-template name="value"><xsl:with-param name="node" select="volume" /></xsl:call-template></xsl:when>
            <xsl:otherwise>0.3f</xsl:otherwise>
          </xsl:choose>
          <xsl:text>, {</xsl:text>
          <xsl:for-each select="note | rest">
            <xsl:if test="position() &gt; 1">,</xsl:if>
            <xsl:variable name="wave">
              <xsl:choose>
                <xsl:when test="@wave"><xsl:value-of select="@wave" /></xsl:when>
                <xsl:when test="$sound/@wave"><xsl:value-of select="$sound/@wave" /></xsl:when>
                <xsl:otherwise>square</xsl:otherwise>
              </xsl:choose>
            </xsl:variable>
            <xsl:text> { Wave::</xsl:text>
            <xsl:value-of select="concat(translate(substring($wave, 1, 1), 'stn', 'STN'), substring($wave, 2))" />
            <xsl:text>, </xsl:text>
            <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="@pitch" /></xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="@to" /></xsl:call-template>
            <xsl:text>, </xsl:text>
            <xsl:call-template name="value" />
            <xsl:text> }</xsl:text>
          </xsl:for-each>
          <xsl:text> } });&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>	}

	void playSounds()
	{
		for (std::size_t sound = 0; sound &lt; soundCount; ++sound)
		{
			if (soundAskedFor[sound]) { voices[sound].play(); }
		}
		soundAskedFor = {};
	}
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>	void loadSounds() {}
	void playSounds() {}
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- States                                                                 -->
  <!-- ===================================================================== -->

  <xsl:template name="states">
    <xsl:text>
	// &lt;states&gt;: a stack, the first state at the bottom and never popped.
	enum StateId
	{
</xsl:text>
    <xsl:for-each select="$states">
      <xsl:text>		</xsl:text>
      <xsl:call-template name="state-name" />
      <xsl:text>,&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>	};

	std::vector&lt;StateId&gt; stack{ </xsl:text>
    <xsl:call-template name="state-name"><xsl:with-param name="name" select="$states[1]/@name" /></xsl:call-template>
    <xsl:text> };
	unsigned long stateChanges = 0;

	void push(StateId state)
	{
		stack.push_back(state);
		++stateChanges;
	}

	void pop()
	{
		if (stack.size() &gt; 1)
		{
			stack.pop_back();
			++stateChanges;
		}
	}

	// Whether an object is in play: the current state shows it, and it has not died.
	bool shown(const Object&amp; object)
	{
		if (!object.visible)
		{
			return false;
		}

		switch (stack.back())
		{
</xsl:text>
    <xsl:for-each select="$states">
      <xsl:text>		case </xsl:text>
      <xsl:call-template name="state-name" />
      <xsl:text>:&#10;			return </xsl:text>
      <xsl:for-each select="shows/show">
        <xsl:if test="position() &gt; 1"> || </xsl:if>
        <xsl:text>&amp;object == &amp;</xsl:text>
        <xsl:call-template name="object-name"><xsl:with-param name="name" select="@object" /></xsl:call-template>
      </xsl:for-each>
      <xsl:if test="not(shows/show)">false</xsl:if>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>		}
		return false;
	}
</xsl:text>
  </xsl:template>

  <!-- Putting an object, or everything, back as it started. -->
  <xsl:template name="resets">
    <xsl:text>
	// &lt;reset object&gt;: an object as it started, its variables too.
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:variable name="object" select="." />
      <xsl:variable name="name">
        <xsl:call-template name="object-name" />
      </xsl:variable>
      <xsl:value-of select="concat('&#9;void reset_', $name, '()&#10;&#9;{&#10;&#9;&#9;resetState(', $name, ');&#10;')" />
      <xsl:for-each select="variables/variable">
        <xsl:variable name="variable">
          <xsl:call-template name="cpp-name">
            <xsl:with-param name="prefix" select="'v_'" />
            <xsl:with-param name="name" select="concat($object/@name, '.', @name)" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;&#9;', $variable, ' = ', $variable, '_start;&#10;')" />
      </xsl:for-each>
      <xsl:text>	}&#10;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>	// A bare &lt;reset /&gt; on a key or in a condition: the whole game starts over
	// from its first state.
	void resetAll()
	{
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:text>		reset_</xsl:text>
      <xsl:call-template name="object-name" />
      <xsl:text>();&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>		stack.assign(1, </xsl:text>
    <xsl:call-template name="state-name"><xsl:with-param name="name" select="$states[1]/@name" /></xsl:call-template>
    <xsl:text>);
		++stateChanges;
	}
</xsl:text>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Rules about the window's edges                                         -->
  <!-- ===================================================================== -->

  <xsl:template name="edges">
    <xsl:text>
	// &lt;collision edge&gt;: before anything moves, each moving object's rules for
	// the edges it is past, top, bottom, left, right.
	void checkEdges()
	{
</xsl:text>
    <xsl:for-each select="$objects[collisions/collision/@edge]">
      <xsl:variable name="object" select="." />
      <xsl:variable name="self">
        <xsl:call-template name="object-name" />
      </xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;if (shown(', $self, ') &amp;&amp; ', $self, '.collides &amp;&amp; isMoving(', $self, '))&#10;&#9;&#9;{&#10;')" />
      <xsl:call-template name="edge-blocks">
        <xsl:with-param name="object" select="$object" />
        <xsl:with-param name="self" select="$self" />
        <xsl:with-param name="list" select="$edges" />
      </xsl:call-template>
      <xsl:text>		}&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>	}

	// &lt;stick /&gt; again after the move, whether or not the object still moves,
	// so it never ends a frame poking out past the wall.
	void keepStuckObjectsIn()
	{
</xsl:text>
    <xsl:for-each select="$objects[collisions/collision[@edge]/stick]">
      <xsl:variable name="object" select="." />
      <xsl:variable name="self">
        <xsl:call-template name="object-name" />
      </xsl:variable>
      <xsl:call-template name="stick-lines">
        <xsl:with-param name="object" select="$object" />
        <xsl:with-param name="self" select="$self" />
        <xsl:with-param name="list" select="$edges" />
      </xsl:call-template>
    </xsl:for-each>
    <xsl:text>	}
</xsl:text>
  </xsl:template>

  <!-- One block for each edge in $list (space-separated) the object has rules for. -->
  <xsl:template name="edge-blocks">
    <xsl:param name="object" />
    <xsl:param name="self" />
    <xsl:param name="list" />
    <xsl:variable name="edge" select="substring-before(concat($list, ' '), ' ')" />
    <xsl:if test="$edge != ''">
      <xsl:variable name="axis">
        <xsl:choose>
          <xsl:when test="$edge = 'top' or $edge = 'bottom'">vertical</xsl:when>
          <xsl:otherwise>horizontal</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="rules" select="$object/collisions/collision[@edge = $edge or @edge = 'all' or @edge = $axis]" />
      <xsl:if test="$rules">
        <xsl:variable name="cppEdge">
          <xsl:call-template name="edge-name"><xsl:with-param name="edge" select="$edge" /></xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;&#9;&#9;if (touchesEdge(', $self, ', ', $cppEdge, '))&#10;&#9;&#9;&#9;{&#10;')" />
        <xsl:apply-templates select="$rules/*" mode="command">
          <xsl:with-param name="context" select="'edge'" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="edge" select="$cppEdge" />
          <xsl:with-param name="indent" select="'&#9;&#9;&#9;&#9;'" />
        </xsl:apply-templates>
        <xsl:text>			}&#10;</xsl:text>
      </xsl:if>
      <xsl:call-template name="edge-blocks">
        <xsl:with-param name="object" select="$object" />
        <xsl:with-param name="self" select="$self" />
        <xsl:with-param name="list" select="substring-after($list, ' ')" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <xsl:template name="stick-lines">
    <xsl:param name="object" />
    <xsl:param name="self" />
    <xsl:param name="list" />
    <xsl:variable name="edge" select="substring-before(concat($list, ' '), ' ')" />
    <xsl:if test="$edge != ''">
      <xsl:variable name="axis">
        <xsl:choose>
          <xsl:when test="$edge = 'top' or $edge = 'bottom'">vertical</xsl:when>
          <xsl:otherwise>horizontal</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:if test="$object/collisions/collision[@edge = $edge or @edge = 'all' or @edge = $axis]/stick">
        <xsl:variable name="cppEdge">
          <xsl:call-template name="edge-name"><xsl:with-param name="edge" select="$edge" /></xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;&#9;if (shown(', $self, ') &amp;&amp; ', $self, '.collides &amp;&amp; touchesEdge(', $self, ', ', $cppEdge, ')) { stick(', $self, ', ', $cppEdge, '); }&#10;')" />
      </xsl:if>
      <xsl:call-template name="stick-lines">
        <xsl:with-param name="object" select="$object" />
        <xsl:with-param name="self" select="$self" />
        <xsl:with-param name="list" select="substring-after($list, ' ')" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Rules about touching another object                                    -->
  <!-- ===================================================================== -->

  <!-- Every pair of objects, both able to collide, where at least one has a
       rule that answers to the other, worked out here once rather than every
       frame: a touch function each, and the list moveObjects sweeps. -->
  <xsl:template name="touches">
    <xsl:variable name="colliders" select="$objects[normalize-space(collisions/enabled) = 'true']" />
    <xsl:text>
	// &lt;collision&gt; without an edge: what each object does when it touches
	// another, both sides' rules in turn.
</xsl:text>
    <xsl:for-each select="$colliders">
      <xsl:variable name="a" select="." />
      <xsl:for-each select="following-sibling::object[normalize-space(collisions/enabled) = 'true']">
        <xsl:variable name="b" select="." />
        <xsl:variable name="rulesOfA" select="$a/collisions/collision[not(@edge)][(not(@class) or @class = $b/@class) and (not(@object) or @object = $b/@name)]" />
        <xsl:variable name="rulesOfB" select="$b/collisions/collision[not(@edge)][(not(@class) or @class = $a/@class) and (not(@object) or @object = $a/@name)]" />
        <xsl:if test="$rulesOfA or $rulesOfB">
          <xsl:variable name="nameA"><xsl:call-template name="object-name"><xsl:with-param name="name" select="$a/@name" /></xsl:call-template></xsl:variable>
          <xsl:variable name="nameB"><xsl:call-template name="object-name"><xsl:with-param name="name" select="$b/@name" /></xsl:call-template></xsl:variable>
          <xsl:value-of select="concat('&#9;void touch_', $nameA, '_', $nameB, '(Edge edgeOfFirst, Edge edgeOfSecond)&#10;&#9;{&#10;')" />
          <xsl:call-template name="touch-side">
            <xsl:with-param name="rules" select="$rulesOfA" />
            <xsl:with-param name="self" select="$nameA" />
            <xsl:with-param name="other" select="$nameB" />
            <xsl:with-param name="edge" select="'edgeOfFirst'" />
          </xsl:call-template>
          <xsl:call-template name="touch-side">
            <xsl:with-param name="rules" select="$rulesOfB" />
            <xsl:with-param name="self" select="$nameB" />
            <xsl:with-param name="other" select="$nameA" />
            <xsl:with-param name="edge" select="'edgeOfSecond'" />
          </xsl:call-template>
          <xsl:if test="not($rulesOfA)">		(void)edgeOfFirst;&#10;</xsl:if>
          <xsl:if test="not($rulesOfB)">		(void)edgeOfSecond;&#10;</xsl:if>
          <xsl:text>	}&#10;&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>

    <xsl:text>	std::vector&lt;Pair&gt; pairs{&#10;</xsl:text>
    <xsl:for-each select="$colliders">
      <xsl:variable name="a" select="." />
      <xsl:for-each select="following-sibling::object[normalize-space(collisions/enabled) = 'true']">
        <xsl:variable name="b" select="." />
        <xsl:if test="$a/collisions/collision[not(@edge)][(not(@class) or @class = $b/@class) and (not(@object) or @object = $b/@name)]
                   or $b/collisions/collision[not(@edge)][(not(@class) or @class = $a/@class) and (not(@object) or @object = $a/@name)]">
          <xsl:variable name="nameA"><xsl:call-template name="object-name"><xsl:with-param name="name" select="$a/@name" /></xsl:call-template></xsl:variable>
          <xsl:variable name="nameB"><xsl:call-template name="object-name"><xsl:with-param name="name" select="$b/@name" /></xsl:call-template></xsl:variable>
          <xsl:value-of select="concat('&#9;&#9;Pair{ &amp;', $nameA, ', &amp;', $nameB, ', touch_', $nameA, '_', $nameB, ' },&#10;')" />
        </xsl:if>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:text>	};
</xsl:text>
  </xsl:template>

  <!-- One side's rules for touching the other. Its speed is taken once, before
       any rule runs, for the rules with slower= or faster=. -->
  <xsl:template name="touch-side">
    <xsl:param name="rules" />
    <xsl:param name="self" />
    <xsl:param name="other" />
    <xsl:param name="edge" />
    <xsl:if test="$rules">
      <xsl:text>		{&#10;</xsl:text>
      <xsl:if test="$rules[@slower or @faster]">
        <xsl:value-of select="concat('&#9;&#9;&#9;const float speed = std::hypot(motionOf(', $self, ').x, motionOf(', $self, ').y);&#10;')" />
      </xsl:if>
      <xsl:for-each select="$rules">
        <xsl:variable name="test">
          <xsl:if test="@slower">
            <xsl:text>speed &lt; </xsl:text>
            <xsl:call-template name="attribute-value"><xsl:with-param name="text" select="@slower" /></xsl:call-template>
          </xsl:if>
          <xsl:if test="@slower and @faster"> &amp;&amp; </xsl:if>
          <xsl:if test="@faster">
            <xsl:text>speed &gt;= </xsl:text>
            <xsl:call-template name="attribute-value"><xsl:with-param name="text" select="@faster" /></xsl:call-template>
          </xsl:if>
        </xsl:variable>
        <xsl:variable name="indent">
          <xsl:choose>
            <xsl:when test="$test != ''"><xsl:text>&#9;&#9;&#9;&#9;</xsl:text></xsl:when>
            <xsl:otherwise><xsl:text>&#9;&#9;&#9;</xsl:text></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:if test="$test != ''">
          <xsl:value-of select="concat('&#9;&#9;&#9;if (', $test, ')&#10;&#9;&#9;&#9;{&#10;')" />
        </xsl:if>
        <xsl:apply-templates select="*" mode="command">
          <xsl:with-param name="context" select="'touch'" />
          <xsl:with-param name="self" select="$self" />
          <xsl:with-param name="other" select="$other" />
          <xsl:with-param name="edge" select="$edge" />
          <xsl:with-param name="indent" select="$indent" />
        </xsl:apply-templates>
        <xsl:if test="$test != ''">
          <xsl:text>			}&#10;</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>		}&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Conditions                                                             -->
  <!-- ===================================================================== -->

  <!-- The objects a condition's class= and object= pick (neither: all of them). -->
  <xsl:template name="conditions">
    <xsl:text>
	// &lt;conditions&gt; of the current state, after everything has moved: the
	// first one met runs its commands, and that is all for the frame.
	void checkConditions()
	{
		switch (stack.back())
		{
</xsl:text>
    <xsl:for-each select="$states[conditions/condition]">
      <xsl:text>		case </xsl:text>
      <xsl:call-template name="state-name" />
      <xsl:text>:&#10;</xsl:text>
      <xsl:for-each select="conditions/condition">
        <xsl:variable name="condition" select="." />
        <xsl:variable name="picked" select="$objects[(not($condition/@class) or @class = $condition/@class) and (not($condition/@object) or @name = $condition/@object)]" />
        <xsl:variable name="test">
          <xsl:choose>
            <xsl:when test="remaining">
              <xsl:text>(</xsl:text>
              <xsl:for-each select="$picked">
                <xsl:if test="position() &gt; 1"> + </xsl:if>
                <xsl:text>(</xsl:text>
                <xsl:call-template name="object-name" />
                <xsl:text>.visible ? 1 : 0)</xsl:text>
              </xsl:for-each>
              <xsl:if test="not($picked)">0</xsl:if>
              <xsl:text>) &lt;= </xsl:text>
              <xsl:call-template name="value"><xsl:with-param name="node" select="remaining" /></xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:variable name="operator">
                <xsl:choose>
                  <xsl:when test="atmost"> &lt;= </xsl:when>
                  <xsl:otherwise> &gt;= </xsl:otherwise>
                </xsl:choose>
              </xsl:variable>
              <xsl:variable name="threshold">
                <xsl:call-template name="value"><xsl:with-param name="node" select="atleast | atmost" /></xsl:call-template>
              </xsl:variable>
              <xsl:for-each select="$picked[variables/variable[@name = $condition/@variable]]">
                <xsl:if test="position() &gt; 1"> || </xsl:if>
                <xsl:call-template name="cpp-name">
                  <xsl:with-param name="prefix" select="'v_'" />
                  <xsl:with-param name="name" select="concat(@name, '.', $condition/@variable)" />
                </xsl:call-template>
                <xsl:value-of select="concat($operator, $threshold)" />
              </xsl:for-each>
              <xsl:if test="not($picked[variables/variable[@name = $condition/@variable]])">false</xsl:if>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:value-of select="concat('&#9;&#9;&#9;if (', $test, ')&#10;&#9;&#9;&#9;{&#10;')" />
        <xsl:apply-templates select="*[not(self::atleast or self::atmost or self::remaining)]" mode="command">
          <xsl:with-param name="context" select="'condition'" />
          <xsl:with-param name="indent" select="'&#9;&#9;&#9;&#9;'" />
        </xsl:apply-templates>
        <xsl:text>				return;&#10;			}&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>			break;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>		default:
			break;
		}
	}
</xsl:text>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Keys                                                                   -->
  <!-- ===================================================================== -->

  <!-- Each <input> is a function run on the press and on the release; one whose
       action holds a <move> has a second function that starts only the held
       part, for a key still down when the state changes. -->
  <xsl:template name="inputs">
    <xsl:text>
	// &lt;inputs&gt;: what each key does in each state.
	struct Binding
	{
		void (*run)(bool pressed) = nullptr;
		void (*held)() = nullptr;
	};

</xsl:text>
    <xsl:for-each select="$states/inputs/input">
      <xsl:variable name="function">
        <xsl:call-template name="input-function" />
      </xsl:variable>
      <xsl:value-of select="concat('&#9;void ', $function, '(bool pressed)&#10;&#9;{&#10;')" />
      <xsl:apply-templates select="*" mode="command">
        <xsl:with-param name="context" select="'input'" />
        <xsl:with-param name="indent" select="'&#9;&#9;'" />
      </xsl:apply-templates>
      <xsl:text>	}&#10;&#10;</xsl:text>
      <xsl:if test="trigger[$objects[@name = current()/trigger/@object]/actions/action[@name = current()/trigger/@action]/move]">
        <xsl:value-of select="concat('&#9;void held_', $function, '()&#10;&#9;{&#10;')" />
        <xsl:for-each select="trigger">
          <xsl:variable name="trigger" select="." />
          <xsl:variable name="self">
            <xsl:call-template name="object-name"><xsl:with-param name="name" select="@object" /></xsl:call-template>
          </xsl:variable>
          <xsl:for-each select="$objects[@name = $trigger/@object]/actions/action[@name = $trigger/@action]/move">
            <xsl:variable name="step"><xsl:call-template name="value" /></xsl:variable>
            <xsl:variable name="direction">
              <xsl:call-template name="direction-name"><xsl:with-param name="direction" select="@direction" /></xsl:call-template>
            </xsl:variable>
            <xsl:value-of select="concat('&#9;&#9;setHeldMove(', $self, ', ', $direction, ', ', $step, ');&#10;')" />
          </xsl:for-each>
        </xsl:for-each>
        <xsl:text>	}&#10;&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>

    <xsl:text>	Binding bindingFor(sf::Keyboard::Key key)
	{
		switch (stack.back())
		{
</xsl:text>
    <xsl:for-each select="$states[inputs/input]">
      <xsl:text>		case </xsl:text>
      <xsl:call-template name="state-name" />
      <xsl:text>:&#10;			switch (key)&#10;			{&#10;</xsl:text>
      <xsl:for-each select="inputs/input">
        <xsl:variable name="function">
          <xsl:call-template name="input-function" />
        </xsl:variable>
        <xsl:variable name="held">
          <xsl:choose>
            <xsl:when test="trigger[$objects[@name = current()/trigger/@object]/actions/action[@name = current()/trigger/@action]/move]">
              <xsl:value-of select="concat('held_', $function)" />
            </xsl:when>
            <xsl:otherwise>nullptr</xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <xsl:call-template name="key-cases">
          <xsl:with-param name="keys" select="normalize-space(@button)" />
        </xsl:call-template>
        <xsl:value-of select="concat('&#9;&#9;&#9;&#9;return { ', $function, ', ', $held, ' };&#10;')" />
      </xsl:for-each>
      <xsl:text>			default:&#10;				return {};&#10;			}&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>		default:
			return {};
		}
	}
</xsl:text>
  </xsl:template>

  <xsl:template name="input-function">
    <xsl:value-of select="concat('input_', translate(../../@name, '.-', '__'), '_', count(preceding-sibling::input) + 1)" />
  </xsl:template>

  <!-- A case label for each key named in button= (space-separated). -->
  <xsl:template name="key-cases">
    <xsl:param name="keys" />
    <xsl:variable name="key" select="substring-before(concat($keys, ' '), ' ')" />
    <xsl:if test="$key != ''">
      <xsl:variable name="sfml" select="$tables/keys/key[@name = $key]/@sfml" />
      <xsl:if test="not($sfml)">
        <xsl:call-template name="refuse">
          <xsl:with-param name="what" select="concat('the key &quot;', $key, '&quot;')" />
        </xsl:call-template>
      </xsl:if>
      <xsl:value-of select="concat('&#9;&#9;&#9;case sf::Keyboard::Key::', $sfml, ':&#10;')" />
      <xsl:call-template name="key-cases">
        <xsl:with-param name="keys" select="substring-after($keys, ' ')" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- ===================================================================== -->
  <!-- Starting, and each frame                                               -->
  <!-- ===================================================================== -->

  <xsl:template name="setup">
    <xsl:text>
	// The game's &lt;variables&gt; in the order written, then each object's own.
	void loadValues()
	{
</xsl:text>
    <xsl:for-each select="/game/variables/variable">
      <xsl:text>		</xsl:text>
      <xsl:call-template name="cpp-name">
        <xsl:with-param name="prefix" select="'v_'" />
        <xsl:with-param name="name" select="@name" />
      </xsl:call-template>
      <xsl:text> = </xsl:text>
      <xsl:call-template name="value" />
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$objects">
      <xsl:variable name="object" select="." />
      <xsl:for-each select="variables/variable">
        <xsl:variable name="name">
          <xsl:call-template name="cpp-name">
            <xsl:with-param name="prefix" select="'v_'" />
            <xsl:with-param name="name" select="concat($object/@name, '.', @name)" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:variable name="value"><xsl:call-template name="value" /></xsl:variable>
        <xsl:value-of select="concat('&#9;&#9;', $name, '_start = ', $value, ';&#10;&#9;&#9;', $name, ' = ', $name, '_start;&#10;')" />
      </xsl:for-each>
    </xsl:for-each>
    <xsl:text>	}

	// Each object's picture, which gives it its size.
	void buildObjects()
	{
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:variable name="self"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;', $self, '.name = &quot;', @name, '&quot;;&#10;')" />
      <xsl:choose>
        <xsl:when test="sprite/circle">
          <xsl:value-of select="concat('&#9;&#9;makeCircle(', $self, ', ')" />
          <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/circle/radius" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="color"><xsl:with-param name="name" select="sprite/circle/color" /></xsl:call-template>
          <xsl:text>);&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="sprite/rectangle">
          <xsl:value-of select="concat('&#9;&#9;makeRectangle(', $self, ', ')" />
          <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/rectangle/width" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/rectangle/height" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="color"><xsl:with-param name="name" select="sprite/rectangle/color" /></xsl:call-template>
          <xsl:text>);&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="sprite/text">
          <xsl:value-of select="concat('&#9;&#9;makeText(', $self, ', ')" />
          <xsl:choose>
            <xsl:when test="sprite/text/number">
              <xsl:text>displayNumber(</xsl:text>
              <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/text/number" /></xsl:call-template>
              <xsl:text>)</xsl:text>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="sprite/text/content" /></xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/text/size" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="color">
            <xsl:with-param name="name">
              <xsl:choose>
                <xsl:when test="sprite/text/color"><xsl:value-of select="sprite/text/color" /></xsl:when>
                <xsl:otherwise>color.white</xsl:otherwise>
              </xsl:choose>
            </xsl:with-param>
          </xsl:call-template>
          <xsl:text>);&#10;</xsl:text>
        </xsl:when>
        <xsl:when test="sprite/image">
          <xsl:value-of select="concat('&#9;&#9;makeImage(', $self, ', ')" />
          <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="normalize-space(sprite/image/path)" /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="cpp-string"><xsl:with-param name="text" select="normalize-space(sprite/image/flip)" /></xsl:call-template>
          <xsl:text>);&#10;</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="refuse">
            <xsl:with-param name="what" select="'an object without a circle, rectangle, text or image'" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>	}

	// What the expressions see as each object's size.
	void measureObjects()
	{
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:variable name="self"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:variable name="v" select="concat('v_', translate(@name, '.-', '__'))" />
      <xsl:if test="not(variables/variable[@name = 'width'])">
        <xsl:value-of select="concat('&#9;&#9;', $v, '_width = ', $self, '.size.x;&#10;')" />
      </xsl:if>
      <xsl:if test="not(variables/variable[@name = 'height'])">
        <xsl:value-of select="concat('&#9;&#9;', $v, '_height = ', $self, '.size.y;&#10;')" />
      </xsl:if>
    </xsl:for-each>
    <xsl:text>	}

	// Where each object starts and how fast it goes, once every size is known.
	// A &lt;random&gt; is drawn here, once, so a reset repeats it.
	void placeObjects()
	{
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:variable name="self"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;', $self, '.positionOriginal = { ')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /></xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /></xsl:call-template>
      <xsl:value-of select="concat(' };&#10;&#9;&#9;', $self, '.velocityOriginal = { ')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="velocity/x" /></xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="velocity/y" /></xsl:call-template>
      <xsl:text> };&#10;</xsl:text>
      <xsl:variable name="visible">
        <xsl:choose>
          <xsl:when test="normalize-space(hidden) = 'true' or @class = 'projectile'">false</xsl:when>
          <xsl:otherwise>true</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:variable name="collides">
        <xsl:choose>
          <xsl:when test="normalize-space(collisions/enabled) = 'true'">true</xsl:when>
          <xsl:otherwise>false</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;', $self, '.visibleOriginal = ', $visible, ';&#10;&#9;&#9;', $self, '.collidesOriginal = ', $collides, ';&#10;&#9;&#9;resetState(', $self, ');&#10;')" />
    </xsl:for-each>
    <xsl:text>	}

	// A position that uses the size of a text, worked out again when that
	// size changes (a score that grows a digit).
	void placeBySize()
	{
</xsl:text>
    <xsl:for-each select="$objects[contains(string(position), '.width') or contains(string(position), '.height') or position//@*[contains(., '.width') or contains(., '.height')]]">
      <xsl:variable name="self"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:text>		{&#10;			const sf::Vector2f place{ </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /></xsl:call-template>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /></xsl:call-template>
      <xsl:value-of select="concat(' };&#10;&#9;&#9;&#9;if (place != ', $self, '.positionOriginal)&#10;&#9;&#9;&#9;{&#10;&#9;&#9;&#9;&#9;', $self, '.positionOriginal = place;&#10;&#9;&#9;&#9;&#9;', $self, '.position = place;&#10;&#9;&#9;&#9;}&#10;&#9;&#9;}&#10;')" />
    </xsl:for-each>
    <xsl:text>	}

	// A text that shows a number shows it as it is now.
	void updateTexts()
	{
</xsl:text>
    <xsl:for-each select="$objects[sprite/text/number]">
      <xsl:variable name="self"><xsl:call-template name="object-name" /></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;setText(', $self, ', displayNumber(')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/text/number" /></xsl:call-template>
      <xsl:text>));&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>	}

	void start()
	{
		windowWidth = static_cast&lt;float&gt;(width);
		windowHeight = static_cast&lt;float&gt;(height);
		loadValues();
		loadSounds();
		buildObjects();
		measureObjects();
		placeObjects();
	}

	// One frame (lib/source/game.cpp, Game::updateObjects).
	void update()
	{
		checkEdges();
		moveObjects(objects, pairs, shown);
		keepStuckObjectsIn();
		checkConditions();
	}

	// After the frame and the keys: texts, the places that follow their sizes, sounds.
	void afterUpdate()
	{
		updateTexts();
		measureObjects();
		placeBySize();
		playSounds();
	}

	void draw(sf::RenderWindow&amp; window)
	{
		for (Object* object : objects)
		{
			if (shown(*object))
			{
				xge::draw(window, *object);
			}
		}
	}
</xsl:text>
  </xsl:template>

</xsl:stylesheet>
