// main.cpp
// XML Game Engine
// author: beefviper
// date: Sept 18, 2020

#include "game.h"
#include "engine.h"

#include <iostream>
#include <string>

#include <stdio.h>

#include <xercesc/util/XMLString.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/validators/common/Grammar.hpp>

using namespace xercesc;

class ParserErrorHandler : public ErrorHandler
{
private:
    void reportParseException(const SAXParseException& ex)
    {
        char* msg = XMLString::transcode(ex.getMessage());
        fprintf(stderr, "at line %llu column %llu, %s\n",
            ex.getColumnNumber(), ex.getLineNumber(), msg);
        XMLString::release(&msg);
    }

public:
    void warning(const SAXParseException& ex)
    {
        reportParseException(ex);
    }

    void error(const SAXParseException& ex)
    {
        reportParseException(ex);
    }

    void fatalError(const SAXParseException& ex)
    {
        reportParseException(ex);
    }

    void resetErrors()
    {
    }
};

void ValidateSchema(const char* schemaFilePath, const char* xmlFilePath)
{
    XercesDOMParser domParser;
    if (domParser.loadGrammar(schemaFilePath, Grammar::SchemaGrammarType) == NULL)
    {
        fprintf(stderr, "couldn't load schema\n");
        return;
    }

    ParserErrorHandler parserErrorHandler;

    domParser.setErrorHandler(&parserErrorHandler);
    domParser.setValidationScheme(XercesDOMParser::Val_Auto);
    domParser.setDoNamespaces(true);
    domParser.setDoSchema(true);
    domParser.setValidationConstraintFatal(true);

    domParser.parse(xmlFilePath);
    if (domParser.getErrorCount() == 0)
        printf("XML file validated against the schema successfully\n");
    else
        printf("XML file doesn't conform to the schema\n");
}

int main(int argc, const char* argv[])
{
    if (argc != 3)
    {
        printf("SchemaValidator <schema file> <xml file>\n");
        return 0;
    }

    XMLPlatformUtils::Initialize();

    ValidateSchema(argv[1], argv[2]);

    XMLPlatformUtils::Terminate();

    return 0;
}




//int main(void)
//{
//	std::string filename{ "games/pong.xml" };
//	xge::Game game{ filename };
//
//	game.printGame();
//	game.setCurrentState(0);
//
//	xge::Engine engine(game);
//	engine.loop();
//
//	return EXIT_SUCCESS;
//}
