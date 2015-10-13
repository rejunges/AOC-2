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
int XX=3000; //Júlio não especificou, minha cache os endereços abaixo de 3000 são acessos a memória
int endereco, le; // le é leitura escrita
int sizeOffset, sizeIndice, sizeTag, indice, tag;

//Funções usadas
void mapeamentoDireto(int endereco, int nsets, int bsize);
void conjAssoc(int endereco, int nsets, int bsize);
void totalAssoc(int endereco, int nsets, int bsize);
void leExtensao(char *nomeArq, char *ext);
void configuraCache(int ass, int nsets, int bsize);
void leArq (char *nomeArq, char *ext);
void carregaArgumentos(char *argv[]);
void carregaArgumentosDefault();
void relatorioDeEstatica ();
void validaArgumentos(char *argv[]);
float logBase2(int num);
void criaCache();
void nomeCache(int ass, int nset);
void dadosRelatorio (int mt, int mcom,int mcap,int mconf, int ht);
void decisaoCache();
void sizeTagIndice(int nsets, int bsize);

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
	relatorioDeEstatica();
	return 0;
}

void conjAssoc(int endereco, int nsets, int bsize){
}

void totalAssoc(int endereco, int nsets, int bsize){
	int tamanhoOffset, tamanhoIndice, tamanhoTag, tag, i, aux, flag=0, flagachouL2=0, flagachouL1=0;
	
	tamanhoOffset = logBase2(bsize);
	tamanhoIndice = logBase2(nsets);
	tamanhoTag = 32-tamanhoOffset-tamanhoIndice;
	
	tag = endereco >> (tamanhoOffset + 1);
	
	if(endereco<XX){ //endereço menor que valor estipulado para divisão entre memória de dados e memória de instruções - se verdadeiro, entra na mem. de dados
		//if(bit é LEITURA)
			for(i=0; i<=assoc_L1d; i++){
				if((cacheL1_d[i].tag == tag) && (cacheL1_d[i].bitVal == 1)){
					hitL1d++; //achou na Cache L1 de dados
					flagachouL1=1;
					break;
				}
			}
			if(flagachouL1 == 0){
				missL1d++; //nao achou na Cache L1 de dados, segue para procurar na Cache L2
				for(i=0; i<assoc_L2; i++){
					if((cacheL2[i].tag == tag) && (cacheL2[i].bitVal == 1)){
						hitL2++; //achou na cache L2 e deve transportar o endereço para a L1
						flagachouL2=1;
						for(i=0; i<assoc_L1d; i++){
							if(cacheL1_d[i].bitVal == 0){ //tenta achar uma posição "vazia" ou invalida na cache para transportar o endereço
								cacheL1_d[i].tag = tag;
								cacheL1_d[i].bitVal = 1;
								flag = 1;
							}
						}
						if(flag == 0){ //caso nao ache uma posição vazia, faz substituição randômica
							aux = rand()%assoc_L1d;
							cacheL1_d[aux].tag = tag;
						}
					}
					if(flagachouL2 == 1) break;
				}
				if(flagachouL2 == 0){
					missL2++; //não está na cache L1 nem na L2, deve-se pegar da memória principal e transportar para L2 e depois para L1
					for(i=0; i<assoc_L2; i++){
						if(cacheL2[i].bitVal == 0){ //achou uma posição "vazia" ou invalida para botar o valor na cache L2
							cacheL2[i].tag = tag;
							cacheL2[i].bitVal = 1;
							flag = 1;
							break;
						}
					}
					if(flag == 0){
						aux = rand()%assoc_L2;
						cacheL2[aux].tag = tag;
						cacheL2[aux].bitVal = 1;
					}
					flag = 0;
					for(i=0; i<assoc_L1d; i++){
						if(cacheL1_d[i].bitVal == 0){
							cacheL1_d[i].tag = tag;
							cacheL1_d[i].bitVal = 1;
							flag = 1;
							break;
						}
					}
					if(flag == 0){
						aux = rand()%assoc_L1d;
						cacheL1_d[aux].tag = tag;
						cacheL1_d[aux].bitVal = 1;
					}
				}
			}
		//else pra ESCRITA
		//
		//
		//
	}
	else{ //endereço é INSTRUÇÃO (acima do XX)
		//para LEITURA
			for(i=0; i<=assoc_L1i; i++){
				if((cacheL1_i[i].tag == tag) && (cacheL1_i[i].bitVal == 1)){
					hitL1i++; //achou na Cache L1 de dados
					flagachouL1=1;
					break;
				}
			}
			if(flagachouL1 == 0){
				missL1i++; //nao achou na Cache L1 de dados, segue para procurar na Cache L2
				for(i=0; i<assoc_L2; i++){
					if((cacheL2[i].tag == tag) && (cacheL2[i].bitVal == 1)){
						hitL2++; //achou na cache L2 e deve transportar o endereço para a L1
						flagachouL2=1;
						for(i=0; i<assoc_L1i; i++){
							if(cacheL1_i[i].bitVal == 0){ //tenta achar uma posição "vazia" ou invalida na cache para transportar o endereço
								cacheL1_i[i].tag = tag;
								cacheL1_i[i].bitVal = 1;
								flag = 1;
							}
						}
						if(flag == 0){ //caso nao ache uma posição vazia, faz substituição randômica
							aux = rand()%assoc_L1d;
							cacheL1_i[aux].tag = tag;
						}
					}
					if(flagachouL2 == 1) break;
				}
				if(flagachouL2 == 0){
					missL2++; //não está na cache L1 nem na L2, deve-se pegar da memória principal e transportar para L2 e depois para L1
					for(i=0; i<assoc_L2; i++){
						if(cacheL2[i].bitVal == 0){ //achou uma posição "vazia" ou invalida para botar o valor na cache L2
							cacheL2[i].tag = tag;
							cacheL2[i].bitVal = 1;
							flag = 1;
							break;
						}
					}
					if(flag == 0){
						aux = rand()%assoc_L2;
						cacheL2[aux].tag = tag;
						cacheL2[aux].bitVal = 1;
					}
					flag = 0;
					for(i=0; i<assoc_L1i; i++){
						if(cacheL1_i[i].bitVal == 0){
							cacheL1_i[i].tag = tag;
							cacheL1_i[i].bitVal = 1;
							flag = 1;
							break;
						}
					}
					if(flag == 0){
						aux = rand()%assoc_L1d;
						cacheL1_i[aux].tag = tag;
						cacheL1_i[aux].bitVal = 1;
					}
				}
			}
		//else pra ESCRITA
		//
		//
		//
	}
}

void sizeTagIndice(int nsets, int bsize){
	sizeOffset = logBase2(bsize);   // Tamanho do offset
	sizeIndice = logBase2(nsets);   // Tamanho do indice
	sizeTag = 32-sizeIndice-sizeOffset; // Tamanho da tag
	
	indice = (endereco >> sizeOffset) && (pow(2,(sizeIndice))-1);  // considerando 2na n, n é o indice
	tag = (endereco >> (sizeOffset + sizeIndice));                 // o que restar do endereço sem offset e indice
}
void mapeamentoDireto(int endereco,int nsets, int bsize){
	sizeTagIndice(nsets, bsize); //dados da L1
	if(endereco < XX && le==0){   // DADOS  e leitura                        
		if(cacheL1_d[indice].bitVal == 0){//Se o bit validade é 0  pq é o primeiro acesso a ela
			missL1d++;                                   // Miss total de dados sobe
			missCompL1d++;                              // Esse é compulsorio
			cacheL1_d[indice].bitVal = 1;       // validade = 1 
			cacheL1_d[indice].tag = tag; 
			//Trata a L2-> se um dado está na memória i ela precisa estar em i+1
			sizeTagIndice(nsets_L2, bsize_L2); //dados L2
			           
		} else {//bit validade 1, já usou essa memoria, vai ocorrer hit ou substituicao
			if(cacheL1_d[indice].tag == tag){             
				hitL1d++; //encontrou, hit
			} else {     //nao encontrou, terá que substituir randomicamente                               
				missL1d++; 
				missConfL1d++;                // miss conflito
				cacheL1_d[indice].tag = tag;           // seta novo tag
			}
		}
	}
	else if(endereco >= XX && le==0){ // INSTRUÇOES
		if(cacheL1_i[indice].bitVal == 0){
			missL1i++;
			missCompL1i++;
			cacheL1_i[indice].bitVal = 1;
			cacheL1_i[indice].tag = tag;
		} else {
			if(cacheL1_i[indice].tag == tag){
				hitL1i++;
			} else { 
				missL1i++;
				missConfL1i++;
				cacheL1_i[indice].tag = tag;
			}
		}
	}
	else if(endereco < XX && le==1){//dados para escrita metodo write-back
		//precisa fazer algo por causa do write-back, ainda nao decidido
	}
	else if(endereco >= XX && le==1){//instruçoes para escrita metodo write-back
		//precisa fazer algo por causa do write-back, ainda nao decidido
	}
}
void decisaoCache(){
	if(endereco < XX && le==0){//dados
		//Vai para sua devida configuração
		configuraCache(assoc_L1d, nsets_L1d, bsize_L1d);
	}
	else if(endereco >= XX && le==0){ //Vai para cache de instruções se o endereço for igual a XX ou superior
		configuraCache(assoc_L1i, nsets_L1i, bsize_L1i);
	}
	else if(endereco < XX && le==1){//dados para escrita metodo write-back
		//precisa fazer algo por causa do write-back, ainda nao decidido
	}
	else if(endereco >= XX && le==1){//instruçoes para escrita metodo write-back
		//precisa fazer algo por causa do write-back, ainda nao decidido
	}
}
void configuraCache(int ass, int nsets, int bsize){
	if((ass==1) && (nsets>1)){                        
		mapeamentoDireto(endereco, nsets, bsize);
	} 
	else if((ass>1) && (nsets>1)){ 
		conjAssoc(endereco, nsets, bsize);
	}
	else if((ass>1) && (nsets==1)){
		totalAssoc(endereco, nsets, bsize);
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

