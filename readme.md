# XMLGameEngine

An engine for describing games built around tinyxml2, exprtk, and SFML.

XMLGameEngine dependencies:
* tinyxml2 https://github.com/leethomason/tinyxml2
* exprtk   https://github.com/ArashPartow/exprtk
* SFML     https://github.com/SFML/SFML

XMLGameEngine stores a complete description of a game in an XML.
The XML file is loaded and parsed by *tinyxml2*.
Any variables, expressions, or function calls in the XML data is evaluated by *exprtk*.
The resulting values and objects are used to render the game with *SFML*.

XMLGameEngine is currently an alpha, and not complete.

Things that work:
* Loading and parsing XML
* Expression and function evaluation
* Creation of variables, objects, and states
* Key handling for objects and states
* Some drawing functions: shape.circle(), shape.rectangle()
* Basic text rendering: text()
* Some collision: screen boundaries, circle-rectangle

Things still missing:
* Collisions of all types
* Scoring system
* Win Condition
