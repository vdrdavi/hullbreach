#include "sim/Flight.hpp"

#include <algorithm>
#include <cmath>

#include "core/Context.hpp"

namespace jogo {
namespace {

constexpr float kTaxaGiro = 1.15f;    // rad/s
constexpr float kTaxaPitch = 0.95f;   // rad/s
constexpr float kLimitePitch = 1.15f; // rad

// Campo de asteroides: o cubo com wrap e tambem o alcance de desenho.
//
// O raio e o que decide ate onde o sensor pode enxergar, e por um caminho
// indireto: a rocha que o wrap traz de volta esta a um raio de distancia, mas em
// **profundidade de camera** pode estar bem mais perto, se vier pelo canto do
// quadro. Se a nevoa ainda a mostrar nessa profundidade, ela aparece do nada --
// e o "asteroide nascendo" que se ve na borda do campo.
//
// Medindo o submeter por 580 quadros de voo manobrado em turbo (o pior caso: o
// fov abre de 62 para 80 graus e aproxima a entrada mais rasa), com raio 280 a
// pedra mais rasa entra a 151 de profundidade. Dai o teto de 150 no fim da nevoa
// (Flight::kSensorVisivelPorPonto). Com o raio anterior, de 170, o mesmo limite
// caia para 118 -- era o que prendia o sensor em 105.
//
// A quantidade acompanha o cubo **ao cubo**, e nao e enfeite: a densidade e que
// decide quantas rochas se cruza por minuto, entao 200 em um cubo de raio 110
// viram 3300 em um de raio 280 (1,88e-5 rocha por unidade cubica nos dois).
// Aumentar o campo sem isso seria baixar a dificuldade pela porta dos fundos.
// Medido em release: 4,9 ms por quadro, contra os 16,7 de 60 Hz.
constexpr float kRaioCampo = 280.0f;
constexpr int kQuantidadeRochas = 3300;
constexpr float kRaioNave = 2.0f;

// Batida: a nave quase para e o baque decai por si.
constexpr float kVelocidadeAposBatida = 18.0f;
constexpr float kDecaimentoBatida = 3.4f;  // 1/s
/// Com que rapidez o alcance do sensor persegue o da reparticao. Lento de
/// proposito: repartir a energia e uma decisao com consequencia, e a nevoa
/// abrindo ou fechando devagar e o que a torna visivel.
constexpr float kTaxaSensor = 1.6f;

// Ruido do casco: sempre presente, mais forte quando o motor abre.
constexpr float kAmbienteCruzeiro = 0.45f;
constexpr float kAmbienteTurbo = 0.9f;
constexpr float kTaxaAmbiente = 2.0f;   // 1/s
constexpr float kAmbienteSaida = 0.35f; // s de fade ao encerrar
/// Quanto do lado de fora atravessa o casco.
constexpr float kAbafamento = 0.34f;

// Alarme do casco critico. Pouco menos de um ciclo por segundo: rapido o
// bastante para soar urgente, lento o bastante para a luz do conves piscar em
// vez de tremular.
constexpr float kFreqAlarme = 0.85f;   // ciclos/s
constexpr float kTaxaAlarme = 2.5f;    // 1/s, entrada e saida do alarme
constexpr float kGanhoSirene = 0.55f;
constexpr float kTau = 6.283185307f;

// O sonar de rota: a rocha que o piloto automatico vai acertar, dita em bipes.
//
// A folga e somada ao raio da nave e ao da rocha, entao o tubo varrido tem de
// 7,2 a 12,5 unidades de raio, contra os 4,2 a 9,5 em que a colisao acontece de
// fato. Sao tres unidades de margem, e essa largura e o ajuste que decide se
// isto e um aviso ou um chiado. Medindo 30 s de piloto automatico com o sensor
// neutro: com uma folga de 8 o sonar soava 58% do tempo, em series de ate 28
// bipes -- isso e a densidade do campo, e nao o risco, e ninguem escuta um
// chiado desses depois do primeiro minuto. Com 3 sao 32% do tempo, em series de
// 4 a 10 bipes, uma a cada 3,3 s, e **8 das 9 series terminaram em batida**: o
// silencio volta a ser a regra e o bipe passa a valer o que promete.
constexpr float kCorredorSonar = 3.0f;
// Os extremos da cadencia. Interpolados em razao, e nao em diferenca: o ouvido
// compara intervalos por quociente, e uma rampa linear entre 0,85 s e 0,10 s
// pareceria nao acelerar nada ate o fim e entao disparar de uma vez.
constexpr float kIntervaloSonarLonge = 0.85f;  // s, na borda do sensor
constexpr float kIntervaloSonarPerto = 0.10f;  // s, com a rocha encostando
constexpr float kGanhoSonarLonge = 0.5f;
constexpr float kGanhoSonarPerto = 1.0f;


/// Suavizacao exponencial estavel em passo fixo.
float aproximar(float atual, float alvo, float taxa, float dt) {
    return atual + (alvo - atual) * (1.0f - std::exp(-taxa * dt));
}

}  // namespace

void Flight::iniciar(Context& ctx, Uint32 semente) {
    pose_ = Pose{};
    poseAnterior_ = pose_;
    // A viagem comeca com a energia dividida em partes iguais, que e a nave
    // como ela sempre foi. Ao contrario da invencibilidade do F4, a reparticao
    // nao sobrevive a um recomeco: ela e uma decisao desta viagem.
    energia_ = Reparticao{};
    alcance_ = alcanceDoSensorDe(energia_.sensor);
    alcanceVisivel_ = alcanceVisivelDe(energia_.sensor);
    velocidade_ = velocidadeDeCruzeiroDe(energia_.motor);
    turbo_ = false;
    batida_ = 0.0f;
    casco_ = 1.0f;
    abafado_ = true;

    rochas_.gerar(semente, kQuantidadeRochas, kRaioCampo);
    rochas_.centralizar(pose_.posicao);

    // O ambiente comeca mudo e sobe em atualizar(): entrar na viagem nao
    // estoura um rugido do nada.
    ambiente_ = 0.0f;
    somAmbiente_ = ctx.audio.carregar("audio/espaco.wav");
    somImpacto_ = ctx.audio.carregar("audio/impacto.wav");
    somDestruicao_ = ctx.audio.carregar("audio/destruicao.wav");
    vozAmbiente_ = ctx.audio.tocarEmLoop(somAmbiente_, ambiente_);

    // A sirene toca a viagem inteira, calada: o que muda com o casco e o ganho
    // dela, nao a existencia da voz. Assim nao ha loop para nascer e morrer no
    // meio do voo -- e uma voz de loop nunca e engolida pelo limite de vozes,
    // entao a nave nunca ficaria sem o aviso justo quando ele importa.
    faseAlarme_ = 0.0f;
    intensidadeAlarme_ = 0.0f;
    alarme_ = 0.0f;
    somSirene_ = ctx.audio.carregar("audio/sirene.wav");
    vozSirene_ = ctx.audio.tocarEmLoop(somSirene_, 0.0f);

    // O sonar, ao contrario dos dois, nao e uma voz em loop com o ganho aberto
    // e fechado: cada bipe e uma reproducao propria, porque o que ele informa e
    // o **intervalo** entre eles. Um loop teria de ter a cadencia gravada, e ai
    // ela seria uma so.
    proximidade_ = 0.0f;
    relogioSonar_ = 0.0f;
    somSonar_ = ctx.audio.carregar("audio/sonar.wav");

    // A viagem comeca com o tanque cheio, e nao vazio: o turbo precisa ser
    // usado uma vez para que faltar depois signifique alguma coisa.
    reservaTurbo_ = 1.0f;
    superaquecido_ = false;
}

void Flight::encerrar(Context& ctx) {
    ctx.audio.parar(vozAmbiente_, kAmbienteSaida);
    vozAmbiente_ = 0;
    ctx.audio.parar(vozSirene_, kAmbienteSaida);
    vozSirene_ = 0;
}

int Flight::batidasSuportadasDe(int pontos) {
    return static_cast<int>(std::ceil(1.0f / danoPorBatidaDe(pontos)));
}

bool Flight::repartirEnergia(const Reparticao& nova) {
    const auto dentro = [](int pontos) {
        return pontos >= kPontoMinimo && pontos <= kPontoMaximo;
    };
    if (!dentro(nova.motor) || !dentro(nova.sensor) || !dentro(nova.casco)) {
        return false;
    }
    if (nova.motor + nova.sensor + nova.casco > kPontosDeEnergia) {
        return false;
    }
    energia_ = nova;
    return true;
}

float Flight::fatorTurbo() const {
    const float cruzeiro = velocidadeDeCruzeiroDe(energia_.motor);
    const float aberto = velocidadeDeTurboDe(energia_.motor);
    // Grampeado porque a velocidade persegue o alvo em rampa: repartir o motor
    // no meio de uma aceleracao move os dois extremos debaixo dela, e por um
    // instante a razao sairia do intervalo. Quem le isto e o ganho do ambiente
    // e o brilho do escapamento, e nenhum dos dois aceita um numero de fora.
    return std::clamp((velocidade_ - cruzeiro) / (aberto - cruzeiro), 0.0f, 1.0f);
}

Flight::Pose Flight::interpolada(float alpha) const {
    Pose pose;
    pose.posicao = lerp(poseAnterior_.posicao, pose_.posicao, alpha);
    pose.yaw = poseAnterior_.yaw + (pose_.yaw - poseAnterior_.yaw) * alpha;
    pose.pitch = poseAnterior_.pitch + (pose_.pitch - poseAnterior_.pitch) * alpha;
    pose.roll = poseAnterior_.roll + (pose_.roll - poseAnterior_.roll) * alpha;
    return pose;
}

void Flight::atualizar(Context& ctx, float dt, const Comando& comando) {
    poseAnterior_ = pose_;

    // Uma nave perdida nao obedece a ninguem: sem motor e sem atrito no vazio,
    // o que sobra dela segue na direcao e na velocidade em que estava. Por isso
    // so a integracao da posicao fica de fora deste "se" -- e e ela que mantem
    // a camera com o que seguir enquanto os destrocos se abrem.
    if (!destruida()) {
        // Curva inclinada: o rolamento acompanha a guinada, como em um caca. Sem
        // comando isso tudo tende a zero e a nave segue reto.
        pose_.roll = aproximar(pose_.roll, -comando.eixo.x * 0.85f, 6.0f, dt);
        pose_.yaw -= comando.eixo.x * kTaxaGiro * dt;
        pose_.pitch =
            std::clamp(pose_.pitch - comando.eixo.y * kTaxaPitch * dt, -kLimitePitch,
                       kLimitePitch);

        // A tecla nao basta: sem tanque o turbo simplesmente nao abre, e a nave
        // segue no cruzeiro. Quem apertou merece a resposta, mas ela e da
        // cabine (que compara a tecla com este `turbo_`), e nao daqui.
        // O superaquecimento, avaliado antes de a tecla valer: zerar o tanque
        // tranca o motor, e destrancar exige a primeira divisao do medidor de
        // volta. E o que impede o furo obvio do recurso -- com o tanque no
        // zero, soltar e apertar devolvia turbo a cada quadro, e a nave ficava
        // rapida em picotes sem que ninguem tivesse de escolher a hora.
        //
        // A leitura usa a reserva do passo anterior, entao a trava so entra no
        // passo seguinte ao que zerou o tanque. Um sexagesimo de segundo de
        // turbo a mais, e a alternativa seria repetir esta decisao no meio do
        // consumo, abaixo.
        if (reservaTurbo_ <= 0.0f) {
            superaquecido_ = true;
        } else if (reservaTurbo_ >= kReligarTurbo) {
            superaquecido_ = false;
        }
        turbo_ = comando.turbo && !superaquecido_;
        // O tanque esvazia enquanto o motor esta aberto e se refaz sozinho
        // enquanto esta fechado -- tres vezes mais devagar do que gasta. A
        // recarga nao tem carencia propria: a trava acima ja e a espera, e uma
        // segunda em cima dela so faria o mesmo servico duas vezes.
        if (turbo_) {
            reservaTurbo_ = std::max(0.0f, reservaTurbo_ - kConsumoTurbo * dt);
        } else {
            reservaTurbo_ = std::min(1.0f, reservaTurbo_ + kRecargaTurbo * dt);
        }
        // O alvo vem da reparticao, e a rampa que ja estava aqui pelo turbo
        // cuida da mudanca de graca: mexer no motor no conves nao da um
        // solavanco na nave, ela so passa a puxar para outra velocidade.
        velocidade_ = aproximar(velocidade_,
                                turbo_ ? velocidadeDeTurboDe(energia_.motor)
                                       : velocidadeDeCruzeiroDe(energia_.motor),
                                3.0f, dt);
    }
    pose_.posicao += rotacaoDe(pose_).frente() * velocidade_ * dt;

    // Campo infinito: as rochas sao reposicionadas em torno da nave, que e
    // quem colide com elas -- enquanto ha nave para colidir.
    rochas_.atualizar(dt);
    rochas_.centralizar(pose_.posicao);
    // Invencivel (F4, so na build de depuracao) e simplesmente nao checar a
    // colisao: sem batida nao ha baque, som nem estrago, e a nave atravessa o
    // campo. Fora dessa build a condicao e uma constante falsa e some na
    // compilacao.
    if (!destruida() && !invencivel()) {
        checarColisao(ctx);
    }
    batida_ = std::max(0.0f, batida_ - kDecaimentoBatida * dt);

    // O ambiente entra do zero, acompanha o esforco do motor e cai quando o
    // casco fica no caminho. A rampa e aqui para a passagem entre o convés e a
    // cabine ser um swell, e nao um degrau -- e e a mesma rampa que, com a nave
    // perdida, leva o alvo a zero: a sequencia de destruicao termina em silencio,
    // que e de onde a tela de fim comeca.
    const float alvo =
        destruida() ? 0.0f
                    : (kAmbienteCruzeiro + (kAmbienteTurbo - kAmbienteCruzeiro) * fatorTurbo()) *
                          (abafado_ ? kAbafamento : 1.0f);
    ambiente_ = aproximar(ambiente_, alvo, kTaxaAmbiente, dt);
    ctx.audio.ajustarGanho(vozAmbiente_, ambiente_);

    // A janela do sensor abre e fecha pela mesma rampa, pelo mesmo motivo: a
    // nevoa e a primeira coisa que o jogador ve da reparticao, e ela recuando
    // devagar e o que mostra a energia chegando ao sistema. As duas pontas
    // andam juntas, senao a faixa de desvanecimento se deformaria no caminho.
    alcance_ = aproximar(alcance_, alcanceDoSensorDe(energia_.sensor), kTaxaSensor, dt);
    alcanceVisivel_ =
        aproximar(alcanceVisivel_, alcanceVisivelDe(energia_.sensor), kTaxaSensor, dt);

    // O alarme do casco critico. A fase anda sempre, com casco inteiro ou nao;
    // quem entra e sai e a intensidade, pela mesma rampa do ambiente -- o
    // alarme abre e fecha em vez de estalar no meio de um ciclo, e uma nave
    // perdida se cala junto com o resto.
    //
    // Sai daqui um numero so, `alarme_`, e ele serve a duas coisas: o ganho da
    // sirene e a luz vermelha que a InteriorScene acende sobre o conves. E um
    // so de proposito. A ida e a volta da sirene poderiam estar gravadas no
    // WAV, mas entao ela andaria pelo relogio do dispositivo de audio enquanto
    // a luz andaria pelo passo fixo, e em poucos minutos de viagem o pisca-pisca
    // estaria fora do compasso do som.
    faseAlarme_ = std::fmod(faseAlarme_ + kTau * kFreqAlarme * dt, kTau);
    intensidadeAlarme_ = aproximar(intensidadeAlarme_, critico() ? 1.0f : 0.0f, kTaxaAlarme, dt);
    alarme_ = intensidadeAlarme_ * (0.5f - 0.5f * std::cos(faseAlarme_));
    ctx.audio.ajustarGanho(vozSirene_, alarme_ * kGanhoSirene);

    // O sonar de rota, que e o unico aviso que atravessa o casco: no conves o
    // jogador nao ve o campo, e o que ele ouve e a proxima rocha se aproximando.
    // Na cabine ele **se cala**, e por isso mesmo -- ali a rocha esta na tela, e
    // o sonar e o substituto da vista, nao o acompanhamento dela. E o mesmo
    // `abafado_` que decide as duas metades da troca: o casco que abafa o lado
    // de fora e o que da ao console o que dizer, e onde se enxerga ele silencia.
    //
    // O que se mede e a pedra **no caminho reto a frente**, e nao a mais
    // proxima em qualquer direcao: a que passa de lado esta perto sem ser
    // ameaca. O corredor varrido e, portanto, a previsao do piloto automatico
    // -- que e exatamente quem esta pilotando enquanto se anda la dentro.
    //
    // O alcance e o do sensor **como ele esta agora**, a mesma rampa da nevoa,
    // e nao um numero proprio. Assim o console avisa sobre exatamente aquilo
    // que a janela da cabine mostra, e um ponto de energia no sensor compra
    // aviso nos dois lugares de uma vez: no minimo o primeiro bipe soa a 12
    // unidades, um piscar antes da batida; no maximo, a 95.
    const float livre = rochas_.distanciaNaRota(pose_.posicao, rotacaoDe(pose_).frente(),
                                                kRaioNave + kCorredorSonar, alcance_);
    proximidade_ = destruida() ? 0.0f : 1.0f - livre / alcance_;
    // O relogio **satura** no intervalo mais longo em vez de zerar com a rota
    // livre: assim a rocha que entra no alcance dispara o bipe no mesmo passo,
    // e nao ate 0,85 s depois -- justo o atraso que um aviso nao pode ter.
    // O relogio anda mesmo com o sonar calado na cabine, pelo mesmo motivo pelo
    // qual ele satura: quem volta ao conves com uma rocha ja no alcance ouve o
    // bipe no primeiro passo, e nao ate 0,85 s depois de atravessar a porta.
    relogioSonar_ = std::min(relogioSonar_ + dt, kIntervaloSonarLonge);
    if (abafado_ && proximidade_ > 0.0f) {
        const float intervalo =
            kIntervaloSonarLonge *
            std::pow(kIntervaloSonarPerto / kIntervaloSonarLonge, proximidade_);
        if (relogioSonar_ >= intervalo) {
            relogioSonar_ = 0.0f;
            ctx.audio.tocar(somSonar_, kGanhoSonarLonge + (kGanhoSonarPerto - kGanhoSonarLonge) *
                                                              proximidade_);
        }
    }
}

void Flight::reparar(float quanto) {
    // As duas recusas ficam aqui, e nao em quem chama: um casco zerado e o fim
    // da nave, nao um estado do qual a bancada traga de volta, e o teto do
    // reparo de campo e propriedade da nave. Assim uma segunda tela que repare
    // (ou o proximo ponto de solda) nao precisa lembrar das regras.
    if (!reparavel()) {
        return;
    }
    casco_ = std::min(kCascoReparado, casco_ + quanto);
}

void Flight::checarColisao(Context& ctx) {
    const int atingida = rochas_.colisao(pose_.posicao, kRaioNave);
    if (atingida < 0) {
        return;
    }

    // A rocha vai para outro canto do cubo em vez de sumir: o campo mantem a
    // mesma densidade sem alocar nada.
    rochas_.reposicionar(atingida, pose_.posicao);
    velocidade_ = kVelocidadeAposBatida;
    // O tranco e proporcional ao estrago, e nao fixo em 1: e assim que a
    // blindagem se faz sentir sem que ninguem precise ler um numero. Casco
    // reforcado sacode menos a camera e da um baque mais surdo; casco
    // sacrificado quase derruba a tela.
    batida_ = danoPorBatida() / danoPorBatidaDe(kPontoNeutro);
    // O estrago nao se desfaz: o casco so cai, e para no zero.
    casco_ = std::max(0.0f, casco_ - danoPorBatida());

    // A ultima rocha soa diferente das outras, e o estouro sai daqui e nao da
    // cena: o casco pode ceder com o jogador no conves, no painel ou na cabine,
    // e o som do fim da nave nao pode depender de quem estava desenhando.
    if (destruida()) {
        ctx.audio.tocar(somDestruicao_);
    } else {
        ctx.audio.tocar(somImpacto_, abafado_ ? 0.55f : 1.0f);
    }
}

}  // namespace jogo
