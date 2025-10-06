Giovanna Joecy Sona de Oliveira
GRR20231947


DUVIDAS:
- como liberar a memoria ao fim da execução?
- o que fazer com a função do - o?
- na gbv add, quando atualiza os metadados do substituido o offset se mantem o mesmo nao? pois so substitui
- mesmo add docs substituindo eles eu tenho que att o diretorio?
- posso deixar os prints de depuracao?
- posso criar um gbv close? para dar free na memoria?
- testar somente com txts?
- como marcar para apresentar
- perguntar se a especificação da biblioteca.gbv ta certa na remove




- achei melhor na add deixar apenas as funções mais simples como subfunções pois tive dificuldade de me ficar indo e voltando e me entender no codigo
- nao achei que copensa fazer uma funcao pra att os metadados pois muitos parametros
- dei prioridade em colocar muitas saidas de erros e printf ao longo do codigo pra facilitar o debug
- na gbv_add coloquei a variavel pos_livre para ficar visualmente mais intuitivo ao inves de apenas me referir usando o ponteiro, sei que noa precisava mas achei que ficaria mais entendivel
- descobri que precisava da memset pra inicializar a posicao do realloc pra nao corromper o arquivo o.o
- to tendo problema com padding no meu codigo oqq eu faço
- O compilador está adicionando padding na estrutura, mas isso vai causar problemas de compatibilidade - se outro compilador ou sistema usar padding diferente, o arquivo ficará corrompido. a solucao foi usar tipos de tamanho fixo nas variaveis para manter em 4 e 8 = 12
- a GBV add foi braba pq o vetor tava com lixo pq nao foi inicializado e tava corrompendo meu arquivo
- lib->count - 1 na remove para nao acessar memoria invalida
- na gbv remove eu podia dar free e apontar pra null caso o doc fosse o ultimo ou unico do doc, mas a gbv close ja eh chamada em todas as execucoes entao
- rb+ preserva conteudo, ponteiro no inicio do arquivo e sobrescreve
- Na gbv remove, tive que especificar o nome da biblioteca para passar ela no ponteiro file, pois isso nao estava no argumento da funcao entao usei um nome fixo para ela, biblioteca.gbv
- o loop da gbv remove puxa os elementos para frente para nao deixar um buraco no vetor