// engine.h
// XML Game Engine
// author: beefviper
// date: 20-Jul-26

namespace xge
{
	class Engine
	{
	public:
		Engine(Game& game);
		void run();
	private:
		Game& game;
	};
}
