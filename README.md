## Autoria 
Giovanna Joecy Sona de Oliveira
GRR20231947

## Lista de arquivos
Os arquivos utilizados para este projeto foram os mesmos fornecidos nos recursos, sem remoções ou adições de novos arquivos.

```plaintext
A1-PROG-II/
├── INCLUDES/
│   ├── gbv.h
│   └── util.h
├── FONTE/
│   ├── gbv.c
│   ├── util.c
│   └── main.c
├── CONFIG/
│   └── Makefile
└── DOCS/
    └── README.md
```

## Alterações realizadas
Algumas alterações foram feitas no código base fornecido como recurso.  
- **Criação da função gbv_close e inclusão na main** para liberar a memória alocada para o vetor de documentos.
- **Inclusão do ponteiro para a GBV na função gbv_remove** para facilitar acesso necessário ao arquivo e melhor desenvolvimento da função.  
- **Inclusao do ponteiro para a GBV na função view** para facilitar também o acesso ao arquivo.  
O restante permaceneceu como fornecido nos recursos.

## Dificuldades enfrentadas
Muitas dificuldades foram enfrentadas ao longo do projeto.  
- **Mesmo a partir da aula e do enunciado o conceito do projeto era muito abstrato, levando bastante tempo para entendimento.**
- **Compreensão da estrutura e divisão da GBV.**  
- **Como saber dividir o superbloco, área de dados e diretório de metadados.**
- **Como fread e fwrite na GBV respeitando seus ponteiro internos e os offsets corretos.**  
- **Uma das maiores dificuldades foi entender o conceito do buffer, como ele funcionava e como lidar com ele dentro do código.**  
- **Após a alteração do enunciado, que passou a ser necessário além de remover os metadados, remover também o documento do arquivo e calcular os offsets para o shift após a remoção, calcular esses offsets foi complexo**  
- **Assim como na view, dividindo o documento em blocos usando os offsets**

## Bugs conhecidos
Aqui foram listados alguns dos problemas enfrentados ao longo desse código, decidi listar também os resolvidos. O bugs não impedem o funcionamento do código, mas podem afetar alguns casos de execução.  

- **Erro com valgrind de conflito de variáveis quando inseria um documento muito grande**. Ocorria um padding nos bytes da estrutura do superbloco e quando era inserido um grande documento ele gerava um erro no valgrind de não incialização, após igualar as variáveis na struct **esse erro foi resolvido.**  
- **arrendondamento do tamanho da GBV após a saída**. Não é um bug, mas algo que me chamou atenção, mesmo refazendo o código com formas de cálculo diferente e obtendo a resposta do offset correto pelos debugs, ele sempe arredonda o tamanho do arquivo para cima.