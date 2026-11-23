<?xml version="1.0" encoding="UTF-8"?>
<!-- program.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 23, 2026 -->

<!-- The rest of a generated web page: source/main.cpp, which plays the game
     with the engine's own Game and Engine on the page's canvas and Web Audio,
     source/page.html, the page Emscripten puts it in, its build and its readme. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template name="main-file">
    <xsl:call-template name="header">
      <xsl:with-param name="file" select="'main.cpp'" />
    </xsl:call-template>
    <xsl:text>//
// </xsl:text>
    <xsl:value-of select="concat($title, ', generated from ', $source, ' by xgecli --generate web. The')" />
    <xsl:text>
// game is in source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp, as the engine's own data; it is played
// by the engine's own code (engine/), on a web page's canvas and Web Audio.

#include "audio/web.h"
#include "engine/engine.h"
#include "game/game.h"
#include "play/frame.h"
#include "window/web.h"

#include &lt;emscripten.h&gt;

#include &lt;algorithm&gt;
#include &lt;exception&gt;
#include &lt;fstream&gt;
#include &lt;iostream&gt;
#include &lt;iterator&gt;
#include &lt;memory&gt;
#include &lt;string&gt;

// The game, as written in </xsl:text>
    <xsl:value-of select="concat($source, ' (source/', $name, '.cpp).')" />
    <xsl:text>
xge::GameData </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>();

// What the game keeps (a best score) lives in the browser's storage for the
// page, put back into the file the engine reads before the game starts, and
// copied out again whenever the engine has written it anew.
EM_JS(void, xge_kept_restore, (const char* key, const char* file), {
	try {
		var kept = window.localStorage.getItem(UTF8ToString(key));
		if (kept === null) { return; }
		var path = UTF8ToString(file);
		FS.mkdirTree(path.substring(0, path.lastIndexOf('/')));
		FS.writeFile(path, kept);
	} catch (error) {}
});

EM_JS(void, xge_kept_store, (const char* key, const char* text), {
	try { window.localStorage.setItem(UTF8ToString(key), UTF8ToString(text)); } catch (error) {}
});

// What went wrong, on the page where the game would be.
EM_JS(void, xge_show_error, (const char* what), {
	var shown = document.createElement('pre');
	shown.style.cssText = 'color:#f66;font:16px monospace;white-space:pre-wrap;padding:16px';
	shown.textContent = 'Error: ' + UTF8ToString(what);
	document.body.appendChild(shown);
});

namespace
{
	constexpr const char* keptFile = "saves/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.xml";
	constexpr const char* keptKey = "xge.</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.kept";

	xge::Engine* engine = nullptr;
	double shortest = 1.0 / 60.0;
	double last = 0.0;
	unsigned frames = 0;
	std::string keptText;

	void storeKept()
	{
		std::ifstream in(keptFile, std::ios::binary);
		std::string text{ std::istreambuf_iterator&lt;char&gt;(in), std::istreambuf_iterator&lt;char&gt;() };
		if (!text.empty() &amp;&amp; text != keptText)
		{
			keptText = text;
			xge_kept_store(keptKey, keptText.c_str());
		}
	}

	// A frame each time the browser draws, as Engine::run plays them: at
	// most &lt;framerate&gt; a second (a millisecond early is soon enough), a
	// step no longer than the engine's longest.
	void frame()
	{
		const double now = emscripten_get_now() / 1000.0;
		if (now - last &lt; shortest - 0.001) { return; }
		const float seconds = static_cast&lt;float&gt;(std::min(now - last, static_cast&lt;double&gt;(xge::kLongestStep)));
		last = now;
		try
		{
			engine-&gt;input();
			engine-&gt;update(seconds);
			engine-&gt;render();
			if (++frames % 60 == 0) { storeKept(); }
		}
		catch (const std::exception&amp; error)
		{
			emscripten_cancel_main_loop();
			std::cerr &lt;&lt; "Error: " &lt;&lt; error.what() &lt;&lt; '\n';
			xge_show_error(error.what());
		}
	}
}

int main()
{
	try
	{
		static xge::Game game{ </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>() };
		static xge::Engine played(game, std::make_unique&lt;xge::WebWindow&gt;(game.getWindowDesc()), std::make_unique&lt;xge::WebAudio&gt;());
		engine = &amp;played;

		xge_kept_restore(keptKey, keptFile);
		played.keepIn(keptFile);

		shortest = 1.0 / std::max(1, game.getWindowDesc().framerate);
		last = emscripten_get_now() / 1000.0;
		emscripten_set_main_loop(frame, 0, 0);
	}
	catch (const std::exception&amp; error)
	{
		std::cerr &lt;&lt; "Error: " &lt;&lt; error.what() &lt;&lt; '\n';
		xge_show_error(error.what());
		return 1;
	}
	return 0;
}
</xsl:text>
  </xsl:template>

  <xsl:template name="page-file">
    <xsl:text>&lt;!doctype html&gt;
&lt;!-- </xsl:text>
    <xsl:value-of select="concat($title, ', generated from ', $source, ' by xgecli --generate web. ')" />
    <xsl:text>--&gt;
&lt;html lang="en"&gt;
&lt;head&gt;
&lt;meta charset="utf-8"&gt;
&lt;meta name="viewport" content="width=device-width, initial-scale=1"&gt;
&lt;title&gt;</xsl:text>
    <xsl:value-of select="$title" />
    <xsl:text>&lt;/title&gt;
&lt;style&gt;
html, body { margin: 0; height: 100%; background: #000; overflow: hidden; }
body { display: flex; align-items: center; justify-content: center; }
canvas { display: block; outline: none; }
&lt;/style&gt;
&lt;/head&gt;
&lt;body&gt;
&lt;canvas id="game" tabindex="0"&gt;&lt;/canvas&gt;
&lt;script&gt;
var Module = { canvas: document.getElementById('game') };
&lt;/script&gt;
{{{ SCRIPT }}}
&lt;/body&gt;
&lt;/html&gt;
</xsl:text>
  </xsl:template>

  <xsl:template name="cmake-file">
    <xsl:text># CMakeLists.txt
# </xsl:text>
    <xsl:value-of select="concat($title, ', generated from ', $source, ' by xgecli --generate web.')" />
    <xsl:text>
# Built with Emscripten (emcmake), into one page: output/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.html.

cmake_minimum_required(VERSION 3.20)

project(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> LANGUAGES C CXX)

if (NOT EMSCRIPTEN)
	message(FATAL_ERROR "A web page is built with Emscripten: emcmake cmake -B build")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/output")
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# The engine says what went wrong by throwing, everywhere.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

include(FetchContent)

# lunasvg for any &lt;svg&gt; picture, and plutovg, inside it, for text and a
# pack's card sheet: fetched and built here.
set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PLUTOVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(lunasvg GIT_REPOSITORY https://github.com/sammycage/lunasvg.git GIT_TAG v3.5.0 EXCLUDE_FROM_ALL SYSTEM)
FetchContent_MakeAvailable(lunasvg)

add_executable(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>
	source/main.cpp
	source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp
	# the engine's own code, as it is in xge/lib/
</xsl:text>
    <xsl:for-each select="$engine">
      <xsl:value-of select="concat('&#9;engine/', substring-after(., '/'), '&#10;')" />
    </xsl:for-each>
    <xsl:value-of select="concat(')&#10;target_include_directories(', $name, ' PRIVATE engine)&#10;target_compile_features(', $name, ' PRIVATE cxx_std_20)&#10;target_link_libraries(', $name, ' PRIVATE lunasvg::lunasvg plutovg::plutovg)&#10;set_target_properties(', $name, ' PROPERTIES SUFFIX &quot;.html&quot;)&#10;')" />
    <xsl:text>
# The verbs are a small file each (engine/verbs/): compiled as one.
file(GLOB verbs CONFIGURE_DEPENDS engine/verbs/*.cpp)
set_source_files_properties(${verbs} PROPERTIES UNITY_GROUP verbs)
set_target_properties(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PROPERTIES UNITY_BUILD ON UNITY_BUILD_MODE GROUP)

# One page with everything in it, opened straight from the disk: the
# WebAssembly and the game's assets inside, memory as the game needs it.
target_link_options(</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> PRIVATE
	-fexceptions
	-sSINGLE_FILE=1
	-sALLOW_MEMORY_GROWTH=1
	-sENVIRONMENT=web
	-sMINIFY_HTML=0
	"--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/source/page.html")
set_property(TARGET </xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text> APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/page.html")
</xsl:text>
    <xsl:if test="$texts or $pictures or $recordings or $fonts">
      <xsl:value-of select="concat('target_link_options(', $name, ' PRIVATE &quot;--embed-file=${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets&quot;)&#10;')" />
    </xsl:if>
  </xsl:template>

  <xsl:template name="readme-file">
    <xsl:value-of select="concat('# ', $title, '&#10;&#10;Generated from `', $source, '` by `xgecli --generate web`. It is the game xgecli and&#10;')" />
    <xsl:text>xgegui play, built with Emscripten into one web page, `output/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.html`, that plays
in a browser opened straight from the disk (double-click it), with nothing else beside
it. The game file is written out as a tree of C++ data in `source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp`
(engine-cpp's stylesheets), and everything that plays it is the engine's own code,
copied unchanged into `engine/`, with a window that is the page's canvas and sound
that is its Web Audio. The page fills the browser's window; sound starts with the
first key pressed or click, as browsers ask. What the game keeps (a best score) is
kept in the browser's storage for the page.

| File | What |
|---|---|
| `source/</xsl:text>
    <xsl:value-of select="concat($name, '.cpp` | the game, as the engine reads it from `', $source, '` |')" />
    <xsl:text>
| `source/main.cpp` | makes the game, the page's window and sound, and plays a frame each time the browser draws |
| `source/page.html` | the page the game is put in |
| `engine/` | the engine's own files, as they are in its `xge/lib/` |
| `assets/` | the fonts, pictures and recordings it opens, built into the page |
| `LICENSE` | the engine's licence (MIT), which goes with its code |
| `CMakeLists.txt` | the build; it downloads and builds lunasvg |

## Build

With Emscripten's `emcmake` on the path (emsdk), from this folder:

```
emcmake cmake -B build
cmake --build build
```

Then open `output/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.html`. Change the game in its XML file and generate it again,
rather than editing `source/</xsl:text>
    <xsl:value-of select="$name" />
    <xsl:text>.cpp`.
</xsl:text>
  </xsl:template>
</xsl:stylesheet>
