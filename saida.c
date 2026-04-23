#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#define QTD_PAR 4
#define REP 5

double calcula_tempo_medio(double tempos[REP]){
    double soma = 0.0;
    for(int i=0; i<REP; i++)
        soma += tempos[i];
    return soma / REP;
}

double calcula_tempo_std(double tempos[REP], double media){
    double soma = 0.0;
    for(int i=0; i<REP; i++)
        soma += (tempos[i] - media) * (tempos[i] - media);
    return sqrt(soma / REP);
}

int main(){
    // ent -> struct R, C, A, N, T
    // ideias de gráficos: para cada carga, um range de threads (ex: T = {1, 2, 4, 8}):
    // 1. carga pequena (R=2, C=2, A=10, N=10) 
    // 2. carga média (R=4, C=4, A=1000, N=10)
    // 3. carga grande (R=8, C=8, A=10000, N=10)

    // essa entrada aqui coloquei arbitrária sem inicialização, só p ter ideia de como add no CSV.
    Entrada ent_pequena[4], ent_media[4], ent_grande[4];

    double tempo_seq_pequena[REP], tempo_seq_media[REP], tempo_seq_grande[REP];
    double tempos_par_pequena[QTD_PAR][REP], tempos_par_media[QTD_PAR][REP], tempos_par_grande[QTD_PAR][REP];
    double media_seq_pequena, media_seq_media, media_seq_grande;
    double media_par_pequena[QTD_PAR], media_par_media[QTD_PAR], media_par_grande[QTD_PAR];
    double std_seq_pequena, std_seq_media, std_seq_grande;
    double std_par_pequena[QTD_PAR], std_par_media[QTD_PAR], std_par_grande[QTD_PAR];
    double speedup, eficiencia;

    // Algoritmo sequencial
    for(int i=0; i<REP; i++)
        tempo_seq_pequena[i] = studentsseq(ent_pequena[0]);
    for(int i=0; i<REP; i++)
        tempo_seq_media[i] = studentsseq(ent_media[0]);
    for(int i=0; i<REP; i++)
        tempo_seq_grande[i] = studentsseq(ent_grande[0]);
    
        // médias e desvios-padrão dos tempos sequenciais
    media_seq_pequena = calcula_tempo_medio(tempo_seq_pequena);
    std_seq_pequena = calcula_tempo_std(tempo_seq_pequena, media_seq_pequena);
    media_seq_media = calcula_tempo_medio(tempo_seq_media);
    std_seq_media = calcula_tempo_std(tempo_seq_media, media_seq_media);
    media_seq_grande = calcula_tempo_medio(tempo_seq_grande);
    std_seq_grande = calcula_tempo_std(tempo_seq_grande, media_seq_grande);

    // estrutura do arquivo: R, C, A, N, T, media_tempo_seq_, std_tempo_seq_, media_tempo_par_, std_tempo_par_, speedup, eficiencia
    FILE *fp;
    // tenta abrir para leitura
    fp = fopen("resultados.csv", "r");
    if (fp == NULL) {
        // arquivo não existe → cria e escreve cabeçalho
        fp = fopen("resultados.csv", "w");
        fprintf(fp, "R,C,A,N,T,Tseq_mean,Tseq_std,Tpar_mean,Tpar_std,Speedup,Eficiencia\n");
    } else {
        // arquivo já existe → só acrescenta dados
        fclose(fp);
        fp = fopen("resultados.csv", "a");
    }

    // Algoritmo paralelo
    // carga pequena
    for(int i=0;i<QTD_PAR;i++){
        for(int r=0; r<REP; r++)
            tempos_par_pequena[i][r] = studentspar(ent_pequena[i]);

        media_par_pequena[i] = calcula_tempo_medio(tempos_par_pequena[i]);
        std_par_pequena[i] = calcula_tempo_std(tempos_par_pequena[i], media_par_pequena[i]);

        speedup = media_seq_pequena / media_par_pequena[i];
        eficiencia = speedup / ent_pequena[i].T;

        fprintf(fp, "%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
            ent_pequena[i].R, ent_pequena[i].C, ent_pequena[i].A, ent_pequena[i].N, ent_pequena[i].T,
            media_seq_pequena, std_seq_pequena, media_par_pequena[i], std_par_pequena[i], speedup, eficiencia);
    }
    // carga média
    for(int i=0;i<QTD_PAR;i++){
        for(int r=0; r<REP; r++)
            tempos_par_media[i][r] = studentspar(ent_media[i]);

        media_par_media[i] = calcula_tempo_medio(tempos_par_media[i]);
        std_par_media[i] = calcula_tempo_std(tempos_par_media[i], media_par_media[i]);

        speedup = media_seq_media / media_par_media[i];
        eficiencia = speedup / ent_media[i].T;

        fprintf(fp, "%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
            ent_media[i].R, ent_media[i].C, ent_media[i].A, ent_media[i].N, ent_media[i].T,
            media_seq_media, std_seq_media, media_par_media[i], std_par_media[i], speedup, eficiencia);
    }
    // carga grande
    for(int i=0;i<QTD_PAR;i++){
        for(int r=0; r<REP; r++)
            tempos_par_grande[i][r] = studentspar(ent_grande[i]);

        media_par_grande[i] = calcula_tempo_medio(tempos_par_grande[i]);
        std_par_grande[i] = calcula_tempo_std(tempos_par_grande[i], media_par_grande[i]);

        speedup = media_seq_grande / media_par_grande[i];
        eficiencia = speedup / ent_grande[i].T;

        fprintf(fp, "%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
            ent_grande[i].R, ent_grande[i].C, ent_grande[i].A, ent_grande[i].N, ent_grande[i].T,
            media_seq_grande, std_seq_grande, media_par_grande[i], std_par_grande[i], speedup, eficiencia);
    }

    fclose(fp);
    return 0;
}

/*
import pandas as pd

df = pd.read_csv("resultados.csv")
# já tem o cabeçalho!!!
cargas = df.groupby(["R","C","A","N"])
*/