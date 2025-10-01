#include <stdio.h>
#include "gbv.h"

//estrutura do superbloco
typedef struct {
    int num_docs; //numero de documentos
    long int offset_dir; //posicao onde comeca o diretorio
} superbloco;

int gbv_create(const char *filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
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
    if (!file) {
        if(gbv_create(filename) != 0) {
            perror("gbv open gbv open nao foi possivel criar arquivo");
            return -1;
        }
    }
    //reabrir o arquivo depois de criar
    file = fopen(filename, "rb+");

    //caso erro ao abrir o arquivo
    if (!file) {
        perror("bgv_open fopen nao foi possivel abrir o arquivo"); //DEPURACAO
        return -1;
    }

    superbloco sb;
    //le o superbloco para RAM
    fread(&sb, sizeof(superbloco), 1, file);
    printf("GBV OPEN - Superbloco lido: %d documentos, offset_dir: %ld\n", sb.num_docs, sb.offset_dir); //DEPURACAO

    //inicializa a lib
    lib->count = sb.num_docs;

    //quando ha documentos
    if (sb.num_docs > 0) {
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
    FILE *file = fopen(archive, "rb+");

    if(!archive) {
        perror("gbv add fopen nao foi possivel abrir o arquivo");
        return -1;
    }

    superbloco sb;
    fread(&sb, sizeof(superbloco), 1, archive);
    printf("GBV ADD - Superbloco lido: %d documentos, offset_dir: %ld\n", sb.num_docs, sb.offset_dir); 

    //verifica se o documento ja existe
    for(int i = 0; i < lib->count; i++) {
        //comparar nomes usando strcmpy do char da library

        //se achar nome igual, substitui

        //se nao achar adiciona novo doc

    }

    //calcular novo offset
    //tamanho o novo documento usando seek e ftell

    //
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
