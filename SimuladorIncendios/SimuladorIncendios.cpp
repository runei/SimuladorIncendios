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
	inline void setLinha(int l) { linha = l; }
	inline void setColuna(int c) { coluna = c; }
	inline std::string getRepresentacao() { return  representacao; }
	inline void setRepresentacao(std::string r) { representacao = r; }
	virtual void moverEntidade();
	void print();
protected:
	int linha, coluna;
};

void Entidade::moverEntidade()
{
	return;
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
std::mutex m_bombeiros;
int tempo = 0;
std::mutex m_tempo;
int n_vitimas_salvas = 0;
std::mutex m_vitimas_salvas;
int n_vitimas_fatais = 0;
std::mutex m_vitimas_fatais;
int n_refugiados = 0;
std::mutex m_refugiados;
int n_fogo = 0;
std::mutex m_fogo;
/*==============================================================================================*/

class Refugiado : public Entidade
{
public:
	Refugiado(std::string r, int l, int c) : Entidade(r, Tipo::REFUGIADO, l, c) {}
	virtual void moverEntidade();
};

void Refugiado::moverEntidade()
{
	int l, c, m;
	l = linha;
	c = coluna;
	do {
		m = rand() % NROLADOS;
		switch (m)
		{
		case ESQUERDA:
			if (coluna > 0)
			{
				c = coluna - 1;
			}
			break;
		case DIREITA:
			if (coluna < TAMANHO_MAPA - 1)
			{
				c = coluna + 1;
			}
			break;
		case NORTE:
			if (linha > 0)
			{
				l = linha - 1;
			}
			break;
		case SUL:
			if (linha < TAMANHO_MAPA - 1)
			{
				l = linha + 1;
			}
			break;
		default:
			break;
		}
	} while (entidades[l][c]->getTipo() != Tipo::FOGO && entidades[l][c]->getTipo() != Tipo::VAZIO);

	linha = l;
	coluna = c;
}

/*==============================================================================================*/

class Vitima : public Entidade
{
	int tempo = 20;
	bool salvo = false;
public:
	Vitima(std::string r, int l, int c) : Entidade(r, Tipo::VITIMA, l, c) {}
	inline bool isSalvo() { return salvo; }
	inline void diminuiVida() { tempo--; }
	inline int getTempo() { return tempo; }
	inline void salvar() { salvo = true; }
};

/*==============================================================================================*/

class Bombeiro : public Entidade
{
	bool carregado = false;
public:
	Bombeiro(std::string r, int l, int c) : Entidade(r, Tipo::BOMBEIRO, l, c) {}
	virtual void moverEntidade();
	void carregar();
	void descarregar();
	bool isCarregado() { return carregado; }
};

void Bombeiro::moverEntidade()
{
	int m;
	if (carregado)
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

	int l, c;
	l = linha;
	c = coluna;
	do {
		switch (m)
		{
		case ESQUERDA:
			if (coluna > 0)
			{
				c = coluna - 1;
			}
			break;
		case DIREITA:
			if (coluna < TAMANHO_MAPA - 1)
			{
				c = coluna + 1;
			}
			break;
		case NORTE:
			if (linha > 0)
			{
				l = linha - 1;
			}
			break;
		case SUL:
			if (linha < TAMANHO_MAPA - 1)
			{
				l = linha + 1;
			}
			break;
		default:
			break;
		}
		m = rand() % NROLADOS;
	} while (entidades[l][c]->getTipo() == Tipo::BOMBEIRO || entidades[l][c]->getTipo() == Tipo::REFUGIADO);

	linha = l;
	coluna = c;
}

void Bombeiro::carregar()
{
	carregado = true;
	setRepresentacao("B_V");
}

void Bombeiro::descarregar()
{
	carregado = false;
	setRepresentacao("B  ");
}

/*==============================================================================================*/

class Fogo : public Entidade
{
	bool apagado = false;
public:
	Fogo(std::string r, int l, int c) : Entidade(r, Tipo::FOGO, l, c) {}
	virtual void moverEntidade();
	inline bool isApagado() { return apagado; }
	inline void apagar() { apagado = true; }
};

void Fogo::moverEntidade()
{
	std::mt19937::result_type seed = time(0);
	auto random_ij = std::bind(std::uniform_int_distribution<int>(0, 9), std::mt19937(seed));
	linha = random_ij();
	coluna = random_ij();
}

/*==============================================================================================*/

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
		if (e->getTipo() == Tipo::BOMBEIRO)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			gotoxy(0, TAMANHO_MAPA + 1);
			{
				std::lock_guard<std::mutex> guard(m_tempo);
				tempo++;
			}
			std::cout << "Tempo " << tempo << std::endl;
			gotoxy(0, TAMANHO_MAPA + 2);
			std::cout << "N bombeiros " << n_bombeiros << std::endl;
			gotoxy(0, TAMANHO_MAPA + 3);
			std::cout << "Refugiados " << n_refugiados << std::endl;
			gotoxy(0, TAMANHO_MAPA + 4);
			std::cout << "Vitimas fatais " << n_vitimas_fatais << std::endl;
			gotoxy(0, TAMANHO_MAPA + 5);
			std::cout << "Vitimas salvas " << n_vitimas_salvas << std::endl;
			gotoxy(0, TAMANHO_MAPA + 6);
			std::cout << "N Fogos " << n_fogo << std::endl;
		}

		{
			std::lock_guard<std::mutex> guard(m_mutex);
			if (e->getTipo() == Tipo::VITIMA)
			{
				// vitima se salvou ?, sai 
				if (std::dynamic_pointer_cast<Vitima> (e)->isSalvo())
				{
					return;
				}
				else if (std::dynamic_pointer_cast<Vitima> (e)->getTempo() > 0)
				{
					std::dynamic_pointer_cast<Vitima> (e)->diminuiVida();
					continue;
				}
				else if (std::dynamic_pointer_cast<Vitima> (e)->getTempo() <= 0)
				{
					{
						std::lock_guard<std::mutex> guard(m_vitimas_fatais);
						n_vitimas_fatais++;
					}

					entidades[e->getLinha()][e->getColuna()] = std::make_shared<Entidade>(std::string{ "   " }, Tipo::VAZIO, e->getLinha(), e->getColuna());
					entidades[e->getLinha()][e->getColuna()]->print();

					return;
				}

			}

			// tirar da posicao anterior
			if (verificaPosicaoValida(e->getLinha(), e->getColuna()))
			{
				// eh vitima, entao fica parada, n se mexe
				if (entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VITIMA && e->getTipo() == Tipo::FOGO)
				{
					continue;
				}

				entidades[e->getLinha()][e->getColuna()] = std::make_shared<Entidade>(std::string{ "   " }, Tipo::VAZIO, e->getLinha(), e->getColuna());
				entidades[e->getLinha()][e->getColuna()]->print();
			}

			e->moverEntidade();
			
			if (verificaPosicaoValida(e->getLinha(), e->getColuna()))
			{
				// refugiado pega fogo, vira vitima
				if (e->getTipo() == Tipo::REFUGIADO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::FOGO)
				{
					{
						std::lock_guard<std::mutex> guard(m_refugiados);
						n_refugiados--;
					}
					e = std::make_shared<Vitima>(std::string{ "V  " }, e->getLinha(), e->getColuna());
					entidades[e->getLinha()][e->getColuna()] = e;
				}
				// fogo e bombeiro, fogo apaga
				else if (e->getTipo() == Tipo::FOGO && std::dynamic_pointer_cast<Fogo> (e)->isApagado())
				{
					std::lock_guard<std::mutex> guard(m_fogo);
					n_fogo--;
					return;
				}
				else if (e->getTipo() == Tipo::BOMBEIRO && entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::FOGO)
				{
					std::dynamic_pointer_cast<Fogo> (entidades[e->getLinha()][e->getColuna()])->apagar();
					entidades[e->getLinha()][e->getColuna()] = e;
				}
				// bombeiro pega vitima
				else if (e->getTipo() == Tipo::BOMBEIRO &&
					!std::dynamic_pointer_cast<Bombeiro> (e)->isCarregado() &&
					entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VITIMA)
				{
					std::dynamic_pointer_cast<Bombeiro> (e)->carregar();
					std::dynamic_pointer_cast<Vitima> (entidades[e->getLinha()][e->getColuna()])->salvar();
					//			n_vitimas_salvas++; // soh qdo levar pra ambulancia 
				}
				else if (e->getTipo() == Tipo::BOMBEIRO &&
					std::dynamic_pointer_cast<Bombeiro> (e)->isCarregado() &&
					e->getLinha() == TAMANHO_MAPA - 1)
				{
					std::dynamic_pointer_cast<Bombeiro> (e)->descarregar();
					{
						std::lock_guard<std::mutex> guard(m_vitimas_salvas);
						n_vitimas_salvas++; // soh qdo levar pra ambulancia 
					}
				}
				else if (entidades[e->getLinha()][e->getColuna()]->getTipo() == Tipo::VAZIO)
				{
					entidades[e->getLinha()][e->getColuna()] = e;
				}
				entidades[e->getLinha()][e->getColuna()]->print();
			}
		}

		if (e->getTipo() == Tipo::FOGO)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5000));
		}
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

	for (int x = 0; x < 10; ++x)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		std::mt19937::result_type seed = time(0);
		auto random_ij = std::bind(std::uniform_int_distribution<int>(0, 9), std::mt19937(seed));
		int l = random_ij(); int c = random_ij();
		if (x == 0) //random_ij() % 3
		{
			n_bombeiros++;
			std::shared_ptr<Bombeiro> b = std::make_shared<Bombeiro>(std::string{ "B  " }, l, c); // + std::to_string(i), l, c};
			futures.push_back(std::async(std::launch::async, setarEntidade, b, l, c));
		}
		else if (x < 6)
		{
			n_refugiados++;
			std::shared_ptr<Refugiado> r = std::make_shared<Refugiado>(std::string{ "R  " }, l, c); // + std::to_string(i), l, c};
			futures.push_back(std::async(std::launch::async, setarEntidade, r, l, c));
		}
		else
		{
			n_fogo++;
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
