#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include "core/Aleatorio.hpp"
#include "gfx3d/Mesh.hpp"
#include "gfx3d/Renderer3D.hpp"

namespace jogo {

/// Campo de asteroides com o mesmo truque do Starfield: as rochas vivem em um
/// cubo que envolve a nave por wrap, o que da um campo infinito com memoria
/// constante. Para a colisao cada rocha e apenas uma esfera -- as malhas sao
/// normalizadas com raio 1, entao a escala com que sao desenhadas ja e o raio.
///
/// **A densidade nao e uniforme.** Um ruido sobre a posicao-mundo desenha
/// bolsoes e veios, e cada rocha e ativada ou nao conforme a densidade do ponto
/// em que ela entra no cubo. Voando reto atravessa-se vazio, aperto e vazio de
/// novo, em vez da mesma chuva constante de pedra do primeiro ao ultimo minuto
/// -- que era a coisa mais parada de uma viagem que nao para.
///
/// A rocha inativa continua alocada e continua acompanhando o wrap: ela nao e
/// desenhada, nao colide e nao aparece no sonar, e volta a existir quando o
/// wrap a levar para uma regiao cheia. E o mesmo compromisso do resto da
/// classe -- memoria constante, sem alocar nem liberar nada em voo.
class AsteroidField {
public:
    struct Asteroide {
        Vec3 posicao{};
        float raio{1.0f};
        // Tombo constante, guardado como angulos: recompor a matriz a cada
        // quadro nao acumula erro, ao contrario de ir multiplicando rotacoes.
        float yaw{0.0f};
        float pitch{0.0f};
        float giroYaw{0.0f};
        float giroPitch{0.0f};
        /// Deriva propria: a rocha nao esta parada no vazio, ela vai para algum
        /// lugar. E o que separa desviar de um obstaculo de prever onde ele
        /// estara -- a pedra que cruza a rota se le de um jeito diferente da que
        /// espera parada nela.
        Vec3 velocidade{};
        std::size_t malha{0};
        /// Fora de um bolsao ou veio a rocha existe na memoria e em mais nada.
        bool ativa{true};
    };

    /// `raio` e a meia-aresta do cubo e tambem o alcance de desenho.
    void gerar(Uint32 semente, int quantidade, float raio);

    /// Faz as rochas tombarem e derivarem; passo fixo.
    void atualizar(float dt);

    /// Mantem o cubo centrado na nave; chamar sempre que ela se mover.
    void centralizar(Vec3 posicao);

    void submeter(Renderer3D& cena) const;

    /// Indice da primeira rocha que encosta na esfera dada, ou -1.
    int colisao(Vec3 posicao, float raio) const;

    /// Distancia ate a superficie da rocha mais proxima **na rota reta a
    /// frente**, medida pela profundidade ao longo de `frente`. So conta quem
    /// estiver dentro do tubo de raio `corredor` mais o raio da propria rocha:
    /// a pedra que passa de lado esta perto, mas nao esta no caminho, e um
    /// medidor que a contasse mediria a densidade do campo em vez do risco.
    ///
    /// Devolve `alcance` quando o tubo esta vazio -- o "nada a vista" e o
    /// proprio limite, e nao um sentinela que quem chama tenha de testar -- e
    /// nunca menos que zero.
    float distanciaNaRota(Vec3 posicao, Vec3 frente, float corredor, float alcance) const;

    /// Manda a rocha para outro canto do cubo, longe de `referencia`. E o que
    /// sobra de uma rocha atingida: o campo nunca perde nem ganha pedras.
    void reposicionar(int indice, Vec3 referencia);

    const Asteroide& asteroide(int indice) const {
        return asteroides_[static_cast<std::size_t>(indice)];
    }
    int quantidade() const { return static_cast<int>(asteroides_.size()); }
    /// Quantas estao de fato em cena. E a leitura que diz se a nave esta num
    /// bolsao ou num vazio, e a unica maneira de ver isso em numero.
    int ativas() const { return ativas_; }
    float raio() const { return raio_; }

    /// A densidade do campo em torno de um ponto, de kDensidadeMinima a 1. E a
    /// probabilidade de uma rocha que entre ali ficar ativa.
    float densidadeEm(Vec3 p) const;

private:
    /// Ponto no cubo em torno de `centro`, a pelo menos `minimo` dele.
    Vec3 sortear(Vec3 centro, float minimo);
    /// Sorteia o raio e a malha juntos: monolito tem malha de monolito.
    void sortearTamanho(Asteroide& rocha);
    /// Uma deriva nova: direcao isotropica e magnitude com vies para o lento.
    Vec3 sortearDeriva();

    /// Decide e aplica a atividade de uma rocha pela densidade onde ela esta.
    void ativarPelaDensidade(Asteroide& rocha);

    std::vector<Mesh> malhas_;
    std::vector<Asteroide> asteroides_;
    Aleatorio rng_{0x9E3779B9u};
    /// A semente da viagem, guardada: o ruido da densidade e funcao da posicao,
    /// e nao de uma sequencia, entao ele precisa dela toda vez que e avaliado.
    Uint32 semente_{0x9E3779B9u};
    int ativas_{0};
    float raio_{160.0f};
};

}  // namespace jogo
