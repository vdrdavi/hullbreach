#include "gfx3d/AsteroidField.hpp"

#include <algorithm>
#include <cmath>

namespace jogo {
namespace {

/// Variedades de rocha sorteadas na geracao: poucas malhas, muitas pedras.
constexpr int kVariedades = 5;

/// Distancia minima entre a nave e uma rocha no sorteio inicial: sem isso o
/// campo poderia nascer com uma pedra dentro da cabine. Vale so para gerar() --
/// a rocha que reaparece com a viagem em curso entra pela borda, adiante.
constexpr float kDistanciaSegura = 55.0f;

constexpr float kRaioMinimo = 2.2f;
constexpr float kRaioMaximo = 7.5f;

// O monolito: a rocha que nao se desvia no ultimo segundo.
//
// A pedra comum, com ate 7,5 de raio, cabe no campo de manobra -- da para
// deixar para depois e ainda escapar. Estas nao: com 14 a 26 de raio, contra os
// 2 da nave, quando ela ja esta perto nao ha guinada que resolva. Ou se decide
// cedo ou se bate, e e isso que as torna outra coisa e nao so uma pedra maior.
//
// Sao **uma em cem** de proposito. A conta que manda e a secao de choque, que
// cresce com o quadrado do raio: uma de raio 20 tem trinta vezes a area de
// travessia de uma de raio 4,85, entao uma frequencia que parece modesta na
// contagem vira comum na rota. A 1% e uma a cada trinta e poucos segundos de
// voo reto -- rara o bastante para ser um acontecimento, comum o bastante para
// o sensor alto valer a pena.
constexpr float kRaioGrandeMinimo = 12.0f;
constexpr float kRaioGrandeMaximo = 20.0f;
constexpr float kChanceGrande = 0.01f;

/// Quantas malhas de monolito, e onde elas comecam em `malhas_`. Ficam no mesmo
/// vetor das outras: o indice em Asteroide::malha ja diz qual e qual, e nao ha
/// segunda lista para manter em dia.
constexpr int kVariedadesGrandes = 5;

/// Quanto a rocha mais rapida deriva. **Este numero e limitado pela colisao, e
/// nao pelo gosto.**
///
/// A colisao e um teste de esferas na posicao do passo, sem varredura: se o
/// deslocamento **relativo** entre nave e rocha em um passo passar da menor
/// sobreposicao possivel, elas se atravessam sem nunca se tocar. A menor e 4,2
/// (raio 2,0 da nave mais 2,2 da menor rocha), o que a 60 Hz da 252 u/s de
/// velocidade relativa. O turbo no talo ja usa 240 desses (veja
/// Flight::kTurboPorPonto), e sobram 12 -- este 6 e metade da folga, contra uma
/// rocha vindo de frente no pior caso.
///
/// Passar disto exige trocar a colisao por uma varredura de segmento, e nao
/// apenas subir o numero.
constexpr float kDerivaMaxima = 6.0f;  // u/s

// A densidade do campo. Estes numeros sao o **ritmo da viagem**: eles decidem
// quanto tempo se passa no vazio e quanto no aperto.
//
// As escalas sao dadas em unidades de mundo, e sao grandes de proposito. A
// atividade de cada rocha e decidida quando ela entra no cubo, a `kRaioCampo`
// da nave, e nao e revista depois -- reavaliar a cada quadro faria a pedra na
// fronteira piscar, e custaria uma avaliacao de ruido por rocha por passo. Com
// feicoes de centenas de unidades, a decisao tomada na borda continua valendo
// quando a nave chega la, que e o que faz a estrutura parecer coerente no
// espaco em vez de sorteada.
constexpr float kEscalaBolsao = 640.0f;
constexpr float kEscalaVeio = 260.0f;
/// Quanto o veio se alonga. O estico e no eixo X do mundo, e nao em Z, porque a
/// viagem comeca apontada para -Z: um veio esticado ao longo da rota seria um
/// corredor em que se entra e se fica: atravessado, ele vira uma faixa que se
/// cruza, com comeco e fim.
constexpr float kEsticoVeio = 4.5f;
/// O piso da densidade e o que impede o vazio de virar vazio de verdade. Uma
/// regiao sem pedra nenhuma nao e alivio, e tedio -- e pior, tiraria do sonar e
/// do sensor qualquer coisa a fazer por dezenas de segundos.
constexpr float kDensidadeMinima = 0.2f;

/// Hash inteiro determinista. E o xorshift do Aleatorio, mas alimentado pela
/// **coordenada** em vez de por um estado que anda: o ruido tem de dar sempre o
/// mesmo valor no mesmo ponto do espaco, viagem afora.
float hashRuido(int x, int y, int z, Uint32 semente) {
    Uint32 h = semente;
    h ^= static_cast<Uint32>(x) * 0x8DA6B343u;
    h ^= static_cast<Uint32>(y) * 0xD8163841u;
    h ^= static_cast<Uint32>(z) * 0xCB1AB31Fu;
    h ^= h << 13;
    h ^= h >> 17;
    h ^= h << 5;
    return static_cast<float>(h >> 8) / 16777216.0f;
}

/// Ruido de valor com interpolacao suave: sorteia um numero em cada canto da
/// grade inteira e mistura os oito. O amaciamento (3t^2 - 2t^3) e o que tira a
/// quina da interpolacao linear -- sem ele as feicoes teriam bordas retas e o
/// campo pareceria quadriculado.
float ruidoDeValor(Vec3 p, Uint32 semente) {
    const float bx = std::floor(p.x);
    const float by = std::floor(p.y);
    const float bz = std::floor(p.z);
    const int ix = static_cast<int>(bx);
    const int iy = static_cast<int>(by);
    const int iz = static_cast<int>(bz);

    const auto amaciar = [](float t) { return t * t * (3.0f - 2.0f * t); };
    const float tx = amaciar(p.x - bx);
    const float ty = amaciar(p.y - by);
    const float tz = amaciar(p.z - bz);

    const auto mistura = [](float a, float b, float t) { return a + (b - a) * t; };
    float face[2];
    for (int dz = 0; dz < 2; ++dz) {
        const float baixo = mistura(hashRuido(ix, iy, iz + dz, semente),
                                    hashRuido(ix + 1, iy, iz + dz, semente), tx);
        const float alto = mistura(hashRuido(ix, iy + 1, iz + dz, semente),
                                   hashRuido(ix + 1, iy + 1, iz + dz, semente), tx);
        face[dz] = mistura(baixo, alto, ty);
    }
    return mistura(face[0], face[1], tz);
}

/// Mantem uma coordenada relativa dentro de [-raio, raio).
float envolver(float distancia, float raio) {
    const float largura = raio * 2.0f;
    float valor = std::fmod(distancia + raio, largura);
    if (valor < 0.0f) {
        valor += largura;
    }
    return valor - raio;
}

}  // namespace

Vec3 AsteroidField::sortear(Vec3 centro, float minimo) {
    // Sorteio com recusa. O caso mais apertado e `minimo == raio_`, que so
    // aceita os cantos do cubo -- e ainda assim aceita 48% dos sorteios, porque
    // e o que sobra do cubo fora da esfera inscrita nele (1 - pi/6). O teto de
    // tentativas so existe para nao depender de sorte; a saida dele tambem
    // respeita o limite, por estar a `raio_` exatos.
    for (int tentativa = 0; tentativa < 16; ++tentativa) {
        const Vec3 deslocamento{rng_.entre(-raio_, raio_), rng_.entre(-raio_, raio_),
                                rng_.entre(-raio_, raio_)};
        if (comprimento(deslocamento) >= minimo) {
            return centro + deslocamento;
        }
    }
    return centro + Vec3{0.0f, 0.0f, -raio_};
}

float AsteroidField::densidadeEm(Vec3 p) const {
    // Duas camadas, e cada uma faz uma coisa que a outra nao faz.
    //
    // O **bolsao** e ruido cru numa escala grande: regioes redondas, umas
    // cheias e outras vazias, com transicao lenta. Sozinho ele daria um campo
    // que so engrossa e afina, sem forma.
    const float bolsao = ruidoDeValor(p * (1.0f / kEscalaBolsao), semente_ ^ 0x51u);

    // O **veio** e o mesmo ruido dobrado no meio (1 - |2n-1|): o valor sobe ate
    // 1 onde o ruido passa por 0,5 e cai para 0 nos dois extremos, o que
    // transforma superficies em cristas. Esticado em X, essas cristas viram
    // faixas alongadas -- correntes de pedra que se atravessa de lado.
    const Vec3 q{p.x / (kEscalaVeio * kEsticoVeio), p.y / kEscalaVeio, p.z / kEscalaVeio};
    const float veio = 1.0f - std::fabs(ruidoDeValor(q, semente_ ^ 0xA7u) * 2.0f - 1.0f);

    // Meio a meio: o bolsao da o fundo lento e o veio, a estrutura por cima. Um
    // so dos dois daria ou um campo amorfo ou um campo listrado.
    const float campo = 0.5f * bolsao + 0.5f * veio;

    // A curva em S puxa o resultado para os extremos. Sem ela a soma de dois
    // ruidos se aperta em torno da media -- o campo passava quase todo o tempo
    // "mais ou menos cheio", que e o mesmo defeito que a densidade uniforme
    // tinha, so que com um numero diferente. Com ela ha vazio de verdade e
    // aperto de verdade, e menos tempo no meio termo. A media quase nao muda,
    // que e o que mantem valido o resto do ajuste da dificuldade.
    const float contraste = campo * campo * (3.0f - 2.0f * campo);
    return kDensidadeMinima + (1.0f - kDensidadeMinima) * contraste;
}

void AsteroidField::ativarPelaDensidade(Asteroide& rocha) {
    const bool antes = rocha.ativa;
    rocha.ativa = rng_.unitario() < densidadeEm(rocha.posicao);
    ativas_ += (rocha.ativa ? 1 : 0) - (antes ? 1 : 0);
}

void AsteroidField::sortearTamanho(Asteroide& rocha) {
    // Tamanho e malha saem juntos, e num lugar so: eles se escolhem um ao outro
    // -- monolito tem malha de monolito -- e os tres caminhos que reciclam uma
    // rocha (o sorteio inicial, o wrap e a pedra atingida) precisam concordar.
    if (rng_.unitario() < kChanceGrande) {
        rocha.raio = rng_.entre(kRaioGrandeMinimo, kRaioGrandeMaximo);
        rocha.malha = static_cast<std::size_t>(kVariedades) +
                      static_cast<std::size_t>(rng_.proximo() %
                                               static_cast<Uint32>(kVariedadesGrandes));
    } else {
        rocha.raio = rng_.entre(kRaioMinimo, kRaioMaximo);
        rocha.malha = static_cast<std::size_t>(rng_.proximo() % kVariedades);
    }
}

Vec3 AsteroidField::sortearDeriva() {
    // Direcao isotropica por sorteio com recusa dentro da esfera: sortear os
    // tres eixos e usar direto daria mais rochas indo para os cantos do cubo
    // que para o meio das faces.
    Vec3 direcao{};
    float tamanho2 = 0.0f;
    for (int tentativa = 0; tentativa < 8; ++tentativa) {
        direcao = Vec3{rng_.entre(-1.0f, 1.0f), rng_.entre(-1.0f, 1.0f), rng_.entre(-1.0f, 1.0f)};
        tamanho2 = dot(direcao, direcao);
        if (tamanho2 > 0.05f && tamanho2 <= 1.0f) {
            break;
        }
    }
    if (tamanho2 <= 0.05f || tamanho2 > 1.0f) {
        return Vec3{};
    }

    // A magnitude e o sorteio **ao quadrado**, e nao ele mesmo, para o campo nao
    // virar um enxame: com todas as pedras na mesma velocidade ele deixaria de
    // se ler como campo. Medido, a curva reparte as rochas em cena em cerca de
    // 30% praticamente paradas, 36% derivando devagar e 34% cruzando de fato --
    // dois tercos que se leem como obstaculo e um terco que se le como
    // movimento.
    const float u = rng_.unitario();
    return direcao * (kDerivaMaxima * u * u / std::sqrt(tamanho2));
}

void AsteroidField::gerar(Uint32 semente, int quantidade, float raio) {
    raio_ = raio;
    semente_ = semente;
    rng_ = Aleatorio(semente);

    malhas_.clear();
    malhas_.reserve(static_cast<std::size_t>(kVariedades + kVariedadesGrandes));
    for (int i = 0; i < kVariedades; ++i) {
        malhas_.push_back(criarAsteroideLowPoly(rng_.proximo()));
    }
    for (int i = 0; i < kVariedadesGrandes; ++i) {
        malhas_.push_back(criarMonolitoLowPoly(rng_.proximo()));
    }

    asteroides_.clear();
    asteroides_.reserve(static_cast<std::size_t>(quantidade));
    for (int i = 0; i < quantidade; ++i) {
        Asteroide rocha;
        rocha.posicao = sortear(Vec3{}, kDistanciaSegura);
        sortearTamanho(rocha);
        rocha.yaw = rng_.entre(0.0f, 6.2831853f);
        rocha.pitch = rng_.entre(0.0f, 6.2831853f);
        rocha.giroYaw = rng_.entre(-0.5f, 0.5f);
        rocha.giroPitch = rng_.entre(-0.5f, 0.5f);
        rocha.velocidade = sortearDeriva();
        rocha.ativa = false;
        asteroides_.push_back(rocha);
    }
    // A atividade so depois de todas nascerem, para `ativas_` contar do zero e
    // nao herdar o que sobrou de uma viagem anterior.
    ativas_ = 0;
    for (Asteroide& rocha : asteroides_) {
        ativarPelaDensidade(rocha);
    }
}

void AsteroidField::atualizar(float dt) {
    for (Asteroide& rocha : asteroides_) {
        rocha.yaw += rocha.giroYaw * dt;
        rocha.pitch += rocha.giroPitch * dt;
        // A deriva vale para todas, inclusive as inativas: elas voltam a ser
        // vistas quando o wrap as levar a um bolsao, e uma pedra que tivesse
        // ficado parada esse tempo todo reapareceria fora de lugar.
        rocha.posicao += rocha.velocidade * dt;
    }
}

void AsteroidField::centralizar(Vec3 posicao) {
    for (Asteroide& rocha : asteroides_) {
        const Vec3 relativo = rocha.posicao - posicao;
        Vec3 envolvido{envolver(relativo.x, raio_), envolver(relativo.y, raio_),
                       envolver(relativo.z, raio_)};

        // Wrap puro deixaria o campo periodico: voando reto, as mesmas rochas
        // voltariam na mesma formacao a cada travessia do cubo (com o Starfield
        // isso passa batido, mas rocha tem forma e a repeticao aparece). Quem
        // atravessa a borda volta sorteada nos eixos que nao viraram -- e
        // sempre a uma aresta inteira de distancia. E por isso que o raio do
        // cubo tem que ficar alem do alcance nitido do melhor sensor: a troca
        // acontece dentro da nevoa, longe dos olhos, e nao na cara do jogador.
        //
        // O teste e por magnitude: quem virou andou uma aresta inteira, e o
        // resto e ruido de arredondamento (envolver() soma e subtrai `raio`,
        // entao nem sempre devolve o mesmo float que entrou). Por igualdade,
        // quase toda rocha seria sorteada de novo a cada quadro.
        const bool virouX = std::fabs(envolvido.x - relativo.x) > raio_;
        const bool virouY = std::fabs(envolvido.y - relativo.y) > raio_;
        const bool virouZ = std::fabs(envolvido.z - relativo.z) > raio_;
        if (virouX || virouY || virouZ) {
            if (!virouX) {
                envolvido.x = rng_.entre(-raio_, raio_);
            }
            if (!virouY) {
                envolvido.y = rng_.entre(-raio_, raio_);
            }
            if (!virouZ) {
                envolvido.z = rng_.entre(-raio_, raio_);
            }
            sortearTamanho(rocha);
            rocha.giroYaw = rng_.entre(-0.5f, 0.5f);
            rocha.giroPitch = rng_.entre(-0.5f, 0.5f);
            rocha.velocidade = sortearDeriva();
            rocha.posicao = posicao + envolvido;
            // Atravessar a borda e o momento em que a rocha pergunta se ha campo
            // onde ela reapareceu. E o unico momento: dai em diante ela carrega a
            // resposta ate a proxima travessia, e e isso que faz o bolsao ter
            // borda em vez de cintilar rocha a rocha.
            ativarPelaDensidade(rocha);
            continue;
        }

        rocha.posicao = posicao + envolvido;
    }
}

void AsteroidField::submeter(Renderer3D& cena) const {
    const Vec3 olho = cena.camera().posicao;
    const Vec3 frente = -cena.camera().orientacao.colunas[2];
    const float fim = cena.nevoaFim();
    for (const Asteroide& rocha : asteroides_) {
        if (!rocha.ativa) {
            continue;
        }
        // O corte e por **profundidade de camera**, a mesma grandeza da nevoa, e
        // isso importa mais do que parece. Aqui ja se descartou por distancia
        // radial, o que so vale enquanto o corte for mais largo que a nevoa: uma
        // rocha longe do eixo pode estar a 300 da nave e a 120 de profundidade,
        // e o corte radial a tirava de cena enquanto a nevoa ainda a mostraria.
        // No quadro em que ela cruzasse o corte, apareceria com cor do nada --
        // era o "asteroide nascendo" que se via na borda do campo.
        //
        // Casando as duas grandezas, o corte vira um subconjunto do que a nevoa
        // apaga: nada e descartado antes de estar invisivel, com qualquer
        // alcance de sensor. O cubo ainda tem canto, e a rocha do vertice segue
        // sendo descartada -- so que por estar atras da nevoa, e nao por um raio
        // que nao sabia dela.
        const Vec3 desvio = rocha.posicao - olho;
        const float profundidade = dot(desvio, frente);
        if (profundidade - rocha.raio > fim || profundidade + rocha.raio < 0.0f) {
            continue;
        }
        cena.submeter(malhas_[rocha.malha], rocha.posicao,
                      Mat3::deEuler(rocha.yaw, rocha.pitch, 0.0f), rocha.raio);
    }
}

int AsteroidField::colisao(Vec3 posicao, float raio) const {
    for (std::size_t i = 0; i < asteroides_.size(); ++i) {
        const Asteroide& rocha = asteroides_[i];
        if (!rocha.ativa) {
            continue;
        }
        // O colisor e o raio de desenho vezes o da malha (veja Mesh). Para a
        // rocha comum o fator e 1 e nada muda; o monolito e que tem forma
        // demais para a esfera circunscrita representar.
        const float alcance = rocha.raio * malhas_[rocha.malha].raioColisao + raio;
        const Vec3 delta = rocha.posicao - posicao;
        if (dot(delta, delta) < alcance * alcance) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

float AsteroidField::distanciaNaRota(Vec3 posicao, Vec3 frente, float corredor,
                                     float alcance) const {
    float maisProxima = alcance;
    for (const Asteroide& rocha : asteroides_) {
        if (!rocha.ativa) {
            continue;
        }
        const Vec3 delta = rocha.posicao - posicao;
        const float profundidade = dot(delta, frente);
        // Ate a **superficie**, e nao ate o centro: o que interessa e quando a
        // pedra encosta. Uma rocha de raio 7 a 20 de profundidade esta a 13 do
        // casco, e e esse o numero que o aviso tem de dizer.
        const float distancia = profundidade - rocha.raio;
        if (profundidade <= 0.0f || distancia >= maisProxima) {
            continue;
        }
        // O afastamento do eixo por Pitagoras sobre o mesmo delta, comparado em
        // quadrados para nao tirar raiz. Este teste vem **depois** do corte por
        // distancia de proposito: ele custa mais, e a esmagadora maioria das
        // milhares de rochas do cubo ja saiu no anterior.
        const float lateral2 = std::max(0.0f, dot(delta, delta) - profundidade * profundidade);
        const float largura = rocha.raio + corredor;
        if (lateral2 > largura * largura) {
            continue;
        }
        maisProxima = std::max(0.0f, distancia);
    }
    return maisProxima;
}

void AsteroidField::reposicionar(int indice, Vec3 referencia) {
    if (indice < 0 || indice >= quantidade()) {
        return;
    }
    Asteroide& rocha = asteroides_[static_cast<std::size_t>(indice)];
    // Uma aresta inteira de distancia, que e de onde vem toda rocha nova: quem
    // atravessa a borda do cubo reaparece a raio_ dali, e a substituta de uma
    // pedra atingida precisa entrar pelo mesmo lugar. Sorteada mais perto, ela
    // se materializava pronta na frente do jogador logo depois da batida --
    // justo quando ele esta olhando.
    rocha.posicao = sortear(referencia, raio_);
    sortearTamanho(rocha);
    rocha.velocidade = sortearDeriva();
    ativarPelaDensidade(rocha);
}

}  // namespace jogo
