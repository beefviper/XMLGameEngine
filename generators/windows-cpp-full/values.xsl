<?xml version="1.0" encoding="UTF-8"?>
<!-- values.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 5, 2026 -->

<!-- Names and numbers: how a name in a game becomes a C++ name, and how a value
     (expression text, <random>, <equation> or <formula>) becomes a C++
     expression. Every number is a float, as in the engine. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:variable name="letters" select="'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_'" />
  <xsl:variable name="digits" select="'0123456789'" />

  <!-- The functions an expression may call (exprtk's), and their C++ names. -->
  <xsl:variable name="functions" select="' min max abs floor ceil sqrt sin cos tan pow round '" />

  <!-- A name from the game as a C++ name: a prefix (v_ for a value, o_ for an
       object, s_ for a state, snd_ for a sound) and the name with its dots and
       dashes made underscores (paddle1.score is v_paddle1_score). -->
  <xsl:template name="cpp-name">
    <xsl:param name="prefix" />
    <xsl:param name="name" />
    <xsl:value-of select="concat($prefix, translate($name, '.-', '__'))" />
  </xsl:template>

  <!-- A number as a float literal: 2 is 2.0f, .5 is 0.5f. -->
  <xsl:template name="cpp-number">
    <xsl:param name="text" />
    <xsl:if test="starts-with($text, '.')">0</xsl:if>
    <xsl:value-of select="$text" />
    <xsl:choose>
      <xsl:when test="contains($text, '.')">f</xsl:when>
      <xsl:otherwise>.0f</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Expression text (exprtk's arithmetic) as C++, a word at a time: a name
       becomes its v_ name, a number a float, a function call std::'s, and the
       rest (operators, brackets, spaces) is kept. XSLT 1.0 has no regular
       expressions, so a word is "the text up to the first character that cannot
       be in one". -->
  <xsl:template name="cpp-expression">
    <xsl:param name="text" />
    <xsl:if test="string-length($text) &gt; 0">
      <xsl:variable name="first" select="substring($text, 1, 1)" />
      <xsl:choose>
        <!-- a name, which may have dots (window.width.center) -->
        <xsl:when test="contains($letters, $first)">
          <xsl:variable name="stop" select="substring(translate($text, concat($letters, $digits, '.'), ''), 1, 1)" />
          <xsl:variable name="word">
            <xsl:choose>
              <xsl:when test="$stop = ''"><xsl:value-of select="$text" /></xsl:when>
              <xsl:otherwise><xsl:value-of select="substring-before($text, $stop)" /></xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:variable name="after" select="substring($text, string-length($word) + 1)" />
          <xsl:choose>
            <xsl:when test="starts-with(normalize-space($after), '(')">
              <xsl:if test="not(contains($functions, concat(' ', $word, ' ')))">
                <xsl:message terminate="yes">windows-cpp-full: the function <xsl:value-of select="$word" />() in "<xsl:value-of select="$text" />" cannot be generated yet</xsl:message>
              </xsl:if>
              <xsl:value-of select="concat('std::', $word)" />
            </xsl:when>
            <xsl:when test="$word = 'and'"><xsl:text>&amp;&amp;</xsl:text></xsl:when>
            <xsl:when test="$word = 'or'"><xsl:text>||</xsl:text></xsl:when>
            <xsl:when test="$word = 'not'"><xsl:text>!</xsl:text></xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="cpp-name">
                <xsl:with-param name="prefix" select="'v_'" />
                <xsl:with-param name="name" select="$word" />
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:call-template name="cpp-expression">
            <xsl:with-param name="text" select="$after" />
          </xsl:call-template>
        </xsl:when>
        <!-- a number -->
        <xsl:when test="contains($digits, $first) or ($first = '.' and contains($digits, substring($text, 2, 1)))">
          <xsl:variable name="stop" select="substring(translate($text, concat($digits, '.'), ''), 1, 1)" />
          <xsl:variable name="number">
            <xsl:choose>
              <xsl:when test="$stop = ''"><xsl:value-of select="$text" /></xsl:when>
              <xsl:otherwise><xsl:value-of select="substring-before($text, $stop)" /></xsl:otherwise>
            </xsl:choose>
          </xsl:variable>
          <xsl:call-template name="cpp-number">
            <xsl:with-param name="text" select="$number" />
          </xsl:call-template>
          <xsl:call-template name="cpp-expression">
            <xsl:with-param name="text" select="substring($text, string-length($number) + 1)" />
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$first = '^' or $first = '%' or $first = ':' or $first = '[' or $first = '{'">
          <xsl:message terminate="yes">windows-cpp-full: "<xsl:value-of select="$first" />" in an expression cannot be generated yet</xsl:message>
        </xsl:when>
        <!-- anything else, a character at a time; line breaks become spaces -->
        <xsl:otherwise>
          <xsl:value-of select="translate($first, '&#9;&#10;&#13;', '   ')" />
          <xsl:call-template name="cpp-expression">
            <xsl:with-param name="text" select="substring($text, 2)" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- A value: the element that holds one (an <x>, a <radius>, an <atleast>).
       It holds expression text, or one value tag. -->
  <xsl:template name="value">
    <xsl:param name="node" select="." />
    <xsl:choose>
      <xsl:when test="$node/random">
        <xsl:apply-templates select="$node/random" mode="value" />
      </xsl:when>
      <xsl:when test="$node/equation">
        <xsl:apply-templates select="$node/equation" mode="value" />
      </xsl:when>
      <xsl:when test="$node/formula">
        <xsl:apply-templates select="$node/formula" mode="value" />
      </xsl:when>
      <xsl:when test="$node/*">
        <xsl:message terminate="yes">windows-cpp-full: the value tag &lt;<xsl:value-of select="name($node/*)" />&gt; cannot be generated yet</xsl:message>
      </xsl:when>
      <xsl:when test="normalize-space($node) = ''">
        <xsl:text>0.0f</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="attribute-value">
          <xsl:with-param name="text" select="$node" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Expression text is put in brackets, so it can stand anywhere a value
       can, unless it is one name or number (15, -1, paddle1.score). -->
  <xsl:template name="attribute-value">
    <xsl:param name="text" />
    <xsl:variable name="expression" select="normalize-space($text)" />
    <xsl:variable name="rest" select="translate($expression, concat($letters, $digits, '.'), '')" />
    <xsl:variable name="single" select="$rest = '' or ($rest = '-' and starts-with($expression, '-'))" />
    <xsl:if test="not($single)">(</xsl:if>
    <xsl:call-template name="cpp-expression">
      <xsl:with-param name="text" select="$expression" />
    </xsl:call-template>
    <xsl:if test="not($single)">)</xsl:if>
  </xsl:template>

  <xsl:template match="random" mode="value">
    <xsl:text>xge::randomBetween(</xsl:text>
    <xsl:call-template name="attribute-value">
      <xsl:with-param name="text" select="@min" />
    </xsl:call-template>
    <xsl:text>, </xsl:text>
    <xsl:call-template name="attribute-value">
      <xsl:with-param name="text" select="@max" />
    </xsl:call-template>
    <xsl:text>)</xsl:text>
  </xsl:template>

  <!-- <equation> and <formula> come out as ordinary C++ arithmetic, written the
       way a person would: the operators between the operands, and brackets only
       where C++'s precedence needs them (a - (b - c), (a + b) * c). One
       difference from the engine: a divisor of 0 gives infinity, as in the
       expression text, where the engine's <divide> gives 0. -->
  <xsl:template name="operator">
    <xsl:param name="operation" />
    <xsl:choose>
      <xsl:when test="$operation = 'add'"> + </xsl:when>
      <xsl:when test="$operation = 'subtract'"> - </xsl:when>
      <xsl:when test="$operation = 'multiply'"> * </xsl:when>
      <xsl:when test="$operation = 'divide'"> / </xsl:when>
      <xsl:otherwise>
        <xsl:message terminate="yes">windows-cpp-full: the operation &lt;<xsl:value-of select="$operation" />&gt; cannot be generated yet</xsl:message>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Whether an operation written inside another needs brackets: when it
       binds less tightly (a + b inside a *), or as the second operand of a
       subtract or divide at the same level (a - (b + c), a / (b * c)). -->
  <xsl:template name="needs-brackets">
    <xsl:param name="inner" />
    <xsl:param name="outer" />
    <xsl:param name="second" />
    <xsl:variable name="innerLevel" select="1 + number($inner = 'multiply' or $inner = 'divide')" />
    <xsl:variable name="outerLevel" select="1 + number($outer = 'multiply' or $outer = 'divide')" />
    <xsl:if test="$innerLevel &lt; $outerLevel or ($second and $innerLevel = $outerLevel and ($outer = 'subtract' or $outer = 'divide'))">yes</xsl:if>
  </xsl:template>

  <!-- An <equation>: one expression, each step written where it is used (the
       steps are names for parts of it, as a person would name them while
       working it out). The last step is the answer. -->
  <xsl:template match="equation" mode="value">
    <xsl:text>(</xsl:text>
    <xsl:apply-templates select="*[last()]" mode="step" />
    <xsl:text>)</xsl:text>
  </xsl:template>

  <xsl:template match="*" mode="step">
    <xsl:call-template name="step-operand">
      <xsl:with-param name="text" select="@augend | @minuend | @multiplicand | @dividend" />
      <xsl:with-param name="outer" select="local-name()" />
      <xsl:with-param name="second" select="false()" />
    </xsl:call-template>
    <xsl:call-template name="operator"><xsl:with-param name="operation" select="local-name()" /></xsl:call-template>
    <xsl:call-template name="step-operand">
      <xsl:with-param name="text" select="@addend | @subtrahend | @multiplier | @divisor" />
      <xsl:with-param name="outer" select="local-name()" />
      <xsl:with-param name="second" select="true()" />
    </xsl:call-template>
  </xsl:template>

  <!-- An operand of a step: an earlier step of that name, written out in its
       place, or a name or number of the game's. -->
  <xsl:template name="step-operand">
    <xsl:param name="text" />
    <xsl:param name="outer" />
    <xsl:param name="second" />
    <xsl:variable name="name" select="normalize-space($text)" />
    <xsl:variable name="step" select="preceding-sibling::*[@name = $name][1]" />
    <xsl:choose>
      <xsl:when test="$step">
        <xsl:variable name="brackets">
          <xsl:call-template name="needs-brackets">
            <xsl:with-param name="inner" select="local-name($step)" />
            <xsl:with-param name="outer" select="$outer" />
            <xsl:with-param name="second" select="$second" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$brackets = 'yes'">(</xsl:if>
        <xsl:apply-templates select="$step" mode="step" />
        <xsl:if test="$brackets = 'yes'">)</xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="attribute-value">
          <xsl:with-param name="text" select="$name" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- A <formula>: its operation, in brackets as a whole so it can stand
       anywhere a value can. -->
  <xsl:template match="formula" mode="value">
    <xsl:text>(</xsl:text>
    <xsl:apply-templates select="*" mode="operation" />
    <xsl:text>)</xsl:text>
  </xsl:template>

  <!-- A formula's operation: its first operand, then each second operand after
       the operator (a chain, a - b - c, is left to right as in C++). -->
  <xsl:template match="add | subtract | multiply | divide" mode="operation">
    <xsl:variable name="operation" select="local-name()" />
    <xsl:apply-templates select="augend | minuend | multiplicand | dividend" mode="operand">
      <xsl:with-param name="outer" select="$operation" />
      <xsl:with-param name="second" select="false()" />
    </xsl:apply-templates>
    <xsl:for-each select="addend | subtrahend | multiplier | divisor">
      <xsl:call-template name="operator"><xsl:with-param name="operation" select="$operation" /></xsl:call-template>
      <xsl:apply-templates select="." mode="operand">
        <xsl:with-param name="outer" select="$operation" />
        <xsl:with-param name="second" select="true()" />
      </xsl:apply-templates>
    </xsl:for-each>
  </xsl:template>

  <!-- An operand of a formula: another operation (in brackets if it needs
       them), a <random>, or a name or number. -->
  <xsl:template match="*" mode="operand">
    <xsl:param name="outer" />
    <xsl:param name="second" />
    <xsl:choose>
      <xsl:when test="random">
        <xsl:apply-templates select="random" mode="value" />
      </xsl:when>
      <xsl:when test="*">
        <xsl:variable name="brackets">
          <xsl:call-template name="needs-brackets">
            <xsl:with-param name="inner" select="local-name(*)" />
            <xsl:with-param name="outer" select="$outer" />
            <xsl:with-param name="second" select="$second" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:if test="$brackets = 'yes'">(</xsl:if>
        <xsl:apply-templates select="*" mode="operation" />
        <xsl:if test="$brackets = 'yes'">)</xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="attribute-value">
          <xsl:with-param name="text" select="." />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Text as a C++ string literal. -->
  <xsl:template name="cpp-string">
    <xsl:param name="text" />
    <xsl:text>"</xsl:text>
    <xsl:call-template name="escape">
      <xsl:with-param name="text" select="$text" />
    </xsl:call-template>
    <xsl:text>"</xsl:text>
  </xsl:template>

  <xsl:template name="escape">
    <xsl:param name="text" />
    <xsl:if test="string-length($text) &gt; 0">
      <xsl:variable name="first" select="substring($text, 1, 1)" />
      <xsl:choose>
        <xsl:when test="$first = '&quot;'">\"</xsl:when>
        <xsl:when test="$first = '\'">\\</xsl:when>
        <xsl:when test="$first = '&#10;'">\n</xsl:when>
        <xsl:otherwise><xsl:value-of select="$first" /></xsl:otherwise>
      </xsl:choose>
      <xsl:call-template name="escape">
        <xsl:with-param name="text" select="substring($text, 2)" />
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
