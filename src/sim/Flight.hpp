#pragma once

#include <SDL3/SDL.h>

#include <cstddef>

#include "audio/Audio.hpp"
#include "gfx3d/AsteroidField.hpp"
#include "gfx3d/Math3D.hpp"

namespace jogo {

struct Context;

/// O voo da nave: para onde ela vai, por onde passa e no que bate.
///
/// Isto e estado da viagem, nao de uma tela. A nave continua voando -- e
/// batendo -- enquanto o piloto anda la dentro, entao quem guarda o Flight e a
/// InteriorScene (a nave em que se anda e a mesma que voa) e a FlightScene so
/// pilota e desenha o mesmo estado. Sem comando o voo segue reto na velocidade
/// de cruzeiro: e o piloto automatico, sem nenhum codigo a mais.
///
/// O ambiente sonoro tambem vive aqui, pelo mesmo motivo: ele acompanha a
/// viagem, nao a cena. La dentro o casco o abafa (`definirAbafado`). O alarme
/// do casco critico segue a mesma regra: a nave e que soa, nao a tela.
class Flight {
public:
    /// Comando do piloto no passo; tudo zerado e piloto automatico.
    struct Comando {
        SDL_FPoint eixo{0.0f, 0.0f};
        bool turbo{false};
    };

    /// Estado interpolavel entre dois passos fixos.
    struct Pose {
        Vec3 posicao{};
        float yaw{0.0f};
        float pitch{0.0f};
        float roll{0.0f};
    };

    /// A energia da nave e uma so, e se reparte entre quatro sistemas: motor,
    /// turbo, sensor e casco. A soma e fixa, entao aqui nao se melhora nada --
    /// so se decide de onde tirar, e nao existe reparticao certa, existe
    /// reparticao adequada ao momento.
    ///
    /// O minimo de 1 nao e detalhe: sensor zerado seria voar cego, o que nao e
    /// risco e sim injustica, motor zerado seria uma nave parada e turbo zerado
    /// seria uma tecla que nao faz nada. O teto de 4 e o que faz o extremo
    /// custar os outros: por um sistema no talo deixa apenas um ponto solto
    /// para os outros tres, que ficam em 2, 1 e 1.
    static constexpr int kPontoMinimo = 1;
    static constexpr int kPontoMaximo = 4;
    static constexpr int kPontoNeutro = 2;
    static constexpr int kSistemasDeEnergia = 4;
    static constexpr int kPontosDeEnergia = kSistemasDeEnergia * kPontoNeutro;

    /// Onde cada ponto de energia esta. O que sobrar para kPontosDeEnergia e a
    /// reserva: energia parada, que nao alimenta sistema nenhum. Sair do painel
    /// com um ponto ali e desperdicio, e nao um erro a impedir -- o painel
    /// avisa, e quem decide e o jogador.
    struct Reparticao {
        int motor{kPontoNeutro};
        /// O turbo e uma reparticao **dentro** da reparticao: o motor da a
        /// velocidade que a nave mantem, e este ponto diz quanto ela consegue
        /// puxar acima disso e por quanto tempo. Sao dois gastos diferentes da
        /// mesma energia -- viajar depressa o tempo todo, ou viajar devagar e
        /// ter como atravessar um trecho ruim.
        int turbo{kPontoNeutro};
        int sensor{kPontoNeutro};
        int casco{kPontoNeutro};
    };

    /// O que cada ponto compra, indexado pelos pontos do sistema. A posicao 0
    /// nao e usada: os pontos comecam em kPontoMinimo, e repetir o primeiro
    /// valor ali evita um "menos um" em cada leitura.
    ///
    /// **kPontoNeutro reproduz o jogo numero por numero** -- 62 u/s de
    /// cruzeiro, 185 de turbo, nevoa comecando a 45 e um oitavo do casco por
    /// rocha. Nao e coincidencia: e o que mantem valido todo o ajuste que ja
    /// tinha sido feito antes de a energia se repartir.
    ///
    /// **O degrau de baixo e o maior de todos, de proposito.** Deixar um sistema
    /// no minimo tem que ser uma perda que se sente no primeiro segundo, senao a
    /// reparticao vira decoracao: o jogador poria tudo em 1 e um so em 4 sem
    /// nada doer. Por isso as tabelas nao sobem em passos iguais -- do 1 para o
    /// 2 se paga caro, e dai para cima o ganho e mais modesto.
    static constexpr float kCruzeiroPorPonto[kPontoMaximo + 1] = {30.0f, 30.0f, 62.0f, 82.0f,
                                                                  100.0f};
    /// **Quanto o turbo puxa acima do cruzeiro**, indexado pelos pontos do
    /// turbo -- e nao pelos do motor. O motor decide de onde a rampa sai; este
    /// numero, ate onde ela vai. Somados, sao a velocidade com o motor aberto.
    ///
    /// O teto da soma e o passo fixo, e nao o gosto: a 240 u/s a nave anda 4,0
    /// unidades por passo, e a menor colisao possivel e 4,2 (raio 2,0 da nave
    /// mais 2,2 da menor rocha). Acima disso ela comecaria a atravessar pedra
    /// sem nunca encostar nela.
    ///
    /// O que conta e a velocidade **relativa**, e desde que as rochas derivam
    /// (AsteroidField::kDerivaMaxima) parte da folga e delas: 4,2 por passo dao
    /// 252 u/s de relativa, este teto usa 240 e a rocha mais rapida vindo de
    /// frente usa mais 6. Mexer em um dos dois numeros e mexer no outro. Quem
    /// confere que nenhuma reparticao possivel passa disso e o static_assert
    /// logo abaixo da classe -- a conta agora tem duas tabelas dentro, e ela
    /// nao pode depender de alguem refaze-la na cabeca.
    static constexpr float kVelocidadeMaximaSegura = 240.0f;
    static constexpr float kGanhoTurboPorPonto[kPontoMaximo + 1] = {45.0f, 45.0f, 123.0f, 150.0f,
                                                                    172.0f};
    /// O sensor e uma **janela de visao**, e nao um so numero: o par abaixo e o
    /// inicio e o fim da nevoa da FlightScene. Ate o primeiro a rocha aparece
    /// como ela e; do primeiro ao segundo ela vai virando a cor do fundo; alem
    /// do segundo o Renderer3D nem a desenha, porque ja seria fundo.
    ///
    /// Mover so o inicio nao dava disparidade nenhuma. O fim era a borda do
    /// campo (raio 110), entao o teto batia ali: com o inicio em 100 sobrariam
    /// 10 unidades de faixa e a pedra estalaria na tela em vez de emergir. Com
    /// os dois soltos, os extremos passam a ser **duas vistas diferentes**, e
    /// nao a mesma vista mais perto ou mais longe:
    ///
    /// - no minimo, alem de 50 unidades nao ha nada desenhado. O campo vira uma
    ///   bolha estreita de bruma que so clareia em cima da nave, e a rocha
    ///   materializa-se do vazio a menos de um segundo do casco;
    /// - no maximo ve-se ate 150, tres vezes o minimo: a vista atravessa boa
    ///   parte do campo e as pedras somem so bem la atras.
    ///
    /// O teto de 150 nao e escolha de gosto, e o que o campo permite. Duas
    /// coisas o prendiam:
    ///
    /// 1. **o corte de desenho**, que descartava por distancia radial enquanto a
    ///    nevoa media profundidade. Grandezas diferentes discordam, e a rocha
    ///    aparecia com cor ao cruzar o corte. Hoje AsteroidField::submeter corta
    ///    pela mesma profundidade, entao o corte e um subconjunto do que a nevoa
    ///    ja apagou -- e deixou de limitar o alcance;
    /// 2. **o wrap**, que traz a rocha de volta a um raio de cubo da nave. Em
    ///    profundidade ela pode estar bem mais perto, se vier pelo canto do
    ///    quadro, e ai a nevoa precisa alcanca-la. Isso ainda limita, e so cede
    ///    com o cubo maior: e a razao de kRaioCampo ter ido a 280.
    static constexpr float kSensorNitidoPorPonto[kPontoMaximo + 1] = {12.0f, 12.0f, 45.0f, 70.0f,
                                                                      95.0f};
    static constexpr float kSensorVisivelPorPonto[kPontoMaximo + 1] = {50.0f, 50.0f, 110.0f,
                                                                       130.0f, 150.0f};
    /// Quanto do casco cada rocha leva embora: quatro, oito, doze ou dezesseis
    /// batidas do casco inteiro ao nada. Os valores sao escolhidos para
    /// batidasSuportadasDe dar numero redondo -- 0,084 e um pouco menos que um
    /// doze avos de proposito, porque o inverso exato arredondaria para treze.
    static constexpr float kDanoPorPonto[kPontoMaximo + 1] = {0.25f, 0.25f, 0.125f, 0.084f,
                                                              0.0625f};

    static constexpr int pontosValidos(int pontos) {
        return pontos < kPontoMinimo ? kPontoMinimo : (pontos > kPontoMaximo ? kPontoMaximo
                                                                             : pontos);
    }
    static constexpr float velocidadeDeCruzeiroDe(int pontos) {
        return kCruzeiroPorPonto[static_cast<std::size_t>(pontosValidos(pontos))];
    }
    /// A velocidade com o motor aberto: o cruzeiro do motor mais o ganho que o
    /// turbo compra. E o **teto**; o que a nave alcanca de fato depende ainda
    /// da carga do tanque (veja forcaDoTurbo).
    static constexpr float velocidadeDeTurboDe(int motor, int turbo) {
        return velocidadeDeCruzeiroDe(motor) +
               kGanhoTurboPorPonto[static_cast<std::size_t>(pontosValidos(turbo))];
    }
    /// Ate onde a rocha aparece sem nevoa nenhuma. E o numero acionavel -- o
    /// que o painel mostra e de onde saem os segundos de aviso --, porque e a
    /// distancia em que ela e inconfundivel.
    static constexpr float alcanceDoSensorDe(int pontos) {
        return kSensorNitidoPorPonto[static_cast<std::size_t>(pontosValidos(pontos))];
    }
    /// Ate onde ainda ha o que ver. Alem disto o Renderer3D descarta a face.
    static constexpr float alcanceVisivelDe(int pontos) {
        return kSensorVisivelPorPonto[static_cast<std::size_t>(pontosValidos(pontos))];
    }
    static constexpr float danoPorBatidaDe(int pontos) {
        return kDanoPorPonto[static_cast<std::size_t>(pontosValidos(pontos))];
    }

    /// O numero que resume a troca inteira: os segundos entre a rocha sair da
    /// nevoa e alcancar a nave, no cruzeiro. Motor e sensor nao sao dois
    /// ajustes independentes -- eles se multiplicam neste, e e ele, e nao a
    /// tabela, que o jogador sente. Com (2,2,2) da 0,73 s; com o motor no talo
    /// e o sensor no minimo, 0,12 s -- e ai o casco tambem esta em 1, porque
    /// nao sobrou ponto: a aposta extrema cobra os tres de uma vez.
    static constexpr float segundosDeAvisoDe(const Reparticao& reparticao) {
        return alcanceDoSensorDe(reparticao.sensor) / velocidadeDeCruzeiroDe(reparticao.motor);
    }

    /// Quantas rochas o casco inteiro aguenta com estes pontos de blindagem.
    static int batidasSuportadasDe(int pontos);

    /// **O turbo e um bem escasso.** Ele nao e um botao que se segura pelo
    /// tempo que se quiser: sai de um tanque que o proprio uso esvazia e que se
    /// recompoe sozinho, devagar, quando o motor esta fechado. Correr deixa de
    /// ser de graca e passa a ser uma decisao sobre **quando** correr.
    ///
    /// **Quanto tanque ha e o ponto de energia do turbo que decide**, e este e o
    /// segundo lado do mesmo ponto: ele compra forca (kGanhoTurboPorPonto) e
    /// duracao. Com o turbo no neutro sao os **cinco segundos** de sempre; no
    /// minimo, dois; no talo, nove.
    ///
    /// O tempo de encher **nao** acompanha, e e de proposito: quarenta e cinco
    /// segundos do vazio ao cheio em qualquer reparticao. Entao o ponto compra
    /// tambem a proporcao entre correr e esperar, que vai de vinte e dois
    /// segundos e meio de espera por segundo de motor aberto (turbo 1) a cinco
    /// (turbo 4) -- no minimo o turbo e um susto que se da uma vez por trecho, e
    /// no talo e uma ferramenta de que se lanca mao com alguma regularidade.
    static constexpr float kSegundosDeTurboPorPonto[kPontoMaximo + 1] = {2.0f, 2.0f, 5.0f, 7.0f,
                                                                         9.0f};
    static constexpr float kSegundosParaEncher = 45.0f;
    static constexpr float kRecargaTurbo = 1.0f / kSegundosParaEncher;
    /// Quantos segundos o tanque cheio da com estes pontos, e quanto dele um
    /// segundo de motor aberto gasta.
    static constexpr float segundosDeTurboDe(int pontos) {
        return kSegundosDeTurboPorPonto[static_cast<std::size_t>(pontosValidos(pontos))];
    }
    static constexpr float consumoDeTurboDe(int pontos) { return 1.0f / segundosDeTurboDe(pontos); }
    /// **Zerar o tanque superaquece o motor**, e dai ele nao volta a abrir ao
    /// primeiro pingo de recarga: exige uma **divisao inteira** do medidor de
    /// volta -- um segundo de turbo, que com o tanque neutro sao nove de espera.
    ///
    /// Sem esta trava o recurso tinha um furo grande: com o tanque no zero,
    /// soltar e apertar de novo devolvia o turbo a cada quadro, e a nave ficava
    /// permanentemente rapida em picotes. A media de velocidade continuava a
    /// razao entre as duas taxas, mas o jogador deixava de ter de **escolher a
    /// hora** -- e era essa escolha, e nao a media, que fazia o turbo escasso.
    ///
    /// O limiar e uma divisao porque e a unidade que o medidor ja desenha: a
    /// regra fica visivel na barra, sem precisar de texto explicando. A divisao
    /// vale sempre um segundo de turbo, entao o que ela custa em tempo sai da
    /// mesma conta de tudo o mais: 45 s dividido pelo tanque -- 22,5 s de espera
    /// com o turbo no minimo, 5 s no talo. Quem tem tanque grande espera menos
    /// para voltar a correr, e nao mais.
    static constexpr float religarTurboDe(int pontos) { return consumoDeTurboDe(pontos); }
    /// **O turbo perde forca junto com a carga.** O tanque nao entrega os mesmos
    /// 185 u/s do primeiro ao ultimo segundo: o ganho sobre o cruzeiro e
    /// multiplicado por uma forca que cai com a reserva, entre um piso (tanque
    /// no fim) e 1 (tanque cheio).
    ///
    /// Isso resolve, de graca, o problema de o medidor morar em outra tela: com
    /// a nave murchando debaixo da mao, **o turbo passa a informar a propria
    /// carga**. O piloto sente o tanque acabar antes de acabar, e nao e mais
    /// surpreendido pelo motor fechando de uma vez.
    ///
    /// O piso existe para o resto do tanque nao virar lixo. Sem ele a forca
    /// tenderia a zero junto com a reserva, e os ultimos goles nao valeriam o
    /// aperto do botao -- o tanque teria, na pratica, encolhido. E ele e a
    /// **terceira coisa que o ponto do turbo compra**: no minimo a nave murcha
    /// depressa e so os primeiros instantes valem alguma coisa; no talo ela
    /// empurra quase igual do comeco ao fim, e ai o tanque inteiro e util.
    static constexpr float kForcaMinimaPorPonto[kPontoMaximo + 1] = {0.18f, 0.18f, 0.35f, 0.48f,
                                                                     0.58f};
    static constexpr float forcaMinimaDe(int pontos) {
        return kForcaMinimaPorPonto[static_cast<std::size_t>(pontosValidos(pontos))];
    }

    /// Ate onde a bancada do conves leva o casco de volta. O reparo de campo
    /// nao deixa a nave nova: acima disto o estrago e de estaleiro, e a viagem
    /// segue com a marca das rochas que ja passaram. E o que impede a bancada
    /// de apagar o risco do jogo -- quem repara compra folego, nao um casco
    /// novo.
    static constexpr float kCascoReparado = 0.75f;

    /// Ate aqui o casco esta em estado critico. A fronteira e uma so para o
    /// alarme, a luz de emergencia e a palavra do diagnostico nunca se
    /// contradizerem -- a sirene nao pode tocar sobre um mostrador que ainda
    /// diz AVARIADO.
    static constexpr float kCascoCritico = 0.3f;

    /// Comeca a viagem: sorteia o campo de rochas e acende o ambiente.
    void iniciar(Context& ctx, Uint32 semente);
    /// Encerra a viagem, apagando o ambiente em fade.
    void encerrar(Context& ctx);

    void atualizar(Context& ctx, float dt, const Comando& comando);

    /// Liga enquanto o jogador estiver no interior: o casco abafa o lado de fora.
    void definirAbafado(bool abafado) { abafado_ = abafado; }

    /// Devolve casco, ate o teto do reparo de campo. Quem chama e a bancada do
    /// conves, um ponto de solda por vez; o teto e a recusa de reparar uma nave
    /// ja perdida ficam aqui, e nao na cena, porque sao regras da nave.
    void reparar(float quanto);
    /// Ha o que a bancada possa fazer? Uma nave perdida nao se conserta, e um
    /// casco acima do teto ja esta no melhor que o reparo de campo alcanca.
    bool reparavel() const { return !destruida() && casco_ < kCascoReparado; }

    /// Como a energia esta repartida agora.
    const Reparticao& energia() const { return energia_; }
    /// Os pontos que sobraram fora dos quatro sistemas.
    int reserva() const {
        return kPontosDeEnergia - energia_.motor - energia_.turbo - energia_.sensor -
               energia_.casco;
    }
    /// Reparte a energia, recusando o que quebraria o invariante (cada sistema
    /// entre o minimo e o maximo, soma dentro do total). A recusa fica aqui, e
    /// nao no painel, pelo mesmo motivo do teto do reparo: e regra da nave, e a
    /// proxima tela que mexer na energia nao pode precisar lembrar dela.
    bool repartirEnergia(const Reparticao& nova);

    /// A janela do sensor como ela esta **agora**: os valores da reparticao
    /// perseguidos em rampa, e nao os da tabela. Repartir no painel nao pode
    /// fazer a nevoa saltar na cara de quem estiver na cabine -- ela abre e
    /// fecha, que e a primeira coisa que se ve da energia chegando ao sistema.
    float alcanceDoSensor() const { return alcance_; }
    float alcanceVisivel() const { return alcanceVisivel_; }
    /// Quanto a proxima rocha vai custar de casco.
    float danoPorBatida() const { return danoPorBatidaDe(energia_.casco); }

#ifdef JOGO_DEBUG
    /// A trapaca de quem desenvolve, que o F4 liga e desliga (so na build de
    /// depuracao): a nave atravessa as rochas sem bater -- sem baque, sem som e
    /// sem estrago. E o jeito de olhar o campo, o ambiente ou uma cena demorada
    /// sem que a viagem acabe no meio da conferencia.
    ///
    /// Ela **nao conserta nada**: o casco fica no valor em que estava, com o
    /// alarme que estiver tocando, e uma nave ja perdida continua perdida --
    /// invencivel e nao levar dano novo, nao voltar do fim. E sobrevive a um
    /// `iniciar()`: quem ligou a trapaca nao a perde ao recomecar a viagem.
    void alternarInvencivel() { invencivel_ = !invencivel_; }
    bool invencivel() const { return invencivel_; }
#else
    /// Fora da build de depuracao a trapaca nao existe. Este `false` constante
    /// e o que apaga a checagem no passo do voo sem espalhar `#ifdef` por ele.
    static constexpr bool invencivel() { return false; }
#endif

    static Mat3 rotacaoDe(const Pose& pose) {
        return Mat3::deEuler(pose.yaw, pose.pitch, pose.roll);
    }

    const Pose& pose() const { return pose_; }
    Pose interpolada(float alpha) const;

    float velocidade() const { return velocidade_; }
    /// O turbo esta **aberto agora**. Nao e a tecla: sem reserva ele nao abre,
    /// e a tecla segurada com o tanque vazio nao faz a nave andar mais.
    bool turbo() const { return turbo_; }

    /// Quanto o turbo empurra agora, do piso da reparticao a 1: a fracao do ganho
    /// sobre o cruzeiro que a carga atual ainda paga. Vale mesmo com o motor
    /// fechado -- e o que o proximo aperto vai render, nao o que esta rendendo.
    float forcaDoTurbo() const {
        const float piso = forcaMinimaDe(energia_.turbo);
        return piso + (1.0f - piso) * reservaTurbo_;
    }

    /// Quantos segundos de motor aberto o tanque cheio da com a reparticao de
    /// agora. E a unidade em que o medidor do diagnostico fala, e por isso ela
    /// sai daqui e nao de uma constante: repartir energia muda a escala da
    /// barra, e a barra tem de dizer a verdade sem ninguem ir lembrar dela.
    float segundosDeTurbo() const { return segundosDeTurboDe(energia_.turbo); }

    /// Quanto resta no tanque de turbo, de 0 a 1. O mostrador dele fica no
    /// diagnostico, e nao na cabine: saber quanto sobrou custa largar os
    /// controles e atravessar a nave, o mesmo pedagio que o casco ja cobra.
    float reservaTurbo() const { return reservaTurbo_; }
    /// O motor superaqueceu e o turbo esta trancado ate a recarga devolver a
    /// primeira divisao do medidor. Enquanto isto for verdade, a tecla nao abre
    /// nada -- e a cabine diz isso na HUD, porque um controle que nao responde
    /// sem explicar por que parece defeito.
    ///
    /// E o **estado da nave**, e nao a resposta a um comando: a cabine o mostra
    /// mesmo com ninguem apertando nada, e e assim que o piloto sabe a hora de
    /// voltar a poder correr sem ter o medidor na frente.
    bool superaquecido() const { return superaquecido_; }

    /// 0 no cruzeiro, 1 no turbo: a medida de esforco do motor.
    float fatorTurbo() const;
    /// 1 no instante da batida, decai ate zero. Cada cena sacode do seu jeito.
    float batida() const { return batida_; }
    /// Integridade do casco em 0..1: comeca inteira e cai a cada batida. Como o
    /// resto da viagem, e estado do Flight -- a nave leva o estrago batendo com
    /// o piloto no conves tanto quanto na cabine.
    float casco() const { return casco_; }
    /// Casco zerado e nave perdida: nao ha um segundo estado para manter em dia.
    /// A partir daqui o Flight nao manobra, nao acelera e nao colide -- so
    /// carrega para a frente o que sobrou, para a camera ter o que seguir
    /// enquanto a cena mostra os destrocos.
    bool destruida() const { return casco_ <= 0.0f; }
    /// Casco no fim, e ainda ha nave para alarmar: uma nave ja perdida nao
    /// avisa mais ninguem.
    bool critico() const { return !destruida() && casco_ <= kCascoCritico; }
    /// O ciclo do alarme, de 0 a 1: fechado no vale, aberto no pico. E o mesmo
    /// numero que abre o ganho da sirene e acende a luz vermelha do conves --
    /// um so, para que luz e som nunca pisquem separados (veja atualizar()).
    float alarme() const { return alarme_; }

    /// O quanto a rocha mais proxima **da rota** ja chegou: 0 com o caminho
    /// livre, 1 encostando no casco. E o que o sonar do console mede, e o que
    /// ele mede e limitado pelo sensor -- entao a reparticao de energia decide
    /// a que distancia o aviso comeca a soar (veja atualizar()).
    ///
    /// O numero e medido sempre, inclusive na cabine, onde o sonar nao soa: e
    /// estado da nave, e nao do som. Quem quiser mostra-lo em outro lugar ja o
    /// tem pronto -- e a tela de depuracao o le assim.
    float proximidade() const { return proximidade_; }

    const AsteroidField& rochas() const { return rochas_; }

private:
    void checarColisao(Context& ctx);

    Pose pose_;
    Pose poseAnterior_;
    AsteroidField rochas_;

    Reparticao energia_;
    /// A janela do sensor perseguindo a da reparticao; veja alcanceDoSensor().
    float alcance_{alcanceDoSensorDe(kPontoNeutro)};
    float alcanceVisivel_{alcanceVisivelDe(kPontoNeutro)};

    float velocidade_{velocidadeDeCruzeiroDe(kPontoNeutro)};
    float batida_{0.0f};
    /// O tanque de turbo, e se o motor esta trancado esperando esfriar.
    float reservaTurbo_{1.0f};
    bool superaquecido_{false};
    float casco_{1.0f};
    bool turbo_{false};

    float ambiente_{0.0f};
    /// Onde o ciclo do alarme esta (rad) e quanto dele passa: a fase anda
    /// sempre, a intensidade e que entra e sai em rampa.
    float faseAlarme_{0.0f};
    float intensidadeAlarme_{0.0f};
    float alarme_{0.0f};
    /// O sonar de rota: o quanto a proxima rocha ja chegou, e quanto ja se
    /// passou desde o ultimo bipe.
    float proximidade_{0.0f};
    float relogioSonar_{0.0f};
    bool abafado_{true};
#ifdef JOGO_DEBUG
    bool invencivel_{false};
#endif
    Audio::SomId somAmbiente_{0};
    Audio::SomId somImpacto_{0};
    Audio::SomId somDestruicao_{0};
    Audio::SomId somSirene_{0};
    Audio::SomId somSonar_{0};
    Audio::VozId vozAmbiente_{0};
    Audio::VozId vozSirene_{0};
};

/// Nenhuma reparticao que a nave aceite pode passar da velocidade em que a
/// colisao deixa de acontecer -- e agora sao duas tabelas somadas, indexadas por
/// dois sistemas diferentes, com o teto da soma limitado ainda pelo total de
/// pontos (motor e turbo no talo ao mesmo tempo nao cabem nos oito). Nenhuma
/// dessas tres coisas se confere de cabeca, entao quem confere e o compilador.
constexpr bool turboCabeNaColisao() {
    const int sobra = Flight::kPontosDeEnergia - 2 * Flight::kPontoMinimo;
    for (int motor = Flight::kPontoMinimo; motor <= Flight::kPontoMaximo; ++motor) {
        for (int turbo = Flight::kPontoMinimo; turbo <= Flight::kPontoMaximo; ++turbo) {
            if (motor + turbo <= sobra &&
                Flight::velocidadeDeTurboDe(motor, turbo) > Flight::kVelocidadeMaximaSegura) {
                return false;
            }
        }
    }
    return true;
}
static_assert(turboCabeNaColisao(),
              "o turbo passou da velocidade em que a nave atravessa rocha sem colidir");

}  // namespace jogo
