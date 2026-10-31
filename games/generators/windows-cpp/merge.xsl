<?xml version="1.0" encoding="UTF-8"?>
<!-- merge.xsl -->
<!-- XML Game Engine -->
<!-- author: beefviper -->
<!-- date: Oct 25, 2026 -->

<!-- The game with its <kit> and from= worked out, as the engine works them
     out before it reads a game (xge/lib/load/merge.cpp, the same rule step
     for step): every <objects> holds whole objects and groups, a piece of
     the kit taken (by from= or by its name) written out changed by the
     take, a state's and a scene's objects those of the one it is from=
     changed by its own, and a piece that a <spawn>, <fire> or <release>
     names put in beside what spawns it with <exists>false</exists>, as
     the one its copies are made from. data.xsl then writes out that game,
     with no <kit> left in it. -->
<xsl:stylesheet version="1.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:exsl="http://exslt.org/common"
    extension-element-prefixes="exsl">

  <xsl:variable name="merge-source" select="/game" />

  <!-- The kit's pieces, each one from= an earlier piece written out whole. -->
  <xsl:variable name="merge-pieces-tree">
    <xsl:for-each select="/game/kit/*">
      <xsl:call-template name="merge-piece" />
    </xsl:for-each>
  </xsl:variable>
  <xsl:variable name="merge-pieces" select="exsl:node-set($merge-pieces-tree)/*" />

  <xsl:template name="merge-piece">
    <xsl:choose>
      <xsl:when test="@from">
        <xsl:variable name="base">
          <xsl:for-each select="preceding-sibling::*[@name = current()/@from][1]">
            <xsl:call-template name="merge-piece" />
          </xsl:for-each>
        </xsl:variable>
        <xsl:call-template name="merge-changed">
          <xsl:with-param name="base" select="exsl:node-set($base)/*" />
          <xsl:with-param name="over" select="." />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="." />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ===================================================================
       The change rule (changed, mergeChildren)
       =================================================================== -->

  <!-- $base changed by $over: over's attributes over base's (from= dropped),
       and their children merged. -->
  <xsl:template name="merge-changed">
    <xsl:param name="base" />
    <xsl:param name="over" />
    <xsl:element name="{name($base)}">
      <xsl:for-each select="$base/@*[name() != 'from']">
        <xsl:if test="not($over/@*[name() = name(current())])">
          <xsl:copy-of select="." />
        </xsl:if>
      </xsl:for-each>
      <xsl:copy-of select="$over/@*[name() != 'from']" />
      <xsl:call-template name="merge-children">
        <xsl:with-param name="parent" select="name($base)" />
        <xsl:with-param name="base" select="$base/*" />
        <xsl:with-param name="over" select="$over/*" />
      </xsl:call-template>
    </xsl:element>
  </xsl:template>

  <!-- How a child of $parent meets base's like it (matchOf): whole, deep,
       named, inner, tag, all or added. -->
  <xsl:template name="merge-match">
    <xsl:param name="parent" />
    <xsl:param name="tag" />
    <xsl:choose>
      <xsl:when test="$parent = 'variables' or $parent = 'actions' or $parent = 'timers'">named</xsl:when>
      <xsl:when test="$parent = 'collisions'">
        <xsl:choose>
          <xsl:when test="$tag = 'collision'">added</xsl:when>
          <xsl:when test="$tag = 'solid'">all</xsl:when>
          <xsl:otherwise>tag</xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:when test="$parent = 'position' or $parent = 'velocity' or $parent = 'acceleration' or $parent = 'pivot' or $parent = 'padding'">tag</xsl:when>
      <xsl:when test="$tag = 'sprite' or $tag = 'animation'">whole</xsl:when>
      <xsl:when test="$tag = 'group' or $tag = 'object'">deep</xsl:when>
      <xsl:when test="$tag = 'position' or $tag = 'velocity' or $tag = 'acceleration' or $tag = 'pivot' or $tag = 'padding'
          or $tag = 'variables' or $tag = 'actions' or $tag = 'timers' or $tag = 'collisions'">inner</xsl:when>
      <xsl:when test="$tag = 'member' or $tag = 'row' or $tag = 'column' or $tag = 'cell'">all</xsl:when>
      <xsl:otherwise>tag</xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The children of a $parent: $base's in order, each kept, or in place
       of or changed by $over's like it, then $over's that took no place (of
       those alike, only the last). -->
  <xsl:template name="merge-children">
    <xsl:param name="parent" />
    <xsl:param name="base" />
    <xsl:param name="over" />
    <xsl:for-each select="$base">
      <xsl:variable name="child" select="." />
      <xsl:variable name="match">
        <xsl:call-template name="merge-match">
          <xsl:with-param name="parent" select="$parent" />
          <xsl:with-param name="tag" select="name()" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:variable name="like" select="($over[name() = name($child)][
          (($match = 'whole' or $match = 'deep') and string(@name) = string($child/@name))
          or ($match = 'named' and string($child/@name) != '' and string(@name) = string($child/@name))
          or $match = 'inner' or $match = 'tag' or $match = 'all']
        | $over[$match = 'all' and ($child/self::member or $child/self::row or $child/self::column or $child/self::cell)
          and (self::member or self::row or self::column or self::cell)])[last()]" />
      <xsl:choose>
        <xsl:when test="($match = 'whole' or $match = 'named') and $like">
          <xsl:copy-of select="$like" />
        </xsl:when>
        <xsl:when test="($match = 'deep' or $match = 'inner') and $like">
          <xsl:call-template name="merge-changed">
            <xsl:with-param name="base" select="." />
            <xsl:with-param name="over" select="$like" />
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$match = 'all' and $like" />
        <xsl:when test="$match = 'tag' and $like">
          <!-- all of over's of the tag, where base's first of it was -->
          <xsl:if test="not(preceding-sibling::*[name() = name($child)])">
            <xsl:copy-of select="$over[name() = name($child)]" />
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:copy-of select="." />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:for-each select="$over">
      <xsl:variable name="child" select="." />
      <xsl:variable name="match">
        <xsl:call-template name="merge-match">
          <xsl:with-param name="parent" select="$parent" />
          <xsl:with-param name="tag" select="name()" />
        </xsl:call-template>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="$match = 'all' or $match = 'added'">
          <xsl:copy-of select="." />
        </xsl:when>
        <xsl:when test="$match = 'tag'">
          <xsl:if test="not($base[name() = name($child)])">
            <xsl:copy-of select="." />
          </xsl:if>
        </xsl:when>
        <xsl:when test="$match = 'named' and string(@name) = ''">
          <xsl:copy-of select="." />
        </xsl:when>
        <xsl:when test="$match = 'inner'">
          <xsl:if test="not($base[name() = name($child)]) and not(following-sibling::*[name() = name($child)])">
            <xsl:copy-of select="." />
          </xsl:if>
        </xsl:when>
        <xsl:otherwise>
          <xsl:if test="not($base[name() = name($child) and string(@name) = string($child/@name)])
              and not(following-sibling::*[name() = name($child) and string(@name) = string($child/@name)])">
            <xsl:copy-of select="." />
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:template>

  <!-- ===================================================================
       Groups: a member or an inner group changed in place, a member taken
       =================================================================== -->

  <!-- The group $group with that member alone in it (onlyMember). -->
  <xsl:template name="merge-only-member">
    <xsl:param name="group" />
    <xsl:param name="member" />
    <xsl:for-each select="$group">
      <xsl:copy>
        <xsl:copy-of select="@*" />
        <xsl:for-each select="*">
          <xsl:choose>
            <xsl:when test="self::member">
              <xsl:if test="@name = $member">
                <xsl:copy-of select="." />
              </xsl:if>
            </xsl:when>
            <xsl:when test="self::group">
              <xsl:if test="member[@name = $member] or .//group/member[@name = $member]">
                <xsl:call-template name="merge-only-member">
                  <xsl:with-param name="group" select="." />
                  <xsl:with-param name="member" select="$member" />
                </xsl:call-template>
              </xsl:if>
            </xsl:when>
            <xsl:otherwise>
              <xsl:copy-of select="." />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </xsl:copy>
    </xsl:for-each>
  </xsl:template>

  <!-- The group $group with $over changing its member $member (changedMember):
       its sprites, animations, position and velocity in the member, the rest
       in the group that holds it. $rename is the member's new name. -->
  <xsl:template name="merge-changed-member">
    <xsl:param name="group" />
    <xsl:param name="member" />
    <xsl:param name="over" />
    <xsl:param name="rename" select="''" />
    <xsl:variable name="own">
      <member>
        <xsl:if test="$rename != '' and $rename != $member">
          <xsl:attribute name="name"><xsl:value-of select="$rename" /></xsl:attribute>
        </xsl:if>
        <xsl:copy-of select="$over/sprite | $over/animation | $over/position | $over/velocity" />
      </member>
    </xsl:variable>
    <xsl:variable name="rest">
      <group>
        <xsl:copy-of select="$over/@*[name() != 'name' and name() != 'from']" />
        <xsl:copy-of select="$over/*[not(self::sprite or self::animation or self::position or self::velocity)]" />
      </group>
    </xsl:variable>
    <xsl:call-template name="merge-member-in">
      <xsl:with-param name="holder" select="$group" />
      <xsl:with-param name="member" select="$member" />
      <xsl:with-param name="own" select="exsl:node-set($own)/*" />
      <xsl:with-param name="rest" select="exsl:node-set($rest)/*" />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="merge-member-in">
    <xsl:param name="holder" />
    <xsl:param name="member" />
    <xsl:param name="own" />
    <xsl:param name="rest" />
    <xsl:variable name="made">
      <xsl:for-each select="$holder">
        <xsl:copy>
          <xsl:copy-of select="@*" />
          <xsl:for-each select="*">
            <xsl:choose>
              <xsl:when test="self::member and @name = $member">
                <xsl:call-template name="merge-changed">
                  <xsl:with-param name="base" select="." />
                  <xsl:with-param name="over" select="$own" />
                </xsl:call-template>
              </xsl:when>
              <xsl:when test="self::group and (member[@name = $member] or .//group/member[@name = $member])">
                <xsl:call-template name="merge-member-in">
                  <xsl:with-param name="holder" select="." />
                  <xsl:with-param name="member" select="$member" />
                  <xsl:with-param name="own" select="$own" />
                  <xsl:with-param name="rest" select="$rest" />
                </xsl:call-template>
              </xsl:when>
              <xsl:otherwise>
                <xsl:copy-of select="." />
              </xsl:otherwise>
            </xsl:choose>
          </xsl:for-each>
        </xsl:copy>
      </xsl:for-each>
    </xsl:variable>
    <xsl:choose>
      <xsl:when test="$holder/member[@name = $member] and ($rest/* or $rest/@*)">
        <xsl:call-template name="merge-changed">
          <xsl:with-param name="base" select="exsl:node-set($made)/*" />
          <xsl:with-param name="over" select="$rest" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$made" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- The group $holder with $over changing the group inside it called $group (changedInner). -->
  <xsl:template name="merge-changed-inner">
    <xsl:param name="holder" />
    <xsl:param name="group" />
    <xsl:param name="over" />
    <xsl:for-each select="$holder">
      <xsl:copy>
        <xsl:copy-of select="@*" />
        <xsl:for-each select="*">
          <xsl:choose>
            <xsl:when test="self::group and @name = $group">
              <xsl:call-template name="merge-changed">
                <xsl:with-param name="base" select="." />
                <xsl:with-param name="over" select="$over" />
              </xsl:call-template>
            </xsl:when>
            <xsl:when test="self::group and (group[@name = $group] or .//group/group[@name = $group])">
              <xsl:call-template name="merge-changed-inner">
                <xsl:with-param name="holder" select="." />
                <xsl:with-param name="group" select="$group" />
                <xsl:with-param name="over" select="$over" />
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:copy-of select="." />
            </xsl:otherwise>
          </xsl:choose>
        </xsl:for-each>
      </xsl:copy>
    </xsl:for-each>
  </xsl:template>

  <!-- What $over makes of the piece (or named member) $name (Kit::take). -->
  <xsl:template name="merge-take">
    <xsl:param name="name" />
    <xsl:param name="over" />
    <xsl:variable name="piece" select="$merge-pieces[@name = $name]" />
    <xsl:choose>
      <xsl:when test="$piece">
        <xsl:call-template name="merge-changed">
          <xsl:with-param name="base" select="$piece" />
          <xsl:with-param name="over" select="$over" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="only">
          <xsl:call-template name="merge-only-member">
            <xsl:with-param name="group" select="$merge-pieces[self::group][.//member[@name = $name]]" />
            <xsl:with-param name="member" select="$name" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="merge-changed-member">
          <xsl:with-param name="group" select="exsl:node-set($only)/*" />
          <xsl:with-param name="member" select="$name" />
          <xsl:with-param name="over" select="$over" />
          <xsl:with-param name="rename" select="string($over/@name)" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ===================================================================
       An <objects> laid out (layOut)
       =================================================================== -->

  <!-- $list, each of $written placed in it in turn. -->
  <xsl:template name="merge-lay-out">
    <xsl:param name="list" />
    <xsl:param name="written" />
    <xsl:choose>
      <xsl:when test="not($written)">
        <xsl:copy-of select="$list" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:variable name="next">
          <xsl:call-template name="merge-place">
            <xsl:with-param name="list" select="$list" />
            <xsl:with-param name="written" select="$written[1]" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:call-template name="merge-lay-out">
          <xsl:with-param name="list" select="exsl:node-set($next)/*" />
          <xsl:with-param name="written" select="$written[position() &gt; 1]" />
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="merge-place">
    <xsl:param name="list" />
    <xsl:param name="written" />
    <xsl:variable name="name" select="string($written/@name)" />
    <xsl:variable name="same" select="$list[@name = $name][1]" />
    <xsl:variable name="holder" select="$list[self::group][member[@name = $name] or .//group/member[@name = $name]][1]" />
    <xsl:variable name="outer" select="$list[self::group][group[@name = $name] or .//group/group[@name = $name]][1]" />
    <xsl:variable name="changing">
      <xsl:choose>
        <xsl:when test="string($written/@from) != ''" />
        <xsl:when test="$same"><xsl:value-of select="generate-id($same)" /></xsl:when>
        <xsl:when test="$holder"><xsl:value-of select="generate-id($holder)" /></xsl:when>
        <xsl:when test="$outer and $written/self::group"><xsl:value-of select="generate-id($outer)" /></xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:for-each select="$list">
      <xsl:choose>
        <xsl:when test="generate-id() != $changing">
          <xsl:copy-of select="." />
        </xsl:when>
        <xsl:when test="$same">
          <xsl:call-template name="merge-changed">
            <xsl:with-param name="base" select="." />
            <xsl:with-param name="over" select="$written" />
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="$holder">
          <xsl:call-template name="merge-changed-member">
            <xsl:with-param name="group" select="." />
            <xsl:with-param name="member" select="$name" />
            <xsl:with-param name="over" select="$written" />
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="merge-changed-inner">
            <xsl:with-param name="holder" select="." />
            <xsl:with-param name="group" select="$name" />
            <xsl:with-param name="over" select="$written" />
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
    <xsl:choose>
      <xsl:when test="string($written/@from) != ''">
        <xsl:call-template name="merge-take">
          <xsl:with-param name="name" select="string($written/@from)" />
          <xsl:with-param name="over" select="$written" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$changing != ''" />
      <xsl:when test="($written/self::map and $merge-pieces[self::map][@name = $name]) or (not($written/self::map) and ($merge-pieces[not(self::map)][@name = $name] or $merge-pieces[self::group]//member[@name = $name]))">
        <xsl:call-template name="merge-take">
          <xsl:with-param name="name" select="$name" />
          <xsl:with-param name="over" select="$written" />
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$written" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- ===================================================================
       What is spawned, and the pieces put in for it (addWaiting)
       =================================================================== -->

  <!-- The names an element can be called by (namesIn), each between spaces. -->
  <xsl:template name="merge-names">
    <xsl:param name="nodes" />
    <xsl:for-each select="$nodes/@name | $nodes//*[self::group or self::object or self::member or self::map]/@name">
      <xsl:value-of select="concat(' ', ., ' ')" />
    </xsl:for-each>
  </xsl:template>

  <!-- What a <spawn>, <fire> or <release> in $nodes names (spawnedIn), and
       in a path they <follow>, a <keys> set their <inputs keys> take or a
       <conditions> set a <condition set> takes, each between spaces.
       $taken: the sets already looked in. -->
  <xsl:template name="merge-spawned">
    <xsl:param name="nodes" />
    <xsl:param name="taken" select="' '" />
    <xsl:for-each select="($nodes | $nodes//*)[self::spawn or self::fire or self::release][@object != '']">
      <xsl:value-of select="concat(' ', @object, ' ')" />
    </xsl:for-each>
    <xsl:variable name="keys">
      <xsl:for-each select="($nodes | $nodes//*)[self::inputs]">
        <xsl:value-of select="concat(' ', normalize-space(@keys), ' ')" />
        <xsl:for-each select="keys">
          <xsl:value-of select="concat(' ', normalize-space(.), ' ')" />
        </xsl:for-each>
      </xsl:for-each>
    </xsl:variable>
    <xsl:variable name="more"
        select="$merge-source/paths/path[@name = ($nodes | $nodes//*)[self::follow]/@path][not(contains($taken, concat(' path:', @name, ' ')))]
          | $merge-source/states/keys[contains(concat(' ', translate($keys, '&#9;&#10;&#13;', '   '), ' '), concat(' ', @name, ' '))][not(contains($taken, concat(' keys:', @name, ' ')))]
          | $merge-source/states/conditions[@name = ($nodes | $nodes//*)[self::condition]/@set][not(contains($taken, concat(' conditions:', @name, ' ')))]" />
    <xsl:if test="$more">
      <xsl:call-template name="merge-spawned">
        <xsl:with-param name="nodes" select="$more" />
        <xsl:with-param name="taken">
          <xsl:value-of select="$taken" />
          <xsl:for-each select="$more">
            <xsl:value-of select="concat(' ', name(), ':', @name, ' ')" />
          </xsl:for-each>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- $list, then a waiting copy of each piece of the kit that $spawned (or
       an element of the list) names and that nothing in the list or $seen
       is called; and so on for what those name. Alphabetical, round by round. -->
  <xsl:template name="merge-add-waiting">
    <xsl:param name="list" />
    <xsl:param name="spawned" select="' '" />
    <xsl:param name="seen" select="' '" />
    <xsl:copy-of select="$list" />
    <xsl:call-template name="merge-waiting-round">
      <xsl:with-param name="spawned">
        <xsl:value-of select="$spawned" />
        <xsl:call-template name="merge-spawned">
          <xsl:with-param name="nodes" select="$list" />
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="placed">
        <xsl:value-of select="$seen" />
        <xsl:call-template name="merge-names">
          <xsl:with-param name="nodes" select="$list" />
        </xsl:call-template>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="merge-waiting-round">
    <xsl:param name="spawned" />
    <xsl:param name="placed" />
    <xsl:variable name="pieces" select="$merge-pieces[not(contains($placed, concat(' ', @name, ' ')))][
        contains($spawned, concat(' ', @name, ' '))
        or (self::group and .//member[@name != ''][contains($spawned, concat(' ', @name, ' ')) and not(contains($placed, concat(' ', @name, ' ')))])]" />
    <xsl:if test="$pieces">
      <xsl:variable name="copies">
        <xsl:for-each select="$pieces">
          <xsl:sort select="@name" data-type="text" order="ascending" />
          <xsl:copy>
            <xsl:copy-of select="@*[name() != 'from']" />
            <xsl:copy-of select="*[not(self::exists)]" />
            <exists>false</exists>
          </xsl:copy>
        </xsl:for-each>
      </xsl:variable>
      <xsl:copy-of select="$copies" />
      <xsl:call-template name="merge-waiting-round">
        <xsl:with-param name="spawned">
          <xsl:call-template name="merge-spawned">
            <xsl:with-param name="nodes" select="exsl:node-set($copies)/*" />
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="placed">
          <xsl:value-of select="$placed" />
          <xsl:call-template name="merge-names">
            <xsl:with-param name="nodes" select="exsl:node-set($copies)/*" />
          </xsl:call-template>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>

  <!-- ===================================================================
       The game, its states and their scenes
       =================================================================== -->

  <xsl:variable name="merge-game-list-tree">
    <xsl:variable name="laid">
      <xsl:call-template name="merge-lay-out">
        <xsl:with-param name="list" select="/.." />
        <xsl:with-param name="written" select="/game/objects/*" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="merge-add-waiting">
      <xsl:with-param name="list" select="exsl:node-set($laid)/*" />
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="merge-game-list" select="exsl:node-set($merge-game-list-tree)/*" />
  <xsl:variable name="merge-game-names">
    <xsl:text> </xsl:text>
    <xsl:call-template name="merge-names">
      <xsl:with-param name="nodes" select="$merge-game-list" />
    </xsl:call-template>
  </xsl:variable>

  <!-- The objects of $state, laid out (readState). -->
  <xsl:template name="merge-state-list">
    <xsl:param name="state" />
    <xsl:variable name="inherited">
      <xsl:if test="$state/@from">
        <xsl:call-template name="merge-state-list">
          <xsl:with-param name="state" select="$state/preceding-sibling::state[@name = $state/@from][1]" />
        </xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="laid">
      <xsl:call-template name="merge-lay-out">
        <xsl:with-param name="list" select="exsl:node-set($inherited)/*" />
        <xsl:with-param name="written" select="$state/objects/*" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="merge-add-waiting">
      <xsl:with-param name="list" select="exsl:node-set($laid)/*" />
      <xsl:with-param name="spawned">
        <xsl:text> </xsl:text>
        <xsl:call-template name="merge-spawned">
          <xsl:with-param name="nodes" select="$state/inputs | $state/conditions | $state/computer | $state/timers" />
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="seen" select="$merge-game-names" />
    </xsl:call-template>
  </xsl:template>

  <!-- The objects of $scene, laid out (readScenes); $seen, the names of its
       state's objects and the game's. -->
  <xsl:template name="merge-scene-list">
    <xsl:param name="scene" />
    <xsl:param name="seen" />
    <xsl:variable name="inherited">
      <xsl:if test="$scene/@from">
        <xsl:call-template name="merge-scene-list">
          <xsl:with-param name="scene" select="$scene/preceding-sibling::scene[@name = $scene/@from][1]" />
          <xsl:with-param name="seen" select="$seen" />
        </xsl:call-template>
      </xsl:if>
    </xsl:variable>
    <xsl:variable name="laid">
      <xsl:call-template name="merge-lay-out">
        <xsl:with-param name="list" select="exsl:node-set($inherited)/*" />
        <xsl:with-param name="written" select="$scene/objects/*" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:call-template name="merge-add-waiting">
      <xsl:with-param name="list" select="exsl:node-set($laid)/*" />
      <xsl:with-param name="spawned">
        <xsl:text> </xsl:text>
        <xsl:call-template name="merge-spawned">
          <xsl:with-param name="nodes" select="$scene/conditions | $scene/timers" />
        </xsl:call-template>
      </xsl:with-param>
      <xsl:with-param name="seen" select="$seen" />
    </xsl:call-template>
  </xsl:template>

  <!-- The whole game, merged: what data.xsl writes out. -->
  <xsl:template name="merge-game">
    <game>
      <xsl:copy-of select="/game/@*" />
      <xsl:for-each select="/game/*">
        <xsl:choose>
          <xsl:when test="self::kit" />
          <xsl:when test="self::objects">
            <objects>
              <xsl:copy-of select="$merge-game-list" />
            </objects>
          </xsl:when>
          <xsl:when test="self::states">
            <states>
              <xsl:for-each select="*">
                <xsl:choose>
                  <xsl:when test="self::state">
                    <xsl:call-template name="merge-state" />
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:copy-of select="." />
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:for-each>
            </states>
          </xsl:when>
          <xsl:otherwise>
            <xsl:copy-of select="." />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </game>
  </xsl:template>

  <xsl:template name="merge-state">
    <xsl:variable name="list-tree">
      <xsl:call-template name="merge-state-list">
        <xsl:with-param name="state" select="." />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="list" select="exsl:node-set($list-tree)/*" />
    <xsl:variable name="seen">
      <xsl:value-of select="$merge-game-names" />
      <xsl:call-template name="merge-names">
        <xsl:with-param name="nodes" select="$list" />
      </xsl:call-template>
    </xsl:variable>
    <state>
      <xsl:copy-of select="@*" />
      <xsl:if test="$list">
        <objects>
          <xsl:copy-of select="$list" />
        </objects>
      </xsl:if>
      <xsl:for-each select="*[not(self::objects)]">
        <xsl:choose>
          <xsl:when test="self::scenes">
            <scenes>
              <xsl:copy-of select="@*" />
              <xsl:for-each select="scene">
                <xsl:variable name="scene-tree">
                  <xsl:call-template name="merge-scene-list">
                    <xsl:with-param name="scene" select="." />
                    <xsl:with-param name="seen" select="$seen" />
                  </xsl:call-template>
                </xsl:variable>
                <scene>
                  <xsl:copy-of select="@*" />
                  <xsl:if test="exsl:node-set($scene-tree)/*">
                    <objects>
                      <xsl:copy-of select="$scene-tree" />
                    </objects>
                  </xsl:if>
                  <xsl:copy-of select="*[not(self::objects)]" />
                </scene>
              </xsl:for-each>
            </scenes>
          </xsl:when>
          <xsl:otherwise>
            <xsl:copy-of select="." />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </state>
  </xsl:template>
</xsl:stylesheet>
