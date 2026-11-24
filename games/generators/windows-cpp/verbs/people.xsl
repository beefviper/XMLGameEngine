<?xml version="1.0" encoding="UTF-8"?>
<!-- people.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Nov 24, 2026 -->

<!-- <people>N</people> on a key: how many people play, the rest of the seats the computer's. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <!-- covered: a number, on a key -->
  <xsl:template match="input/people[string(number(.)) != 'NaN' and not(*)]" mode="covers" />

  <xsl:template match="people" mode="do">
    <xsl:value-of select="concat('people = ', normalize-space(.), ';')" />
  </xsl:template>
</xsl:stylesheet>
