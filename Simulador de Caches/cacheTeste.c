// Nomes: Renata Junges; Yan Soares
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

typedef struct{
	int bitVal;
	int tag;
	int dirtyBit; //bit que irá indicar que há necessidade de levar o valor que foi substituido para a memória principal (para não haver inconsistência de dados)
}cache;
typedef struct{
	int hit;
	int miss, missComp, missConf, missCap;
	int leitura, escrita;
}Estatistica;
//Variaveis globais;
FILE *arq;
int numAcess=0;
int endereco, le; // le é leitura escrita
int sizeOffset, sizeIndice, sizeTag, indice, tag;
int XX=400; //Júlio não especificou, na cache os endereços abaixo de XX
int nsets_L1i, bsize_L1i, assoc_L1i, nsets_L1d, bsize_L1d, assoc_L1d, nsets_L2, bsize_L2, assoc_L2; //referente as caches
int tipoL1d=0, tipoL1i=0, tipoL2=0;//tipo=1=MP ; tipo=2=CA ; tipo=3=TA
cache *cacheL1i, *cacheL1d, *cacheL2, **cacheL1Md, **cacheL1Mi, **cacheL2M;
Estatistica L1i, L1d, L2;

//Funções
void zeraEstatistica(Estatistica L);
void dadosRelatorio (Estatistica L);
void nomeCache(int tipo);
void relatorioDeEstatistica ();
void criaCache();
cache** criaCacheMatriz(int nsets, int assoc);
cache* criaCacheVetor(int nsets);
int defineTipos(int ass, int nsets);
void configuraCache(int nsets, int bsize, int tipo);
void decisaoCache();
void validaArgumentos(char *argv[]);
void carregaArgumentos(char *argv[]);
float logBase2(int num);
void leExtensao(char *nomeArq, char *ext);
void carregaArgumentosDefault();
void leArq (char *nomeArq, char *ext);

int main(int argc,char *argv[]){ // argc é o numero de elementos e argv são os elementos, começa no 1( pq o 0 é o ./cache )
	char nomeArq[50], ext[4];
	//configuração default
	if (argc==2){
		strcpy(nomeArq,argv[1]);
		leArq(nomeArq, ext);
		carregaArgumentosDefault();
		//Significa que todas as caches são mapeamento direto
		tipoL1d=1;
		tipoL1i=1;
		tipoL2=1;
		//printf("\n################### Configuracao da Cache: Default ###################\n");
	}
	else if(argc==11){
		strcpy(nomeArq,argv[10]);
		leArq(nomeArq, ext);
		validaArgumentos(argv);
		carregaArgumentos(argv);//no carrega argumentos já define o tipo
	}
	else{
		printf ("ERRO: Número de argumentos inválido, tente novamente !");
		exit(1);
	}
	criaCache();//Cria as 3 caches
	if (strcmp(ext, "txt")==0){//se o arquivo é texto
		while (fscanf(arq, "%d", &endereco) != EOF){//vai ler linha por linha
			fscanf(arq, "%d", &le);// A segunda linha informa se é leitura=0 ou escrita=1
			numAcess++;
			decisaoCache();
		}
	}
	else { //arquivo binário
		int a,b,c,d;
		while(1){
			//Como é binário ele vai pegar 1byte por fez, e depois fazer o cálculo para conseguir o valor inteiro
			a=fgetc(arq);
			b=fgetc(arq);
			c=fgetc(arq);
			d=fgetc(arq);
			endereco=(a * 256 * 256 * 256) + (b * 256 * 256) + (c * 256) + d;
			//descobrir se é 0 ou 1
			a=fgetc(arq);
			b=fgetc(arq);
			c=fgetc(arq);
			d=fgetc(arq);
			le = (a * 256 * 256 * 256) + (b * 256 * 256) + (c * 256) + d;
			if(feof(arq)){
				break;
			}
			numAcess++;
			decisaoCache();
		}
	}
	fclose(arq);
	relatorioDeEstatistica();
	return 0;
}
void zeraEstatistica(Estatistica L){
	//Acredito que esses valores já venham zerados, mas é sempre melhor garantir
	L.hit=0;
	L.miss=0;
	L.missComp=0;
	L.missConf=0;
	L.missCap=0;
	L.leitura=0;
	L.escrita=0;
}
void dadosRelatorio (Estatistica L){
	float hr, mr; //hit ratio e miss ratio
	hr= (float)(L.hit*100)/numAcess; // hit total * 100 / num acesso = hit ratio
	mr= (float)(L.miss*100)/numAcess;
	printf ("\t*Leitura: %d\n", L.leitura);
	printf ("\t*Escrita: %d\n", L.escrita);
	printf ("\t*Total de Hit: %d\n", L.hit);
	printf ("\t*Total de Miss: %d\n", L.miss);
	printf ("\t\t-Quantidade de Miss Compulsório: %d\n", L.missComp);
	printf ("\t\t-Quantidade de Miss de Capacidade: %d\n",L.missCap);
	printf ("\t\t-Quantidade de Miss de Conflito: %d\n", L.missConf);
	printf ("\t*Hit Ratio: %.2f\n", hr);
	printf ("\t*Miss Ratio: %.2f\n", mr);
}
void nomeCache(int tipo){
	// Informa qual cache se refere
	if(tipo==1){
		printf("-----> Cache Direta\n\n");
	}
	else if(tipo==2){
		printf("-----> Cache Conjunto Associativo\n\n");
	}
	else if(tipo==3){
		printf("-----> Cache Totalmente Associativa\n\n");
	}
}
void relatorioDeEstatistica (){
	printf ("\n\t\t###############################\n\t\t## Relatório de Estatísticas ##\n\t\t###############################\n\n");
	printf ("*Número de acessos: %d\n", numAcess);
	printf("\n\n################### Cache L1 de dados ##################\n\n");
	// Informa qual cache se refere
	nomeCache(tipoL1d);
	dadosRelatorio(L1d);
	printf("\n\n################ Cache L1 de instruções ################\n\n");
	// Informa qual cache se refere
	nomeCache(tipoL1i);
	dadosRelatorio(L1i);
	printf("\n\n####################### Cache L2 #######################\n\n");
	// Informa qual cache se refere
	nomeCache(tipoL2);
	dadosRelatorio(L2);
	printf("\n\n########################################################\n\n");
}
void criaCache(){
	zeraEstatistica(L1i);
	zeraEstatistica(L1d);
	zeraEstatistica(L2);
	//cria L1 de dados
	if (tipoL1d==1 || tipoL1d==3){
		//cria cache dado vetor -> Mapeamento direto ou Totalmente Associativa
		cacheL1d=criaCacheVetor(nsets_L1d);
	}
	else{
		//cria cache dado matriz->Conjunto Associativo
		cacheL1Md=criaCacheMatriz(nsets_L1d, assoc_L1d);
	}
	//cria L1 de instrucoes
	if (tipoL1i==1 || tipoL1i==3){
		//cria cache dado vetor -> Mapeamento direto ou Totalmente Associativa
		cacheL1i=criaCacheVetor(nsets_L1i);
	}
	else{
		//cria cache dado matriz->Conjunto Associativo
		cacheL1Mi=criaCacheMatriz(nsets_L1i, assoc_L1i);
	}
	//cria L2 
	if (tipoL2==1 || tipoL2==3){
		//cria cache dado vetor -> Mapeamento direto ou Totalmente Associativa
		cacheL2=criaCacheVetor(nsets_L2);
	}
	else{
		//cria cache dado matriz->Conjunto Associativo
		cacheL2M=criaCacheMatriz(nsets_L2, assoc_L2);
	}
}
cache** criaCacheMatriz(int nsets, int assoc){
	int i,j;
	cache **cacheM;
	//Matriz por causa da associatividade
	cacheM = malloc((sizeof(cache))*nsets);
	for(j = 0; j< nsets; j++){
		cacheM[j]=malloc((sizeof(cache))*assoc);
	}
	//"limpa" as posições
	for (i=0; i<nsets; i++){
		for (j=0; j<assoc; j++){
			cacheM[i][j].bitVal = 0;
			cacheM[i][j].tag = 0;
			cacheM[i][j].dirtyBit = 0;
		}
	}
	return cacheM;
}
cache* criaCacheVetor(int nsets){
	//Não é necessário matriz
	int i;
	cache *cacheV;
	cacheV = malloc((sizeof(cache))*nsets);
	//"limpa" as posições
	for(i=0; i<nsets; i++){
		cacheV[i].tag = 0;
		cacheV[i].bitVal = 0;
		cacheV[i].dirtyBit = 0;
	}
	return cacheV;
}
int defineTipos(int ass, int nsets){
	int tipo=0;
	if((ass==1) && (nsets>1)){
		tipo=1; //mapeamento direto
	}
	else if((ass>1) && (nsets>1)){
		tipo=2; //conjunto associativa
	}
	else if((ass>1) && (nsets==1)){
		tipo=3; //totalmente associativa
	}
	return tipo;
}
void configuraCache(int nsets, int bsize, int tipo){
	if(tipo==1){
		mapeamentoDireto(endereco, nsets, bsize);
	}
	else if(tipo==2){
		conjAssoc(endereco, nsets, bsize);
	}
	else if(tipo==3){
		totalAssoc(endereco, nsets, bsize);
	}
}
void decisaoCache(){
	if(endereco < XX ){//dados
		//Vai para sua devida configuração
		configuraCache(nsets_L1d, bsize_L1d, tipoL1d);
	}
	else if(endereco >= XX ){ //Vai para cache de instruções se o endereço for igual a XX ou superior
		configuraCache(nsets_L1i, bsize_L1i, tipoL1i);
	}
}
void validaArgumentos(char *argv[]){
	int i;
	for(i=1; i<10; i++){
		if((atoi(argv[i]))<=0){                                // Verifica Argumentos
			printf("ERRO: Argumentos Invalidos!\n");
			exit(1);
		}
	}
}
void carregaArgumentos(char *argv[]){
	nsets_L1i = atoi(argv[1]);               // carrega argumentos
	bsize_L1i = atoi(argv[2]);
	assoc_L1i = atoi(argv[3]);
	nsets_L1d = atoi(argv[4]);
	bsize_L1d = atoi(argv[5]);
	assoc_L1d = atoi(argv[6]);
	nsets_L2 = atoi(argv[7]);
	bsize_L2 = atoi (argv[8]);
	assoc_L2 = atoi (argv[9]);
	//Define tipos
	tipoL1d= defineTipos(assoc_L1d, nsets_L1d);
	tipoL1i= defineTipos(assoc_L1i, nsets_L1i);
	tipoL2= defineTipos(assoc_L2, nsets_L2);
	
}
float logBase2(int num){
     return log10(num)/log10(2);         //log base 2

}
void leExtensao(char *nomeArq, char *ext){
	int i,tam,j=3;
	tam=strlen(nomeArq);
	//Armazena em ext a extensao do arquivo(txt ou dat)
	for (i=tam; i>(tam-4); i--){
		 ext[j]=nomeArq[i];
		 j--;
	}
}
void carregaArgumentosDefault(){
	/*cache com mapeamento direto com tamanho de bloco de 4 bytes
	 * e com 1024 conjuntos (nas duas caches)*/
	nsets_L1i = 8;
	bsize_L1i = 4;
	assoc_L1i = 1;
	nsets_L1d = 8;
	bsize_L1d = 4;
	assoc_L1d = 1;
	nsets_L2 = 8;
	bsize_L2 = 4;
	assoc_L2 = 1;
}
void leArq (char *nomeArq, char *ext){
	leExtensao(nomeArq, ext);
	if (strcmp(ext,"txt")==0){//extensao txt
		arq = fopen (nomeArq, "r");
		if (arq == NULL){
			printf ("Erro, não foi possível abrir o arquivo");
			exit(1);
		}
	}
	else if(strcmp(ext,"dat")==0){//extensao dat(binaria)
			arq=fopen(nomeArq, "rb");
			if (arq == NULL){
				printf ("Erro, não foi possível abrir o arquivo");
				exit(1);
			 }
		 }
	else{ // extensao inválida
		printf ("ERRO: Extensão do arquivo inválida, tente novamente!");
		exit(1);
	}
}
