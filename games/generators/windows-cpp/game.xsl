<?xml version="1.0" encoding="UTF-8"?>
<!-- game.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- The game itself: source/<game>.h, its class, and source/<game>.cpp, its
     numbers as constants, its objects in the file's order and its answers to
     xge::FrameRules, each rule written by its verb's file (verbs/) in one of
     three modes: edge (at the window's edge, on object and edge), touch (on
     a meeting, on self, other and touch) and action (on a key, on who). -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- What has rules: the state's objects and groups, and their rules at an
       edge, on a touch and that stop at an edge. -->
  <xsl:variable name="owners" select="$state/objects/object | $state/objects/group" />
  <xsl:variable name="edge-rules" select="$owners/collisions/collision[@edge]" />
  <xsl:variable name="touch-rules" select="$owners/collisions/collision[not(@edge)]" />
  <xsl:variable name="stick-rules" select="$edge-rules[stick]" />
  <xsl:variable name="ruled" select="$owners[collisions/collision]" />
  <xsl:variable name="numbers" select="$state//sign | $state//sine | $state//cosine" />

  <!-- source/<game>.h -->
  <xsl:template name="header-file">
    <xsl:variable name="class"><xsl:call-template name="class-name" /></xsl:variable>
    <xsl:call-template name="header"><xsl:with-param name="file" select="concat($name, '.h')" /></xsl:call-template>
    <xsl:value-of select="concat('//&#10;// ', $title, ', from ', $source, '.&#10;')" />
    <xsl:text>//
// Written by xgecli --generate windows-cpp as a person would write it. It
// decides what happens; the engine's own small functions do it: playOneFrame
// (play/frame.h) runs the frame in the engine's order and the verbs
// (play/verbs.h) move things. No Game, no reader, no XML.

#pragma once

#include "engine/keys.h"
#include "game/object.h"
#include "play/frame.h"

#include &lt;vector&gt;

namespace </xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:value-of select="concat('&#10;{&#10;&#9;class ', $class, ' : public xge::FrameRules&#10;&#9;{&#10;&#9;public:&#10;&#9;&#9;// The game as it starts.&#10;&#9;&#9;', $class, '();&#10;')" />
    <xsl:text>
		// &lt;window&gt;
		xge::WindowDesc window;

		// Its objects, in the file's order, which is the order they are
		// moved, met and drawn in.
		std::vector&lt;xge::Object&gt; objects;

		// A key went down (pressed) or came up: the state's &lt;inputs&gt;.
		void key(xge::KeyCode key, bool pressed);

		// One frame of play.
		void frame();

</xsl:text>
    <xsl:value-of select="concat('&#9;&#9;// What ', $source, ' says should happen, as playOneFrame asks.&#10;')" />
    <xsl:text>		bool inPlay(const xge::Object&amp; object) override;
		void atEdge(xge::Object&amp; object, xge::Edge edge) override;
		bool sticksAt(const xge::Object&amp; object, xge::Edge edge) override;
		bool canCollide(const xge::Object&amp; a, const xge::Object&amp; b) override;
		bool hasRules(const xge::Object&amp; object) override;
		void touched(xge::Object&amp; a, xge::Object&amp; b, const xge::Contact&amp; contact) override;

	private:
</xsl:text>
    <xsl:for-each select="$objects">
      <xsl:value-of select="'&#9;&#9;xge::Object&amp; '" />
      <xsl:call-template name="object-name" />
      <xsl:value-of select="'() { return objects['" />
      <xsl:call-template name="index-of" />
      <xsl:text>]; }&#10;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$ruled">
      <xsl:value-of select="'&#9;&#9;bool '" />
      <xsl:call-template name="test-name" />
      <xsl:text>(const xge::Object&amp; object) const { return </xsl:text>
      <xsl:for-each select=". | member">
        <xsl:if test="not(self::group)">
          <xsl:if test="self::member and position() &gt; 2"> || </xsl:if>
          <xsl:text>&amp;object == &amp;objects[</xsl:text>
          <xsl:call-template name="index-of" />
          <xsl:text>]</xsl:text>
        </xsl:if>
      </xsl:for-each>
      <xsl:text>; }&#10;</xsl:text>
    </xsl:for-each>
    <xsl:for-each select="$state/objects/object">
      <xsl:variable name="starts"><xsl:call-template name="has-start" /></xsl:variable>
      <xsl:if test="$starts != ''">
        <xsl:variable name="object"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:value-of select="concat('&#10;&#9;&#9;// Where the ', @name, ' starts: its &lt;random&gt;s drawn, then its place and way.&#10;&#9;&#9;void ', $object, 'Start();&#10;')" />
      </xsl:if>
    </xsl:for-each>
    <xsl:text>	};
}
</xsl:text>
  </xsl:template>

  <!-- Whether an object has a start function: its start draws a <random>,
       or its place or way reads its own variables. (Only objects; a group's
       members cannot have either yet.) -->
  <xsl:template name="has-start">
    <xsl:param name="object" select="." />
    <xsl:variable name="random"><xsl:call-template name="draws-random"><xsl:with-param name="object" select="$object" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$random = 'yes'">yes</xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="$object/variables/variable">
          <xsl:variable name="own" select="@name" />
          <xsl:if test="$object/position//*[not(*)][normalize-space(.) = concat($object/@name, '.', $own) or normalize-space(.) = concat('self.', $own)]
              | $object/velocity//*[not(*)][normalize-space(.) = concat($object/@name, '.', $own) or normalize-space(.) = concat('self.', $own)]">yes</xsl:if>
        </xsl:for-each>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- source/<game>.cpp -->
  <xsl:template name="game-file">
    <xsl:variable name="class"><xsl:call-template name="class-name" /></xsl:variable>
    <xsl:call-template name="header"><xsl:with-param name="file" select="concat($name, '.cpp')" /></xsl:call-template>
    <xsl:value-of select="concat('//&#10;// ', $title, ', from ', $source, '.&#10;')" />
    <xsl:text>//
// Each part of the file is the code below that it names, in the file's order;
// nothing here moves anything itself.

</xsl:text>
    <xsl:value-of select="concat('#include &quot;', $name, '.h&quot;&#10;&#10;#include &quot;game/random.h&quot;&#10;')" />
    <xsl:if test="$numbers"><xsl:text>#include "play/numbers.h"&#10;</xsl:text></xsl:if>
    <xsl:text>#include "play/verbs.h"

namespace </xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:text>
{
	using namespace xge;

	namespace
	{
		// &lt;window&gt;
		const float windowWidth = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/width" /></xsl:call-template>
    <xsl:text>;&#10;&#9;&#9;const float windowHeight = </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="window/height" /></xsl:call-template>
    <xsl:text>;&#10;</xsl:text>
    <xsl:call-template name="constants" />
    <xsl:call-template name="helpers" />
    <xsl:text>	}
</xsl:text>
    <xsl:call-template name="constructor"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    <xsl:for-each select="$state/objects/object">
      <xsl:variable name="starts"><xsl:call-template name="has-start" /></xsl:variable>
      <xsl:if test="$starts != ''">
        <xsl:call-template name="start-function"><xsl:with-param name="class" select="$class" /></xsl:call-template>
      </xsl:if>
    </xsl:for-each>
    <xsl:call-template name="key-function"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    <xsl:value-of select="concat('&#10;&#9;void ', $class, '::frame()&#10;&#9;{&#10;&#9;&#9;playOneFrame(objects, window, *this);&#10;&#9;}&#10;')" />
    <xsl:value-of select="concat('&#10;&#9;// The one state shows everything it owns.&#10;&#9;bool ', $class, '::inPlay(const Object&amp; object)&#10;&#9;{&#10;&#9;&#9;return object.isVisible;&#10;&#9;}&#10;')" />
    <xsl:call-template name="edge-functions"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    <xsl:call-template name="touch-functions"><xsl:with-param name="class" select="$class" /></xsl:call-template>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- The game's <variables>, each a constant: those the game reads. -->
  <xsl:template name="constants">
    <xsl:if test="/game/variables/variable">
      <xsl:text>&#10;&#9;&#9;// &lt;variables&gt;&#10;</xsl:text>
    </xsl:if>
    <xsl:for-each select="/game/variables/variable">
      <xsl:variable name="named" select="@name" />
      <xsl:if test="$state//*[not(*)][normalize-space(.) = $named] | /game/variables/variable[@name != $named]//*[normalize-space(.) = $named] | /game/variables/variable[@name != $named][normalize-space(.) = $named]">
        <xsl:text>&#9;&#9;const float </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="." /></xsl:call-template>
        <xsl:text>;&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- The small functions the game is written with: a rectangle, where a
       thing starts, and the edges a rule names. -->
  <xsl:template name="helpers">
    <xsl:if test="$owners/sprite/rectangle">
      <xsl:text>
		// A &lt;rectangle&gt;, white unless it says, that touches what it meets.
		Object rectangle(const char* name, float across, float down, const char* color = "color.white")
		{
			Object object;
			object.name = name;
			object.ownerState = "</xsl:text>
      <xsl:value-of select="$state/@name" />
      <xsl:text>";
			object.picture.kind = ShapeKind::Rectangle;
			object.picture.size = { across, down };
			object.picture.color = color;
			object.collisionsOn = true;
			return object;
		}
</xsl:text>
    </xsl:if>
    <xsl:text>
		// Where it starts, and where a reset puts it back.
		void startAt(Object&amp; object, Vector2f position, Vector2f velocity)
		{
			object.position = position;
			object.velocity = velocity;
			object.start = object;
		}
</xsl:text>
    <xsl:if test="$edge-rules[@edge = 'vertical'] or $edge-rules[@edge = 'horizontal']">
      <xsl:text>&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$edge-rules[@edge = 'vertical']">
      <xsl:text>		bool vertical(Edge edge) { return edge == Edge::Top || edge == Edge::Bottom; }&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$edge-rules[@edge = 'horizontal']">
      <xsl:text>		bool horizontal(Edge edge) { return edge == Edge::Left || edge == Edge::Right; }&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The constructor: the window, then each object as the file has it. -->
  <xsl:template name="constructor">
    <xsl:param name="class" />
    <xsl:value-of select="concat('&#10;&#9;', $class, '::', $class, '()&#10;&#9;{&#10;&#9;&#9;window = { ')" />
    <xsl:call-template name="string"><xsl:with-param name="text" select="string(/game/window/@name)" /></xsl:call-template>
    <xsl:text>, windowWidth, windowHeight, </xsl:text>
    <xsl:choose>
      <xsl:when test="/game/window/scale"><xsl:value-of select="normalize-space(/game/window/scale)" /></xsl:when>
      <xsl:otherwise>1</xsl:otherwise>
    </xsl:choose>
    <xsl:text>, </xsl:text>
    <xsl:choose>
      <xsl:when test="/game/window/background"><xsl:call-template name="text"><xsl:with-param name="node" select="/game/window/background" /></xsl:call-template></xsl:when>
      <xsl:otherwise>"color.black"</xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="/game/window/fullscreen">, "true", </xsl:when>
      <xsl:otherwise>, "false", </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="/game/window/framerate"><xsl:value-of select="normalize-space(/game/window/framerate)" /></xsl:when>
      <xsl:otherwise>60</xsl:otherwise>
    </xsl:choose>
    <xsl:text> };&#10;&#9;&#9;seedRandom();&#10;</xsl:text>
    <xsl:for-each select="$owners">
      <xsl:text>&#10;</xsl:text>
      <xsl:choose>
        <xsl:when test="self::group">
          <xsl:value-of select="concat('&#9;&#9;// &lt;group name=&quot;', @name, '&quot;&gt;&#10;')" />
          <xsl:for-each select="member">
            <xsl:call-template name="make-object"><xsl:with-param name="sprite" select="../sprite" /></xsl:call-template>
          </xsl:for-each>
          <xsl:text>&#9;&#9;</xsl:text>
          <xsl:for-each select="member">
            <xsl:call-template name="object-name" />
            <xsl:text>().groupName = </xsl:text>
          </xsl:for-each>
          <xsl:call-template name="string"><xsl:with-param name="text" select="concat($state/@name, '.', @name)" /></xsl:call-template>
          <xsl:text>;&#10;</xsl:text>
          <xsl:for-each select="member">
            <xsl:call-template name="start-at" />
          </xsl:for-each>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="make-object"><xsl:with-param name="sprite" select="sprite" /></xsl:call-template>
          <xsl:for-each select="variables/variable[not(.//random)]">
            <xsl:text>&#9;&#9;</xsl:text>
            <xsl:call-template name="object-name"><xsl:with-param name="object" select="../.." /></xsl:call-template>
            <xsl:text>().variable[</xsl:text>
            <xsl:call-template name="string"><xsl:with-param name="text" select="string(@name)" /></xsl:call-template>
            <xsl:text>] = </xsl:text>
            <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="../.." /></xsl:call-template>
            <xsl:text>;&#10;</xsl:text>
          </xsl:for-each>
          <xsl:call-template name="start-at" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:text>	}&#10;</xsl:text>
  </xsl:template>

  <!-- objects.push_back(rectangle("playing.ball", size, size)); -->
  <xsl:template name="make-object">
    <xsl:param name="sprite" />
    <xsl:text>&#9;&#9;objects.push_back(rectangle(</xsl:text>
    <xsl:call-template name="string"><xsl:with-param name="text"><xsl:call-template name="full-name" /></xsl:with-param></xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/width" /><xsl:with-param name="self" select="." /></xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/height" /><xsl:with-param name="self" select="." /></xsl:call-template>
    <xsl:if test="$sprite/rectangle/color">
      <xsl:text>, </xsl:text>
      <xsl:call-template name="text"><xsl:with-param name="node" select="$sprite/rectangle/color" /></xsl:call-template>
    </xsl:if>
    <xsl:text>));&#10;</xsl:text>
  </xsl:template>

  <!-- startAt(paddle1(), { margin, ... }, { 0, 0 }), or its start function
       first when it has one. -->
  <xsl:template name="start-at">
    <xsl:variable name="object"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="starts"><xsl:if test="self::object"><xsl:call-template name="has-start" /></xsl:if></xsl:variable>
    <xsl:choose>
      <xsl:when test="$starts != ''">
        <xsl:value-of select="concat('&#9;&#9;', $object, 'Start();&#10;&#9;&#9;startAt(', $object, '(), ', $object, '().start.position, ', $object, '().start.velocity);&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;&#9;startAt(', $object, '(), ')" />
        <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity" /></xsl:call-template>
        <xsl:text>);&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- { x, y } of a <position> or <velocity>; { 0, 0 } when there is none. -->
  <xsl:template name="pair">
    <xsl:param name="of" />
    <xsl:param name="self" select="." />
    <xsl:choose>
      <xsl:when test="$of">
        <xsl:text>{ </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$of/x" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'x'" /></xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$of/y" /><xsl:with-param name="self" select="$self" /><xsl:with-param name="axis" select="'y'" /></xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise>{ 0, 0 }</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ballStart(): its <random>s drawn (by name, as the engine draws them
       again), the variables its start reads, then its place and way. -->
  <xsl:template name="start-function">
    <xsl:param name="class" />
    <xsl:variable name="object" select="." />
    <xsl:variable name="accessor"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:value-of select="concat('&#10;&#9;// Where the ', @name, ' starts, and starts again after a reset: its&#10;&#9;// &lt;random&gt;s drawn, then its place and its way from them.&#10;&#9;void ', $class, '::', $accessor, 'Start()&#10;&#9;{&#10;&#9;&#9;Object&amp; self = ', $accessor, '();&#10;')" />
    <xsl:for-each select="variables/variable[.//random]">
      <xsl:sort select="@name" />
      <xsl:variable name="key"><xsl:call-template name="string"><xsl:with-param name="text" select="string(@name)" /></xsl:call-template></xsl:variable>
      <xsl:value-of select="concat('&#9;&#9;self.variable[', $key, '] = self.start.variable[', $key, '] = ')" />
      <xsl:call-template name="value"><xsl:with-param name="node" select="." /><xsl:with-param name="self" select="$object" /></xsl:call-template>
      <xsl:text>;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#10;</xsl:text>
    <xsl:for-each select="variables/variable">
      <xsl:variable name="own" select="@name" />
      <xsl:if test="$object/position//*[not(*)][normalize-space(.) = concat($object/@name, '.', $own) or normalize-space(.) = concat('self.', $own)]
          | $object/velocity//*[not(*)][normalize-space(.) = concat($object/@name, '.', $own) or normalize-space(.) = concat('self.', $own)]">
        <xsl:text>&#9;&#9;const float </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:text> = self.variable[</xsl:text>
        <xsl:call-template name="string"><xsl:with-param name="text" select="string(@name)" /></xsl:call-template>
        <xsl:text>];&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>&#9;&#9;self.start.position = </xsl:text>
    <xsl:call-template name="pair"><xsl:with-param name="of" select="position" /></xsl:call-template>
    <xsl:text>;&#10;&#9;&#9;self.start.velocity = </xsl:text>
    <xsl:call-template name="pair"><xsl:with-param name="of" select="velocity" /></xsl:call-template>
    <xsl:text>;&#10;&#9;}&#10;</xsl:text>
  </xsl:template>

  <!-- key(): the state's <inputs>, a case a button, each trigger's action
       done by its verbs (mode action). -->
  <xsl:template name="key-function">
    <xsl:param name="class" />
    <xsl:variable name="inputs" select="$state/inputs/input" />
    <xsl:text>&#10;&#9;// &lt;inputs&gt;&#10;</xsl:text>
    <xsl:choose>
      <xsl:when test="not($inputs)">
        <xsl:value-of select="concat('&#9;void ', $class, '::key(KeyCode, bool)&#10;&#9;{&#10;&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;void ', $class, '::key(KeyCode key, bool pressed)&#10;&#9;{&#10;&#9;&#9;switch (key)&#10;&#9;&#9;{&#10;')" />
        <xsl:for-each select="$inputs">
          <xsl:for-each select="@button | button">
            <xsl:text>&#9;&#9;case </xsl:text>
            <xsl:call-template name="key-code"><xsl:with-param name="button" select="normalize-space(.)" /></xsl:call-template>
            <xsl:text>:</xsl:text>
            <xsl:if test="position() != last()"><xsl:text>&#10;</xsl:text></xsl:if>
          </xsl:for-each>
          <xsl:apply-templates select="*[not(self::button)]" mode="key" />
          <xsl:text> break;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>		default: break;
		}
	}
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- atEdge() and sticksAt(): the <collision edge>s, an if an object or
       group, and in it an if an edge. -->
  <xsl:template name="edge-functions">
    <xsl:param name="class" />
    <xsl:text>&#10;&#9;// At the window's edges: each one's &lt;collision edge&gt;s.&#10;</xsl:text>
    <xsl:choose>
      <xsl:when test="not($edge-rules)">
        <xsl:value-of select="concat('&#9;void ', $class, '::atEdge(Object&amp;, Edge)&#10;&#9;{&#10;&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;void ', $class, '::atEdge(Object&amp; object, Edge edge)&#10;&#9;{&#10;')" />
        <xsl:for-each select="$owners[collisions/collision[@edge]]">
          <xsl:variable name="owner" select="." />
          <xsl:text>&#9;&#9;if (</xsl:text>
          <xsl:call-template name="test-name" />
          <xsl:text>(object))&#10;&#9;&#9;{&#10;</xsl:text>
          <xsl:for-each select="collisions/collision[@edge]">
            <xsl:variable name="test"><xsl:call-template name="edge-test" /></xsl:variable>
            <xsl:text>&#9;&#9;&#9;</xsl:text>
            <xsl:if test="$test != ''"><xsl:value-of select="concat('if (', $test, ') { ')" /></xsl:if>
            <xsl:apply-templates select="*" mode="edge"><xsl:with-param name="owner" select="$owner" /></xsl:apply-templates>
            <xsl:if test="$test != ''"><xsl:text> }</xsl:text></xsl:if>
            <xsl:text>&#10;</xsl:text>
          </xsl:for-each>
          <xsl:text>&#9;&#9;}&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>&#10;&#9;// What stops at an edge rather than going past it: a &lt;stick /&gt;.&#10;</xsl:text>
    <xsl:choose>
      <xsl:when test="not($stick-rules)">
        <xsl:value-of select="concat('&#9;bool ', $class, '::sticksAt(const Object&amp;, Edge)&#10;&#9;{&#10;&#9;&#9;return false;&#10;&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;bool ', $class, '::sticksAt(const Object&amp; object, Edge', substring(' edge', 1, 5 * count($stick-rules[@edge != 'all'])), ')&#10;&#9;{&#10;&#9;&#9;return ')" />
        <xsl:for-each select="$stick-rules">
          <xsl:variable name="test"><xsl:call-template name="edge-test" /></xsl:variable>
          <xsl:if test="position() &gt; 1"> || </xsl:if>
          <xsl:if test="last() &gt; 1 and $test != ''">(</xsl:if>
          <xsl:call-template name="test-name"><xsl:with-param name="owner" select="../.." /></xsl:call-template>
          <xsl:text>(object)</xsl:text>
          <xsl:if test="$test != ''"><xsl:value-of select="concat(' &amp;&amp; ', $test)" /></xsl:if>
          <xsl:if test="last() &gt; 1 and $test != ''">)</xsl:if>
        </xsl:for-each>
        <xsl:text>;&#10;&#9;}&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- vertical(edge), edge == Edge::Top, or nothing for all. -->
  <xsl:template name="edge-test">
    <xsl:choose>
      <xsl:when test="@edge = 'vertical' or @edge = 'horizontal'"><xsl:value-of select="concat(@edge, '(edge)')" /></xsl:when>
      <xsl:when test="@edge = 'all'" />
      <xsl:otherwise>
        <xsl:value-of select="concat('edge == Edge::', translate(substring(@edge, 1, 1), $lower, $upper), substring(@edge, 2))" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- canCollide(), hasRules() and touched(): the <collision>s that name no
       edge, each answering to anything it meets. -->
  <xsl:template name="touch-functions">
    <xsl:param name="class" />
    <xsl:variable name="toucher" select="$owners[collisions/collision[not(@edge)]]" />
    <xsl:text>&#10;&#9;// Which pairs answer to meeting: a &lt;collision&gt; naming nothing answers to&#10;&#9;// anything its object meets.&#10;</xsl:text>
    <xsl:choose>
      <xsl:when test="not($toucher)">
        <xsl:value-of select="concat('&#9;bool ', $class, '::canCollide(const Object&amp;, const Object&amp;)&#10;&#9;{&#10;&#9;&#9;return false;&#10;&#9;}&#10;')" />
        <xsl:value-of select="concat('&#10;&#9;bool ', $class, '::hasRules(const Object&amp;)&#10;&#9;{&#10;&#9;&#9;return false;&#10;&#9;}&#10;')" />
        <xsl:value-of select="concat('&#10;&#9;void ', $class, '::touched(Object&amp;, Object&amp;, const Contact&amp;)&#10;&#9;{&#10;&#9;}&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('&#9;bool ', $class, '::canCollide(const Object&amp; a, const Object&amp; b)&#10;&#9;{&#10;&#9;&#9;return ')" />
        <xsl:for-each select="$toucher">
          <xsl:variable name="test"><xsl:call-template name="test-name" /></xsl:variable>
          <xsl:if test="position() &gt; 1"> || </xsl:if>
          <xsl:value-of select="concat($test, '(a) || ', $test, '(b)')" />
        </xsl:for-each>
        <xsl:value-of select="concat(';&#10;&#9;}&#10;&#10;&#9;bool ', $class, '::hasRules(const Object&amp; object)&#10;&#9;{&#10;&#9;&#9;return ')" />
        <xsl:for-each select="$toucher">
          <xsl:if test="position() &gt; 1"> || </xsl:if>
          <xsl:call-template name="test-name" />
          <xsl:text>(object)</xsl:text>
        </xsl:for-each>
        <xsl:text>;&#10;&#9;}&#10;</xsl:text>
        <xsl:text>&#10;&#9;// When they meet: each one's &lt;collision&gt;s, done to it.&#10;</xsl:text>
        <xsl:value-of select="concat('&#9;void ', $class, '::touched(Object&amp; a, Object&amp; b, const Contact&amp; contact)&#10;&#9;{&#10;')" />
        <xsl:for-each select="$toucher">
          <xsl:variable name="owner" select="." />
          <xsl:variable name="test"><xsl:call-template name="test-name" /></xsl:variable>
          <xsl:value-of select="concat('&#9;&#9;if (', $test, '(a)) { ')" />
          <xsl:for-each select="collisions/collision[not(@edge)]/*">
            <xsl:if test="position() &gt; 1"><xsl:text> </xsl:text></xsl:if>
            <xsl:apply-templates select="." mode="touch">
              <xsl:with-param name="owner" select="$owner" />
              <xsl:with-param name="self" select="'a'" />
              <xsl:with-param name="other" select="'b'" />
              <xsl:with-param name="touch" select="'touchOfFirst(contact)'" />
            </xsl:apply-templates>
          </xsl:for-each>
          <xsl:value-of select="concat(' }&#10;&#9;&#9;if (', $test, '(b)) { ')" />
          <xsl:for-each select="collisions/collision[not(@edge)]/*">
            <xsl:if test="position() &gt; 1"><xsl:text> </xsl:text></xsl:if>
            <xsl:apply-templates select="." mode="touch">
              <xsl:with-param name="owner" select="$owner" />
              <xsl:with-param name="self" select="'b'" />
              <xsl:with-param name="other" select="'a'" />
              <xsl:with-param name="touch" select="'touchOfSecond(contact)'" />
            </xsl:apply-templates>
          </xsl:for-each>
          <xsl:text> }&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A verb windows-cpp has no words for yet, in any mode. -->
  <xsl:template match="*" mode="edge">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; at an edge yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="touch">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; on a touch yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="action">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; in an action yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
  <xsl:template match="*" mode="key">
    <xsl:message terminate="yes">windows-cpp cannot write &lt;<xsl:value-of select="local-name()" />&gt; on a key yet; use engine-cpp for this game</xsl:message>
  </xsl:template>
</xsl:stylesheet>
