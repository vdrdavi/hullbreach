# Como jogar

Este é o manual do **piloto**, não o do programador: ele explica o que a nave
faz, o que cada tela mostra e que decisões você tem para tomar. Como o jogo
funciona por dentro está em [FUNCIONAMENTO.md](FUNCIONAMENTO.md), e como o
código é organizado, em [ARQUITETURA.md](ARQUITETURA.md).

O documento descreve o jogo **como ele é hoje**. Mecânica nova ganha seção nova
aqui, no mesmo commit em que entra no jogo.

---

## Índice

1. [A viagem em três frases](#a-viagem-em-três-frases)
2. [Os primeiros minutos](#os-primeiros-minutos)
3. [Controles](#controles)
4. [O convés: a nave por dentro](#o-convés-a-nave-por-dentro)
5. [A cabine: pilotar](#a-cabine-pilotar)
6. [O turbo](#o-turbo)
7. [A repartição de energia](#a-repartição-de-energia)
8. [O diagnóstico da nave](#o-diagnóstico-da-nave)
9. [A bancada de reparo](#a-bancada-de-reparo)
10. [Os avisos: o que a nave conta sem você ver](#os-avisos-o-que-a-nave-conta-sem-você-ver)
11. [O campo de asteroides](#o-campo-de-asteroides)
12. [O fim da viagem](#o-fim-da-viagem)
13. [Pausa, menu e preferências](#pausa-menu-e-preferências)
14. [Teclas de quem desenvolve](#teclas-de-quem-desenvolve)
15. [Estratégias e erros comuns](#estratégias-e-erros-comuns)
16. [Todos os números em uma página](#todos-os-números-em-uma-página)

---

## A viagem em três frases

Você é o único tripulante de uma nave que atravessa um campo de asteroides. A
nave **nunca para**: ela voa sozinha enquanto você anda lá dentro, e cada rocha
que ela acerta leva um pedaço do casco embora. Quando o casco acaba, a viagem
acaba — e é só isso que existe de fim.

Não há linha de chegada, placar nem fases. O que se joga é **quanto tempo você
mantém o casco**, e a única moeda que você gasta em tudo é **atenção**: cada
segundo passado num painel do convés é um segundo em que ninguém está
desviando.

---

## Os primeiros minutos

1. No menu, `Jogar`. Você aparece no **convés**, de pé diante do painel de
   pilotagem. Lá fora a viagem já começou.
2. Uma tarja flutua sobre o painel com três opções. Aperte **`E`**: a tela
   fecha numa cortina e reabre na **cabine**, em terceira pessoa, com a nave à
   sua frente e o campo de rochas ao redor.
3. Pilote com **WASD** ou as **setas**. `W`/cima levanta o nariz, `A`/esquerda
   guina para a esquerda com a nave inclinando na curva. Não há freio nem ré: a
   nave anda sempre para a frente, na velocidade de cruzeiro.
4. Segure **`Espaço`** para o turbo. Repare que a tela abre, o motor ruge e a
   nave **vai murchando** enquanto o tanque esvazia. Solte antes de zerar.
5. Bata numa rocha de propósito, uma vez: a tela sacode, dá um clarão e a nave
   quase para. Você acabou de perder um oitavo do casco.
6. **`Esc`** volta ao convés. Ande até o painel de novo e aperte **`Q`**: o
   diagnóstico mostra o casco em 87% e o tanque de turbo. Feche com `Q` ou
   `Esc`.
7. No mesmo painel, **`R`** abre a repartição de energia — motor, turbo, sensor
   e casco, oito pontos para dividir entre os quatro.
8. Ande até o canto oposto do convés, onde fica a **bancada**, e aperte `E`:
   ali se solda o casco de volta, um ponto por vez, num compasso de ponteiro.

Enquanto você fazia tudo isso, a nave continuou voando. Se ouviu bipes, era o
**sonar de rota** avisando que havia pedra no caminho.

---

## Controles

Tudo é lido como **ação**, não como tecla: teclado e gamepad fazem a mesma coisa
e os vínculos podem ser trocados no `config.ini` (veja
[Pausa, menu e preferências](#pausa-menu-e-preferências)).

### Em toda parte

| Ação | Teclado | Gamepad |
| --- | --- | --- |
| Andar / pilotar / navegar | WASD ou setas | analógico esquerdo / direcional |
| Confirmar | Enter, Espaço | A (botão sul) |
| Voltar / fechar | Esc | B / Back |
| Tela cheia | F11 | — |

### No convés

| Ação | Teclado | Gamepad |
| --- | --- | --- |
| Assumir os controles (no painel) | `E` | X (botão oeste) |
| Diagnóstico da nave (no painel) | `Q` | Y (botão norte) |
| Repartir a energia (no painel) | `R` | RB (ombro direito) |
| Soldar o casco (na bancada) | `E` | X (botão oeste) |
| Pausar | Esc ou P | Start |

### Na cabine

| Ação | Teclado | Gamepad |
| --- | --- | --- |
| Manobrar | WASD ou setas | analógico esquerdo |
| Turbo | Espaço (segurar) | A (segurar) |
| Voltar ao convés | Esc ou P | B / Start |

Não há pausa na cabine: `Esc` e `P` ali fecham a cabine e devolvem o convés. A
pausa se abre do convés — de onde, aliás, é o único lugar em que a viagem
inteira está sob os seus pés.

### Nos painéis do convés

| Tela | Fechar | Dentro |
| --- | --- | --- |
| Diagnóstico | `Q`, `Esc` ou `P` | nada a mexer: é um mostrador |
| Repartição de energia | `R`, `Esc` ou `P` | ↑↓ escolhe o sistema, ←→ move o ponto |
| Bancada de reparo | `E`, `Esc` ou `P` | Enter ou Espaço solda |

Quem abriu a tela é quem a fecha: a mesma tecla vai e volta.

---

## O convés: a nave por dentro

O convés é a nave vista de cima, em 2D. Ele tem dois móveis, e é neles que
está o jogo inteiro fora da cabine:

- **o painel de pilotagem**, com três bocas — `E` assume os controles, `Q` abre
  o diagnóstico, `R` abre a repartição de energia;
- **a bancada de reparo**, no canto oposto, onde `E` solda o casco.

Chegue perto de um dos dois e uma tarja aparece flutuando sobre ele dizendo o
que dá para fazer ali. Longe dos dois, não há nada a apertar.

**A nave não espera por você.** Andar pelo convés não é uma pausa: lá fora o
piloto automático segue reto, e reto é exatamente para onde está a próxima
rocha. Duas coisas atravessam o casco para contar isso:

- o **sonar de rota**, um bipe que aperta a cadência conforme a pedra chega;
- com o casco no fim, a **sirene** e a **luz vermelha** que banha o convés.

Quando a nave bate com você lá dentro, o convés inteiro sacode e você ouve o
baque abafado. O estrago é o mesmo de uma batida na cabine.

---

## A cabine: pilotar

`E` no painel fecha uma cortina sobre a tela e a reabre na vista externa: a nave
à frente, o campo de estrelas ao redor, a névoa do sensor fechando o horizonte.

- **A nave anda sempre para a frente.** Você controla para onde ela aponta, não
  o quanto ela acelera — só o turbo muda a velocidade.
- **Guinada e arfagem**: esquerda/direita giram, cima/baixo levantam e baixam o
  nariz. O nariz sobe com `W` (não é invertido) e trava a cerca de 66° para
  cima e para baixo.
- **A câmera tem atraso de propósito.** Ela persegue a nave, então a manobra
  tem peso e você vê a nave inclinar na curva antes de o quadro acompanhar.

A HUD do canto superior esquerdo tem **uma linha só**:

```
VEL 168 u/s  [TURBO]
```

- `VEL` é a velocidade real, em unidades por segundo;
- `[TURBO]` aparece enquanto o motor está de fato aberto — não quando você
  aperta a tecla;
- `[SUPERAQUECIDO]` aparece no lugar dele enquanto o motor está trancado, e
  **fica no ar até a trava passar**. É assim que você sabe a hora de voltar a
  correr sem atravessar a nave para ler o medidor.

Quanto do campo você enxerga depende do sensor: as rochas emergem da névoa a
uma distância e são desenhadas até outra, ambas escolhidas na repartição de
energia. Bater faz a tela sacudir, dar um clarão alaranjado e a nave quase
parar (cai para 18 u/s, e a rampa do motor a leva de volta ao cruzeiro).

---

## O turbo

O turbo **é escasso**, e essa é a mecânica inteira:

- o tanque cheio dá **5 segundos** de motor aberto — com o turbo em 2, que é
  como a viagem começa; a repartição de energia move isso de 2 a 9 segundos;
- do vazio ao cheio são **45 segundos**, sempre, em qualquer repartição — com o
  tanque de 5 s, nove segundos de espera por segundo de turbo;
- o tanque só se recompõe com o motor **fechado**.

Três consequências que você sente antes de ler qualquer número:

**1. O turbo perde força junto com a carga.** Ele não entrega os mesmos 185 u/s
do primeiro ao último segundo: o ganho sobre o cruzeiro encolhe conforme o
tanque esvazia (até 35% dele com o turbo em 2 — 18% no mínimo, 58% no talo). A
nave murcha debaixo da sua mão em vez de fechar o motor de uma vez — é o próprio
turbo contando quanto ainda resta. Por isso a velocidade de pico que você vê na
HUD (uns 168 u/s com motor e turbo em 2) fica **abaixo** do número nominal da
tabela: a carga cai antes de a rampa chegar lá.

**2. Zerar o tanque superaquece o motor.** Ele não volta a abrir ao primeiro
pingo de recarga: exige **uma divisão inteira** do medidor de volta — um segundo
de turbo, nove de espera. Enquanto isso, a tecla não faz nada e a HUD diz
`[SUPERAQUECIDO]`.

**3. O medidor mora em outra tela.** Quanto resta no tanque só se lê no
diagnóstico, no convés. Saber custa largar os controles e atravessar a nave —
o mesmo pedágio que o casco cobra.

Turbo não é jeito de viajar; é carta que se joga. Cinco segundos corridos custam
quase um minuto de espera — e quanto vale essa carta é você quem decide, no
painel de energia.

---

## A repartição de energia

`R` no painel. A nave tem **oito pontos de energia** e quatro sistemas —
**motor**, **turbo**, **sensor** e **casco**. Cada sistema aceita de **1 a 4**
pontos, e a soma é fixa: aqui não se melhora nada, só se decide **de onde
tirar**.

↑↓ escolhem o sistema; → tira um ponto da reserva e põe no sistema, ← faz o
contrário. A energia sempre passa pela **reserva**, à vista — não há atalho que
mova um ponto direto de um sistema para o outro. Ponto parado na reserva não
alimenta nada: o painel avisa com `ENERGIA PARADA`, mas não impede.

### O que cada ponto compra

| Pontos | Motor (cruzeiro) | Turbo (acima do cruzeiro) | Turbo (tanque) | Sensor (nítido) | Sensor (visível) | Casco (dano por rocha) | Casco (batidas) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 30 u/s | +45 u/s | 2 s | 12 u | 50 u | 25% | 4 |
| 2 | 62 u/s | +123 u/s | 5 s | 45 u | 110 u | 12,5% | 8 |
| 3 | 82 u/s | +150 u/s | 7 s | 70 u | 130 u | 8,4% | 12 |
| 4 | 100 u/s | +172 u/s | 9 s | 95 u | 150 u | 6,25% | 16 |

**O degrau de baixo é o maior dos quatro, de propósito**: deixar um sistema no
mínimo dói no primeiro segundo. Do 1 para o 2 se paga caro; daí para cima o
ganho é mais modesto.

O turbo se soma ao cruzeiro: com motor e turbo em 2 são 62 + 123 = **185 u/s**, o
número que a fileira do painel mostra. Mexer no motor muda a linha do turbo
junto, porque é do cruzeiro que ele parte. E o ponto de turbo compra três coisas
de uma vez — quanto ele puxa, quantos segundos dura e quanto ainda empurra no fim
do tanque —, então um turbo no mínimo não é "o mesmo turbo mais curto": é um
empurrão de dois segundos que murcha quase todo, e ainda custa 22 segundos de
espera para religar depois de zerar.

O sensor tem **dois** números porque ele é uma janela, não uma distância: até o
primeiro a rocha aparece como ela é, do primeiro ao segundo ela vai virando a
cor do fundo, e além do segundo não há nada desenhado. No mínimo, o campo é uma
bolha estreita de bruma e a pedra se materializa a um piscar do casco; no
máximo, você vê boa parte do campo.

### O número que resume a troca

Motor e sensor não são dois ajustes independentes — eles se multiplicam nos
**segundos de aviso**: quanto tempo passa entre a rocha sair da névoa e alcançar
a nave, no cruzeiro.

| Repartição (motor/turbo/sensor/casco) | Segundos de aviso |
| --- | --- |
| 2 / 2 / 2 / 2 (o padrão) | 0,73 s |
| 1 / 1 / 4 / 2 (vista de longe, devagar) | 3,2 s |
| 4 / 1 / 1 / 2 (correr às cegas) | 0,12 s |

Repare que a aposta extrema cobra o resto da nave de uma vez: pôr o motor no talo
deixa **um ponto solto** para os outros três, e é você quem escolhe qual deles
não fica no mínimo.

A energia repartida vale por **esta** viagem: uma partida nova começa em
2/2/2/2.
E a mudança não é instantânea — a névoa abre e fecha em rampa, e é ela que
mostra a energia chegando ao sistema.

---

## O diagnóstico da nave

`Q` no painel. É um mostrador: não há nada para mexer, só para ler.

- **CASCO**, em barra e em porcentagem, com a palavra do estado —
  `INTEGRO` acima de 60%, `AVARIADO` até 30%, `CRITICO` daí para baixo. Quando
  uma batida acabou de acontecer, a barra mostra em vermelho o pedaço que se
  foi e a palavra pisca.
- **quanto cada rocha custa**, que sai da blindagem que você escolheu.
- **TURBO**: o tanque, em barra e em segundos, com uma marca por segundo — de
  duas a nove marcas, conforme o ponto de energia do turbo, e é assim que a
  barra diz em que escala está falando. Com o motor superaquecido, a primeira
  marca acende: é o alvo que a recarga precisa alcançar para o turbo religar.

O diagnóstico se abre por cima do convés, que continua visível atrás — inclusive
a luz vermelha do casco crítico. E a nave continua voando enquanto você lê: o
tanque de turbo se recompõe na sua frente justamente porque o piloto automático
está com o motor fechado.

---

## A bancada de reparo

`E` na bancada, no canto oposto do convés. Um ponteiro varre uma barra de ponta
a ponta e você aperta **Enter** ou **Espaço** dentro da zona verde.

- **Acertar** devolve **3,5%** de casco, encolhe a zona e acelera o ponteiro. A
  série vai ficando mais difícil justamente porque está indo bem.
- **Errar** não custa casco: custa **0,55 s** de maçarico frio (o ponteiro para
  e pisca) e a série volta ao começo — zona larga, ponteiro lento.
- **Uma batida quebra a solda.** Se a nave acertar uma rocha enquanto você
  solda, a série se perde do mesmo jeito, esteja você olhando ou não.

Dois limites decidem quando descer da bancada:

- **o teto de 75%.** O reparo de campo não deixa a nave nova: acima disso o
  estrago é de estaleiro, e a barra aparece apagada com
  `CASCO NO LIMITE DO REPARO DE CAMPO`. Quem repara compra fôlego, não um casco
  novo.
- **a aritmética.** Uma batida com a blindagem padrão tira 12,5%; são **mais de
  três acertos** só para pagá-la. E enquanto você solda, ninguém desvia — a
  rocha que chegar tira mais do que a série inteira devolveu.

Não há cronômetro dizendo a hora de parar. Quem decide é você, ouvindo o sonar.

---

## Os avisos: o que a nave conta sem você ver

| Aviso | Onde | O que significa |
| --- | --- | --- |
| **Sonar de rota** (bipe) | só no convés e nos painéis | há rocha no caminho reto à frente; quanto mais apertada a cadência, mais perto |
| **Sirene** (loop grave) | em toda parte | casco em 30% ou menos |
| **Luz vermelha** | convés | o mesmo alarme da sirene, em imagem — os dois pulsam juntos |
| **Tremor + clarão** | em toda parte | acabou de bater; quanto mais fraca a blindagem, mais forte o tranco |
| **Ambiente mais alto** | cabine | o motor abriu — o rugido acompanha o esforço, não a tecla |

O **sonar** merece um parágrafo. Ele mede a pedra **na rota**, não a mais
próxima em qualquer direção: a que passa de lado está perto sem ser ameaça. O
corredor que ele varre é a previsão do piloto automático — que é exatamente
quem está pilotando enquanto você anda lá dentro. O intervalo entre bipes vai de
0,85 s (na borda do sensor) a 0,10 s (encostando), e o alcance é o do **seu**
sensor: um ponto de energia ali compra aviso no convés e vista na cabine de uma
vez.

**Na cabine o sonar se cala.** Ali a rocha está na tela: ele é o substituto da
vista, não o acompanhamento dela.

Uma série de bipes que aperta e não para quase sempre termina em batida. Quando
ouvir isso da bancada, largue a solda e volte para os controles.

---

## O campo de asteroides

O campo não é uma chuva constante. Quatro coisas o tornam um lugar, e não um
gerador de obstáculos:

- **Bolsões e veios.** A densidade varia com a posição: há vazios em que se voa
  minutos sem ver nada e apertos em que a pedra vem em onda. Um trecho calmo
  não quer dizer que o campo acabou.
- **As rochas derivam.** Cada uma tem movimento próprio (até 6 u/s). Desviar não
  é mirar onde a pedra está: é prever onde ela vai estar. Dois terços delas se
  leem como obstáculo parado; o resto, como movimento.
- **Os monólitos.** Uma em cada cem é enorme — raio de 12 a 20, contra 7,5 da
  maior rocha comum. Ela não se contorna no último segundo: ou você decidiu
  cedo, ou bateu. Em compensação, ela aparece na névoa muito antes das outras.
- **O campo não se repete.** A rocha que fica para trás é sorteada de novo, e
  não devolvida na mesma formação: voar reto não faz as mesmas pedras voltarem.

Toda rocha custa o mesmo pedaço de casco, do cascalho ao monólito. O tamanho
muda a chance de você acertá-la, não o preço.

---

## O fim da viagem

Quando o casco chega a zero, a nave está perdida. A partir daí ela não manobra,
não acelera e não colide — só carrega para a frente o que sobrou.

Aconteça isso onde acontecer — na cabine, no convés, num painel aberto ou na
bancada —, a vista vai para **fora**: a malha se estilhaça em destroços que
correm com a velocidade que a nave tinha, as faíscas se abrem, e só então a tela
apaga para o `NAVE PERDIDA`.

`Enter` ou `Esc` volta ao menu. Não há continuar: a viagem seguinte começa do
zero, com o casco inteiro, o tanque cheio e a energia em 2/2/2/2.

---

## Pausa, menu e preferências

**A pausa** (`Esc` ou `P` no convés) congela o mundo de verdade: o som não é
silenciado, o dispositivo de áudio é suspenso — o ambiente e a sirene retomam do
ponto exato, sem emenda e sem terem adiantado. `Esc`/`Enter` continua, `M` volta
ao menu.

**O menu** abre com o logotipo `HULLBREACH` no topo — `HULL` em aço, `BREACH` em
âmbar e uma fenda faiscando entre as sílabas — sobre um campo de estrelas que
deriva devagar ao fundo, e traz `Jogar`, `Volume` (←→ ajusta de 5 em 5; o blip
toca já no volume novo, então o som é a própria prévia), `Tela cheia` e `Sair`.

**As preferências** — volume, tela cheia e os vínculos de cada ação — ficam em
`config.ini`, no diretório de configuração do sistema (`~/.local/share/jogo-sdl/jogo/`
no Linux, `%APPDATA%\jogo-sdl\jogo\` no Windows,
`~/Library/Application Support/jogo-sdl/jogo/` no macOS). Como ainda não há tela
de remapeamento, editar esse arquivo é o jeito de trocar um controle:

```ini
volume=0.6
tela-cheia=0

tecla.interagir.1=E
botao.interagir.1=x
```

Os nomes são os do SDL (`Left`, `Keypad Enter`, `dpleft`). Citar uma ação
substitui os vínculos de fábrica dela; apagar o arquivo restaura tudo.

---

## Teclas de quem desenvolve

Só existem no build `debug` — no `release`, nem o código nem as teclas existem.

- **F3** abre a tela de depuração de qualquer lugar: quadros por segundo,
  drivers, renderer, tamanho da janela, de onde os assets vêm e o estado da
  nave. Ela congela o que está embaixo sem parar o mundo — e, aberta sobre a
  pausa, **não** despausa o jogo.
- **F4** deixa a nave atravessar as rochas sem bater: sem baque, sem som e sem
  estrago. Um selo âmbar **INVENCIVEL (F4)** fica no canto avisando que aquela
  execução não vale como teste. Ela não conserta nada — o casco fica onde
  estava, e uma nave já perdida continua perdida.

---

## Estratégias e erros comuns

**Fique na cabine por padrão.** Todo o resto do jogo é feito às cegas. Vá ao
convés com propósito: repartir a energia depois de entender o trecho, soldar
quando o casco pesar, ler o tanque quando o turbo importar.

**Solde nos vazios, não nos apertos.** A bancada é o lugar mais caro da nave
para se estar. O sinal de que dá para ir é o silêncio do sonar; o sinal de que
acabou é o primeiro bipe.

**O turbo é para atravessar, não para viajar.** Ele serve para cruzar um trecho
denso que você já enxergou inteiro, ou para escapar de uma formação. Cinco
segundos gastos à toa é um minuto de viagem sem carta na mão — e o motor
superaquecido dá justamente o intervalo em que você mais queria correr.

**Turbo alto ou motor alto são viagens diferentes.** O motor rende o tempo todo e
encurta o aviso o tempo todo; o turbo não rende nada até você apertar, e aí rende
muito. Se a viagem tem sido de trechos limpos com apertos curtos, o ponto vale
mais no turbo; se ela é densa do começo ao fim, o turbo é energia parada com
outro nome.

**Sacrificar o sensor é diferente de sacrificar o casco.** Casco no mínimo
significa quatro batidas; sensor no mínimo significa **não ver** a batida
chegar. O primeiro é uma conta, o segundo é uma aposta.

**Não deixe ponto na reserva.** A nave não guarda energia para depois: um ponto
parado é uma vantagem que você simplesmente não está usando.

**Um bipe que acelera termina em batida.** Nas medições, oito de cada nove
séries de bipes acabaram em rocha. Se ouvir a cadência apertando e você não
está nos controles, é hora de estar.

---

## Todos os números em uma página

| Coisa | Valor |
| --- | --- |
| Cruzeiro (motor 2) | 62 u/s |
| Turbo nominal (motor 2, turbo 2) | 185 u/s (pico real perto de 168) |
| Turbo mais rápido possível (motor 2, turbo 4) | 234 u/s (pico real perto de 223) |
| Velocidade logo após uma batida | 18 u/s |
| Tanque de turbo | 5 s de uso com o turbo em 2 (2 a 9 s), 45 s do vazio ao cheio |
| Religar após superaquecer | 1 divisão do medidor = 1 s de turbo (22,5 s a 5 s de espera) |
| Força do turbo no fim do tanque | 35% do ganho com o turbo em 2 (18% a 58%) |
| Dano por rocha (casco 2) | 12,5% — 8 batidas do casco inteiro |
| Casco crítico (sirene e luz) | 30% ou menos |
| Solda | +3,5% por acerto, teto de 75% |
| Erro de solda / batida na bancada | 0,55 s de maçarico frio e série perdida |
| Pontos de energia | 8, entre 4 sistemas, de 1 a 4 cada |
| Sensor (nítido/visível, sensor 2) | 45 u / 110 u |
| Cadência do sonar | 0,85 s (longe) a 0,10 s (encostando) |
| Deriva das rochas | até 6 u/s |
| Monólitos | 1 em cada 100, raio 12 a 20 |
| Raio de colisão da nave | 2 u |
