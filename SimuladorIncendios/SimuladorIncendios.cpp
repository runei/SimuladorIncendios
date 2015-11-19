#include <iostream>
#include <thread>
#include <cstring>
#include <functional>
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

class Individuo
{
public:
	Individuo(int linha, int col);
	virtual void print();
	virtual void mover();
	posicao getPos();
protected:
	posicao pos;
private:
	char valor = 219;
};

Individuo::Individuo(int linha, int col)
{
	pos.coluna = col;
	pos.linha = linha;
}

void Individuo::print()
{
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

/*==============================================================================================*/

class Bombeiro : public Individuo
{
public:
	Bombeiro(int linha, int col) : Individuo(linha, col) {}
	void print();
	void mover();
private:
	char valor = 'B';
};

void Bombeiro::print()
{
	std::cout << termcolor::red << valor;
}

void Bombeiro::mover()
{
	srand(time(NULL));
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

class Ambiente
{
public:
	Ambiente();
	void print();
	void adicionaIndividuo(Individuo &ind);
	void movimentar(Individuo &ind);
private:
	Individuo *mapa[TAM_AMBIENTE_VERT][TAM_AMBIENTE_HOR];
	std::thread mover;
};

Ambiente::Ambiente()
{
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
		system("cls");
		posicao pos = ind.getPos();
		mapa[pos.linha][pos.coluna] = new Individuo(pos.linha, pos.coluna);
		ind.mover();
		pos = ind.getPos();
		//std::cout << pos.coluna << " - " << pos.linha << std::endl;
		mapa[pos.linha][pos.coluna] = &ind;
		print();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void Ambiente::adicionaIndividuo(Individuo &ind)
{
	posicao pos = ind.getPos();
	mapa[pos.linha][pos.coluna] = &ind;
	print();
	std::thread t(&Ambiente::movimentar, this, std::ref(ind));
	t.join();
}

/*==============================================================================================*/

int main()
{
	Ambiente amb;

	//amb.print();

	int linha, coluna;
	srand(time(NULL));
	linha = rand() % TAM_AMBIENTE_VERT;
	coluna = rand() % TAM_AMBIENTE_HOR;

	Bombeiro b(linha, coluna);
	amb.adicionaIndividuo(b);

	system("pause");

	return 0;
}
