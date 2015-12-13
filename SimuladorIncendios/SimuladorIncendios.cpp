#include <iostream>
#include <future>
#include <random>
#include <functional>
#include <chrono>
#include <thread>
#include <memory>
#include <string>
#include <sstream>

#define TAMANHO_MAPA 10
#define NROLADOS 4
#define ESQUERDA 0
#define NORTE 1
#define DIREITA 2
#define SUL 3

#ifdef _WIN32

#include <windows.h>

void cls()
{

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coordScreen = { 0, 0 };    // home for the cursor 
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	// Get the number of character cells in the current buffer. 

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.

	if (!FillConsoleOutputCharacter(hConsole, (TCHAR) ' ',
		dwConSize, coordScreen, &cCharsWritten))
		return;

	// Get the current text attribute.

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;

	// Set the buffer's attributes accordingly.

	if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes,
		dwConSize, coordScreen, &cCharsWritten))
		return;

	// Put the cursor at its home coordinates.

	SetConsoleCursorPosition(hConsole, coordScreen);
}

void gotoxy(int x, int y)
{
	COORD p = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), p);
}


#else

#include <unistd.h>
#include <term.h>
#include <curses.h>

void cls()
{
	std::cout << "\x1B[2J\x1B[H";
}

void gotoxy(int x, int y)
{
	int err;
	if (!cur_term)
	if (setupterm(NULL, STDOUT_FILENO, &err) == ERR)
		return;
	putp(tparm(tigetstr((char *)"cup"), y, x, 0, 0, 0, 0, 0, 0, 0));
}

#endif 

enum Tipo
{
	VAZIO,
	BOMBEIRO,
	FOGO,
	REFUGIADO,
	VITIMA
};

class Entidade
{
	std::string representacao;
	Tipo tipo;
public:
	Entidade(std::string _r, Tipo _t, int _l, int _c) : representacao(_r), tipo(_t), linha(_l), coluna(_c) {}
	inline Tipo getTipo() { return tipo; }
	inline int getLinha() { return linha; }
	inline int getColuna() { return coluna; }
	std::string getRepresentacao();
	void setRepresentacao(std::string);
	virtual void moverEntidade();
	void print();
protected:
	int linha, coluna;
};

void Entidade::moverEntidade()
{
	int m = rand() % NROLADOS;
	switch (m)
	{
	case ESQUERDA:
		if (coluna > 0)
		{
			coluna--;
		}
		break;
	case DIREITA:
		if (coluna < TAMANHO_MAPA)
		{
			coluna++;
		}
		break;
	case NORTE:
		if (linha > 0)
		{
			linha--;
		}
		break;
	case SUL:
		if (linha < TAMANHO_MAPA)
		{
			linha++;
		}
		break;
	default:
		break;
	}
}

void Entidade::setRepresentacao(std::string r)
{
	representacao = r;
}

std::string Entidade::getRepresentacao()
{
	return representacao;
}

void Entidade::print()
{
	gotoxy(coluna * 3, linha);
	std::cout << representacao;
}

class Bombeiro : public Entidade
{
	bool carregado = false;
public:
	Bombeiro(std::string r, int l, int c) : Entidade(r, Tipo::BOMBEIRO, l, c) {}
	virtual void moverEntidade();
	void carregar();
	void descarregar();
};

void Bombeiro::moverEntidade()
{
	if (carregado)
	{
		//move pra ambulancia
	}

	int m = rand() % NROLADOS;
	switch (m)
	{
	case ESQUERDA:
		if (coluna > 0)
		{
			coluna--;
		}
		break;
	case DIREITA:
		if (coluna < TAMANHO_MAPA)
		{
			coluna++;
		}
		break;
	case NORTE:
		if (linha > 0)
		{
			linha--;
		}
		break;
	case SUL:
		if (linha < TAMANHO_MAPA)
		{
			linha++;
		}
		break;
	default:
		break;
	}
}

void Bombeiro::carregar()
{
	carregado = true;
}

void Bombeiro::descarregar()
{
	carregado = false;
}

class Fogo : public Entidade
{
public:
	Fogo(std::string r, int l, int c) : Entidade(r, Tipo::FOGO, l, c) {}
	virtual void moverEntidade();
};

void Fogo::moverEntidade()
{
	linha = rand() % TAMANHO_MAPA;
	coluna = rand() % TAMANHO_MAPA;
}

class Vitima : public Entidade
{
public:
	Vitima(std::string r, int l, int c) : Entidade(r, Tipo::VITIMA, l, c) {}
};

class Refugiado : public Entidade
{
public:
	Refugiado(std::string r, int l, int c) : Entidade(r, Tipo::REFUGIADO, l, c) {}
	virtual void moverEntidade();
};

void Refugiado::moverEntidade()
{
	int m = rand() % NROLADOS;
	switch (m)
	{
	case ESQUERDA:
		if (coluna > 0)
		{
			coluna--;
		}
		break;
	case DIREITA:
		if (coluna < TAMANHO_MAPA)
		{
			coluna++;
		}
		break;
	case NORTE:
		if (linha > 0)
		{
			linha--;
		}
		break;
	case SUL:
		if (linha < TAMANHO_MAPA)
		{
			linha++;
		}
		break;
	default:
		break;
	}
}

/////////////////// Entidades

using VetorPtrEntidades = std::vector<std::vector<std::shared_ptr<Entidade>>>;

VetorPtrEntidades entidades;
std::mutex m_mutex;
VetorPtrEntidades iniEntidades();

int n_bombeiros = 0;

VetorPtrEntidades iniEntidades()
{
	VetorPtrEntidades _entidades;
	for (int l = 0; l < TAMANHO_MAPA; ++l)
	{
		std::vector<std::shared_ptr<Entidade>> _e;
		for (int c = 0; c < TAMANHO_MAPA; ++c)
		{
			_e.push_back(std::make_shared<Entidade>(std::string{ "   " }, Tipo::VAZIO, l, c));
		}
		_entidades.push_back(_e);
	}
	return _entidades;
}

void printMapa()
{
	std::stringstream msg;
	for (auto & i : entidades)
	{
		for (auto & j : i)
		{
			//msg << " | ";
			msg << j->getRepresentacao();
			//msg << " | ";
		}
		msg << "\n";
	}
	msg << "N Bombeiros " << n_bombeiros << "\n";
	//msg << "\x1B[2J\x1B[H"; // limpar tela unix, verificar windows
	std::cout << msg.str();
}

bool verificaPosicaoValida(int linha, int coluna)
{
	if (linha < TAMANHO_MAPA && linha > -1
		&& coluna < TAMANHO_MAPA && coluna > -1)
		return true;
	return false;
}

void setarEntidade(std::shared_ptr<Entidade> e, int linha, int coluna)
{
	if (verificaPosicaoValida(linha, coluna))
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		entidades[linha][coluna] = e;
	}
	else
	{
		return;
	}
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		{
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 1);
				std::cout << "Tempo 0000" << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 2);
				std::cout << "N bombeiros " << n_bombeiros << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 3);
				std::cout << "Refugiados" << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 4);
				std::cout << "Vitimas fatais" << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 5);
				std::cout << "Vitimas salvas" << std::endl;
			}
		}

		// tirar da posicao anterior
		if (verificaPosicaoValida(e->getLinha(), e->getColuna()))
		{
			// escopo do mutex
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				entidades[e->getLinha()][e->getColuna()] = std::make_shared<Entidade>(std::string{ "   " }, Tipo::VAZIO, e->getLinha(), e->getColuna());
				entidades[e->getLinha()][e->getColuna()]->print();
			}
		}

		e->moverEntidade();

		if (verificaPosicaoValida(e->getLinha(), e->getColuna()))
		{
			if ((e->getTipo() == Tipo::FOGO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::REFUGIADO) ||
				(e->getTipo() == Tipo::REFUGIADO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::FOGO))
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				entidades[e->getLinha()][e->getColuna()] = std::make_shared<Vitima>(std::string{ "V  " }, e->getLinha(), e->getColuna());
			}
			else if (e->getTipo() == Tipo::FOGO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::BOMBEIRO)
			{
				continue;
			}
			else if (e->getTipo() == Tipo::BOMBEIRO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VITIMA)
			{
				//bombeiro carregar
			}
			else if (entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VAZIO)
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				entidades[e->getLinha()][e->getColuna()] = e;
			}

			{
				std::lock_guard<std::mutex> guard(m_mutex);
				entidades[e->getLinha()][e->getColuna()]->print();
			}
		}
		/*
		else
		{
			// cai fora do mapa
			return;
		}
		*/
	}
}

void printAmbulancia()
{
	int linha = TAMANHO_MAPA;
	int coluna = (TAMANHO_MAPA - 3) * 3;
	gotoxy(coluna, linha);
	std::cout << "A  A  A  ";
}

int main()
{
	entidades = iniEntidades();
	std::vector<std::future<void>> futures;

	printAmbulancia();

	for (int i = 0; i < 10; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::mt19937::result_type seed = time(0);
		auto random_ij = std::bind(std::uniform_int_distribution<int>(0, 9), std::mt19937(seed));
		int l = random_ij(); int c = random_ij();
		if (random_ij() % 3)
		{
			std::shared_ptr<Bombeiro> b = std::make_shared<Bombeiro>(std::string{ "B  " }, l, c); // + std::to_string(i), l, c};
			n_bombeiros++;
			futures.push_back(std::async(std::launch::async, setarEntidade, b, l, c));
		}
		else if (random_ij() % 2)
		{
			std::shared_ptr<Refugiado> f = std::make_shared<Refugiado>(std::string{ "R  " }, l, c); // + std::to_string(i), l, c};
			futures.push_back(std::async(std::launch::async, setarEntidade, f, l, c));
		}
		else
		{
			std::shared_ptr<Fogo> f = std::make_shared<Fogo>(std::string{ "F  " }, l, c); // + std::to_string(i), l, c};
			futures.push_back(std::async(std::launch::async, setarEntidade, f, l, c));
		}
	}

	for (auto & e : futures)
	{
		e.get();
	}

	return 0;
}
