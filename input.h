#ifndef INPUT_H
#define INPUT_H

typedef struct {
  int R;             // regioes
  int C;             // cidades por regiao
  int A;             // alunos por cidade
  int N;             // notas por aluno
  int T;             // threads
  unsigned int seed; // semente
} Entrada;

// dados prontos pra processar
// notas[indice_aluno(r,c,a) * N + n] = nota do aluno
typedef struct {
  Entrada entrada;
  int total_alunos; // R * C * A
  long total_notas; // R * C * A * N
  double *notas;    // vetor continuo
} DadosAlunos;

// le os 6 parametros do arquivo (0=ok, -1=erro)
int ler_entrada(const char *caminho, Entrada *out);

// valida se todos os parametros sao > 0
int validar_entrada(const Entrada *ent);

// aloca e gera as notas com a seed
DadosAlunos criar_dados(const Entrada *ent);

// libera memoria
void liberar_dados(DadosAlunos *dados);

// indice global do aluno: r*C*A + c*A + a
int indice_aluno(const Entrada *ent, int r, int c, int a);

// posicao de uma nota no vetor: indice_aluno * N + n
long pos_nota(const Entrada *ent, int r, int c, int a, int n);

// acesso direto a uma nota
double pegar_nota(const DadosAlunos *dados, int r, int c, int a, int n);

// print teste resumo
void imprimir_resumo(const DadosAlunos *dados);

// print teste amostra de notas (0 = todas)
void imprimir_notas(const DadosAlunos *dados, int max);

#endif
