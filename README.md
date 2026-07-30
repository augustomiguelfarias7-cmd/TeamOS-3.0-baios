# TeamOS 3.0 baios



Baios é o kernel de alto nível e código aberto do TeamOS 3.0. Ele adota uma arquitetura descentralizada de Duplo Microkernel (Dual Microkernel), projetada para entregar alto desempenho, isolamento de falhas e leveza no gerenciamento do sistema.
​Em vez de ser um bloco monolítico tradicional, o Bales opera através de dois microsserviços centrais e independentes (Hardware e Software) que trabalham em sintonia por meio de um barramento de Memória Compartilhada Direta (Zero-Copy IPC).
​ Como o Baios Funciona na Prática
​1. Divisão dos Dois Hemisférios (Duplo Microkernel)
​O Baios divide o controle das operações em duas metades com responsabilidades estritamente separadas:
​Microkernel de Hardware (Baixo Nível - C / Assembly):
​Interface direta com o processador, memória física e dispositivos periféricos.
​Executa os drivers de dispositivos através de uma camada de compatibilidade para reaproveitar módulos do Linux.
​Gerencia as interrupções de hardware (Interrupt Handlers) e o controle de energia.
​Microkernel de Software (Alto Nível - C++ / C#):
​Gerencia a lógica do sistema operacional, o ciclo de vida e as permissões de acesso.
​Processa a estrutura de arquivos, subsistemas de áudio, conexão e renderização.
​Controla as requisições enviadas pelas aplicações.
​📱 A Camada de Aplicação e o Fluxo de Execução
​As aplicações rodando no TeamOS 3.0 não acessam o hardware diretamente. Elas interagem exclusivamente com a camada superior do sistema, garantindo segurança e estabilidade.
​Fluxo de Execução da Aplicação:
​Requisição da Aplicação: A aplicação faz uma chamada de sistema (ex: solicitar acesso à câmera, ler um arquivo ou enviar dados de rede) para o Microkernel de Software.
​Validação de Segurança: O Microkernel de Software verifica as permissões da aplicação e valida o pedido em alto nível.
​Sinalização via Memória Compartilhada: O pedido validado é enviado ao Microkernel de Hardware repassando apenas ponteiros leves no bloco de memória RAM compartilhada (Zero-Copy IPC).
​Ação no Hardware: O Microkernel de Hardware aciona o driver Linux correspondente e executa a instrução física no componente.
​Retorno: O dado/resultado é disponibilizado na memória para a aplicação em tempo recorde, sem cópias desnecessárias de dados na CPU.
