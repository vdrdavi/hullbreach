# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Jogo 2D/3D em C++20 sobre SDL3: o jogador anda pelo interior de uma nave em 2D e,
no painel de pilotagem, passa para uma visão 3D de voo por estrelas procedurais.

## O guia do programa

**[docs/FUNCIONAMENTO.md](docs/FUNCIONAMENTO.md) explica como cada peça do
programa funciona**, para quem não é necessariamente da área: o laço, as
coordenadas, a pilha de cenas, a entrada, o 2D, o mapa de tiles, o 3D, o voo, as
cenas, o áudio, os assets e o build. Todo termo técnico é explicado onde aparece
e repetido no glossário.

Não é um diário nem um changelog: **descreve o programa como ele é hoje**.
Quando uma mudança altera um mecanismo descrito lá — o laço, a pilha, o pipeline
3D, o formato do mapa, o jeito de tocar som —, a seção correspondente é
reescrita **no mesmo commit** da implementação, e não acrescentada como nota de
"antes era assim". Mecanismo novo ganha seção nova; termo técnico novo entra no
texto e no glossário.

O registro do que mudou e quando é o histórico do git; o **porquê** de cada
mudança vai no corpo da mensagem de commit.

## Comandos

```sh
cmake --preset debug && cmake --build build/debug     # preset release tambem existe
./build/debug/jogo
python tools/gen_assets.py                            # regera assets (precisa de Pillow)
cmake --install build/release --prefix <dir>
```

**Não há suíte de testes nem linter configurado.** O build é o controle de
qualidade: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`, hoje sem nenhum
warning — mantenha assim. Como fumaça, rode o jogo sem sessão gráfica:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/release/jogo   # mate depois de alguns segundos
```

Arquivos `.cpp` novos precisam entrar na lista de fontes do `CMakeLists.txt` — há
glob só para assets, nunca para código.

## Como olhar o jogo de fato

Para conferir uma mudança visual, a forma determinística é um patch temporário no
laço de `App::rodar()` que grava o viewport com `SDL_RenderReadPixels` +
`SDL_SavePNG` e encerra. Rodando com `SDL_VIDEODRIVER=dummy`, a janela sai exatos
1280×720 (2× a resolução lógica), sem nada do desktop em volta — foi assim que
`docs/*.png` foram gerados. Injetar teclas no compositor (ydotool) é frágil: a
janela do jogo perde o foco e as teclas vão parar em outra aplicação do usuário.
Para exercitar uma cena específica, aponte `main.cpp` para ela num build
temporário e reverta depois.

Com o jogo na frente, **F3 abre a tela de depuração** (só no build debug,
`src/scenes/DebugScene.*`): quadros por segundo, drivers de vídeo e áudio,
renderer, tamanho da janela, de onde os assets estão vindo e o estado da nave.
É o painel de instrumentos, não uma medição determinística. **F4**, do mesmo
arquivo, deixa a nave invencível — sem colisão, sem estrago — para conferir uma
cena demorada sem que uma rocha encerre a viagem; o selo âmbar no canto avisa que
aquela execução não vale como teste. Ele não repara nada: o casco continua onde
estava, e uma nave já perdida não volta.

Para o áudio existe o equivalente: o driver `disk` do SDL grava o PCM cru em vez
de tocar, e aí o som vira número.

```sh
SDL_AUDIO_DRIVER=disk SDL_AUDIO_DISK_OUTPUT_FILE=saida.raw ./build/debug/jogo
```

O arquivo sai no formato do dispositivo (aqui S16LE estéreo a 44100 Hz); foi
assim que o loop do ambiente, os fades e o abafamento do casco foram conferidos.

Marque todo patch temporário com `// TEMP` e confira com `grep -rn "TEMP" src/`
antes de commitar.

## Arquitetura

O `App` é dono de janela, renderer e de todos os subsistemas; cada cena recebe um
`Context` com referências para eles (`input`, `assets`, `audio`, `fonte`,
`cenas`). Não há estado global.

**Laço de passo fixo.** `App::rodar()` acumula o tempo real e simula em fatias de
1/60 s, com teto de 0,25 s por quadro; o resto do acumulador vira o `alpha` que as
cenas usam para interpolar o desenho. Um acoplamento não óbvio: `App` só chama
`Input::marcarConsumido()` quando ao menos um passo rodou, e é isso que preserva
as bordas (`acaoPressionada`) em quadros sem update — com a tela a 240 Hz e a
simulação a 60 Hz, três de cada quatro quadros não chamam `atualizar()`. Se mexer
no laço, preserve essa relação.

**O voo não é uma cena.** `Flight` (`src/sim/Flight.*`) guarda pose, velocidade,
campo de rochas, colisão, integridade do casco e o ambiente sonoro, e vive na
`InteriorScene` — a nave continua voando enquanto o piloto anda lá dentro. Quem
chama `Flight::atualizar` é a cena ativa: a `FlightScene` com o comando do
jogador, a `InteriorScene` e a `StatusScene` com `Comando{}` (piloto automático,
sem código extra: sem entrada tudo tende a seguir reto).
Toda cena que bloqueia o update de quem está embaixo herda a obrigação de dar o
passo do voo, e por isso ele avança **exatamente um passo por passo fixo** nos
três casos — se mexer nisso, confira que continua assim. A `DebugScene` é a
exceção: bloqueia o update e **não** simula nada, devolvendo o passo a quem
congelou (adiante). A `FlightScene` guarda uma referência para o `Flight` da
cena de baixo; isso é seguro porque a pilha só desempilha do topo, então o
interior sempre sobrevive à cabine.

**O turbo é escasso.** Ele sai de um tanque (`Flight::reservaTurbo`) que o uso
esvazia e que se refaz sozinho enquanto o motor está fechado: cinco segundos de
turbo, quarenta e cinco para encher do vazio — nove de espera por segundo de
motor aberto. Nessa proporção o turbo não é um jeito de viajar e sim uma carta
que se joga: cinco segundos corridos custam quase um minuto de espera.

**O turbo perde força junto com a carga** (`Flight::forcaDoTurbo`): o ganho sobre
o cruzeiro é multiplicado por um fator que cai de 1 (cheio) a `kForcaMinimaTurbo`
(fim do tanque), então a nave murcha durante o próprio turbo em vez de fechar o
motor de uma vez. É o que faz o turbo **informar a própria carga**, já que o
medidor mora em outra tela; o piso existe para os últimos goles não virarem lixo.
Consequência: `kTurboPorPonto` virou assíntota — a rampa da velocidade não
alcança o teto antes de a carga cair (pico medido de 168 contra 185 nominais), e
como aquele teto é o limite de segurança da colisão, a nave só se afasta dele. O
fov da cabine, o ganho do ambiente e o brilho do escapamento acompanham sozinhos
porque os três leem `fatorTurbo()` — velocidade real contra o teto —, e não a
tecla.

**Zerar o tanque superaquece o motor** (`Flight::superaquecido`), e ele só reabre
com uma divisão inteira do medidor de volta (`kReligarTurbo`) — não com o
primeiro pingo de recarga. Sem essa trava havia um furo que a média escondia: com
o tanque no zero, soltar e apertar devolvia turbo a cada quadro e a nave ficava
rápida em picotes. A velocidade média continuava a razão entre as taxas, mas o
jogador deixava de ter de **escolher a hora**, e era isso que fazia o turbo ser
recurso e não botão. O limiar vale uma divisão porque é a unidade que a barra já
desenhava: a regra fica visível no medidor sem texto explicando.

`Flight::turbo()` não é a tecla, então a HUD da cabine escreve `[SUPERAQUECIDO]`
— e o deixa no ar **enquanto a trava durar**, não só quando alguém aperta. É
estado da nave, não resposta a comando: sem o medidor por perto, ver o aviso
sumir é como o piloto sabe a hora de voltar a correr. O medidor fica no
diagnóstico, longe da cabine, de propósito: é o mesmo pedágio que o casco cobra.

**O fim da nave.** Cada batida tira 0,125 do casco; `Flight::destruida()` é
`casco() <= 0`, e não um segundo estado a manter em dia. A partir daí o `Flight`
não manobra, não acelera, não colide e cala o ambiente — só carrega para a
frente o que sobrou. Quem estiver no topo entrega a vez à `FlightScene` (a
`InteriorScene` empilhando, a `StatusScene` substituindo-se), que estilhaça a
malha em `Destrocos`, mostra os cacos de fora e termina em `GameOverScene`. Os
três caminhos convergem para `MenuScene ▸ InteriorScene ▸ FlightScene`, e é isso
que deixa o "voltar ao menu" do fim de jogo ser sempre dois `desempilhar`.

**Pilha de cenas.** `empilhar`/`desempilhar`/`substituir` são adiados para o fim
do quadro, então uma cena pode trocar a si mesma durante o próprio `atualizar()`.
`bloqueiaUpdate()` e `bloqueiaRender()` decidem se as cenas abaixo continuam
simulando e aparecendo (`PauseScene` congela sem esconder; `FlightScene` cobre o
interior por inteiro). Depois de um `desempilhar`, quem voltou ao topo recebe
`aoRetomar`.

**Congelar sem parar o mundo se faz pelo `acompanhar`, nunca simulando por
cima.** `Scene::acompanhar(ctx, dt)` é o `atualizar` da própria cena em piloto
automático: o passo do voo, se é ela quem o dá, mais o que persegue a simulação
— câmera e fov da cabine, campo de estrelas, ponteiro do diagnóstico, o relógio
que pulsa a luz de emergência. Sem entrada e sem mexer na pilha. A `DebugScene`
só chama `SceneStack::acompanharAbaixoDoTopo` (de cima para baixo, parando na
primeira que bloqueia) e não toca no `Flight` a não ser para ler. Duas coisas
saem de graça daí, e as duas já foram bugs: o desenho de quem está embaixo não
descola do mundo (a cabine ficava para trás da nave, num falso zoom, e o fov
travava no turbo que o `Comando{}` soltava), e **o F3 aberto sobre a pausa não
despausa o jogo** — não implementar `acompanhar` é como a `PauseScene` diz que
para de verdade. Cena nova que dá o passo do voo precisa dá-lo no `acompanhar`
também, senão a viagem congela quando abrirem o painel em cima dela.

**A passagem convés ↔ cabine é uma cortina, não uma cena.** `Transicao` é
membro das duas cenas e é desenhada por cima do próprio HUD de cada uma: a saída
fecha e **fica fechada** até a cena de destino abrir a entrada a partir dela, e
a troca de cena acontece no passo em que a tela ficou coberta. Não transforme
isso em cena de overlay: ela teria que sobreviver à troca da cena de baixo (a
pilha só mexe no topo) e, no topo, tomaria de quem está embaixo o passo do voo.
Enquanto a cortina está em cena, nenhuma tecla é lida — mas `Flight::atualizar`
continua sendo chamado uma vez por passo.

**Coordenadas.** Tudo é desenhado em 640×360 lógicos com letterbox; o `App` já
converte as coordenadas de mouse dos eventos. Não escreva em pixels de janela.

**Assets.** `paths::assetsRoot()` prefere o diretório do binário
(`SDL_GetBasePath()`) e só cai no `JOGO_ASSETS_DIR` das fontes como último
recurso. Por isso a cópia de `assets/` no `CMakeLists.txt` depende dos *arquivos*
de assets (carimbo + lista gravada por `file(CONFIGURE)`), e não do relink do
alvo: amarrada a um `POST_BUILD`, mudar só um asset deixava a cópia velha e o
jogo carregava o arquivo antigo. Não volte para `POST_BUILD`.

**3D sem shaders.** `Renderer3D` é um rasterizador por software que termina em uma
chamada de `SDL_RenderGeometry` por lote: culling por normal, sombreamento flat,
recorte no plano próximo e **algoritmo do pintor** — sem z-buffer, geometria que se
interpenetra ou translúcida ordena errado. Convenção de orientação: a frente é
**-Z** (`Mat3::frente()` devolve `-colunas[2]`), então malhas novas devem ter o
nariz em -Z; a ordem dos vértices não importa, porque `orientarFacesParaFora()`
corrige o winding pela normal contra o centro da malha.

`Starfield` mantém um cubo de estrelas com wrap em torno da câmera (campo infinito
com memória constante); o rastro sai de projetar a estrela deslocada de `+v·Δt`,
porque a câmera é que andou.

`AsteroidField` usa o mesmo cubo com wrap, com quatro diferenças que não são
enfeite: a rocha que atravessa a borda é **sorteada de novo** nos eixos que não
viraram (wrap puro deixa o campo periódico — voando reto, as mesmas pedras
voltam na mesma formação a cada travessia) e as malhas são normalizadas para
raio 1, o que faz a escala de desenho ser também o raio da esfera de colisão. E a
**densidade não é uniforme**: `densidadeEm` (ruído de valor sobre a posição-mundo,
uma camada de bolsões e outra de veios esticados em X) decide se cada rocha fica
ativa, e a decisão é tomada quando ela entra no cubo e não é revista — reavaliar
por quadro faria a pedra da fronteira piscar. Rocha inativa continua alocada e
acompanhando o wrap, só não é desenhada, não colide e não entra no sonar. Por
isso `kQuantidadeRochas` é o **alocado** e não o que se vê: ele subiu para 4600
para a média em cena continuar na casa das 3300 que o resto do ajuste pressupõe.
Se mexer nas escalas ou no piso da densidade, meça a média em cena junto — é ela,
e não o total, que é a dificuldade.

E cada rocha tem uma **deriva própria** (`kDerivaMaxima`), com a magnitude
sorteada ao quadrado para o campo não virar enxame — dois terços das pedras se
leem como obstáculo e um terço, como movimento. Esse teto é **a colisão que o
fixa**, não o gosto: ela testa esferas na posição do passo, sem varredura, então
a relativa nave-rocha não pode passar de 252 u/s (4,2 unidades por passo, a menor
sobreposição possível). O turbo reserva 240 e a deriva usa 6 — mexer num dos dois
é mexer no outro, e ir além exige varrer o segmento.

A
névoa do `Renderer3D` (`definirNevoa`) existe para a rocha emergir do vazio em
vez de aparecer inteira na borda do campo, e de quebra descarta as faces que já
viraram a cor do fundo.

**Texto.** `BitmapFont` lê um atlas PNG mais metadados `.fnt` gerados por
`tools/gen_assets.py`; a ordem do charset no `.fnt` é o índice no atlas. Posicione
texto com `medir()` e `alturaLinha()` em vez de constantes — trocar a fonte muda a
métrica da célula (já aconteceu: 11×18 → 8×16).

**Entrada.** As cenas consultam ações (`Acao::Interagir`, ...), nunca scancodes; o
mapeamento para teclado e gamepad vive numa tabela única no topo de
`src/input/Input.cpp`, na mesma ordem do enum.

**Áudio.** O ambiente do lado de fora toca a viagem inteira, abafado por um
ganho menor enquanto o jogador está no convés (`Flight::definirAbafado`); a
rampa que faz a passagem entre convés e cabine ser um swell fica no `Flight`,
não na cena, senão a troca de cena viraria um degrau. Sem SDL3_mixer: cada reprodução é um `SDL_AudioStream` ligado ao
dispositivo, que faz a mixagem, com carência de 250 ms antes de recolher a voz
para não cortar o fim do som. Um loop (`tocarEmLoop`) é a mesma voz reabastecida
em `atualizar()` enquanto a fila do fluxo estiver com menos de meio segundo, e é
o único caso em que **não** se chama `SDL_FlushAudioStream`: o flush anuncia o
fim do sinal e faria o reamostrador zerar o estado a cada volta, marcando a
emenda. O WAV do loop também precisa emendar sozinho — `gerar_ambiente` em
`tools/gen_assets.py` roda o filtro do ruído marrom em círculo justamente para
isso.

**A pausa cala tudo suspendendo o dispositivo** (`Audio::suspender`, pedido no
`aoEntrar` da `PauseScene` e desfeito no `aoSair`), e não zerando ganhos nem
parando vozes: as filas ficam onde estavam, então o ambiente e a sirene retomam
do ponto exato, sem emenda e sem adiantar. Enquanto suspenso, `Audio::atualizar`
só anota o relógio e sai — recolher voz com o dispositivo parado descartaria som
que ainda não tocou.

**A sirene do casco crítico** é outra voz em loop que toca a viagem inteira,
calada até `casco() <= kCascoCritico`. `Flight::alarme()` é **um número só** que
abre o ganho dela e acende a luz vermelha da `InteriorScene`: não grave o vaivém
no WAV, senão o som passa a andar pelo relógio do dispositivo de áudio e sai do
compasso da luz, que anda pelo passo fixo. `kCascoCritico` também é a fronteira
do `CRITICO` do diagnóstico — é uma só de propósito.

**O sonar de rota** é o aviso que atravessa o casco: no convés o jogador não vê o
campo, e o que ele ouve é um bipe cuja **cadência** aperta conforme a rocha chega
(`Flight::proximidade`). Ele mede a pedra no caminho reto à frente
(`AsteroidField::distanciaNaRota`), e não a mais próxima em qualquer direção — o
corredor varrido é a previsão do piloto automático, que é quem pilota enquanto se
anda lá dentro. **Na cabine ele se cala**: ali a rocha está na tela, e o sonar é o
substituto da vista, não o acompanhamento dela — quem decide é o mesmo `abafado_`
do ambiente, um booleano só para as duas metades da mesma troca. O alcance é o do
sensor **como ele está agora**, a mesma rampa da névoa, então o convés ouve até
onde a cabine veria e um ponto de energia no sensor compra alcance nas duas
formas sem uma segunda tabela a manter em dia. O relógio do bipe anda mesmo
calado, para quem volta ao convés ouvir no primeiro passo. Ao contrário do ambiente
e da sirene não é voz em loop — cada bipe é um `tocar`, porque o que informa é o
intervalo entre eles. A largura do corredor é o ajuste sensível: alargá-la faz o
sonar medir a densidade do campo em vez do risco e virar chiado.

## Dependências

**Só SDL3 ≥ 3.4.** O SDL 3.4 traz `SDL_LoadPNG` e `SDL_LoadWAV` no core, o texto
usa fonte bitmap e o áudio usa `SDL_AudioStream` — não acrescente `SDL3_image`,
`SDL3_ttf` ou `SDL3_mixer` sem uma necessidade real (OGG, JPG, TrueType
escalável). A fonte `assets/fonts/unscii-16.ttf` está em domínio público e
acompanha o repositório justamente para que gerar assets não dependa do sistema.

## Convenções

Identificadores e comentários em português; arquivos de `src/` sem acentuação,
`README.md` e `docs/` com acentuação normal. Mensagens de commit em português,
com o corpo explicando **por que** a mudança foi feita.

**Cada tela tem três nomes** — o tipo (`<Palavra>Scene`, uma palavra em inglês),
o nome de ficção usado em prosa ("a bancada", "o convés") e o título em caixa
alta desenhado na tela —, mais o `Acao::<Nome>` da tecla que a abre. A tabela com
os nomes de todas elas e as regras de cada um dos três está em
[docs/ARQUITETURA.md § Como se chamam as telas](docs/ARQUITETURA.md#como-se-chamam-as-telas):
tela nova ganha uma linha ali **antes** do `.cpp`.

Mais detalhes em [README.md](README.md), [docs/ARQUITETURA.md](docs/ARQUITETURA.md)
e [docs/FUNCIONAMENTO.md](docs/FUNCIONAMENTO.md).
