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

void AsteroidField::gerar(Uint32 semente, int quantidade, float raio) {
    raio_ = raio;
    rng_ = Aleatorio(semente);

    malhas_.clear();
    malhas_.reserve(kVariedades);
    for (int i = 0; i < kVariedades; ++i) {
        malhas_.push_back(criarAsteroideLowPoly(rng_.proximo()));
    }

    asteroides_.clear();
    asteroides_.reserve(static_cast<std::size_t>(quantidade));
    for (int i = 0; i < quantidade; ++i) {
        Asteroide rocha;
        rocha.posicao = sortear(Vec3{}, kDistanciaSegura);
        rocha.raio = rng_.entre(kRaioMinimo, kRaioMaximo);
        rocha.yaw = rng_.entre(0.0f, 6.2831853f);
        rocha.pitch = rng_.entre(0.0f, 6.2831853f);
        rocha.giroYaw = rng_.entre(-0.5f, 0.5f);
        rocha.giroPitch = rng_.entre(-0.5f, 0.5f);
        rocha.malha = static_cast<std::size_t>(rng_.proximo() % kVariedades);
        asteroides_.push_back(rocha);
    }
}

void AsteroidField::atualizar(float dt) {
    for (Asteroide& rocha : asteroides_) {
        rocha.yaw += rocha.giroYaw * dt;
        rocha.pitch += rocha.giroPitch * dt;
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
            rocha.raio = rng_.entre(kRaioMinimo, kRaioMaximo);
            rocha.giroYaw = rng_.entre(-0.5f, 0.5f);
            rocha.giroPitch = rng_.entre(-0.5f, 0.5f);
            rocha.malha = static_cast<std::size_t>(rng_.proximo() % kVariedades);
        }

        rocha.posicao = posicao + envolvido;
    }
}

void AsteroidField::submeter(Renderer3D& cena) const {
    const Vec3 olho = cena.camera().posicao;
    const Vec3 frente = -cena.camera().orientacao.colunas[2];
    const float fim = cena.nevoaFim();
    for (const Asteroide& rocha : asteroides_) {
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
        const float alcance = rocha.raio + raio;
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

float AsteroidField::distanciaVarrida(Vec3 de, Vec3 para, float raio, float alcance) const {
    const Vec3 passo = para - de;
    const float passo2 = dot(passo, passo);
    const float comprimentoPasso = std::sqrt(passo2);
    float menor = alcance;
    for (const Asteroide& rocha : asteroides_) {
        const Vec3 w = rocha.posicao - de;
        // Corte barato antes de qualquer raiz: do inicio do segmento, a rocha
        // nao pode chegar a menos de |w| - |passo|. Se nem isso alcanca o que
        // ja se tem, ela nao interessa -- e e o caso de praticamente todas as
        // milhares de pedras do cubo, a cada passo.
        const float limite = rocha.raio + raio + menor + comprimentoPasso;
        if (dot(w, w) > limite * limite) {
            continue;
        }
        // Ponto do segmento mais proximo do centro da rocha. Com a nave parada
        // o segmento degenera em um ponto, e t = 0 e esse ponto.
        const float t = passo2 > 0.0f ? std::clamp(dot(w, passo) / passo2, 0.0f, 1.0f) : 0.0f;
        menor = std::min(menor, comprimento(w - passo * t) - rocha.raio - raio);
    }
    return menor;
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
    rocha.raio = rng_.entre(kRaioMinimo, kRaioMaximo);
    rocha.malha = static_cast<std::size_t>(rng_.proximo() % kVariedades);
}

}  // namespace jogo
