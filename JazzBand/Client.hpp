#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

using namespace sf;

enum Stage { Error, Connection, InstrumentAsk, PortAsk, PortAnswer, Playing };

class Client
{
public:

	TcpSocket tcpSocket;
	UdpSocket udpSocket;
	IpAddress ip;
	unsigned short clientPort, serverPort, instrument, keys;
	Stage stage;

	Client() 
	{ 
		stage = Stage::Connection;
		keys = 0;
	}

	void disconnect()
	{
		stage = Stage::Connection;
		keys = 0;
	}
};