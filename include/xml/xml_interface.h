// xml_interface.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

namespace xge
{
enum class XMLBackEnd;

class XmlInterface
{
public:
	XmlInterface() = default;
	~XmlInterface() = default;

	bool loadFromFile(const std::string& filePath, Game& game, XMLBackEnd backend);

};
}