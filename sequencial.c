#include <stdio.h> //print
#include <string.h> //string
#include <stdlib.h>
#include <time.h> //rand
#include <math.h> //pow

const int QTDE_CIDADE = 52;
const int TAM_POPULACAO = 512;
const int QTDE_ITERACOES = 10000;
const int TX_MUTACAO = 20;

struct Cidade{
    int id;
    int coordX;
    int coordY;
};

struct Gene{
    int rota[52];
    float custo;
};

struct Cidade cidades[52];
struct Gene populacao[512];
int pai1, pai2;
struct Gene novaPopulacao[512];

void criaCidade(int id, int cX, int cY){
    cidades[id-1].id = id;
    cidades[id-1].coordX = cX;
    cidades[id-1].coordY = cY;
}

void lerArquivo(){
    char nomeArquivo[] = "berlin52.tsp";
    char c;

    char * atributo;
    char cid[10];
    char cX[10] ;
    char cY[10];

    int tamAtr;
    int l = 0;

    char * cidade, coordX, coordY;

    int fimArquivo = 0;

    int contador = 0;
    char linha[20] = "0";
    int comecoLinha;
    int tamanhoLinha = 0;
    int i = 0;
    FILE * arquivo;
    arquivo = fopen(nomeArquivo, "r");

    c = fgetc(arquivo);
    while(contador < 6){
        c = fgetc(arquivo);
        if(c == '\n')
            contador++;
    }

    while (!fimArquivo && l < 52){
        comecoLinha = ftell(arquivo);
        c = fgetc(arquivo);
        while (c != '\n'){
            tamanhoLinha++;
            c = fgetc(arquivo);
        }
        fseek(arquivo, comecoLinha, SEEK_SET);
        fgets(linha, tamanhoLinha, arquivo);
        fseek(arquivo, comecoLinha + tamanhoLinha+1, SEEK_SET);
        tamanhoLinha = 0;
        if (linha[0] == 'E'){
            fimArquivo = 1;
        }

        atributo = strtok(linha, " ");
        tamAtr = strlen(atributo);
        for (int i = 0; i < tamAtr; i++){ // pega numero da cidade
            cid[i] = atributo[i];
        }

        atributo = strtok(NULL, " ");
        tamAtr = strlen(atributo);
        for (int i = 0; i < tamAtr; i++){ // pega coordenada X da cidade
            cX[i] = atributo[i];
        }

        atributo = strtok(NULL, " ");
        tamAtr = strlen(atributo);
        for (int i = 0; i < tamAtr; i++){ // pega coordenada Y da cidade
            cY[i] = atributo[i];
        }

        criaCidade(atoi(cid), atoi(cX), atoi(cY)); // insere id, cX e cY no array de structs de cidades na pos id-1

        for (int i = 0; i < 10; i++){
            cX[i] = ' ';
            cY[i] = ' ';
        }

        l++;
    }
    fclose(arquivo);
}

void criaGeneInicial(int tamPopulacao){
    struct Gene geneGuloso;
    int aleatorio;
    int estaNaRota = -1;
    int iRota = 0;
    int cidadesNaRota = 0;
    int auxTroca;

    for (int contPopulacao = 0; contPopulacao < tamPopulacao; contPopulacao++){
        for (int i = 0; i < QTDE_CIDADE; i++){
            geneGuloso.rota[i] = i+1;
        }

        for (int i = 0; i < QTDE_CIDADE; i++){
            aleatorio = rand() % 51 + 1;
            auxTroca = geneGuloso.rota[i];
            geneGuloso.rota[i] = geneGuloso.rota[aleatorio];
            geneGuloso.rota[aleatorio] = auxTroca;
        }
        populacao[contPopulacao] = geneGuloso;
    }
}

float custoRota(int iGene){
    int custo = 0;
    float distancia = 0;
    for (int i = 0; i < QTDE_CIDADE - 1; i++){
        distancia += sqrt(pow((cidades[populacao[iGene].rota[i]-1].coordX - cidades[populacao[iGene].rota[i+1]-1].coordX), 2) + pow((cidades[populacao[iGene].rota[i]-1].coordY - cidades[populacao[iGene].rota[i+1]-1].coordY), 2));
    }
    distancia += sqrt(pow((cidades[populacao[iGene].rota[QTDE_CIDADE-1]-1].coordX - cidades[populacao[iGene].rota[0]-1].coordX), 2) + pow((cidades[populacao[iGene].rota[QTDE_CIDADE-1]-1].coordY - cidades[populacao[iGene].rota[0]-1].coordY), 2));
    return(distancia);
}

int custosIniciais(){
    for (int i = 0; i < TAM_POPULACAO; i++){
        populacao[i].custo = custoRota(i);
    }
}

void imprimePopulacao(){
    for (int i = 0; i < TAM_POPULACAO; i++){
        for(int j = 0; j < QTDE_CIDADE; j++){
            printf("[%i]", populacao[i].rota[j]);
        }
        printf("<%f>\n", populacao[i].custo);
    }
}

void selecionaPais(){
    int melhorPai = 0;
    for (int i = 1; i < 512; i++){
        if (populacao[i].custo < populacao[melhorPai].custo){
            melhorPai = i;
        }
    }

    pai1 = melhorPai;
    pai2 = rand() % 512;
    while (pai2 == pai1){
        pai2 = rand() % 512;
    }
}

void cruzamento(){
    struct Gene filho1, filho2;
    int posFilho;
    int posPai;

    for(int cont = 0; cont < 512; cont+=2){
        selecionaPais();
        for (int i = 0; i < QTDE_CIDADE; i++){
            filho1.rota[i] = 0;
            filho2.rota[i] = 0;
        }

        for (int i = 0; i < QTDE_CIDADE/2; i++){//preenche primeira metade do gene com gene de 1 pai
            filho1.rota[i] = populacao[pai1].rota[i];
            filho2.rota[i] = populacao[pai2].rota[i];
        }

        posPai = 0;
        posFilho = QTDE_CIDADE/2;
        while(posFilho < 52){//preenche segunda metade do gene percorrendo o outro pai a partir da pos 0 e verificando se já está no filho
            int estaNaRota = 0;
            int pos = 0;
            while (estaNaRota != 1 && pos < 52){
                if (populacao[pai2].rota[posPai] == filho1.rota[pos]){
                    estaNaRota = 1;
                }
                else{
                    pos++;
                }
            }
            if (estaNaRota == 0){
                filho1.rota[posFilho] = populacao[pai2].rota[posPai];
                posFilho++;
            }
            posPai++;
        }

        posPai = 0;
        posFilho = QTDE_CIDADE/2;
        while(posFilho < 52){
            int estaNaRota = 0;
            int pos = 0;
            while (estaNaRota != 1 && pos < 52){
                if (populacao[pai1].rota[posPai] == filho2.rota[pos]){
                    estaNaRota = 1;
                }
                else{
                    pos++;
                }
            }
            if (estaNaRota == 0){
                filho2.rota[posFilho] = populacao[pai1].rota[posPai];
                posFilho++;
            }
            posPai++;
        }

        novaPopulacao[cont] = filho1;
        novaPopulacao[cont+1] = filho2;
    }


}

void atualizaPopulacao(){
    for(int i = 0; i < TAM_POPULACAO; i++){
        populacao[i] = novaPopulacao[i];
    }
}

void mutacao(){
    int sorteado;
    int alcance;
    int auxMut;
    int posTroca;

    alcance = 100/TX_MUTACAO;
    for (int i = 0; i < TAM_POPULACAO; i++){
        sorteado = rand() % alcance;
        if(sorteado == 0){
            posTroca = rand() % 50 + 1;
            auxMut = populacao[i].rota[0];
            populacao[i].rota[0] = populacao[i].rota[posTroca];
            populacao[i].rota[posTroca] = auxMut;
            populacao[i].custo = custoRota(i);
        }
    }
}

int main(){
    lerArquivo();
    srand(100);
    criaGeneInicial(TAM_POPULACAO);
    custosIniciais();

    imprimePopulacao();

    //inicio genetico
    for (int i = 0; i < QTDE_ITERACOES; i++){
        cruzamento();
        atualizaPopulacao();
        mutacao();
        custosIniciais();
    }

    int iMelhorCusto = 0;
    for (int i = 0; i < TAM_POPULACAO; i++){
        if(populacao[i].custo < populacao[iMelhorCusto].custo){
            iMelhorCusto = i;
        }
    }

    printf("\n\n\n");
    printf("Melhor custo entre todos processos e threads: %f\n", populacao[iMelhorCusto].custo);
    printf("Melhor rota:\n");
    for(int i = 0; i < 52; i++){
        printf("[%i]", populacao[iMelhorCusto].rota[i]);
    }
    printf("\n");    

}
