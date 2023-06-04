#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include "Client.hpp"

using namespace sf;

const int CLIENTS_SIZE = 2;

const unsigned short C = 1;
const unsigned short C_ = 2;
const unsigned short D = 4;
const unsigned short D_ = 8;
const unsigned short E = 16;
const unsigned short F = 32;
const unsigned short F_ = 64;
const unsigned short G = 128;
const unsigned short G_ = 256;
const unsigned short A = 512;
const unsigned short A_ = 1024;
const unsigned short B = 2048;

const unsigned short INST_BASS = 4096;

const int NOTES_NUMBER = 24;
const int OCTAV = 12;
const unsigned short NOTES[OCTAV] = { C, C_, D, D_, E, F, F_, G, G_, A, A_, B };

void startServer()
{
	Client** clients = new Client * [CLIENTS_SIZE];
	for (int i = 0; i < CLIENTS_SIZE; i++)
	{
		clients[i] = new Client();
	}	

	TcpListener listener;
	Packet rPacket, sPacket;

	listener.listen(Socket::AnyPort);
	std::cout << "My IP is: " << IpAddress::getLocalAddress().toString() << "\nMy port is: " << listener.getLocalPort();

	unsigned char clientsNumber = 0;

	Clock clock;
	while (true)
	{
		if (clock.getElapsedTime().asMilliseconds() > 60)
		{
			clock.restart();

			for (int i = 0; i < CLIENTS_SIZE; i++)
			{
				switch (clients[i]->stage)
				{
				case Stage::Error:
				{
					rPacket.clear();
					std::cout << "\nSomeone disconnected!";
					clients[i]->disconnect();

					clientsNumber--;
					break;
				}

				case Stage::Connection:
				{
					if (listener.accept(clients[i]->tcpSocket) == Socket::Done)
					{
						std::cout << "\nAccepted Jazzman!";
						if (listener.isBlocking())
						{
							listener.setBlocking(false);
						}
						clients[i]->tcpSocket.setBlocking(false);

						clientsNumber++;

						clients[i]->stage = Stage::InstrumentAsk;
					}
					break;
				}

				case Stage::InstrumentAsk:
				{
					Socket::Status s = clients[i]->tcpSocket.receive(rPacket);
					if (s == Socket::Disconnected || s == Socket::Error)
					{
						clients[i]->stage = Stage::Error;
						break;
					}

					if (s == Socket::Done)
					{
						unsigned char c;
						rPacket >> c;
						rPacket.clear();

						if (c == '1')
						{
							clients[i]->instrument = INST_BASS;
						}
						else
						{
							clients[i]->instrument = 0;
						}

						clients[i]->udpSocket.setBlocking(false);
						clients[i]->udpSocket.bind(Socket::AnyPort);
						clients[i]->ip = clients[i]->tcpSocket.getRemoteAddress();

						clients[i]->serverPort = clients[i]->udpSocket.getLocalPort();
						sPacket << clients[i]->serverPort;
						clients[i]->tcpSocket.send(sPacket);
						sPacket.clear();

						clients[i]->stage = Stage::PortAsk;
					}
					break;
				}
				
				case Stage::PortAsk:
				{
					Socket::Status s = clients[i]->tcpSocket.receive(rPacket);
					if (s == Socket::Disconnected || s == Socket::Error)
					{
						clients[i]->stage = Stage::Error;
						break;
					}

					if (s == Socket::Done)
					{
						rPacket >> clients[i]->clientPort;
						rPacket.clear();

						clients[i]->stage = Stage::Playing;
					}
					break;
				}

				case Stage::Playing:
				{
					Socket::Status s = clients[i]->tcpSocket.receive(rPacket);
					if (s == Socket::Disconnected || s == Socket::Error)
					{
						clients[i]->stage = Stage::Error;
						break;
					}

					IpAddress ip;
					if (clients[i]->udpSocket.receive(rPacket, ip, clients[i]->serverPort) == Socket::Done)
					{
						rPacket >> clients[i]->keys;
						rPacket.clear();

						clients[i]->keys = clients[i]->keys | clients[i]->instrument;
					}
					break;
				}
				}
			}
			
			if (clientsNumber > 0)
			{				
				sPacket << clientsNumber;
				for (int i = 0; i < CLIENTS_SIZE; i++)
				{
					if (clients[i]->stage == Stage::Playing)
					{
						sPacket << clients[i]->keys;
					}
				}

				for (int i = 0; i < CLIENTS_SIZE; i++)
				{
					if (clients[i]->stage == Stage::Playing)
					{
						clients[i]->udpSocket.send(sPacket, clients[i]->ip, clients[i]->clientPort);
					}
				}
				sPacket.clear();
			}			
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void startClient()
{
	//RenderWindow window;

	std::string notes[NOTES_NUMBER] = { "C", "C_", "D", "D_", "E", "F", "F_", "G", "G_", "A", "A_", "B",
										"C", "C_", "D", "D_", "E", "F", "F_", "G", "G_", "A", "A_", "B"};

	std::string path = "files\\", inst = "Bass\\";

	SoundBuffer soundBuffers[NOTES_NUMBER];
	for (int i = 0; i < NOTES_NUMBER; i++)
	{
		if (i == OCTAV)
		{
			inst = "Solo\\";
		}
		soundBuffers[i].loadFromFile(path + inst + notes[i] + ".ogg");		
	}

	Sound sounds[NOTES_NUMBER];
	for (int i = 0; i < NOTES_NUMBER; i++)
	{
		new(&sounds[i]) Sound(soundBuffers[i]);
		sounds[i].setLoop(true);
	}

	TcpSocket tcpSocket;
	UdpSocket udpSocket;
	Packet rPacket, sPacket;

	IpAddress serverIp;
	unsigned short serverPort, myPort;

	unsigned char playersNumber;

	Stage stage = Stage::Connection;
	unsigned short keys;
	int octavs;

	while (true)
	{
		switch (stage)
		{
		case Stage::Error:
		{
			rPacket.clear();
			tcpSocket.disconnect();
			new(&tcpSocket) TcpSocket;
			///window.close();
			std::cout << "Disconnected from the server.\n\n";
			stage = Stage::Connection;
			break;
		}

		case Stage::Connection:
		{
			std::fflush(stdin);
			std::fflush(stdout);
			std::cout << "Input server's IP: ";
			std::string sServerIp;
			std::cin >> sServerIp;
			serverIp = IpAddress(sServerIp);
			std::cout << "Input server's port: ";
			std::cin >> serverPort;

			Socket::Status s = tcpSocket.connect(serverIp, serverPort, seconds(2));
			if (s == Socket::Disconnected || s == Socket::Error)
			{
				stage = Stage::Error;
				break;
			}

			if (s == Socket::Done)
			{
				tcpSocket.setBlocking(false);
				std::cout << "There we have Bass - 1, Solo - 2: ";
				unsigned char c;
				std::cin >> c;

				sPacket << c;
				tcpSocket.send(sPacket);
				sPacket.clear();

				//new(&window) RenderWindow(VideoMode(400, 200), "JazzBand");
				stage = Stage::PortAsk;
			}
			break;
		}

		case Stage::PortAsk:
		{
			Socket::Status s = tcpSocket.receive(rPacket);
			if (s == Socket::Disconnected || s == Socket::Error)
			{
				stage = Stage::Error;
				break;
			}

			if (s == Socket::Done)
			{
				rPacket >> serverPort;
				rPacket.clear();

				udpSocket.setBlocking(false);
				udpSocket.bind(Socket::AnyPort);

				myPort = udpSocket.getLocalPort();
				sPacket << myPort;
				tcpSocket.send(sPacket);
				sPacket.clear();

				sPacket << keys;
				udpSocket.send(sPacket, serverIp, serverPort);
				sPacket.clear();

				stage = Stage::Playing;
			}
			break;
		}

		case Stage::Playing:
		{
			Socket::Status s = tcpSocket.receive(rPacket);
			if (s == Socket::Disconnected || s == Socket::Error)
			{
				stage = Stage::Error;
				break;
			}
			
			IpAddress ip;
			if (udpSocket.receive(rPacket, ip, myPort) == Socket::Done)
			{
				rPacket >> playersNumber;
				for (int i = 0; i < playersNumber; i++)
				{					
					rPacket >> keys;

					if (keys & INST_BASS)
					{
						octavs = 0;
					}
					else
					{
						octavs = OCTAV;
					}

					for (int j = 0; j < OCTAV; j++)
					{
						if (keys & NOTES[j])
						{
							if (sounds[j + octavs].getStatus() != SoundSource::Status::Playing)
							{
								sounds[j + octavs].play();
							}
						}
						else
						{							
							if (sounds[j + octavs].getStatus() == SoundSource::Status::Playing)
							{
								sounds[j + octavs].stop();
							}
						}
					}
				}

				rPacket.clear();

				keys = 0;
				if (Keyboard::isKeyPressed(Keyboard::A))
				{
					keys = keys | C;
				}
				if (Keyboard::isKeyPressed(Keyboard::W))
				{
					keys = keys | C_;
				}
				if (Keyboard::isKeyPressed(Keyboard::S))
				{
					keys = keys | D;
				}
				if (Keyboard::isKeyPressed(Keyboard::E))
				{
					keys = keys | D_;
				}
				if (Keyboard::isKeyPressed(Keyboard::D))
				{
					keys = keys | E;
				}
				if (Keyboard::isKeyPressed(Keyboard::F))
				{
					keys = keys | F;
				}
				if (Keyboard::isKeyPressed(Keyboard::T))
				{
					keys = keys | F_;
				}
				if (Keyboard::isKeyPressed(Keyboard::G))
				{
					keys = keys | G;
				}
				if (Keyboard::isKeyPressed(Keyboard::Y))
				{
					keys = keys | G_;
				}
				if (Keyboard::isKeyPressed(Keyboard::H))
				{
					keys = keys | A;
				}
				if (Keyboard::isKeyPressed(Keyboard::U))
				{
					keys = keys | A_;
				}
				if (Keyboard::isKeyPressed(Keyboard::J))
				{
					keys = keys | B;
				}				
				
				sPacket << keys;
				udpSocket.send(sPacket, serverIp, serverPort);
				sPacket.clear();

				if (Keyboard::isKeyPressed(Keyboard::Escape))
				{
					stage = Stage::Error;					
				}
			}
			break;
		}
		}
	}
}

int main()
{
	int i;
	std::cout << "For server input 1, for client 2: ";
	std::cin >> i;
	if (i == 1)
	{
		startServer();
	}
	else
	{
		startClient();
	}

    return 0;
}