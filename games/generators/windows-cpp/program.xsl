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
  <xsl:variable name="plain-sounds" select="$sounds[not(@wave or note/@wave or note/@to or volume) and count(note) = 1]" />
  <xsl:variable name="shaped-sounds" select="$sounds[count(. | $plain-sounds) != count($plain-sounds)]" />
  <xsl:variable name="wave-list">
    <xsl:if test="$plain-sounds or $sounds[not(@wave)]/note[not(@wave)] or $sounds[not(@wave)]/part[not(@wave)]/note[not(@wave)] or ($sounds/@wave | $sounds//note/@wave | $sounds/part/@wave) = 'square'"><w>square</w></xsl:if>
    <xsl:for-each select="$sounds/@wave | $sounds/part/@wave | $sounds//note/@wave">
      <xsl:if test=". != 'square'"><w><xsl:value-of select="." /></w></xsl:if>
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="waves" select="exsl:node-set($wave-list)/w[not(. = preceding-sibling::w)]" />

  <!-- Pictures with colours of their own, or their style's: a palette each,
       Colors, by the picture's name or the style's. -->
  <xsl:variable name="looked-grids" select="$looking-grids[group][(sprite | group/sprite)/bitmap]" />
  <xsl:variable name="colored-pictures" select="$pictures[color or @style = /game/styles/style[color]/@name]
      [@name = ($everything | $fired)/sprite/bitmap/@from or @name = $everything/parent::group/sprite/bitmap/@from
        or @name = $grids[count(. | $looked-grids) != count($looked-grids)]/sprite/bitmap/@from or @name = $marching/row/sprite/bitmap/@from]" />
  <!-- Pictures drawn in colours: a palette a picture, or a grid's by look. -->
  <xsl:variable name="painted" select="boolean($colored-pictures | $looked-grids)" />

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
    <xsl:if test="$uses-pads or $colored-grids or $looked-grids"><xsl:text>#include &lt;array&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$sounds"><xsl:text>#include &lt;cmath&gt;&#10;#include &lt;cstdint&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$texts-drawn"><xsl:text>#include &lt;filesystem&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$texts-drawn or $sounds"><xsl:text>#include &lt;iostream&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$sounds or $painted"><xsl:text>#include &lt;map&gt;&#10;</xsl:text></xsl:if>
    <xsl:text>#include &lt;optional&gt;&#10;</xsl:text>
    <xsl:if test="$waves = 'noise'"><xsl:text>#include &lt;random&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$texts-drawn"><xsl:text>#include &lt;string&gt;&#10;</xsl:text></xsl:if>
    <xsl:if test="$sounds"><xsl:text>#include &lt;vector&gt;&#10;</xsl:text></xsl:if>
    <xsl:text>
namespace
{
	using namespace </xsl:text>
    <xsl:call-template name="namespace-name" />
    <xsl:text>;&#10;</xsl:text>
    <xsl:call-template name="color-constants" />
    <xsl:if test="$painted">
      <xsl:text>
	// A picture's colours: a letter's own, or the rest's.
	struct Colors
	{
		sf::Color rest{ sf::Color::White };
		std::map&lt;char, sf::Color&gt; letters;
	};
</xsl:text>
    </xsl:if>
    <xsl:if test="$looked-grids">
      <xsl:text>
	// How one look is drawn: its picture's rows, in its colours.
	struct Drawing
	{
		const std::vector&lt;std::string&gt;&amp; rows;
		Colors colors;
	};
</xsl:text>
    </xsl:if>
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
</xsl:text>
      <xsl:if test="$cell-maps//circle">
        <xsl:text>
		void disc(sf::Vector2f middle, float radius, sf::Color color = sf::Color::White)
		{
			sf::CircleShape shape(radius);
			shape.setPosition(middle - sf::Vector2f{ radius, radius });
			shape.setFillColor(color);
			window.draw(shape);
		}
</xsl:text>
      </xsl:if>
      <xsl:if test="$pictures and not($painted)">
        <xsl:text>
		// The lit pixels of a picture's rows over the box at `position`, `extent` big.
		void pixels(const std::vector&lt;std::string&gt;&amp; rows, sf::Vector2f position, sf::Vector2f extent, sf::Color color = sf::Color::White)
		{
			const sf::Vector2f pixelSize{ extent.x / static_cast&lt;float&gt;(rows.front().size()), extent.y / static_cast&lt;float&gt;(rows.size()) };
			for (std::size_t row = 0; row &lt; rows.size(); ++row)
			{
				for (std::size_t column = 0; column &lt; rows[row].size(); ++column)
				{
					const sf::Vector2f corner{ static_cast&lt;float&gt;(column) * pixelSize.x, static_cast&lt;float&gt;(row) * pixelSize.y };
					if (rows[row][column] != '.') { box(position + corner, pixelSize, color); }
				}
			}
		}
</xsl:text>
      </xsl:if>
      <xsl:if test="$painted">
        <xsl:text>
		// The lit pixels of a picture's rows over the box at `position`, `extent`
		// big, each its letter's colour.
		void pixels(const std::vector&lt;std::string&gt;&amp; rows, sf::Vector2f position, sf::Vector2f extent, const Colors&amp; colors = {})
		{
			const sf::Vector2f pixelSize{ extent.x / static_cast&lt;float&gt;(rows.front().size()), extent.y / static_cast&lt;float&gt;(rows.size()) };
			for (std::size_t row = 0; row &lt; rows.size(); ++row)
			{
				for (std::size_t column = 0; column &lt; rows[row].size(); ++column)
				{
					const char letter = rows[row][column];
					if (letter == '.') { continue; }
					const sf::Vector2f corner{ static_cast&lt;float&gt;(column) * pixelSize.x, static_cast&lt;float&gt;(row) * pixelSize.y };
					const auto own = colors.letters.find(letter);
					box(position + corner, pixelSize, own != colors.letters.end() ? own-&gt;second : colors.rest);
				}
			}
		}
</xsl:text>
      </xsl:if>
      <xsl:text>
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
    <xsl:if test="not($texts-drawn) and $painted">
      <xsl:message terminate="yes">windows-cpp cannot write pictures of their own colours in a game with no words yet; use engine-cpp for this game</xsl:message>
    </xsl:if>
    <xsl:if test="not($texts-drawn) and $cell-maps//circle">
      <xsl:message terminate="yes">windows-cpp cannot write circles in a game with no words yet; use engine-cpp for this game</xsl:message>
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
      <xsl:if test="$pictures">
        <xsl:text>
	// The lit pixels of a picture's rows over the box at `position`, `extent` big.
	void pixels(sf::RenderWindow&amp; window, const std::vector&lt;std::string&gt;&amp; rows, sf::Vector2f position, sf::Vector2f extent, sf::Color color = sf::Color::White)
	{
		const sf::Vector2f pixelSize{ extent.x / static_cast&lt;float&gt;(rows.front().size()), extent.y / static_cast&lt;float&gt;(rows.size()) };
		for (std::size_t row = 0; row &lt; rows.size(); ++row)
		{
			for (std::size_t column = 0; column &lt; rows[row].size(); ++column)
			{
				const sf::Vector2f corner{ static_cast&lt;float&gt;(column) * pixelSize.x, static_cast&lt;float&gt;(row) * pixelSize.y };
				if (rows[row][column] != '.') { box(window, position + corner, pixelSize, color); }
			}
		}
	}
</xsl:text>
      </xsl:if>
    </xsl:if>
    <xsl:call-template name="map-place-functions" />
    <xsl:if test="$colored-pictures">
      <xsl:call-template name="palettes" />
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
    <xsl:for-each select="$looked-grids">
      <xsl:call-template name="grid-drawings" />
    </xsl:for-each>
    <xsl:if test="$screens">
      <xsl:call-template name="draw-functions" />
    </xsl:if>
    <xsl:if test="$sounds and not($shaped-sounds)">
      <xsl:text>
	// A square wave, faded in and out over two thousandths of a second so it
	// doesn't click.
	sf::SoundBuffer tone(double hertz, double duration)
	{
		constexpr unsigned rate = 44100;
		constexpr double volume = 0.3;
		const std::size_t count = static_cast&lt;std::size_t&gt;(duration * rate);
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
    <xsl:if test="$mouse-input[@axis = 'mouse.x']">
      <xsl:text>			if (const auto* moved = event-&gt;getIf&lt;sf::Event::MouseMoved&gt;()) { game.mouse(window.mapPixelToCoords(moved-&gt;position).x); }&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="$stick-input">
      <xsl:text>			// pad 1's stick across, -100 to 100, is the mouse across the window
			if (const auto* stick = event-&gt;getIf&lt;sf::Event::JoystickMoved&gt;(); stick &amp;&amp; stick-&gt;joystickId == 0 &amp;&amp; stick-&gt;axis == sf::Joystick::Axis::X)
			{
				game.mouse((stick-&gt;position / 100 + 1) / 2 * windowWidth);
			}
</xsl:text>
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
		window.clear(</xsl:text>
    <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(/game/window/background)" /><xsl:with-param name="none" select="'sf::Color::Black'" /></xsl:call-template>
    <xsl:text>);
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
    <xsl:variable name="used" select="(//rectangle/color | //circle/color | //text/color | $styles/color | /game/window/background | $colored-pictures/color
        | /game/styles/style[@name = $colored-pictures/@style]/color
        | /game/pictures/bitmap[@name = $looked-grids/descendant::bitmap/@from]/color | $looked-grids/descendant::bitmap/color)[not(contains($basic-colors, concat('|', normalize-space(.), '|')))]" />
    <xsl:variable name="list">
      <xsl:for-each select="$used">
        <c><xsl:value-of select="normalize-space(.)" /></c>
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
    <xsl:param name="none" select="'sf::Color::White'" />
    <xsl:choose>
      <xsl:when test="$color = ''"><xsl:value-of select="$none" /></xsl:when>
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

  <!-- const Colors bomberColors{ sf::Color::White, { { 'k', sf::Color::Black } } }:
       each picture's palette, or its style's, once. -->
  <xsl:template name="palettes">
    <xsl:text>&#10;</xsl:text>
    <xsl:for-each select="$colored-pictures">
      <xsl:variable name="named"><xsl:call-template name="palette-name" /></xsl:variable>
      <xsl:variable name="earlier">
        <xsl:for-each select="preceding-sibling::bitmap[count(. | $colored-pictures) = count($colored-pictures)]">
          <xsl:variable name="each"><xsl:call-template name="palette-name" /></xsl:variable>
          <xsl:if test="$each = $named">x</xsl:if>
        </xsl:for-each>
      </xsl:variable>
      <xsl:if test="$earlier = ''">
        <xsl:variable name="style" select="/game/styles/style[@name = current()/@style]" />
        <xsl:variable name="colors" select="color | $style/color[not(@char = current()/color/@char)]" />
        <xsl:value-of select="concat('&#9;const Colors ', $named, '{ ')" />
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space((color[not(@char)] | $style/color[not(@char)])[1])" /></xsl:call-template>
        <xsl:text>, {</xsl:text>
        <xsl:for-each select="$colors[@char]">
          <xsl:if test="position() &gt; 1">,</xsl:if>
          <xsl:value-of select="concat(' { ', $quote, @char, $quote, ', ')" />
          <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(.)" /></xsl:call-template>
          <xsl:text> }</xsl:text>
        </xsl:for-each>
        <xsl:text> } };&#10;</xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <!-- trafficDrawings: a grid's picture and colours by look, each kind's
       colour in place of its picture's own. -->
  <xsl:template name="grid-drawings">
    <xsl:variable name="grid" select="." />
    <xsl:text>&#10;&#9;// a</xsl:text>
    <xsl:if test="contains('aeiou', substring(@class, 1, 1))">n</xsl:if>
    <xsl:value-of select="concat(' ', (@class | @name[not(../@class)]), &quot;'s picture and colours by its look&#10;&quot;)" />
    <xsl:variable name="named"><xsl:call-template name="grid-looks" /></xsl:variable>
    <xsl:variable name="looks" select="exsl:node-set($named)/look" />
    <xsl:value-of select="concat('&#9;const std::array&lt;Drawing, ', count($looks), '&gt; ')" />
    <xsl:call-template name="grid-drawings-name" />
    <xsl:text>{ {</xsl:text>
    <xsl:for-each select="$looks">
      <xsl:variable name="look" select="." />
      <xsl:for-each select="$grid/group[generate-id() = $look/@kind]">
        <xsl:variable name="sprite" select="sprite[generate-id() = $look/@sprite] | self::*[not(sprite)]/../sprite[1]" />
        <xsl:variable name="picture"><xsl:call-template name="kind-picture"><xsl:with-param name="sprite" select="$sprite" /></xsl:call-template></xsl:variable>
        <xsl:variable name="bitmap" select="/game/pictures/bitmap[@name = $picture]" />
        <xsl:variable name="colors"><xsl:call-template name="kind-colors"><xsl:with-param name="sprite" select="$sprite" /></xsl:call-template></xsl:variable>
        <xsl:if test="$look/preceding-sibling::look">,</xsl:if>
        <xsl:text>&#10;&#9;&#9;{ </xsl:text>
        <xsl:call-template name="picture-name"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template>
        <xsl:text>, { </xsl:text>
        <xsl:call-template name="color"><xsl:with-param name="color" select="string(exsl:node-set($colors)/rest)" /></xsl:call-template>
        <xsl:text>, {</xsl:text>
        <xsl:for-each select="exsl:node-set($colors)/letter">
          <xsl:if test="position() &gt; 1">,</xsl:if>
          <xsl:value-of select="concat(' { ', $quote, @char, $quote, ', ')" />
          <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(.)" /></xsl:call-template>
          <xsl:text> }</xsl:text>
        </xsl:for-each>
        <xsl:text> } } }</xsl:text>
      </xsl:for-each>
    </xsl:for-each>
    <xsl:text>&#10;&#9;} };&#10;</xsl:text>
  </xsl:template>

  <xsl:template name="grid-drawings-name">
    <xsl:call-template name="camel"><xsl:with-param name="name" select="concat((@class | @name[not(../@class)]), ' drawings')" /></xsl:call-template>
  </xsl:template>


  <!-- drawTitle(), drawPlaying() ...: each screen's objects, then those it
       shows; draw(): the screen on top, over those it shows still. -->
  <xsl:template name="draw-functions">
    <xsl:for-each select="$states">
      <xsl:variable name="state" select="." />
      <xsl:variable name="drawn"><xsl:call-template name="state-draw-lines" /></xsl:variable>
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

  <!-- What a screen draws: for the one played, what its scene takes; its
       objects, the shared ones it shows; then what is in flight. -->
  <xsl:template name="state-draw-lines">
    <xsl:variable name="state" select="." />
    <xsl:variable name="played-here" select="count(. | $played) = 1" />
    <xsl:if test="$played-here">
      <xsl:for-each select="$taken">
        <xsl:call-template name="draw-lines" />
      </xsl:for-each>
    </xsl:if>
    <xsl:for-each select="objects/*">
      <xsl:call-template name="draw-lines" />
    </xsl:for-each>
    <xsl:for-each select="$shared[@name = $state/shows/show/@object]">
      <xsl:call-template name="draw-lines" />
    </xsl:for-each>
    <xsl:if test="$played-here">
      <xsl:for-each select="$fired">
        <xsl:call-template name="draw-lines" />
      </xsl:for-each>
    </xsl:if>
  </xsl:template>

  <!-- drawPlaying(picture, game); when that screen draws anything. -->
  <xsl:template name="draw-call">
    <xsl:variable name="state" select="." />
    <xsl:variable name="drawn"><xsl:call-template name="state-draw-lines" /></xsl:variable>
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
    <xsl:param name="in-play" select="false()" />
    <xsl:variable name="object" select="." />
    <xsl:variable name="sprite" select="(sprite | parent::group/sprite)[1]" />
    <xsl:variable name="color">
      <xsl:if test="$sprite/rectangle/color">
        <xsl:text>, </xsl:text>
        <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space($sprite/rectangle/color)" /></xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="count(. | $dying) = count($dying) and not($in-play)">
        <!-- only while it is in play -->
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:variable name="lines"><xsl:call-template name="draw-lines"><xsl:with-param name="in-play" select="true()" /></xsl:call-template></xsl:variable>
        <xsl:call-template name="block-lines">
          <xsl:with-param name="header" select="concat('if (game.', $thing, '.inPlay)')" />
          <xsl:with-param name="lines" select="exsl:node-set($lines)/line" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="count(. | $marching) = count($marching)">
        <xsl:call-template name="march-draw-lines" />
      </xsl:when>
      <xsl:when test="self::map">
        <xsl:call-template name="map-draw-lines" />
      </xsl:when>
      <xsl:when test="count(. | $holed) = count($holed)">
        <!-- each with its own picture, holes and all -->
        <xsl:variable name="cell"><xsl:call-template name="cell-name" /></xsl:variable>
        <xsl:variable name="list"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:variable name="bitmap" select="$pictures[@name = current()/sprite/bitmap/@from]" />
        <line>
          <xsl:text>for (const </xsl:text>
          <xsl:call-template name="cell-type" />
          <xsl:value-of select="concat('&amp; ', $cell, ' : game.', $list, ') { ', substring-before($box, 'box('), 'pixels(', $cell, '.picture, ', $cell, '.position, ', $cell, '.size')" />
          <xsl:if test="count($bitmap | $colored-pictures) = count($colored-pictures)">
            <xsl:text>, </xsl:text>
            <xsl:call-template name="palette-name"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template>
          </xsl:if>
          <xsl:text>); }</xsl:text>
        </line>
      </xsl:when>
      <xsl:when test="count(. | $screen-animated) = count($screen-animated)">
        <!-- its picture by how long its screen has been on top -->
        <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
        <xsl:variable name="size"><xsl:call-template name="size" /></xsl:variable>
        <xsl:variable name="time"><xsl:call-template name="screen-time"><xsl:with-param name="state" select="../.." /></xsl:call-template></xsl:variable>
        <line>
          <xsl:value-of select="concat('const int frame = static_cast&lt;int&gt;(game.', $time, ' / ')" />
          <xsl:call-template name="value"><xsl:with-param name="node" select="animation/seconds" /><xsl:with-param name="self" select="." /><xsl:with-param name="in" select="'game.'" /><xsl:with-param name="wrap" select="4" /></xsl:call-template>
          <xsl:value-of select="concat(') % ', count(animation/frame), ';')" />
        </line>
        <xsl:variable name="frames">
          <xsl:for-each select="animation/frame[position() &gt; 1]">
            <look test="frame == {position()}" at="{count(../../sprite[@name = current()/@sprite]/preceding-sibling::sprite) + 1}" />
          </xsl:for-each>
          <look at="{count(sprite[@name = current()/animation/frame[1]/@sprite]/preceding-sibling::sprite) + 1}" />
        </xsl:variable>
        <xsl:variable name="looks" select="exsl:node-set($frames)/look" />
        <xsl:variable name="colors"><xsl:call-template name="look-choice"><xsl:with-param name="looks" select="$looks" /><xsl:with-param name="part" select="'colors'" /></xsl:call-template></xsl:variable>
        <line>
          <xsl:value-of select="concat(substring-before($box, 'box('), 'pixels(')" />
          <xsl:call-template name="look-choice"><xsl:with-param name="looks" select="$looks" /><xsl:with-param name="part" select="'picture'" /></xsl:call-template>
          <xsl:choose>
            <xsl:when test="substring($x, string-length($x) - 1) = '.x' and substring($x, 1, string-length($x) - 2) = substring($y, 1, string-length($y) - 2)">
              <!-- where another thing is -->
              <xsl:value-of select="concat(', ', substring($x, 1, string-length($x) - 2), ', ', $size)" />
            </xsl:when>
            <xsl:otherwise><xsl:value-of select="concat(', { ', $x, ', ', $y, ' }, ', $size)" /></xsl:otherwise>
          </xsl:choose>
          <xsl:if test="$colors != ''"><xsl:value-of select="concat(', ', $colors)" /></xsl:if>
          <xsl:text>);</xsl:text>
        </line>
      </xsl:when>
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
        <xsl:choose>
          <xsl:when test="not(group) and count(sprite) &gt; 1">
            <!-- each look of its own, the first when it has none of the others -->
            <xsl:variable name="draws">
              <xsl:for-each select="sprite">
                <xsl:variable name="bitmap" select="$pictures[@name = current()/bitmap/@from]" />
                <d>
                  <xsl:choose>
                    <xsl:when test="$bitmap">
                      <xsl:value-of select="concat(substring-before($box, 'box('), 'pixels(')" />
                      <xsl:call-template name="picture-name"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template>
                      <xsl:value-of select="concat(', ', $cell, '.position, ', $cell, '.size')" />
                      <xsl:if test="count($bitmap | $colored-pictures) = count($colored-pictures)">
                        <xsl:text>, </xsl:text>
                        <xsl:call-template name="palette-name"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template>
                      </xsl:if>
                      <xsl:text>);</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                      <xsl:value-of select="concat($box, $cell, '.position, ', $cell, '.size, ')" />
                      <xsl:call-template name="color"><xsl:with-param name="color" select="normalize-space(rectangle/color)" /></xsl:call-template>
                      <xsl:text>);</xsl:text>
                    </xsl:otherwise>
                  </xsl:choose>
                </d>
              </xsl:for-each>
            </xsl:variable>
            <xsl:variable name="each" select="exsl:node-set($draws)/d" />
            <line>
              <xsl:text>for (const </xsl:text>
              <xsl:call-template name="cell-type" />
              <xsl:value-of select="concat('&amp; ', $cell, ' : game.', $list, ')')" />
            </line>
            <line>{</line>
            <xsl:for-each select="sprite[position() &gt; 1]">
              <xsl:variable name="at" select="position() + 1" />
              <line depth="1">
                <xsl:if test="position() &gt; 1">else </xsl:if>
                <xsl:value-of select="concat('if (', $cell, '.look == ')" />
                <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@name" /></xsl:call-template>
                <xsl:value-of select="concat(') { ', $each[$at], ' }')" />
              </line>
            </xsl:for-each>
            <line depth="1"><xsl:value-of select="concat('else { ', $each[1], ' }')" /></line>
            <line>}</line>
          </xsl:when>
          <xsl:when test="count(. | $looked-grids) = count($looked-grids)">
            <xsl:variable name="drawings"><xsl:call-template name="grid-drawings-name" /></xsl:variable>
            <line>
              <xsl:text>for (const </xsl:text>
              <xsl:call-template name="cell-type" />
              <xsl:value-of select="concat('&amp; ', $cell, ' : game.', $list, ')')" />
            </line>
            <line>{</line>
            <line><xsl:value-of select="concat('&#9;const Drawing&amp; drawing = ', $drawings, '[static_cast&lt;std::size_t&gt;(', $cell, '.look)];')" /></line>
            <line><xsl:value-of select="concat('&#9;picture.pixels(drawing.rows, ', $cell, '.position, ', $cell, '.size, drawing.colors);')" /></line>
            <line>}</line>
          </xsl:when>
          <xsl:otherwise>
            <line>
              <xsl:text>for (const </xsl:text>
              <xsl:call-template name="cell-type" />
              <xsl:value-of select="concat('&amp; ', $cell, ' : game.', $list, ') { ', $box, $cell, '.position, ', $cell, '.size', $cell-color, '); }')" />
            </line>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="self::group[member]">
        <xsl:for-each select="member"><xsl:call-template name="draw-lines" /></xsl:for-each>
      </xsl:when>
      <xsl:when test="count(. | $text-rows) = count($text-rows)">
        <xsl:call-template name="text-row-lines" />
      </xsl:when>
      <xsl:when test="count(. | $fired) = count($fired)">
        <!-- each in flight, its picture by how long it has flown -->
        <xsl:variable name="one"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:variable name="type"><xsl:call-template name="fired-type" /></xsl:variable>
        <xsl:variable name="list"><xsl:call-template name="fired-list-name" /></xsl:variable>
        <xsl:variable name="draws">
          <xsl:choose>
            <xsl:when test="animation/once">
              <!-- each picture until its time is up, the last after -->
              <xsl:variable name="frames">
                <xsl:for-each select="animation/frame">
                  <xsl:variable name="end" select="count((preceding-sibling::frame | .)[not(seconds)]) * number(../seconds) + sum((preceding-sibling::frame | .)/seconds)" />
                  <look at="{count(../../sprite[@name = current()/@sprite]/preceding-sibling::sprite) + 1}">
                    <xsl:attribute name="test">
                      <xsl:value-of select="concat($one, '.age &lt; ', $end)" />
                      <xsl:if test="floor($end) != $end">f</xsl:if>
                    </xsl:attribute>
                  </look>
                </xsl:for-each>
              </xsl:variable>
              <line><xsl:call-template name="pixels-call"><xsl:with-param name="looks" select="exsl:node-set($frames)/look" /><xsl:with-param name="thing" select="$one" /><xsl:with-param name="ordered" select="true()" /></xsl:call-template></line>
            </xsl:when>
            <xsl:when test="animation">
              <line>
                <xsl:value-of select="concat('const int frame = static_cast&lt;int&gt;(', $one, '.age / ')" />
                <xsl:call-template name="value"><xsl:with-param name="node" select="animation/seconds" /><xsl:with-param name="self" select="." /><xsl:with-param name="in" select="'game.'" /><xsl:with-param name="wrap" select="4" /></xsl:call-template>
                <xsl:value-of select="concat(') % ', count(animation/frame), ';')" />
              </line>
              <xsl:variable name="frames">
                <xsl:for-each select="animation/frame">
                  <look test="frame == {position() - 1}" at="{count(../../sprite[@name = current()/@sprite]/preceding-sibling::sprite) + 1}" />
                </xsl:for-each>
              </xsl:variable>
              <line><xsl:call-template name="pixels-call"><xsl:with-param name="looks" select="exsl:node-set($frames)/look" /><xsl:with-param name="thing" select="$one" /></xsl:call-template></line>
            </xsl:when>
            <xsl:when test="$sprite/rectangle">
              <line><xsl:value-of select="concat($box, $one, '.position, ', $one, '.size', $color, ');')" /></line>
            </xsl:when>
            <xsl:otherwise>
              <xsl:variable name="frames"><look at="1" /></xsl:variable>
              <line><xsl:call-template name="pixels-call"><xsl:with-param name="looks" select="exsl:node-set($frames)/look" /><xsl:with-param name="thing" select="$one" /></xsl:call-template></line>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>
        <line><xsl:value-of select="concat('for (const ', $type, '&amp; ', $one, ' : game.', $list, ')')" /></line>
        <line>{</line>
        <xsl:for-each select="exsl:node-set($draws)/line">
          <line depth="1"><xsl:value-of select="." /></line>
        </xsl:for-each>
        <line>}</line>
      </xsl:when>
      <xsl:when test="count(. | $things) = count($things) and $pictures[@name = $sprite/bitmap/@from]">
        <xsl:variable name="thing"><xsl:call-template name="object-name" /></xsl:variable>
        <xsl:variable name="looks">
          <xsl:for-each select="sprite[position() &gt; 1]">
            <look at="{position() + 1}">
              <xsl:attribute name="test">
                <xsl:value-of select="concat('game.', $thing, '.look == ')" />
                <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@name" /></xsl:call-template>
              </xsl:attribute>
            </look>
          </xsl:for-each>
          <look at="1" />
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="count(sprite) &gt; 1">
            <!-- the picture of its look -->
            <line>
              <xsl:value-of select="concat('const std::vector&lt;std::string&gt;&amp; ', $thing, 'Look = ')" />
              <xsl:call-template name="look-choice"><xsl:with-param name="looks" select="exsl:node-set($looks)/look" /><xsl:with-param name="part" select="'picture'" /></xsl:call-template>
              <xsl:text>;</xsl:text>
            </line>
            <line>
              <xsl:call-template name="pixels-call">
                <xsl:with-param name="looks" select="exsl:node-set($looks)/look" />
                <xsl:with-param name="thing" select="concat('game.', $thing)" />
                <xsl:with-param name="rows" select="concat($thing, 'Look')" />
              </xsl:call-template>
            </line>
          </xsl:when>
          <xsl:otherwise>
            <line><xsl:call-template name="pixels-call"><xsl:with-param name="looks" select="exsl:node-set($looks)/look" /><xsl:with-param name="thing" select="concat('game.', $thing)" /></xsl:call-template></line>
          </xsl:otherwise>
        </xsl:choose>
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

  <!-- A march's cells: each look's picture by the frame the march is on,
       a row's own pictures for the rows that have them. -->
  <xsl:template name="march-draw-lines">
    <xsl:variable name="grid" select="." />
    <xsl:variable name="cell"><xsl:call-template name="cell-name" /></xsl:variable>
    <xsl:variable name="list"><xsl:call-template name="object-name" /></xsl:variable>
    <xsl:variable name="named"><xsl:call-template name="grid-looks" /></xsl:variable>
    <xsl:variable name="looks" select="exsl:node-set($named)/look" />
    <xsl:variable name="draws">
      <xsl:for-each select="$looks">
        <xsl:variable name="from" select="$grid/row[generate-id() = current()/@row] | $grid[current()/@row = '']" />
        <xsl:variable name="frames">
          <xsl:for-each select="$grid/animation/frame[@sprite != $grid/animation/frame[1]/@sprite]">
            <xsl:variable name="name" select="@sprite" />
            <xsl:if test="not(preceding-sibling::frame[@sprite = $name])">
              <look at="{count($from/sprite[@name = $name]/preceding-sibling::sprite) + 1}">
                <xsl:attribute name="test">
                  <xsl:variable name="all" select="$grid/animation/frame" />
                  <xsl:choose>
                    <xsl:when test="count($all) mod 2 = 0 and not($all[position() mod 2 = 1][@sprite != $all[1]/@sprite]) and not($all[position() mod 2 = 0][@sprite != $all[2]/@sprite])">
                      <!-- every other frame -->
                      <xsl:value-of select="concat('game.', $cell, 'Frame % 2 == 1')" />
                    </xsl:when>
                    <xsl:otherwise>
                  <xsl:for-each select="../frame[@sprite = $name]">
                    <xsl:if test="position() &gt; 1"> || </xsl:if>
                    <xsl:value-of select="concat('game.', $cell, 'Frame == ', count(preceding-sibling::frame))" />
                  </xsl:for-each>
                    </xsl:otherwise>
                  </xsl:choose>
                </xsl:attribute>
              </look>
            </xsl:if>
          </xsl:for-each>
          <look at="{count($from/sprite[@name = $grid/animation/frame[1]/@sprite]/preceding-sibling::sprite) + 1}" />
        </xsl:variable>
        <d>
          <xsl:call-template name="pixels-call">
            <xsl:with-param name="looks" select="exsl:node-set($frames)/look" />
            <xsl:with-param name="thing" select="$cell" />
            <xsl:with-param name="object" select="$from" />
          </xsl:call-template>
        </d>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="each" select="exsl:node-set($draws)/d" />
    <line>
      <xsl:text>for (const </xsl:text>
      <xsl:call-template name="cell-type" />
      <xsl:value-of select="concat('&amp; ', $cell, ' : game.', $list, ')')" />
    </line>
    <line>{</line>
    <xsl:for-each select="$looks[position() &gt; 1]">
      <xsl:variable name="at" select="position() + 1" />
      <line depth="1">
        <xsl:if test="position() &gt; 1">else </xsl:if>
        <xsl:value-of select="concat('if (', $cell, '.look == ')" />
        <xsl:call-template name="look-name"><xsl:with-param name="object" select="$grid" /><xsl:with-param name="look" select="@name" /></xsl:call-template>
        <xsl:value-of select="concat(') { ', $each[$at], ' }')" />
      </line>
    </xsl:for-each>
    <line depth="1"><xsl:value-of select="concat('else { ', $each[1], ' }')" /></line>
    <line>}</line>
  </xsl:template>

  <!-- picture.pixels(rows, thing.position, thing.size, colours): the
       picture and palette of the look whose test holds, or the last's. -->
  <xsl:template name="pixels-call">
    <xsl:param name="looks" />
    <xsl:param name="thing" />
    <xsl:param name="rows" select="''" />
    <xsl:param name="object" select="." />
    <xsl:param name="ordered" select="false()" />
    <xsl:variable name="colors"><xsl:call-template name="look-choice"><xsl:with-param name="looks" select="$looks" /><xsl:with-param name="part" select="'colors'" /><xsl:with-param name="object" select="$object" /><xsl:with-param name="ordered" select="$ordered" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$texts-drawn">picture.pixels(</xsl:when>
      <xsl:otherwise>pixels(window, </xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="$rows != ''"><xsl:value-of select="$rows" /></xsl:when>
      <xsl:otherwise><xsl:call-template name="look-choice"><xsl:with-param name="looks" select="$looks" /><xsl:with-param name="part" select="'picture'" /><xsl:with-param name="object" select="$object" /><xsl:with-param name="ordered" select="$ordered" /></xsl:call-template></xsl:otherwise>
    </xsl:choose>
    <xsl:value-of select="concat(', ', $thing, '.position, ', $thing, '.size')" />
    <xsl:if test="$colors != ''"><xsl:value-of select="concat(', ', $colors)" /></xsl:if>
    <xsl:text>);</xsl:text>
  </xsl:template>

  <!-- a ? x : b ? y : z, the picture (or palette) of each look by its test,
       the last's when none holds; one name when they all have the same. -->
  <xsl:template name="look-choice">
    <xsl:param name="looks" />
    <xsl:param name="part" />
    <xsl:param name="object" select="." />
    <!-- tests each holding when the one before does, age &lt; 0.2 then
         age &lt; 0.4: a look the same as the next is left to the next's test -->
    <xsl:param name="ordered" select="false()" />
    <xsl:variable name="names">
      <xsl:for-each select="$looks">
        <xsl:variable name="bitmap" select="$pictures[@name = ($object/sprite | $object/parent::group[not($object/sprite)]/sprite)[number(current()/@at)]/bitmap/@from]" />
        <n>
          <xsl:choose>
            <xsl:when test="$part = 'picture'"><xsl:call-template name="picture-name"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template></xsl:when>
            <xsl:when test="$bitmap and count($bitmap | $colored-pictures) = count($colored-pictures)"><xsl:call-template name="palette-name"><xsl:with-param name="bitmap" select="$bitmap" /></xsl:call-template></xsl:when>
          </xsl:choose>
        </n>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="all" select="exsl:node-set($names)/n" />
    <xsl:choose>
      <xsl:when test="not($all[. != $all[1]])"><xsl:value-of select="$all[1]" /></xsl:when>
      <xsl:otherwise>
        <xsl:for-each select="$looks[position() &lt; last()]">
          <xsl:variable name="at" select="position()" />
          <xsl:if test="not($ordered and $all[$at] = $all[$at + 1])">
            <xsl:value-of select="concat(@test, ' ? ', $all[$at], ' : ')" />
          </xsl:if>
        </xsl:for-each>
        <xsl:value-of select="$all[last()]" />
      </xsl:otherwise>
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
    <xsl:variable name="first-style"><xsl:call-template name="text-style"><xsl:with-param name="sprite" select="sprite[1]" /><xsl:with-param name="style" select="$base" /></xsl:call-template></xsl:variable>
    <xsl:variable name="styled">
      <xsl:for-each select="sprite[position() &gt; 1]">
        <xsl:variable name="own"><xsl:call-template name="text-style"><xsl:with-param name="sprite" select="." /><xsl:with-param name="style" select="$base" /></xsl:call-template></xsl:variable>
        <xsl:if test="$own != $first-style">yes</xsl:if>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="colored" select="$styled != ''" />
    <xsl:variable name="style">
      <xsl:choose>
        <xsl:when test="$colored"><xsl:value-of select="concat($named, 'Style')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="$first-style" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="words">
      <xsl:choose>
        <xsl:when test="count(sprite) &gt; 1">
          <xsl:for-each select="sprite[position() &gt; 1]">
            <xsl:value-of select="concat('game.', $named, '.look == ')" />
            <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@name" /></xsl:call-template>
            <xsl:text> ? </xsl:text>
            <xsl:call-template name="string"><xsl:with-param name="text" select="text/content" /></xsl:call-template>
            <xsl:text> : </xsl:text>
          </xsl:for-each>
          <xsl:call-template name="string"><xsl:with-param name="text" select="sprite[1]/text/content" /></xsl:call-template>
        </xsl:when>
        <xsl:when test="sprite/text/number and sprite/text/content">
          <xsl:call-template name="string"><xsl:with-param name="text" select="concat(sprite/text/content, ' ')" /></xsl:call-template>
          <xsl:text> + </xsl:text>
          <xsl:call-template name="number-words"><xsl:with-param name="number" select="sprite/text/number" /></xsl:call-template>
        </xsl:when>
        <xsl:when test="sprite/text/number">
          <xsl:call-template name="number-words"><xsl:with-param name="number" select="sprite/text/number" /></xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="string"><xsl:with-param name="text" select="sprite/text/content" /></xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:variable name="x"><xsl:call-template name="value"><xsl:with-param name="node" select="position/x" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'x'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="y"><xsl:call-template name="value"><xsl:with-param name="node" select="position/y" /><xsl:with-param name="self" select="." /><xsl:with-param name="axis" select="'y'" /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
    <xsl:variable name="measured" select="contains(concat($x, $y), concat($named, 'Size'))" />
    <xsl:variable name="literal" select="not(sprite/text/number) and count(sprite) = 1" />
    <xsl:variable name="shown">
      <xsl:choose>
        <xsl:when test="$measured and not($literal)"><xsl:value-of select="concat($named, 'Text')" /></xsl:when>
        <xsl:otherwise><xsl:value-of select="$words" /></xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$colored">
      <line>
        <xsl:value-of select="concat('const Style ', $named, 'Style = ')" />
        <xsl:for-each select="sprite[position() &gt; 1]">
          <xsl:value-of select="concat('game.', $named, '.look == ')" />
          <xsl:call-template name="look-name"><xsl:with-param name="object" select="$object" /><xsl:with-param name="look" select="@name" /></xsl:call-template>
          <xsl:text> ? Style</xsl:text>
          <xsl:call-template name="text-style"><xsl:with-param name="sprite" select="." /><xsl:with-param name="style" select="$base" /><xsl:with-param name="always" select="true()" /></xsl:call-template>
          <xsl:text> : </xsl:text>
        </xsl:for-each>
        <xsl:text>Style</xsl:text>
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

  <!-- A text's style: the style it names, or that style with a size or colour
       of the text's own. -->
  <xsl:template name="text-style">
    <xsl:param name="sprite" />
    <xsl:param name="style" />
    <xsl:param name="always" select="false()" />
    <xsl:choose>
      <xsl:when test="$sprite/text/color or $sprite/text/size or $always">
        <xsl:text>{ </xsl:text>
        <xsl:choose>
          <xsl:when test="$sprite/text/size"><xsl:value-of select="normalize-space($sprite/text/size)" /></xsl:when>
          <xsl:otherwise><xsl:value-of select="concat($style, '.size')" /></xsl:otherwise>
        </xsl:choose>
        <xsl:text>, </xsl:text>
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
      <xsl:variable name="own"><xsl:call-template name="text-style"><xsl:with-param name="sprite" select="sprite" /><xsl:with-param name="style" select="$style" /></xsl:call-template></xsl:variable>
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
        <line><xsl:value-of select="concat('const sf::Vector2f ', $named, $number, 'Size = picture.measure(', $words, ', ', $own, ');')" /></line>
      </xsl:if>
      <line><xsl:value-of select="concat('picture.text(', $words, ', ', $own, ', { ', $at-x, ', ', $at-y, ' });')" /></line>
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

  <!-- tone(220, 0.03), or tone({ { Waveform::Triangle, 329.63, 82.41, 0.4 } }, 0.4):
       a plain sound's note, or a shaped one's notes, each its wave, pitch,
       slide and length, and the sound's loudness. -->
  <xsl:template name="tone-call">
    <xsl:variable name="hertz"><xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(note/@pitch)" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="count(. | $plain-sounds) = count($plain-sounds)">
        <xsl:value-of select="concat('tone(', $hertz, ', ', normalize-space(note), ')')" />
      </xsl:when>
      <xsl:when test="part">
        <xsl:variable name="sound" select="." />
        <xsl:text>together({</xsl:text>
        <xsl:for-each select="part">
          <xsl:if test="position() &gt; 1">,</xsl:if>
          <xsl:text>&#10;&#9;&#9;&#9;</xsl:text>
          <xsl:call-template name="notes-tone">
            <xsl:with-param name="sound" select="$sound" />
            <xsl:with-param name="wave" select="@wave | $sound/@wave[not(current()/@wave)]" />
            <xsl:with-param name="indent" select="'&#9;&#9;&#9;'" />
          </xsl:call-template>
        </xsl:for-each>
        <xsl:text>&#10;&#9;&#9;})</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="notes-tone">
          <xsl:with-param name="sound" select="." />
          <xsl:with-param name="wave" select="@wave" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- tone({ ... }, 0.4): the notes of a sound, or of one of its parts. -->
  <xsl:template name="notes-tone">
    <xsl:param name="sound" />
    <xsl:param name="wave" />
    <xsl:param name="indent" select="'&#9;&#9;'" />
    <xsl:variable name="default" select="$wave" />
    <xsl:text>tone({</xsl:text>
    <xsl:for-each select="note">
      <xsl:if test="position() &gt; 1">,</xsl:if>
      <xsl:choose>
        <xsl:when test="last() &gt; 1"><xsl:value-of select="concat('&#10;&#9;', $indent)" /></xsl:when>
        <xsl:otherwise><xsl:text> </xsl:text></xsl:otherwise>
      </xsl:choose>
      <xsl:variable name="note-wave" select="@wave | $default[not(current()/@wave)]" />
      <xsl:variable name="from"><xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(@pitch)" /></xsl:call-template></xsl:variable>
      <xsl:text>{ Waveform::</xsl:text>
      <xsl:choose>
        <xsl:when test="$note-wave"><xsl:call-template name="type-word"><xsl:with-param name="name" select="$note-wave" /></xsl:call-template></xsl:when>
        <xsl:otherwise>Square</xsl:otherwise>
      </xsl:choose>
      <xsl:value-of select="concat(', ', $from, ', ')" />
      <xsl:choose>
        <xsl:when test="@to"><xsl:call-template name="hertz"><xsl:with-param name="pitch" select="normalize-space(@to)" /></xsl:call-template></xsl:when>
        <xsl:otherwise><xsl:value-of select="$from" /></xsl:otherwise>
      </xsl:choose>
      <xsl:value-of select="concat(', ', normalize-space(.), ' }')" />
    </xsl:for-each>
    <xsl:choose>
      <xsl:when test="count(note) &gt; 1"><xsl:value-of select="concat('&#10;', $indent, '}, ')" /></xsl:when>
      <xsl:otherwise><xsl:text> }, </xsl:text></xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="$sound/volume"><xsl:value-of select="normalize-space($sound/volume)" /></xsl:when>
      <xsl:otherwise>0.3</xsl:otherwise>
    </xsl:choose>
    <xsl:text>)</xsl:text>
  </xsl:template>

  <!-- The waves the sounds use, notes of any of them sliding in pitch one
       after another, and a plain square tone when some sounds are plain. -->
  <xsl:template name="shaped-tone">
    <xsl:variable name="pitched" select="$waves[. != 'noise']" />
    <xsl:variable name="noisy" select="$waves[. = 'noise']" />
    <xsl:text>
	enum class Waveform { </xsl:text>
    <xsl:for-each select="$waves">
      <xsl:if test="position() &gt; 1">, </xsl:if>
      <xsl:call-template name="type-word"><xsl:with-param name="name" select="." /></xsl:call-template>
    </xsl:for-each>
    <xsl:text> };

	// A wave sliding evenly in pitch from `hertz` to `toHertz`.
	struct Note
	{
		Waveform wave;
		double hertz;
		double toHertz;
		double duration;
	};
</xsl:text>
    <xsl:if test="$pitched">
      <xsl:text>
	// How high the wave is, -1 to 1, at `phase` (0 to 1) through one cycle.
	double height(Waveform wave, double phase)
	{
</xsl:text>
      <xsl:for-each select="$pitched[position() &gt; 1]">
        <xsl:text>&#9;&#9;if (wave == Waveform::</xsl:text>
        <xsl:call-template name="type-word"><xsl:with-param name="name" select="." /></xsl:call-template>
        <xsl:text>) { return </xsl:text>
        <xsl:call-template name="wave-height" />
        <xsl:text>; }&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>&#9;&#9;</xsl:text>
      <xsl:if test="count($pitched) = 1">static_cast&lt;void&gt;(wave);&#10;&#9;&#9;</xsl:if>
      <xsl:text>return </xsl:text>
      <xsl:for-each select="$pitched[1]"><xsl:call-template name="wave-height" /></xsl:for-each>
      <xsl:text>;
	}
</xsl:text>
    </xsl:if>
    <xsl:text>
	// The notes one after another, each faded in and out over two
	// thousandths of a second so it doesn't click</xsl:text>
    <xsl:if test="$noisy">
      <xsl:text>; noise is a new random
	// level every half cycle, held in between, so the pitch sets how coarse
	// the hiss is</xsl:text>
    </xsl:if>
    <xsl:text>.
	sf::SoundBuffer tone(const std::vector&lt;Note&gt;&amp; notes, double volume)
	{
		constexpr unsigned rate = 44100;
</xsl:text>
    <xsl:if test="$noisy">
      <xsl:text>		std::minstd_rand random;
		std::uniform_real_distribution&lt;double&gt; level(-1, 1);
</xsl:text>
    </xsl:if>
    <xsl:text>
		std::vector&lt;std::int16_t&gt; samples;
		for (const Note&amp; note : notes)
		{
			const std::size_t count = static_cast&lt;std::size_t&gt;(note.duration * rate);
			const std::size_t fade = std::min&lt;std::size_t&gt;(count / 4, rate / 500);
			double phase = 0.0;
</xsl:text>
    <xsl:if test="$noisy"><xsl:text>			double held = level(random);&#10;</xsl:text></xsl:if>
    <xsl:text>			for (std::size_t i = 0; i &lt; count; ++i)
			{
				double value = </xsl:text>
    <xsl:choose>
      <xsl:when test="$noisy and $pitched">(note.wave == Waveform::Noise ? held : height(note.wave, phase))</xsl:when>
      <xsl:when test="$noisy">held</xsl:when>
      <xsl:otherwise>height(note.wave, phase)</xsl:otherwise>
    </xsl:choose>
    <xsl:text> * volume;
				if (i &lt; fade) { value *= static_cast&lt;double&gt;(i) / static_cast&lt;double&gt;(fade); }
				if (i &gt;= count - fade) { value *= static_cast&lt;double&gt;(count - 1 - i) / static_cast&lt;double&gt;(fade); }
				samples.push_back(static_cast&lt;std::int16_t&gt;(std::lround(value * 32767)));

				const double through = static_cast&lt;double&gt;(i) / static_cast&lt;double&gt;(count);
				phase += note.hertz * std::pow(note.toHertz / note.hertz, through) / rate;
</xsl:text>
    <xsl:if test="$noisy">
      <xsl:text>				if (note.wave == Waveform::Noise &amp;&amp; phase &gt;= 0.5)
				{
					held = level(random);
					phase -= 0.5;
				}
</xsl:text>
    </xsl:if>
    <xsl:text>				phase -= std::floor(phase);
			}
		}

		sf::SoundBuffer buffer;
		if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, rate, { sf::SoundChannel::Mono }))
		{
			std::cerr &lt;&lt; "warning: could not make a sound\n";
		}
		return buffer;
	}
</xsl:text>
    <xsl:if test="$sounds/part">
      <xsl:text>
	// Sounds played at once: their samples added, as long as the longest,
	// cut off where the sum is louder than a sample can be.
	sf::SoundBuffer together(const std::vector&lt;sf::SoundBuffer&gt;&amp; parts)
	{
		std::vector&lt;std::int16_t&gt; samples;
		for (const sf::SoundBuffer&amp; part : parts)
		{
			const std::int16_t* from = part.getSamples();
			const std::size_t count = part.getSampleCount();
			if (samples.size() &lt; count) { samples.resize(count, 0); }
			for (std::size_t i = 0; i &lt; count; ++i)
			{
				samples[i] = static_cast&lt;std::int16_t&gt;(std::clamp(samples[i] + from[i], -32768, 32767));
			}
		}

		sf::SoundBuffer buffer;
		if (!buffer.loadFromSamples(samples.data(), samples.size(), 1, 44100, { sf::SoundChannel::Mono }))
		{
			std::cerr &lt;&lt; "warning: could not make a sound\n";
		}
		return buffer;
	}
</xsl:text>
    </xsl:if>
    <xsl:if test="$plain-sounds">
      <xsl:text>
	// A plain tone: a square wave at the usual loudness.
	sf::SoundBuffer tone(double hertz, double duration)
	{
		return tone({ { Waveform::Square, hertz, hertz, duration } }, 0.3);
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
  <!-- std::to_string(game.chicken1.score): a shown number as words, a
       float one as the whole number it holds. -->
  <xsl:template name="number-words">
    <xsl:param name="number" />
    <xsl:variable name="path" select="normalize-space($number)" />
    <xsl:variable name="owner" select="$everything[@name = substring-before($path, '.')]" />
    <xsl:variable name="variable" select="($owner/variables/variable | $owner/parent::group/variables/variable)[@name = substring-after($path, '.')][1]" />
    <xsl:variable name="type"><xsl:for-each select="$variable"><xsl:call-template name="variable-type" /></xsl:for-each></xsl:variable>
    <xsl:variable name="value"><xsl:call-template name="value"><xsl:with-param name="node" select="$number" /><xsl:with-param name="self" select="." /><xsl:with-param name="in" select="'game.'" /></xsl:call-template></xsl:variable>
    <xsl:choose>
      <xsl:when test="$type = 'float'"><xsl:value-of select="concat('std::to_string(static_cast&lt;int&gt;(', $value, '))')" /></xsl:when>
      <xsl:otherwise><xsl:value-of select="concat('std::to_string(', $value, ')')" /></xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
