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
  <xsl:variable name="styles" select="/game/styles/style[@name = ($texts | $text-rows)/sprite/text/@style]" />
  <xsl:variable name="basic-colors" select="'|white|black|red|green|blue|yellow|magenta|cyan|'" />

  <!-- Sounds: plain ones are a square wave at the usual volume; the rest
       name their wave, slide or loudness. -->
  <xsl:variable name="plain-sounds" select="$sounds[not(@wave or note/@wave or note/@to or volume)]" />
  <xsl:variable name="shaped-sounds" select="$sounds[@wave or note/@wave or note/@to or volume]" />
  <xsl:variable name="wave-list">
    <xsl:if test="$plain-sounds or $sounds[not(@wave or note/@wave)]"><w>square</w></xsl:if>
    <xsl:for-each select="$sounds/@wave | $sounds/note/@wave">
      <xsl:if test=". != 'square'"><w><xsl:value-of select="." /></w></xsl:if>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="waves" select="exsl:node-set($wave-list)/w[not(. = preceding-sibling::w)]" />

  <!-- Grids whose kinds each have a colour of their own: an array of them by look. -->
  <xsl:variable name="colored-grids" select="$grids[count(group) &gt; 1][group/sprite/rectangle/color]" />

  <xsl:template name="main-file">
    <xsl:call-template name="header"><xsl:with-param name="file" select="'main.cpp'" /></xsl:call-template>
    <xsl:text>//
// The window: keys</xsl:text>
    <xsl:if test="$mouse-input"> and the mouse</xsl:if>
    <xsl:text> in, a step of the time since the last frame, the picture</xsl:text>
    <xsl:if test="$sounds"> and the sounds</xsl:if>
    <xsl:text> out.
</xsl:text>
    <xsl:value-of select="concat('&#10;#include &quot;', $name, '.h&quot;&#10;&#10;')" />
    <xsl:if test="$sounds"><xsl:text>#include &lt;SFML/Audio.hpp&gt;&#10;</xsl:text></xsl:if>
    <xsl:text>#include &lt;SFML/Graphics.hpp&gt;

#include &lt;algorithm&gt;
</xsl:text>
    <xsl:if test="$uses-pads or $colored-grids"><xsl:text>#include &lt;array&gt;&#10;</xsl:text></xsl:if>
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

		void box(sf::Vector2f position, sf::Vector2f extent, sf::Color color = sf::Color::White)
		{
			sf::RectangleShape shape(extent);
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
	void box(sf::RenderWindow&amp; window, sf::Vector2f position, sf::Vector2f extent, sf::Color color = sf::Color::White)
	{
		sf::RectangleShape shape(extent);
		shape.setPosition(position);
		shape.setFillColor(color);
		window.draw(shape);
	}
</xsl:text>
    </xsl:if>
    <xsl:for-each select="$colored-grids">
      <xsl:variable name="colors"><xsl:call-template name="grid-colors-name" /></xsl:variable>
      <xsl:text>&#10;&#9;// a</xsl:text>
      <xsl:if test="contains('aeiou', substring(@class, 1, 1))">n</xsl:if>
      <xsl:value-of select="concat(' ', @class)" />
      <xsl:text>'s colour by its look&#10;</xsl:text>
      <xsl:value-of select="concat('&#9;const std::array&lt;sf::Color, ', count(group), '&gt; ', $colors, '{ ')" />
      <xsl:for-each select="group">
        <xsl:if test="position() &gt; 1">, </xsl:if>
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space((sprite/rectangle/color | ../sprite/rectangle/color)[1])" /></xsl:call-template>
      </xsl:for-each>
      <xsl:text> };&#10;</xsl:text>
    </xsl:for-each>
    <xsl:if test="$screens">
      <xsl:call-template name="draw-functions" />
    </xsl:if>
    <xsl:if test="$sounds and not($shaped-sounds)">
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
    <xsl:if test="$shaped-sounds">
      <xsl:call-template name="shaped-tone" />
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
        <xsl:text>, </xsl:text>
        <xsl:call-template name="tone-call" />
        <xsl:text> },&#10;</xsl:text>
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
</xsl:text>
    <xsl:if test="$mouse-input">
      <xsl:text>			if (const auto* moved = event-&gt;getIf&lt;sf::Event::MouseMoved&gt;()) { game.mouse(window.mapPixelToCoords(moved-&gt;position).x); }&#10;</xsl:text>
    </xsl:if>
    <xsl:text>		}
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
      <xsl:when test="count(. | $grids) = count($grids)">
        <xsl:variable name="cell"><xsl:call-template name="cell-name" /></xsl:variable>
        <xsl:variable name="list"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:variable name="cell-color">
          <xsl:choose>
            <xsl:when test="count(. | $colored-grids) = count($colored-grids)">
              <xsl:text>, </xsl:text>
              <xsl:call-template name="grid-colors-name" />
              <xsl:value-of select="concat('[static_cast&lt;std::size_t&gt;(', $cell, '.look)]')" />
            </xsl:when>
            <xsl:when test="group/sprite/rectangle/color">
              <xsl:text>, </xsl:text>
              <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(group[1]/sprite/rectangle/color)" /></xsl:call-template>
            </xsl:when>
            <xsl:otherwise><xsl:value-of select="$color" /></xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <line>
          <xsl:text>for (const </xsl:text>
          <xsl:call-template name="cell-type" />
          <xsl:value-of select="concat('&amp; ', $cell, ' : game.', $list, ') { ', $box, $cell, '.position, ', $cell, '.size', $cell-color, '); }')" />
        </line>
      </xsl:when>
      <xsl:when test="self::group[member]">
        <xsl:for-each select="member"><xsl:call-template name="draw-lines" /></xsl:for-each>
      </xsl:when>
      <xsl:when test="count(. | $text-rows) = count($text-rows)">
        <xsl:call-template name="text-row-lines" />
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
    <xsl:variable name="base"><xsl:call-template name="camel"><xsl:with-param name="name" select="sprite[1]/text/@style" /></xsl:call-template></xsl:variable>
    <xsl:variable name="colored" select="count(sprite) = 2 and sprite/text/color" />
    <xsl:variable name="style">
      <xsl:choose>
        <xsl:when test="$colored"><xsl:value-of select="concat($named, 'Style')" /></xsl:when>
        <xsl:otherwise><xsl:call-template name="text-style"><xsl:with-param name="sprite" select="sprite[1]" /><xsl:with-param name="style" select="$base" /></xsl:call-template></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
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
    <xsl:if test="$colored">
      <line>
        <xsl:value-of select="concat('const Style ', $named, 'Style = game.', $named, '.look == ')" />
        <xsl:call-template name="look-name"><xsl:with-param name="object" select="." /><xsl:with-param name="look" select="sprite[2]/@name" /></xsl:call-template>
        <xsl:text> ? Style</xsl:text>
        <xsl:call-template name="text-style"><xsl:with-param name="sprite" select="sprite[2]" /><xsl:with-param name="style" select="$base" /><xsl:with-param name="always" select="true()" /></xsl:call-template>
        <xsl:text> : Style</xsl:text>
        <xsl:call-template name="text-style"><xsl:with-param name="sprite" select="sprite[1]" /><xsl:with-param name="style" select="$base" /><xsl:with-param name="always" select="true()" /></xsl:call-template>
        <xsl:text>;</xsl:text>
      </line>
    </xsl:if>
    <xsl:if test="$measured and not($literal)">
      <line><xsl:value-of select="concat('const std::string ', $named, 'Text = ', $words, ';')" /></line>
    </xsl:if>
    <xsl:if test="$measured">
      <line><xsl:value-of select="concat('const sf::Vector2f ', $named, 'Size = picture.measure(', $shown, ', ', $style, ');')" /></line>
    </xsl:if>
    <line><xsl:value-of select="concat('picture.text(', $shown, ', ', $style, ', { ', $x, ', ', $y, ' });')" /></line>
  </xsl:template>

  <!-- A text's style: the style it names, or that style's size in a colour
       of the text's own. -->
  <xsl:template name="text-style">
    <xsl:param name="sprite" />
    <xsl:param name="style" />
    <xsl:param name="always" select="false()" />
    <xsl:choose>
      <xsl:when test="$sprite/text/color or $always">
        <xsl:value-of select="concat('{ ', $style, '.size, ')" />
        <xsl:choose>
          <xsl:when test="$sprite/text/color"><xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space($sprite/text/color)" /></xsl:call-template></xsl:when>
          <xsl:otherwise><xsl:value-of select="concat($style, '.color')" /></xsl:otherwise>
        </xsl:choose>
        <xsl:text> }</xsl:text>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$style" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A group of texts in rows: each row's words, centred on themselves
       across, the first row placed as the group is and each next one the
       padding further down. -->
  <xsl:template name="text-row-lines">
    <xsl:variable name="group" select="." />
    <xsl:variable name="named"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="style"><xsl:call-template name="camel"><xsl:with-param name="name" select="sprite/text/@style" /></xsl:call-template></xsl:variable>
    <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="size" select="concat($named, 'Size.')" />
    <xsl:variable name="first" select="concat($named, '1Size.')" />
    <xsl:variable name="padding" select="padding/y" />
    <xsl:for-each select="row">
      <xsl:sort select="@number" data-type="number" />
      <xsl:variable name="number" select="normalize-space(@number)" />
      <xsl:variable name="row-size" select="concat($named, $number, 'Size.')" />
      <xsl:variable name="words"><xsl:call-template name="string"><xsl:with-param name="text" select="sprite/text/content" /></xsl:call-template></xsl:variable>
      <xsl:variable name="at-x"><xsl:call-template name="replace"><xsl:with-param name="text" select="$x" /><xsl:with-param name="from" select="$size" /><xsl:with-param name="to" select="$row-size" /></xsl:call-template></xsl:variable>
      <xsl:variable name="at-y">
        <xsl:call-template name="replace"><xsl:with-param name="text" select="$y" /><xsl:with-param name="from" select="$size" /><xsl:with-param name="to" select="$first" /></xsl:call-template>
        <xsl:if test="$number &gt; 1 and $padding">
          <xsl:text> + </xsl:text>
          <xsl:choose>
            <xsl:when test="string(number($padding)) != 'NaN'"><xsl:value-of select="($number - 1) * number($padding)" /></xsl:when>
            <xsl:otherwise>
              <xsl:if test="$number &gt; 2"><xsl:value-of select="concat($number - 1, ' * ')" /></xsl:if>
              <xsl:call-template name="value"><xsl:with-param name="node" select="$padding" /><xsl:with-param name="self" select="$group" /><xsl:with-param name="wrap" select="3" /></xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:if>
      </xsl:variable>
      <xsl:if test="contains(concat($at-x, $at-y), concat($named, $number, 'Size'))">
        <line><xsl:value-of select="concat('const sf::Vector2f ', $named, $number, 'Size = picture.measure(', $words, ', ', $style, ');')" /></line>
      </xsl:if>
      <line><xsl:value-of select="concat('picture.text(', $words, ', ', $style, ', { ', $at-x, ', ', $at-y, ' });')" /></line>
    </xsl:for-each>
  </xsl:template>

  <!-- Every `from` in `text` written as `to`. -->
  <xsl:template name="replace">
    <xsl:param name="text" />
    <xsl:param name="from" />
    <xsl:param name="to" />
    <xsl:choose>
      <xsl:when test="contains($text, $from)">
        <xsl:value-of select="concat(substring-before($text, $from), $to)" />
        <xsl:call-template name="replace"><xsl:with-param name="text" select="substring-after($text, $from)" /><xsl:with-param name="from" select="$from" /><xsl:with-param name="to" select="$to" /></xsl:call-template>
      </xsl:when>
      <xsl:otherwise><xsl:value-of select="$text" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- brickColors: a grid's colours by look. -->
  <xsl:template name="grid-colors-name">
    <xsl:call-template name="camel">
      <xsl:with-param name="name">
        <xsl:choose>
          <xsl:when test="@class"><xsl:value-of select="@class" /></xsl:when>
          <xsl:otherwise><xsl:call-template name="singular"><xsl:with-param name="name" select="@name" /></xsl:call-template></xsl:otherwise>
        </xsl:choose>
      </xsl:with-param>
    </xsl:call-template>
    <xsl:text>Colors</xsl:text>
  </xsl:template>

  <!-- tone(220, 0.03), or tone(Wave::Triangle, 329.63, 82.41, 0.4, 0.4):
       a sound's note, its wave, slide and loudness when it has any. -->
  <xsl:template name="tone-call">
    <xsl:variable name="hertz"><xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(note/@pitch)" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="count(. | $plain-sounds) = count($plain-sounds)">
        <xsl:value-of select="concat('tone(', $hertz, ', ', normalize-space(note), ')')" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="wave" select="(note/@wave | @wave)[1]" />
        <xsl:text>tone(Wave::</xsl:text>
        <xsl:choose>
          <xsl:when test="$wave"><xsl:call-template name="type-word"><xsl:with-param name="name" select="$wave" /></xsl:call-template></xsl:when>
          <xsl:otherwise>Square</xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="concat(', ', $hertz, ', ')" />
        <xsl:choose>
          <xsl:when test="note/@to"><xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(note/@to)" /></xsl:call-template></xsl:when>
          <xsl:otherwise><xsl:value-of select="$hertz" /></xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="concat(', ', normalize-space(note), ', ')" />
        <xsl:choose>
          <xsl:when test="volume"><xsl:value-of select="normalize-space(volume)" /></xsl:when>
          <xsl:otherwise>0.3</xsl:otherwise>
        </xsl:choose>
        <xsl:text>)</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The waves the sounds use, a tone of any of them sliding in pitch, and
       a plain square one when some sounds are plain. -->
  <xsl:template name="shaped-tone">
    <xsl:text>
	enum class Wave { </xsl:text>
    <xsl:for-each select="$waves">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="type-word"><xsl:with-param name="name" select="." /></xsl:call-template>
    </xsl:for-each>
    <xsl:text> };

	// How high the wave is, -1 to 1, at `phase` (0 to 1) through one cycle.
	double height(Wave wave, double phase)
	{
</xsl:text>
    <xsl:for-each select="$waves[position() &gt; 1]">
      <xsl:text>&#9;&#9;if (wave == Wave::</xsl:text>
      <xsl:call-template name="type-word"><xsl:with-param name="name" select="." /></xsl:call-template>
      <xsl:text>) { return </xsl:text>
      <xsl:call-template name="wave-height" />
      <xsl:text>; }&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>&#9;&#9;</xsl:text>
    <xsl:if test="count($waves) = 1">static_cast&lt;void&gt;(wave);&#10;&#9;&#9;</xsl:if>
    <xsl:text>return </xsl:text>
    <xsl:for-each select="$waves[1]"><xsl:call-template name="wave-height" /></xsl:for-each>
    <xsl:text>;
	}

	// A wave sliding evenly in pitch from `hertz` to `toHertz`, faded in and
	// out over two thousandths of a second so it doesn't click.
	sf::SoundBuffer tone(Wave wave, double hertz, double toHertz, double seconds, double volume)
	{
		constexpr unsigned rate = 44100;
		const std::size_t count = static_cast&lt;std::size_t&gt;(seconds * rate);
		const std::size_t fade = std::min&lt;std::size_t&gt;(count / 4, rate / 500);

		std::vector&lt;std::int16_t&gt; samples(count);
		double phase = 0.0;
		for (std::size_t i = 0; i &lt; count; ++i)
		{
			double level = height(wave, phase) * volume;
			if (i &lt; fade) { level *= static_cast&lt;double&gt;(i) / static_cast&lt;double&gt;(fade); }
			if (i &gt;= count - fade) { level *= static_cast&lt;double&gt;(count - 1 - i) / static_cast&lt;double&gt;(fade); }
			samples[i] = static_cast&lt;std::int16_t&gt;(std::lround(level * 32767));

			const double through = static_cast&lt;double&gt;(i) / static_cast&lt;double&gt;(count);
			phase += hertz * std::pow(toHertz / hertz, through) / rate;
			phase -= std::floor(phase);
		}

		sf::SoundBuffer buffer;
		if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, rate, { sf::SoundChannel::Mono }))
		{
			std::cerr &lt;&lt; "warning: could not make a sound\n";
		}
		return buffer;
	}
</xsl:text>
    <xsl:if test="$plain-sounds">
      <xsl:text>
	// A plain tone: a square wave at the usual loudness.
	sf::SoundBuffer tone(double hertz, double seconds)
	{
		return tone(Wave::Square, hertz, hertz, seconds, 0.3);
	}
</xsl:text>
    </xsl:if>
  </xsl:template>

  <xsl:template name="wave-height">
    <xsl:choose>
      <xsl:when test=". = 'triangle'">phase &lt; 0.25 ? 4 * phase : phase &lt; 0.75 ? 2 - 4 * phase : 4 * phase - 4</xsl:when>
      <xsl:when test=". = 'sawtooth'">2 * phase - 1</xsl:when>
      <xsl:when test=". = 'sine'">std::sin(2 * 3.14159265358979 * phase)</xsl:when>
      <xsl:otherwise>phase &lt; 0.5 ? 1.0 : -1.0</xsl:otherwise>
    </xsl:choose>
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
    <xsl:text>| `source/main.cpp` | the window: keys</xsl:text>
    <xsl:if test="$mouse-input"> and the mouse</xsl:if>
    <xsl:text> in, a step of the time since the last frame, the picture</xsl:text>
    <xsl:if test="$sounds"> and the sounds</xsl:if>
    <xsl:text> out |
</xsl:text>
    <xsl:if test="$texts-drawn">
      <xsl:text>| `assets/tuffy.ttf` | the font, copied beside the program |
</xsl:text>
    </xsl:if>
    <xsl:text>| `LICENSE` | the licence (MIT) |
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
