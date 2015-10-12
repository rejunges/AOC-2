// Nomes: Renata Junges; Yan Soares
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

/*A  política  de  substituição  será  sempre  randômica -> rand() % posições.  Já  a  política  de  escrita  deverá  ser write-back. 
 * A informação é escrita somente no bloco da cache, e só será escrito na memória principal quando for substituído
 * Endereço do bloco % Numero de blocos da cache
 * assoc=1; nSet=8, bSize=4 ->Mapeamento direto
 *1<nsets_L1i> 2<bsize_L1i> 3<assoc_L1i> 4<nsets_L1d> 5<bsize_L1d> 6<assoc_L1d> 7<nsets_L2> 8<bsize_L2> 9<assoc_L2> 10arquivo_de_entrada*/
//leitura = 0 , escrita = 1
typedef struct{
	int bitVal;
	int tag;
}cache;

//Declarações de variáveis 
cache *cacheL1_i,*cacheL1_d, *cacheL2, **cacheL1Mi, **cacheL1Md, **cacheL2M; 
FILE *arq;
int write=0, read=0; // Número de escritas e leituras
int nsets_L1i, bsize_L1i, assoc_L1i, nsets_L1d, bsize_L1d, assoc_L1d, nsets_L2, bsize_L2, assoc_L2; //referente as caches
int numAcess=0;
int hitL1d=0, hitL1i=0, hitL2=0; //hit da L1 d e L1 i, hit da L2
int missL1i=0, missCompL1i=0, missCapL1i=0, missConfL1i=0; //miss da L1 instruções
int missL1d=0, missCompL1d=0, missCapL1d=0, missConfL1d=0; //miss da L1 dados
int missL2=0, missCompL2=0, missCapL2=0, missConfL2=0; // miss da L2
void criaCache();
void nomeCache(int ass, int nset);
void dadosRelatorio (int mt, int mcom,int mcap,int mconf, int ht);
int XX=3000; //Júlio não especificou, minha cache os endereços abaixo de 3000 são acessos a memória

//Funções usadas
void mapeamentoDireto(int endereco);
void leExtensao(char *nomeArq, char *ext);
void leArq (char *nomeArq, char *ext);
void carregaArgumentos(char *argv[]);
void carregaArgumentosDefault();
void relatorioDeEstatica ();
void validaArgumentos(char *argv[]);
float logBase2(int num);

int main(int argc,char *argv[]){ // argc é o numero de elementos e argv são os elementos, começa no 1( pq o 0 é o ./cache )
	char nomeArq[50], ext[4];
	//configuração default
	if (argc==2){
		strcpy(nomeArq,argv[1]);
		leArq(nomeArq, ext);
		carregaArgumentosDefault();
		//printf("\n################### Configuracao da Cache: Default ###################\n");
	}
	else if(argc==11){
		strcpy(nomeArq,argv[10]);
		leArq(nomeArq, ext);
		validaArgumentos(argv);
		carregaArgumentos(argv);
	}
	else{
		printf ("ERRO: Número de argumentos inválido, tente novamente !");
		exit(1);
	}
	criaCache();//Cria a L1 de instruções e dados, e cria a L2 também
	
	if (strcmp(ext, "txt")==0){//se o arquivo é texto
		
	}
	/*	
	int endereco,i;
	for (i=0; i<8; i++){
		cacheL1[i].bitVal=false;
	}
	arq = fopen ("entrada.txt", "r");
	if (arq == NULL){
		printf ("Erro, não foi possível abrir o arquivo");
		return 0;
	}
	while (fscanf(arq, "%d", &endereco) != EOF){// vai ler endereço por endereço até acabar o arquivo
		mapeamentoDireto(endereco);
	}

	printf("MISS:%d\nHIT:%d\n",miss, hit);*/
	fclose(arq);
	relatorioDeEstatica();
	return 0;
}
/*
void mapeamentoDireto(int endereco){
	int indice, tag,sizeOffset=2, sizeIndice=3;
	indice=endereco % 8; 
	tag=endereco >>(sizeOffset+sizeIndice);
	if (cacheL1[indice].bitVal == false){
		miss++;
		cacheL1[indice].bitVal = true;
		cacheL1[indice].tag =tag;
	}
	else{
		if (cacheL1[indice].tag== tag){
			hit++;
		}
		else{
			miss++;
			cacheL1[indice].tag=tag;
		}
	}
}*/

void validaArgumentos(char *argv[]){
	int i;
	for(i=1; i<10; i++){
		if((atoi(argv[i]))<=0){                                // Verifica Argumentos 
			printf("ERRO: Argumentos Invalidos!\n");
			exit(1);
		}
	}
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
void relatorioDeEstatica (){
	printf ("\n\t\t###############################\n\t\t## Relatório de Estatísticas ##\n\t\t###############################\n\n");
	printf ("*Número de acessos a memória: %d\n", numAcess);
	printf ("*Número de escritas na memória: %d\n", write);
	printf ("*Número de leitura na memória: %d\n\n", read);
	printf("\n\n################### Cache L1 de dados ##################\n\n");
	// Informa qual cache se refere
	nomeCache(assoc_L1d, nsets_L1d);
	dadosRelatorio(missL1d, missCompL1d, missCapL1d, missConfL1d, hitL1d);
	printf("\n\n################ Cache L1 de instruções ################\n\n");
	// Informa qual cache se refere
	nomeCache(assoc_L1i, nsets_L1i);
	dadosRelatorio(missL1i, missCompL1i, missCapL1i, missConfL1i, hitL1i);
	printf("\n\n####################### Cache L2 #######################\n\n");
	// Informa qual cache se refere
	nomeCache(assoc_L2, nsets_L2);
	dadosRelatorio(missL2, missCompL2, missCapL2, missConfL2, hitL2);
	printf("\n\n########################################################\n\n");

}
void dadosRelatorio (int mt, int mcom,int mcap,int mconf, int ht){
	float hr, mr; //hit ratio e miss ratio
	hr= (float)(ht*100)/numAcess; // hit total * 100 / num acesso = hit ratio
	mr= (float)(mt*100)/numAcess;
	printf ("\t*Total de Hit: %d\n", ht);
	printf ("\t*Total de Miss: %d\n", mt);
	printf ("\t*Hit Ratio: %f\n", hr);
	printf ("\t*MIss Ratio: %f\n", mr);
	printf ("\t\t-Quantidade de Miss Compulsório: %d\n", mcom);
	printf ("\t\t-Quantidade de Miss de Capacidade: %d\n", mcap);
	printf ("\t\t-Quantidade de Miss de Conflito: %d\n", mconf);
}
	
void nomeCache(int ass, int nset){
	// Informa qual cache se refere
	if((ass==1) && (nset>1)){                       
		printf("-----> Cache Direta\n\n");
	}
	else if((ass>1) && (nset>1)){ 
		printf("-----> Cache Conjunto Associativo\n\n");
	}
	else if((ass>1) && (nset==1)){
		printf("-----> Cache Totalmente Associativa\n\n");
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
}
void carregaArgumentosDefault(){
	/*cache com mapeamento direto com tamanho de bloco de 4 bytes
	 * e com 1024 conjuntos (nas duas caches)*/
	nsets_L1i = 1024;
	bsize_L1i = 4;
	assoc_L1i = 1;
	nsets_L1d = 1024;
	bsize_L1d = 4;
	assoc_L1d = 1;
	nsets_L2 = 1024;
	bsize_L2 = 4;
	assoc_L2 = 1; 
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
float logBase2(int num){
     return log10(num)/log10(2);         //log base 2
    
}
void criaCache(){
	int i,j;
	//tratando a L1 de instruções
	if((assoc_L1i>1) && (nsets_L1i>1)){
		//Matriz por causa da associatividade
		cacheL1Mi = malloc((sizeof(cache))*nsets_L1i);
		for(j = 0; j< nsets_L1i; j++){
			cacheL1Mi[j]=malloc((sizeof(cache))*assoc_L1i);
		}
		//"limpa" as posições
		for (i=0; i<nsets_L1i; i++){
			for (j=0; j<assoc_L1i; j++){
				cacheL1Mi[i][j].bitVal = 0;               
				cacheL1Mi[i][j].tag = 0;
			}
		}
	} 
	else {
		//Não é necessário matriz
		cacheL1_i = malloc((sizeof(cache))*nsets_L1i); 
		//"limpa" as posições   
		for(i=0; i<nsets_L1i; i++){
			cacheL1_i[i].tag = 0;
			cacheL1_i[i].bitVal = 0;
		}
	} 
	//Mesmo código acima para L1 de dados                                            
	if((assoc_L1d>1) && (nsets_L1d>1)){                 
		cacheL1Md = malloc((sizeof(cache))*nsets_L1d);
		for(j = 0; j< nsets_L1i; j++){
			cacheL1Md[j] = malloc((sizeof(cache))*assoc_L1d);
		}
		for (i=0; i<nsets_L1d; i++){
			for (j=0; j<assoc_L1d; j++){
				cacheL1Md[i][j].bitVal = 0;
				cacheL1Md[i][j].tag = 0;
			}
		}
	}
	else {
		cacheL1_d = malloc((sizeof(cache))*nsets_L1d);
		for(i=0; i<nsets_L1d; i++){
			cacheL1_d[i].tag = 0;
			cacheL1_d[i].bitVal = 0;
		}
	}
	//Mesmo código para a L2
	if((assoc_L2>1) && (nsets_L2>1)){                 
		cacheL2M = malloc((sizeof(cache))*nsets_L2);
		for(j = 0; j< nsets_L1i; j++){
			cacheL2M[j] = malloc((sizeof(cache))*assoc_L2);
		}
		for (i=0; i<nsets_L1d; i++){
			for (j=0; j<assoc_L1d; j++){
				cacheL2M[i][j].bitVal = 0;
				cacheL2M[i][j].tag = 0;
			}
		}
	}
	else {
		cacheL2 = malloc((sizeof(cache))*nsets_L2);
		for(i=0; i<nsets_L1d; i++){
			cacheL2[i].tag = 0;
			cacheL2[i].bitVal = 0;
		}
	}
}

