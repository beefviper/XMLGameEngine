<?xml version="1.0" encoding="UTF-8"?>
<!-- tree.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 19, 2026 -->

<!-- The game file as C++ data, source/<game>.cpp: every element a
     TreeElement (xml/tree.h) with its name, attributes, trimmed text and
     children, each element's children an array written before it. The
     program reads it with the engine's own reader (load/xml.cpp), pieces,
     from= and pictures worked out there as for a file. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template name="tree-file">
    <xsl:call-template name="header">
      <xsl:with-param name="file" select="concat($name, '.cpp')" />
    </xsl:call-template>
    <xsl:text>//&#10;// </xsl:text>
    <xsl:value-of select="$title" />
    <xsl:text>, as written in </xsl:text>
    <xsl:value-of select="$source" />
    <xsl:text>, written out by xgecli --generate&#10;// engine-cpp (games/generators/engine-cpp/tree.xsl) as a tree of C++ data&#10;// (xml/tree.h) that </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>() reads with the engine's own reader. Change the&#10;// game file and generate it again rather than this.&#10;&#10;</xsl:text>
    <xsl:text>#include "game/data.h"&#10;#include "load/xml.h"&#10;#include "xml/tree.h"&#10;&#10;namespace&#10;{&#10;&#9;using xge::TreeAttribute;&#10;&#9;using xge::TreeElement;&#10;&#10;</xsl:text>
    <xsl:apply-templates select="/game" mode="tree" />
    <xsl:text>&#9;const TreeElement file</xsl:text>
    <xsl:apply-templates select="/game" mode="tree-entry" />
    <xsl:text>;&#10;}&#10;&#10;xge::GameData </xsl:text>
    <xsl:value-of select="$function" />
    <xsl:text>()&#10;{&#10;&#9;return xge::readGame(*xge::viewOfTree(file));&#10;}&#10;</xsl:text>
  </xsl:template>

  <!-- The arrays an element's entry points at: its children's first, then
       its attributes and its children. -->
  <xsl:template match="*" mode="tree">
    <xsl:apply-templates select="*" mode="tree" />
    <xsl:if test="@*">
      <xsl:value-of select="concat('&#9;const TreeAttribute a', generate-id(), '[]{ ')" />
      <xsl:for-each select="@*">
        <xsl:if test="position() > 1">, </xsl:if>
        <xsl:text>{ </xsl:text>
        <xsl:call-template name="string">
          <xsl:with-param name="text" select="name()" />
        </xsl:call-template>
        <xsl:text>, </xsl:text>
        <xsl:call-template name="string">
          <xsl:with-param name="text" select="string(.)" />
        </xsl:call-template>
        <xsl:text> }</xsl:text>
      </xsl:for-each>
      <xsl:text> };&#10;</xsl:text>
    </xsl:if>
    <xsl:if test="*">
      <xsl:value-of select="concat('&#9;const TreeElement c', generate-id(), '[]{&#10;')" />
      <xsl:for-each select="*">
        <xsl:text>&#9;&#9;</xsl:text>
        <xsl:apply-templates select="." mode="tree-entry" />
        <xsl:text>,&#10;</xsl:text>
      </xsl:for-each>
      <xsl:text>&#9;};&#10;</xsl:text>
    </xsl:if>
  </xsl:template>

  <!-- An element's entry: { name, attributes, count, text, children, count, line }. -->
  <xsl:template match="*" mode="tree-entry">
    <xsl:variable name="text">
      <xsl:call-template name="trim">
        <xsl:with-param name="text">
          <xsl:for-each select="text()"><xsl:value-of select="." /></xsl:for-each>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:variable>
    <xsl:text>{ </xsl:text>
    <xsl:call-template name="string">
      <xsl:with-param name="text" select="name()" />
    </xsl:call-template>
    <xsl:choose>
      <xsl:when test="@*"><xsl:value-of select="concat(', a', generate-id(), ', ', count(@*))" /></xsl:when>
      <xsl:otherwise>, nullptr, 0</xsl:otherwise>
    </xsl:choose>
    <xsl:text>, </xsl:text>
    <xsl:choose>
      <xsl:when test="$text != ''">
        <xsl:call-template name="string">
          <xsl:with-param name="text" select="$text" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>nullptr</xsl:otherwise>
    </xsl:choose>
    <xsl:choose>
      <xsl:when test="*"><xsl:value-of select="concat(', c', generate-id(), ', ', count(*))" /></xsl:when>
      <xsl:otherwise>, nullptr, 0</xsl:otherwise>
    </xsl:choose>
    <xsl:text>, 0 }</xsl:text>
  </xsl:template>
</xsl:stylesheet>
