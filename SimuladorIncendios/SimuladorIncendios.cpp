#include <iostream>
#include <future>
#include <random>
#include <functional>
#include <chrono>
#include <thread>
#include <memory>
#include <string>
#include <sstream>

/*==============================================================================================*/

#define TAMANHO_MAPA 10
#define NROLADOS 4
#define ESQUERDA 0
#define NORTE 1
#define DIREITA 2
#define SUL 3

/*==============================================================================================*/

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

/*==============================================================================================*/

enum Tipo
{
	VAZIO,
	BOMBEIRO,
	FOGO,
	REFUGIADO,
	VITIMA
};

enum SituacaoVitima
{
	MORRENDO,
	CARREGADO,
	SALVO
};

/*==============================================================================================*/

class Entidade
{
	std::string representacao;
	Tipo tipo;
public:
	Entidade(std::string _r, Tipo _t, int _l, int _c) : representacao(_r), tipo(_t), linha(_l), coluna(_c) {}
	inline Tipo getTipo() { return tipo; }
	inline int getLinha() { return linha; }
	inline int getColuna() { return coluna; }
	inline void setLinha(int l) { linha - l; }
	inline void setColuna(int c) { coluna = c; }
	std::string getRepresentacao();
	void setRepresentacao(std::string);
	virtual void moverEntidade();
	void print();
protected:
	int linha, coluna;
};

void Entidade::moverEntidade()
{
	return;
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

/*==============================================================================================*/
//VARIAVEIS GLOBAIS

using VetorPtrEntidades = std::vector<std::vector<std::shared_ptr<Entidade>>>;

VetorPtrEntidades entidades;
std::mutex m_mutex;
VetorPtrEntidades iniEntidades();
void setarEntidade(std::shared_ptr<Entidade> e, int linha, int coluna);
std::vector<std::future<void>> futures;

int n_bombeiros = 0;
int tempo = 0;
int n_vitimas_salvas = 0;
int n_vitimas_fatais = 0;
int n_refugiados = 0;

/*==============================================================================================*/

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

/*==============================================================================================*/

class Vitima : public Entidade
{
	int tempo;
	SituacaoVitima sit;
public:
	Vitima(std::string r, int l, int c) : Entidade(r, Tipo::VITIMA, l, c) { tempo = 100; sit = MORRENDO; }
	SituacaoVitima getSit() { return sit; }
	void carregar() { sit = CARREGADO;  }
	void salvar() { sit = SALVO; }
	void diminuiVida() { tempo--; }
	int getTempo() { return tempo; }
};

/*==============================================================================================*/

class Bombeiro : public Entidade
{
	std::shared_ptr<Entidade> vit = NULL;
public:
	Bombeiro(std::string r, int l, int c) : Entidade(r, Tipo::BOMBEIRO, l, c) {}
	virtual void moverEntidade();
	void carregar(std::shared_ptr<Entidade> vit);
	void descarregar();
	std::shared_ptr<Entidade> getVit() { return vit; }
};

void Bombeiro::moverEntidade()
{
	int m;
	if (vit != NULL)
	{
		if (coluna < TAMANHO_MAPA - 2)
		{
			m = DIREITA;
		}
		else
		{
			if (linha == TAMANHO_MAPA - 1)
			{
				descarregar();
			}
			m = SUL;
		}
	}
	else if (coluna > 0 &&
		(entidades[getLinha()][getColuna() - 1]->getTipo() == Tipo::FOGO ||
		entidades[getLinha()][getColuna() - 1]->getTipo() == Tipo::VITIMA))
	{
		m = ESQUERDA;
	}
	else if (coluna < TAMANHO_MAPA - 1 &&
		(entidades[getLinha()][getColuna() + 1]->getTipo() == Tipo::FOGO ||
		entidades[getLinha()][getColuna() + 1]->getTipo() == Tipo::VITIMA))
	{
		m = DIREITA;
	}
	else if (linha > 0 &&
		(entidades[getLinha() - 1][getColuna()]->getTipo() == Tipo::FOGO ||
		entidades[getLinha() - 1][getColuna()]->getTipo() == Tipo::VITIMA))
	{
		m = NORTE;
	}
	else if (linha < TAMANHO_MAPA - 1 &&
		(entidades[getLinha() + 1][getColuna()]->getTipo() == Tipo::FOGO ||
		entidades[getLinha() + 1][getColuna()]->getTipo() == Tipo::VITIMA))
	{
		m = SUL;
	}
	else
	{
		m = rand() % NROLADOS;
	}

	switch (m)
	{
	case ESQUERDA:
		if (coluna > 0)
		{
			coluna--;
		}
		break;
	case DIREITA:
		if (coluna < TAMANHO_MAPA - 1)
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
		if (linha < TAMANHO_MAPA - 1)
		{
			linha++;
		}
		break;
	default:
		break;
	}
}

void Bombeiro::carregar(std::shared_ptr<Entidade> vit)
{
	this->vit = vit;
}

void Bombeiro::descarregar()
{
	std::dynamic_pointer_cast<Vitima> (vit)->salvar();
	vit = NULL;
}

/*==============================================================================================*/

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

/*==============================================================================================*/

/////////////////// Entidades

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
				std::cout << "Tempo " << tempo++ << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 2);
				std::cout << "N bombeiros " << n_bombeiros << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 3);
				std::cout << "Refugiados " << n_refugiados << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 4);
				std::cout << "Vitimas fatais " << n_vitimas_fatais << std::endl;
			}
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				gotoxy(0, TAMANHO_MAPA + 5);
				std::cout << "Vitimas salvas " << n_vitimas_salvas << std::endl;
			}
		}

		if (e->getTipo() == Tipo::VITIMA)
		{
			if (std::dynamic_pointer_cast<Vitima> (e)->getSit() == SituacaoVitima::SALVO)
			{
				e->setLinha(TAMANHO_MAPA - 1);
				e->setColuna(TAMANHO_MAPA - 3);
				entidades[e->getLinha()][e->getColuna()] = std::make_shared<Refugiado>(std::string{ "R  " }, e->getLinha(), e->getColuna());
			}
			else if (std::dynamic_pointer_cast<Vitima> (e)->getTempo() > 0)
			{
				std::dynamic_pointer_cast<Vitima> (e)->diminuiVida();
				continue;
			}
		}

		// tirar da posicao anterior
		if (verificaPosicaoValida(e->getLinha(), e->getColuna()))
		{
			// escopo do mutex
			{
				if (entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VITIMA)
				{
					continue;
				}

				std::lock_guard<std::mutex> guard(m_mutex);
				entidades[e->getLinha()][e->getColuna()] = std::make_shared<Entidade>(std::string{ "   " }, Tipo::VAZIO, e->getLinha(), e->getColuna());
				entidades[e->getLinha()][e->getColuna()]->print();

				if (entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VITIMA &&
					std::dynamic_pointer_cast<Vitima> (entidades[e->getLinha()][e->getColuna()])->getTempo() <= 0)
				{
					return;
				}
			}
		}

		e->moverEntidade();

		if (verificaPosicaoValida(e->getLinha(), e->getColuna()))
		{
			if ((e->getTipo() == Tipo::FOGO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::REFUGIADO) ||
				(e->getTipo() == Tipo::REFUGIADO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::FOGO))
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				e = std::make_shared<Vitima>(std::string{ "V  " }, e->getLinha(), e->getColuna());
				entidades[e->getLinha()][e->getColuna()] = e;
				n_refugiados--;
			}
			else if (e->getTipo() == Tipo::FOGO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::BOMBEIRO)
			{
				continue;
			}
			else if (e->getTipo() == Tipo::BOMBEIRO && 
					std::dynamic_pointer_cast<Bombeiro> (e)->getVit() == NULL &&
					entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VITIMA)
			{
				std::dynamic_pointer_cast<Bombeiro> (e)->carregar(entidades[e->getLinha()][e->getColuna()]);
				n_vitimas_salvas++; // soh qdo levar pra ambulancia 
			}
			else if (e->getTipo() == Tipo::BOMBEIRO &&
				std::dynamic_pointer_cast<Bombeiro> (e)->getVit() != NULL &&
				e->getLinha() == TAMANHO_MAPA - 1)
			{
				std::dynamic_pointer_cast<Bombeiro> (e)->carregar(entidades[e->getLinha()][e->getColuna()]);
				n_vitimas_salvas++; // soh qdo levar pra ambulancia 
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

		if (e->getTipo() == Tipo::FOGO)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
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

	printAmbulancia();

	for (int i = 0; i < 5; ++i)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::mt19937::result_type seed = time(0);
		auto random_ij = std::bind(std::uniform_int_distribution<int>(0, 9), std::mt19937(seed));
		int l = random_ij(); int c = random_ij();
		if (i == 0) //random_ij() % 3
		{
			std::shared_ptr<Bombeiro> b = std::make_shared<Bombeiro>(std::string{ "B  " }, l, c); // + std::to_string(i), l, c};
			n_bombeiros++;
			futures.push_back(std::async(std::launch::async, setarEntidade, b, l, c));
		}
		else if (i != 1)
		{
			std::shared_ptr<Refugiado> f = std::make_shared<Refugiado>(std::string{ "R  " }, l, c); // + std::to_string(i), l, c};
			n_refugiados++;
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
