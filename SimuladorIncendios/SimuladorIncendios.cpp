#include <iostream>
#include <thread>
#include <cstring>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <list>
#include "termcolor.hpp"

/*==============================================================================================*/

#define TAM_AMBIENTE_VERT 22
#define TAM_AMBIENTE_HOR 75
#define NROLADOS 4
#define ESQUERDA 0
#define NORTE 1
#define DIREITA 2
#define SUL 3

/*==============================================================================================*/

struct posicao {
	int linha;
	int coluna;
};

/*==============================================================================================*/
//MOVER O CURSOR

#ifdef _WIN32

#include <windows.h>

void gotoxy(int x, int y)
{
	COORD p = { x, y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), p);
}

#else

#include <unistd.h>
#include <term.h>

void gotoxy(int x, int y)
{
	int err;
	if (!cur_term)
	if (setupterm(NULL, STDOUT_FILENO, &err) == ERR)
		return;
	putp(tparm(tigetstr("cup"), y, x, 0, 0, 0, 0, 0, 0, 0));
}

#endif 

/*==============================================================================================*/

class Semaforo
{
public:
	Semaforo(int recursos);
	void up();
	void down();
private:
	int recursos;
	std::mutex mutex;
	std::condition_variable cv;
};

Semaforo::Semaforo(int recursos)
{
	this->recursos = recursos;
}

void Semaforo::up()
{
	mutex.lock();
	this->recursos++;
	cv.notify_all();
	mutex.unlock();
}

void Semaforo::down()
{
	mutex.lock();
	while (this->recursos == 0)
	{
		std::unique_lock<std::mutex> lck(mutex);
		cv.wait(lck);
	}
	this->recursos--;
	mutex.unlock();
}

/*==============================================================================================*/

class Individuo
{
public:
	Individuo(int linha, int col);
	Individuo() {}
	virtual void print();
	virtual void mover();
	posicao getPos();
	void setPos(posicao pos);
protected:
	posicao pos;
private:
	static const unsigned char valor;
};

const unsigned char Individuo::valor = 219;

Individuo::Individuo(int linha, int col)
{
	pos.coluna = col;
	pos.linha = linha;
}

void Individuo::print()
{
	gotoxy(pos.coluna, pos.linha);
	std::cout << termcolor::white << valor;
}

void Individuo::mover()
{
	return;
}

posicao Individuo::getPos()
{
	return this->pos;
}

void Individuo::setPos(posicao pos)
{
	this->pos = pos;
}

/*==============================================================================================*/

class Bombeiro : public Individuo
{
public:
	Bombeiro(int linha, int col) : Individuo(linha, col) {}
	Bombeiro() {}
	void print();
	void mover();
private:
	static const unsigned char valor;
};

const unsigned char Bombeiro::valor = 'B';

void Bombeiro::print()
{
	gotoxy(pos.coluna, pos.linha);
	std::cout << termcolor::red << valor;
}

void Bombeiro::mover()
{
	int m = rand() % NROLADOS;
	switch (m)
	{
	case ESQUERDA:
		if (pos.coluna > 0)
		{
			pos.coluna--;
		}
		break;
	case DIREITA:
		if (pos.coluna < TAM_AMBIENTE_HOR)
		{
			pos.coluna++;
		}
		break;
	case NORTE:
		if (pos.linha > 0)
		{
			pos.linha--;
		}
		break;
	case SUL:
		if (pos.linha < TAM_AMBIENTE_VERT)
		{
			pos.linha++;
		}
		break;
	default:
		break;
	}

}

/*==============================================================================================*/

class Refugiado : public Individuo
{
public:
	Refugiado(int linha, int col) : Individuo(linha, col) {}
	Refugiado() {}
	void print();
	void mover();
private:
	static const unsigned char valor;
};

const unsigned char Refugiado::valor = 'R';

void Refugiado::print()
{
	gotoxy(pos.coluna, pos.linha);
	std::cout << termcolor::cyan << valor;
}

void Refugiado::mover()
{
	int m = rand() % NROLADOS;
	switch (m)
	{
	case ESQUERDA:
		if (pos.coluna > 0)
		{
			pos.coluna--;
		}
		break;
	case DIREITA:
		if (pos.coluna < TAM_AMBIENTE_HOR)
		{
			pos.coluna++;
		}
		break;
	case NORTE:
		if (pos.linha > 0)
		{
			pos.linha--;
		}
		break;
	case SUL:
		if (pos.linha < TAM_AMBIENTE_VERT)
		{
			pos.linha++;
		}
		break;
	default:
		break;
	}

}

/*==============================================================================================*/

class Fogo : public Individuo
{
public:
	Fogo(int linha, int col) : Individuo(linha, col) {}
	Fogo() {}
	void print();
	void mover();
private:
	static const unsigned char valor;
};

const unsigned char Fogo::valor = 'F';

void Fogo::print()
{
	gotoxy(pos.coluna, pos.linha);
	std::cout << termcolor::yellow << valor;
}

void Fogo::mover()
{
	pos.linha = rand() % TAM_AMBIENTE_VERT;
	pos.coluna = rand() % TAM_AMBIENTE_HOR;
}

/*==============================================================================================*/

class Ambiente
{
public:
	Ambiente();
	void print();
	void adicionaIndividuo(Individuo &ind);
	void movimentar(Individuo &ind);
private:
	void joinAllThreads();
	void detachAllThreads();
	Individuo *mapa[TAM_AMBIENTE_VERT][TAM_AMBIENTE_HOR];
	Semaforo *sem;
	std::list<std::thread> lista_threads;
};

Ambiente::Ambiente()
{
	sem = new Semaforo(1);

	int i, j;
	for (i = 0; i < TAM_AMBIENTE_VERT; i++)
	{
		for (j = 0; j < TAM_AMBIENTE_HOR; j++)
		{
			mapa[i][j] = new Individuo(i, j);
		}
		std::cout << std::endl;
	}
}

void Ambiente::print()
{
	int i, j;
	for (i = 0; i < TAM_AMBIENTE_VERT; i++)
	{
		for (j = 0; j < TAM_AMBIENTE_HOR; j++)
		{
			mapa[i][j]->print();
		}
		std::cout << std::endl;
	}
}

void Ambiente::movimentar(Individuo &ind)
{
	while (1)
	{
		//sem->down();
		posicao pos = ind.getPos();
		mapa[pos.linha][pos.coluna] = new Individuo(pos.linha, pos.coluna);
		mapa[pos.linha][pos.coluna]->print();
		ind.mover();
		pos = ind.getPos();
		mapa[pos.linha][pos.coluna] = &ind;
		mapa[pos.linha][pos.coluna]->print();
		//sem->up();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void Ambiente::adicionaIndividuo(Individuo &ind)
{
	//sem->down();
	posicao pos = ind.getPos();
	mapa[pos.linha][pos.coluna] = &ind;
	print();

	detachAllThreads();
	std::thread t(&Ambiente::movimentar, this, std::ref(ind));
	t.detach();
	lista_threads.push_back(std::move(t));
	joinAllThreads();
	//sem->up();
}

void Ambiente::joinAllThreads()
{
	for (auto it = lista_threads.begin(); it != lista_threads.end(); ++it)
	{
		it->join();
	}
}

void Ambiente::detachAllThreads()
{
	for (auto it = lista_threads.begin(); it != lista_threads.end(); ++it)
	{
		it->detach();
	}
}

/*==============================================================================================*/

int main()
{
	Ambiente amb;
	Fogo b[2];
	int i;
	int linha, coluna;
	srand(time(NULL));

	for (i = 0; i < 2; i++)
	{
		posicao pos;
		pos.linha = rand() % TAM_AMBIENTE_VERT;
		pos.coluna = rand() % TAM_AMBIENTE_HOR;
		b[i].setPos(pos);
		amb.adicionaIndividuo(b[i]);
	}

	std::cin.ignore();

	return 0;
}
