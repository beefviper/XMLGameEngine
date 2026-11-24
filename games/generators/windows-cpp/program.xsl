<?xml version="1.0" encoding="UTF-8"?>
<!-- program.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 13, 2026 -->

<!-- The rest of a windows-cpp program: source/main.cpp, the loop on SFML
     (keys, a step of the time since the last frame, the picture), its build and its
     readme. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <!-- What the game is called: its window's name, or else the program's. -->
  <xsl:variable name="title">
    <xsl:choose>
      <xsl:when test="/game/window/@name != ''"><xsl:value-of select="/game/window/@name" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="$name" /></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <!-- What the window draws with: a Picture when it writes words too, the
       window itself when it draws boxes only. -->
  <xsl:variable name="texts-drawn" select="boolean($texts)" />
  <xsl:variable name="canvas">
    <xsl:choose>
      <xsl:when test="$texts-drawn">Picture&amp; picture</xsl:when>
      <xsl:otherwise>sf::RenderWindow&amp; window</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="canvas-name">
    <xsl:choose>
      <xsl:when test="$texts-drawn">picture</xsl:when>
      <xsl:otherwise>window</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="box">
    <xsl:choose>
      <xsl:when test="$texts-drawn">picture.box(</xsl:when>
      <xsl:otherwise>box(window, </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <xsl:variable name="styles" select="/game/styles/style[@name = $texts/sprite/text/@style]" />
  <xsl:variable name="basic-colors" select="'|white|black|red|green|blue|yellow|magenta|cyan|'" />

  <xsl:template name="main-file">
    <xsl:call-template name="header"><xsl:with-param name="file" select="'main.cpp'" /></xsl:call-template>
    <xsl:text>//
// The window: keys in, a step of the time since the last frame, the picture</xsl:text>
    <xsl:if test="$sounds"> and the sounds</xsl:if>
    <xsl:text> out.
</xsl:text>
    <xsl:value-of select="concat('&#10;#include &quot;', $name, '.h&quot;&#10;&#10;')" />
    <xsl:if test="$sounds"><xsl:text>#include &lt;SFML/Audio.hpp&gt;&#10;</xsl:text></xsl:if>
    <xsl:text>#include &lt;SFML/Graphics.hpp&gt;

#include &lt;algorithm&gt;
</xsl:text>
    <xsl:if test="$uses-pads"><xsl:text>#include &lt;array&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$sounds"><xsl:text>#include &lt;cmath&gt;&#10;#include &lt;cstdint&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$texts-drawn"><xsl:text>#include &lt;filesystem&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$texts-drawn or $sounds"><xsl:text>#include &lt;iostream&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$sounds"><xsl:text>#include &lt;map&gt;&#10;</xsl:text></xsl:if>
    <xsl:text>#include &lt;optional&gt;&#10;</xsl:text>
    <xsl:if test="$texts-drawn"><xsl:text>#include &lt;string&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$sounds"><xsl:text>#include &lt;vector&gt;&#10;</xsl:text></xsl:if>
    <xsl:text>
namespace
{
	using namespace </xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:text>;&#10;</xsl:text>
    <xsl:call-template name="color-constants" />
    <xsl:if test="$texts-drawn">
      <xsl:text>
	struct Style
	{
		unsigned size;
		sf::Color color;
	};

</xsl:text>
      <xsl:for-each select="$styles">
        <xsl:text>&#9;const Style </xsl:text>
        <xsl:call-template name="camel"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:value-of select="concat('{ ', normalize-space(size), ', ')" />
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(color)" /></xsl:call-template>
        <xsl:text> };&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>
	class Picture
	{
	public:
		Picture(sf::RenderWindow&amp; into, const sf::Font&amp; with) : window(into), font(with) {}

		void box(sf::Vector2f position, sf::Vector2f size, sf::Color color = sf::Color::White)
		{
			sf::RectangleShape shape(size);
			shape.setPosition(position);
			shape.setFillColor(color);
			window.draw(shape);
		}

		// How big these words are drawn: the box their letters cover.
		sf::Vector2f measure(const std::string&amp; words, const Style&amp; style) const
		{
			return sf::Text(font, words, style.size).getLocalBounds().size;
		}

		// Words with their letters' top left at `position`.
		void text(const std::string&amp; words, const Style&amp; style, sf::Vector2f position)
		{
			sf::Text shown(font, words, style.size);
			shown.setFillColor(style.color);
			shown.setPosition(position - shown.getLocalBounds().position);
			window.draw(shown);
		}

	private:
		sf::RenderWindow&amp; window;
		const sf::Font&amp; font;
	};
</xsl:text>
    </xsl:if>
    <xsl:if test="not($texts-drawn)">
      <xsl:text>
	void box(sf::RenderWindow&amp; window, sf::Vector2f position, sf::Vector2f size, sf::Color color = sf::Color::White)
	{
		sf::RectangleShape shape(size);
		shape.setPosition(position);
		shape.setFillColor(color);
		window.draw(shape);
	}
</xsl:text>
    </xsl:if>
    <xsl:if test="$screens">
      <xsl:call-template name="draw-functions" />
    </xsl:if>
    <xsl:if test="$sounds">
      <xsl:text>
	// A square wave, faded in and out over two thousandths of a second so it
	// doesn't click.
	sf::SoundBuffer tone(double hertz, double seconds)
	{
		constexpr unsigned rate = 44100;
		constexpr double volume = 0.3;
		const std::size_t count = static_cast&lt;std::size_t&gt;(seconds * rate);
		const std::size_t fade = std::min&lt;std::size_t&gt;(count / 4, rate / 500);

		std::vector&lt;std::int16_t&gt; samples(count);
		for (std::size_t i = 0; i &lt; count; ++i)
		{
			const double phase = std::fmod(static_cast&lt;double&gt;(i) * hertz / rate, 1.0);
			double level = phase &lt; 0.5 ? volume : -volume;
			if (i &lt; fade) { level *= static_cast&lt;double&gt;(i) / static_cast&lt;double&gt;(fade); }
			if (i &gt;= count - fade) { level *= static_cast&lt;double&gt;(count - 1 - i) / static_cast&lt;double&gt;(fade); }
			samples[i] = static_cast&lt;std::int16_t&gt;(std::lround(level * 32767));
		}

		sf::SoundBuffer buffer;
		if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, rate, { sf::SoundChannel::Mono }))
		{
			std::cerr &lt;&lt; "warning: could not make a sound\n";
		}
		return buffer;
	}
</xsl:text>
    </xsl:if>
    <xsl:if test="$uses-pads">
      <xsl:text>
	// The pads' buttons as they are now: an Xbox pad's buttons in SFML's
	// order, and its directions by the d-pad or the left stick past halfway.
	std::array&lt;bool, 12&gt; padButtons(unsigned pad)
	{
		std::array&lt;bool, 12&gt; buttons{};
		if (!sf::Joystick::isConnected(pad)) { return buttons; }
		for (unsigned button = 0; button &lt; 8; ++button) { buttons[button] = sf::Joystick::isButtonPressed(pad, button); }
		const auto axis = [pad](sf::Joystick::Axis which) { return sf::Joystick::hasAxis(pad, which) ? sf::Joystick::getAxisPosition(pad, which) : 0.0f; };
		buttons[static_cast&lt;std::size_t&gt;(PadButton::Up)] = axis(sf::Joystick::Axis::PovY) &lt; -50 || axis(sf::Joystick::Axis::Y) &lt; -50;
		buttons[static_cast&lt;std::size_t&gt;(PadButton::Down)] = axis(sf::Joystick::Axis::PovY) &gt; 50 || axis(sf::Joystick::Axis::Y) &gt; 50;
		buttons[static_cast&lt;std::size_t&gt;(PadButton::Left)] = axis(sf::Joystick::Axis::PovX) &lt; -50 || axis(sf::Joystick::Axis::X) &lt; -50;
		buttons[static_cast&lt;std::size_t&gt;(PadButton::Right)] = axis(sf::Joystick::Axis::PovX) &gt; 50 || axis(sf::Joystick::Axis::X) &gt; 50;
		return buttons;
	}
</xsl:text>
    </xsl:if>
    <xsl:text>}

int main(</xsl:text>
    <xsl:if test="$texts-drawn">int, char* argv[]</xsl:if>
    <xsl:text>)
{
	sf::RenderWindow window(sf::VideoMode({ static_cast&lt;unsigned&gt;(windowWidth), static_cast&lt;unsigned&gt;(windowHeight) }), </xsl:text>
    <xsl:call-template name="string"><xsl:with-param name="text" select="$title" /></xsl:call-template>
    <xsl:text>);
	window.setFramerateLimit(</xsl:text>
    <xsl:call-template name="framerate" />
    <xsl:text>);
	window.setKeyRepeatEnabled(false);
</xsl:text>
    <xsl:if test="$texts-drawn">
      <xsl:text>
	// The font sits beside the program.
	sf::Font font;
	const std::filesystem::path beside = std::filesystem::path(argv[0]).parent_path() / "assets/tuffy.ttf";
	if (!font.openFromFile(beside) &amp;&amp; !font.openFromFile("assets/tuffy.ttf"))
	{
		std::cerr &lt;&lt; "error: assets/tuffy.ttf is missing\n";
		return 1;
	}
</xsl:text>
    </xsl:if>
    <xsl:if test="$sounds">
      <xsl:text>
	const std::map&lt;Sound, sf::SoundBuffer&gt; tones{
</xsl:text>
      <xsl:for-each select="$sounds">
        <xsl:text>&#9;&#9;{ </xsl:text>
        <xsl:call-template name="sound-name"><xsl:with-param name="sound" select="@name" /></xsl:call-template>
        <xsl:value-of select="concat(', tone(', normalize-space(note/@pitch), ', ', normalize-space(note), ') },&#10;')" />
      </xsl:for-each>
      <xsl:text>	};
	std::map&lt;Sound, sf::Sound&gt; sounds;
	for (const auto&amp; [sound, buffer] : tones) { sounds.emplace(sound, buffer); }
</xsl:text>
    </xsl:if>
    <xsl:text>&#10;&#9;</xsl:text>
    <xsl:value-of select="$class" />
    <xsl:text> game;&#10;</xsl:text>
    <xsl:if test="$kept">
      <xsl:value-of select="concat('&#9;game.keepIn(&quot;', $name, '-kept.txt&quot;);&#10;')" />
    </xsl:if>
    <xsl:if test="$texts-drawn"><xsl:text>&#9;Picture picture(window, font);&#10;</xsl:text></xsl:if>
    <xsl:if test="$uses-pads"><xsl:text>&#9;std::array&lt;std::array&lt;bool, 12&gt;, 2&gt; pads{};&#10;</xsl:text></xsl:if>
    <xsl:text>	sf::Clock clock;
	while (window.isOpen())
	{
		while (const std::optional event = window.pollEvent())
		{
			if (event-&gt;is&lt;sf::Event::Closed&gt;()) { window.close(); }
			if (const auto* key = event-&gt;getIf&lt;sf::Event::KeyPressed&gt;()) { game.key(key-&gt;code, true); }
			if (const auto* key = event-&gt;getIf&lt;sf::Event::KeyReleased&gt;()) { game.key(key-&gt;code, false); }
		}
</xsl:text>
    <xsl:if test="$uses-pads">
      <xsl:text>
		// a pad's button is pressed or let go when it is not as it was
		for (unsigned pad = 0; pad &lt; 2; ++pad)
		{
			const std::array&lt;bool, 12&gt; now = padButtons(pad);
			for (std::size_t button = 0; button &lt; now.size(); ++button)
			{
				if (now[button] != pads[pad][button]) { game.pad(pad + 1, static_cast&lt;PadButton&gt;(button), now[button]); }
			}
			pads[pad] = now;
		}
</xsl:text>
    </xsl:if>
    <xsl:text>
		// the time since the last frame, at most a quarter of a second
		game.frame(std::min(clock.restart().asSeconds(), 0.25f));
</xsl:text>
    <xsl:if test="$sounds">
      <xsl:text>		for (const Sound sound : game.takeSounds()) { sounds.at(sound).play(); }&#10;</xsl:text>
    </xsl:if>
    <xsl:text>
		window.clear(sf::Color::Black);
</xsl:text>
    <xsl:choose>
      <xsl:when test="$screens">
        <xsl:value-of select="concat('&#9;&#9;draw(', $canvas-name, ', game);&#10;')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="drawn">
          <xsl:for-each select="$states[1]/objects/*">
            <xsl:call-template name="draw-lines" />
          </xsl:for-each>
        </xsl:variable>
        <xsl:call-template name="print-lines">
          <xsl:with-param name="lines" select="exsl:node-set($drawn)/line" />
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>		window.display();
	}
</xsl:text>
    <xsl:if test="$kept"><xsl:text>	game.keep();&#10;</xsl:text></xsl:if>
    <xsl:text>}
</xsl:text>
  </xsl:template>

  <!-- const sf::Color lightgrey{ 192, 192, 192 }: the colours SFML has no
       name for. -->
  <xsl:template name="color-constants">
    <xsl:variable name="used" select="(//rectangle/color | $styles/color)[not(contains($basic-colors, concat('|', normalize-space(.), '|')))]" />
    <xsl:variable name="list">
      <xsl:for-each select="$used">
        <xsl:if test="not(normalize-space(.) = preceding::color[count(. | $used) = count($used)]/text())"><c><xsl:value-of select="normalize-space(.)" /></c></xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:if test="exsl:node-set($list)/c">
      <xsl:text>&#10;</xsl:text>
      <xsl:for-each select="exsl:node-set($list)/c[not(. = preceding-sibling::c)]">
        <xsl:value-of select="concat('&#9;const sf::Color ', ., '{ ')" />
        <xsl:call-template name="color-rgb"><xsl:with-param name="color" select="string(.)" /></xsl:call-template>
        <xsl:text> };&#10;</xsl:text>
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <!-- A colour as the code says it: sf::Color::White, or its constant. -->
  <xsl:template name="color">
    <xsl:param name="color" />
    <xsl:choose>
      <xsl:when test="$color = ''">sf::Color::White</xsl:when>
      <xsl:when test="contains($basic-colors, concat('|', $color, '|'))">
        <xsl:value-of select="concat('sf::Color::', translate(substring($color, 1, 1), $lower, $upper), substring($color, 2))" />
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$color" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The engine's colours, red, green, blue. -->
  <xsl:template name="color-rgb">
    <xsl:param name="color" />
    <xsl:variable name="table" select="'|grey 128, 128, 128|darkgrey 64, 64, 64|lightgrey 192, 192, 192|brown 139, 69, 19|orange 255, 165, 0|purple 128, 0, 128|darkblue 0, 0, 139|darkgreen 0, 100, 0|forestgreen 34, 139, 34|gold 230, 180, 30|peach 255, 214, 170|'" />
    <xsl:value-of select="substring-before(substring-after($table, concat('|', $color, ' ')), '|')" />
  </xsl:template>

  <!-- drawTitle(), drawPlaying() ...: each screen's objects, then those it
       shows; draw(): the screen on top, over those it shows still. -->
  <xsl:template name="draw-functions">
    <xsl:for-each select="$states">
      <xsl:variable name="state" select="." />
      <xsl:variable name="drawn">
        <xsl:for-each select="objects/*">
          <xsl:call-template name="draw-lines" />
        </xsl:for-each>
        <xsl:for-each select="$shared[@name = $state/shows/show/@object]">
          <xsl:call-template name="draw-lines" />
        </xsl:for-each>
      </xsl:variable>
      <xsl:if test="exsl:node-set($drawn)/line">
        <xsl:variable name="body">
          <xsl:for-each select="exsl:node-set($drawn)/line"><xsl:value-of select="." /></xsl:for-each>
        </xsl:variable>
        <xsl:text>&#10;&#9;void draw</xsl:text>
        <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
        <xsl:value-of select="concat('(', $canvas)" />
        <xsl:if test="contains($body, 'game.')">
          <xsl:value-of select="concat(', const ', $class, '&amp; game')" />
        </xsl:if>
        <xsl:text>)&#10;&#9;{&#10;</xsl:text>
        <xsl:call-template name="print-lines">
          <xsl:with-param name="lines" select="exsl:node-set($drawn)/line" />
          <xsl:with-param name="indent" select="'&#9;&#9;'" />
        </xsl:call-template>
        <xsl:text>&#9;}&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>
	// The screen on top, over those it shows still beneath it.
	void draw(</xsl:text>
    <xsl:value-of select="concat($canvas, ', const ', $class, '&amp; game)&#10;&#9;{&#10;&#9;&#9;switch (game.screen())&#10;&#9;&#9;{&#10;')" />
    <xsl:for-each select="$states">
      <xsl:text>&#9;&#9;case </xsl:text>
      <xsl:call-template name="screen-name"><xsl:with-param name="state" select="@name" /></xsl:call-template>
      <xsl:text>:</xsl:text>
      <xsl:for-each select="$states[@name = current()/shows/show/@state] | .">
        <xsl:call-template name="draw-call" />
      </xsl:for-each>
      <xsl:text> break;&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>		}
	}
</xsl:text>
  </xsl:template>

  <!-- drawPlaying(picture, game); when that screen draws anything. -->
  <xsl:template name="draw-call">
    <xsl:variable name="state" select="." />
    <xsl:variable name="drawn">
      <xsl:for-each select="objects/*">
        <xsl:call-template name="draw-lines" />
      </xsl:for-each>
      <xsl:for-each select="$shared[@name = $state/shows/show/@object]">
        <xsl:call-template name="draw-lines" />
      </xsl:for-each>
    </xsl:variable>
    <xsl:if test="exsl:node-set($drawn)/line">
      <xsl:text> draw</xsl:text>
      <xsl:call-template name="type-word"><xsl:with-param name="name" select="@name" /></xsl:call-template>
      <xsl:value-of select="concat('(', $canvas-name)" />
      <xsl:if test="contains(string($drawn), 'game.')">, game</xsl:if>
      <xsl:text>);</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- The lines that draw one object, group or member. -->
  <xsl:template name="draw-lines">
    <xsl:variable name="object" select="." />
    <xsl:variable name="sprite" select="(sprite | parent::group/sprite)[1]" />
    <xsl:variable name="color">
      <xsl:if test="$sprite/rectangle/color">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space($sprite/rectangle/color)" /></xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="self::group[member]">
        <xsl:for-each select="member"><xsl:call-template name="draw-lines" /></xsl:for-each>
      </xsl:when>
      <xsl:when test="count(. | $things) = count($things)">
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <line><xsl:value-of select="concat($box, 'game.', $thing, '.position, game.', $thing, '.size', $color, ');')" /></line>
      </xsl:when>
      <xsl:when test="$sprite/rectangle">
        <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="size">
          <xsl:text>{ </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/width" /><xsl:with-param name="self" select="." /></xsl:call-template>
          <xsl:text>, </xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="$sprite/rectangle/height" /><xsl:with-param name="self" select="." /></xsl:call-template>
          <xsl:text> }</xsl:text>
        </xsl:variable>
        <xsl:variable name="across">
          <xsl:call-template name="cell-step"><xsl:with-param name="along" select="'x'" /></xsl:call-template>
        </xsl:variable>
        <xsl:variable name="down">
          <xsl:call-template name="cell-step"><xsl:with-param name="along" select="'y'" /></xsl:call-template>
        </xsl:variable>
        <xsl:variable name="columns" select="number(concat('0', normalize-space(columns)))" />
        <xsl:variable name="rows" select="number(concat('0', normalize-space(rows)))" />
        <xsl:variable name="at-x">
          <xsl:value-of select="$x" />
          <xsl:if test="$columns &gt; 1"><xsl:value-of select="concat(' + ', $across, ' * static_cast&lt;float&gt;(column)')" /></xsl:if>
        </xsl:variable>
        <xsl:variable name="at-y">
          <xsl:value-of select="$y" />
          <xsl:if test="$rows &gt; 1"><xsl:value-of select="concat(' + ', $down, ' * static_cast&lt;float&gt;(row)')" /></xsl:if>
        </xsl:variable>
        <xsl:variable name="draw" select="concat($box, '{ ', $at-x, ', ', $at-y, ' }, ', $size, $color, ');')" />
        <xsl:choose>
          <xsl:when test="$columns &gt; 1 and $rows &gt; 1">
            <line><xsl:value-of select="concat('for (int row = 0; row &lt; ', $rows, '; ++row)')" /></line>
            <line>{</line>
            <line depth="1"><xsl:value-of select="concat('for (int column = 0; column &lt; ', $columns, '; ++column) { ', $draw, ' }')" /></line>
            <line>}</line>
          </xsl:when>
          <xsl:when test="$columns &gt; 1 or $rows &gt; 1">
            <xsl:variable name="along">
              <xsl:choose>
                <xsl:when test="$rows &gt; 1">row</xsl:when>
                <xsl:otherwise>column</xsl:otherwise>
              </xsl:choose>
            </xsl:variable>
            <line><xsl:value-of select="concat('for (int ', $along, ' = 0; ', $along, ' &lt; ', $rows + $columns - 1, '; ++', $along, ')')" /></line>
            <line>{</line>
            <line depth="1"><xsl:value-of select="$draw" /></line>
            <line>}</line>
          </xsl:when>
          <xsl:otherwise><line><xsl:value-of select="$draw" /></line></xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$sprite/text">
        <xsl:call-template name="text-lines" />
      </xsl:when>
    </xsl:choose>
  </xsl:template>

  <!-- How far one cell of a block is from the next: its size and the
       padding, worked out when both are numbers. -->
  <xsl:template name="cell-step">
    <xsl:param name="along" />
    <xsl:variable name="rectangle" select="(sprite | parent::group/sprite)[1]/rectangle" />
    <xsl:variable name="size" select="$rectangle/*[local-name() = substring('widthheight', 1 + 5 * number($along = 'y'), 5 + number($along = 'y'))]" />
    <xsl:variable name="padding" select="padding/*[local-name() = $along]" />
    <xsl:choose>
      <xsl:when test="not($padding)">
        <xsl:call-template name="value"><xsl:with-param name="node" select="$size" /><xsl:with-param name="self" select="." /><xsl:with-param name="wrap" select="3" /></xsl:call-template>
      </xsl:when>
      <xsl:when test="string(number($size)) != 'NaN' and string(number($padding)) != 'NaN'">
        <xsl:value-of select="number($size) + number($padding)" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:text>(</xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$size" /><xsl:with-param name="self" select="." /></xsl:call-template>
        <xsl:text> + </xsl:text>
        <xsl:call-template name="value"><xsl:with-param name="node" select="$padding" /><xsl:with-param name="self" select="." /><xsl:with-param name="wrap" select="1" /></xsl:call-template>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A text: its words (a look's, or a number the game counts), measured
       when where it goes depends on its size, then drawn. -->
  <xsl:template name="text-lines">
    <xsl:variable name="object" select="." />
    <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="style"><xsl:call-template name="camel"><xsl:with-param name="name" select="sprite[1]/text/@style" /></xsl:call-template></xsl:variable>
    <xsl:variable name="words">
      <xsl:choose>
        <xsl:when test="count(sprite) = 2">
          <xsl:value-of select="concat('game.', $named, '.look == ')" />
          <xsl:call-template name="look-name"><xsl:with-param name="object" select="." /><xsl:with-param name="look" select="sprite[2]/@name" /></xsl:call-template>
          <xsl:text> ? </xsl:text>
          <xsl:call-template name="string"><xsl:with-param name="text" select="sprite[2]/text/content" /></xsl:call-template>
          <xsl:text> : </xsl:text>
          <xsl:call-template name="string"><xsl:with-param name="text" select="sprite[1]/text/content" /></xsl:call-template>
        </xsl:when>
        <xsl:when test="sprite/text/number">
          <xsl:text>std::to_string(</xsl:text>
          <xsl:call-template name="value"><xsl:with-param name="node" select="sprite/text/number" /><xsl:with-param name="self" select="." /><xsl:with-param name="in" select="'game.'" /></xsl:call-template>
          <xsl:text>)</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="string"><xsl:with-param name="text" select="sprite/text/content" /></xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="measured" select="contains(concat($x, $y), concat($named, 'Size'))" />
    <xsl:variable name="literal" select="starts-with($words, '&quot;')" />
    <xsl:variable name="shown">
      <xsl:choose>
        <xsl:when test="$measured and not($literal)"><xsl:value-of select="concat($named, 'Text')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="$words" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$measured and not($literal)">
      <line><xsl:value-of select="concat('const std::string ', $named, 'Text = ', $words, ';')" /></line>
    </xsl:if>
    <xsl:if test="$measured">
      <line><xsl:value-of select="concat('const sf::Vector2f ', $named, 'Size = picture.measure(', $shown, ', ', $style, ');')" /></line>
    </xsl:if>
    <line><xsl:value-of select="concat('picture.text(', $shown, ', ', $style, ', { ', $x, ', ', $y, ' });')" /></line>
  </xsl:template>

  <xsl:template name="framerate">
    <xsl:choose>
      <xsl:when test="/game/window/framerate"><xsl:value-of select="normalize-space(/game/window/framerate)" /></xsl:when>
      <xsl:otherwise>60</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:variable name="sfml-components">
    <xsl:text>Graphics</xsl:text>
    <xsl:if test="$sounds"> Audio</xsl:if>
  </xsl:variable>

  <xsl:template name="cmake-file">
    <xsl:text># CMakeLists.txt
# </xsl:text>
    <xsl:value-of select="concat($title, ', from ', $source, '.')" />
    <xsl:text>

cmake_minimum_required(VERSION 3.28)

project(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> LANGUAGES CXX)

# The built game, and any DLLs it needs, go to output/ (with a folder for the
# configuration, Debug or Release, under Visual Studio).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output")

# SFML 3 for the window: as installed (vcpkg install sfml), or else fetched
# and built here.
find_package(SFML 3 COMPONENTS </xsl:text>
    <xsl:value-of select="$sfml-components" />
    <xsl:text> QUIET)
if (NOT SFML_FOUND)
	message(STATUS "SFML 3 not found, using FetchContent to download and build it.")
	include(FetchContent)
	set(SFML_BUILD_AUDIO </xsl:text>
    <xsl:choose>
      <xsl:when test="$sounds">ON</xsl:when>
      <xsl:otherwise>OFF</xsl:otherwise>
    </xsl:choose>
    <xsl:text> CACHE BOOL "" FORCE)
	set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
	FetchContent_Declare(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 3.1.0 EXCLUDE_FROM_ALL SYSTEM)
	FetchContent_MakeAvailable(SFML)
endif()

add_executable(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>
	source/main.cpp
	source/</xsl:text>
    <xsl:value-of select="concat($name, '.h&#10;&#9;source/', $name, '.cpp&#10;')" />
    <xsl:value-of select="concat(')&#10;target_compile_features(', $name, ' PRIVATE cxx_std_20)&#10;target_link_libraries(', $name, ' PRIVATE SFML::Graphics')" />
    <xsl:if test="$sounds"> SFML::Audio</xsl:if>
    <xsl:text>)&#10;</xsl:text>
    <xsl:text>
# Every warning worth having, kept at zero.
target_compile_options(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE
	$&lt;$&lt;CXX_COMPILER_ID:MSVC&gt;:/W4
		/w14242 /w14254 /w14263 /w14265 /w14287 /w14296 /w14311 /w14545 /w14546 /w14547
		/w14549 /w14555 /w14619 /w14640 /w14826 /w14905 /w14906 /w14928
		/external:anglebrackets /external:W0 /analyze:external- /utf-8&gt;
	$&lt;$&lt;CXX_COMPILER_ID:GNU,Clang,AppleClang&gt;:-Wall -Wextra -Wpedantic -Wshadow -Wconversion
		-Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wunused
		-Wnull-dereference -Wformat=2 -Wimplicit-fallthrough
		-Wold-style-cast -Wmisleading-indentation&gt;
	$&lt;$&lt;CXX_COMPILER_ID:GNU&gt;:-Wdouble-promotion -Wduplicated-cond -Wduplicated-branches
		-Wlogical-op -Wuseless-cast&gt;)

</xsl:text>
    <xsl:value-of select="concat('set_property(TARGET ', $name, ' PROPERTY VS_DEBUGGER_WORKING_DIRECTORY &quot;$&lt;TARGET_FILE_DIR:', $name, '&gt;&quot;)&#10;')" />
    <xsl:text>
# Visual Studio starts the game when you press F5, rather than ALL_BUILD.
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>)

</xsl:text>
    <xsl:if test="$texts-drawn">
      <xsl:text>
# The font goes beside the program.
add_custom_command(TARGET </xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text> POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$&lt;TARGET_FILE_DIR:</xsl:text>
      <xsl:value-of select="$name" />
      <xsl:text>&gt;/assets")
</xsl:text>
    </xsl:if>
    <xsl:text>
# On Windows the program needs SFML's DLLs beside it when they are shared.
if (WIN32)
	add_custom_command(TARGET </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different "$&lt;TARGET_RUNTIME_DLLS:</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>&gt;" "$&lt;TARGET_FILE_DIR:</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>&gt;"
		COMMAND_EXPAND_LISTS)
endif()
</xsl:text>
  </xsl:template>

  <xsl:template name="readme-file">
    <xsl:value-of select="concat('# ', $title, '&#10;&#10;', $title, ' from `', $source, '`, written as a small program of its own on SFML 3.&#10;')" />
    <xsl:text>

| File | What |
|---|---|
| `source/</xsl:text>
    <xsl:value-of select="concat($name, '.h` | its numbers, its things and its class |&#10;| `source/', $name, '.cpp` | its rules: where things start, the keys, and a frame |&#10;')" />
    <xsl:text>| `source/main.cpp` | the window: keys in, a step of the time since the last frame, the picture</xsl:text>
    <xsl:if test="$sounds"> and the sounds</xsl:if>
    <xsl:text> out |
</xsl:text>
    <xsl:if test="$texts-drawn">
      <xsl:text>| `assets/tuffy.ttf` | the font, copied beside the program |
</xsl:text>
    </xsl:if>
    <xsl:text>
| `LICENSE` | the licence (MIT) |
| `CMakeLists.txt` | the build; it uses installed SFML 3, or downloads and builds it |
| `.gitignore` | leaves out `build/` and `output/` |

## Build and play

On Windows with Visual Studio and vcpkg (`vcpkg install sfml`), from this folder:

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
output\Release\</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.exe
```

Without vcpkg, leave out the toolchain file: SFML is downloaded and built with the game.
</xsl:text>
  </xsl:template>
</xsl:stylesheet>
