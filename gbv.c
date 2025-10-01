#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gbv.h"

//estrutura do superbloco
typedef struct {
    int num_docs; //numero de documentos
    long int offset_dir; //posicao onde comeca o diretorio
} superbloco;

int gbv_create(const char *filename) {
    FILE* file = fopen(filename, "wb");
    if(!file) {
        perror("gbv_create fopen"); 
        return -1;
    }

    superbloco sb;
    sb.num_docs = 0; //biblioteca comeca vazia
    sb.offset_dir = sizeof(superbloco); 

    //escreve superbloco
    fwrite(&sb, sizeof(superbloco), 1, file); 
 
    fclose(file);
    printf("biblioteca criada: %s\n", filename); 
    return 0; //retorna 0 em caso de sucesso
}

//alocar memoria parao vetor de documento
// a area de diretorio eh um vetor de struct documents
int gbv_open(Library *lib, const char *filename) {
    FILE* file = fopen(filename, "rb+"); 

    //caso o arquivo nao exista entao chama a gbv create
    if(!file) {
        if(gbv_create(filename) != 0) {
            perror("gbv_open gbv open nao foi possivel criar arquivo");
            return -1;
        }
    }
    //reabrir o arquivo depois de criar
    file = fopen(filename, "rb+");
    if(!file) {
        perror("gbv_open gbv open nao foi possivel abrir arquivo depois de criar");
        return -1;
    }

    superbloco sb;
    //le o superbloco para RAM
    fread(&sb, sizeof(superbloco), 1, file);
    printf("GBV OPEN - Superbloco lido: %d documentos, offset_dir: %ld\n", sb.num_docs, sb.offset_dir); 

    //inicializa a lib
    lib->count = sb.num_docs;

    //quando ha documentos
    if(sb.num_docs > 0) {
        lib->docs = malloc(sb.num_docs * sizeof(Document));
        if(!lib->docs) {
            perror("gbv open erro ao alocar memoria"); //DEPURACAO
            fclose(file);
            return -1;
        }
        
        //pular para o diretorio e ler documento para a RAM
        fseek(file, sb.offset_dir, SEEK_SET);
        fread(lib->docs, sizeof(Document), sb.num_docs, file);
    }
    else {
        lib->docs = NULL; //sem documentos
    }

    fclose(file);
    return 0;
}

int gbv_add(Library *lib, const char *archive, const char *docname) {
    FILE* file = fopen(archive, "rb+"); //abrir arquivao
    FILE* novo_doc = fopen(docname, "rb"); // doc que vai ser add

    if(!file || !novo_doc) {
        perror("gbv add fopen nao foi possivel abrir os arquivos");
        if(file)
            fclose(file);
        if(novo_doc)
            fclose(file);
        return -1;
    }

    //calcula a o tamanho do novo documento (dentro dele)
    fseek(novo_doc, 0, SEEK_END); //vai pro fim do arquivo
    long tam_doc = ftell(novo_doc); // pega o tamanho do novo doc
    fseek(novo_doc, 0, SEEK_SET); //volta para o inicio


    //le o superbloco para a RAM
    superbloco sb;
    fread(&sb, sizeof(superbloco), 1, file);
    printf("GBV ADD - Superbloco lido: %d documentos, offset_dir: %ld\n", sb.num_docs, sb.offset_dir); 

    //verifica se o documento ja existe
    int doc_existente = -1;
    for(int i = 0; i < lib->count; i++) {
        if(strcmp(lib->docs[i].name, docname) == 0) {
            doc_existente = i;
            break;
        }
    }

    //novo offset onde os dados serao escritos
    long offset_dados;
    if(doc_existente != -1) {
        //substitui o arquivo anterior
        offset_dados = (lib->docs[doc_existente].offset);

    } else {
        //novo doc offset vai pro fim do arquivo
        fseek(file, 0, SEEK_END);
        offset_dados = ftell(file); //me diga onde estou dentro do file
    }

    //criar funcao para substituir metadados?
    //funcao para alterar os metadados
    //criar uma funcao para dar free e chamar ela dentro das outras funcoes
    //como funciona o vetor dentro desse arquivao? confuso
    
    //realocar memoria para o vetor adicionando o novo doc 
    //tem que att o numero de documentos na biblioteca ou no superbloco

    //escrever metadados do doc no arquivo usando o buffer no while
   
    fclose(file);
    return 0;
}
int gbv_remove(Library *lib, const char *docname) {

}

int gbv_list(const Library *lib){

}

int gbv_view(const Library *lib, const char *docname) {

}

int gbv_order(Library *lib, const char *archive, const char *criteria) {
    
}
