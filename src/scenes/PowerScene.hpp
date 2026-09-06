#pragma once

#include <SDL3/SDL.h>

#include "audio/Audio.hpp"
#include "scene/Scene.hpp"
#include "sim/Flight.hpp"

namespace jogo {

/// O repartidor de energia do conves: motor, turbo, sensor e casco saem da
/// mesma fonte, e o painel e onde se decide de onde tirar.
///
/// Aqui nao se melhora nada. Cada ponto sai de um sistema e entra em outro
/// passando pela reserva, entao toda vantagem e comprada com o risco de outro
/// lugar: motor alto chega mais longe por segundo e deixa menos tempo entre ver
/// a rocha e bater nela; turbo alto guarda mais folego para atravessar um
/// trecho ruim e nao adianta nada no resto da viagem; sensor alto enxerga cedo e
/// anda devagar; casco alto aguenta mais rocha e e lento e cego. As regras de
/// quanto cada ponto compra
/// -- e a recusa de uma reparticao invalida -- ficam no Flight, porque sao
/// regras da nave e nao desta tela.
///
/// O que impede isto de virar um menu que se otimiza e a geometria do conves: a
/// energia so se reparte aqui, e chegar ate aqui custa largar os controles e
/// atravessar a nave, que segue voando sozinha o tempo todo. Como a bancada e o
/// diagnostico, esta cena bloqueia o update de quem esta embaixo e passa a ser
/// quem chama Flight::atualizar -- um passo por passo fixo.
class PowerScene : public Scene {
public:
    explicit PowerScene(Flight& voo);

    void aoEntrar(Context& ctx) override;
    void atualizar(Context& ctx, float dt) override;
    /// O passo desta cena sem nenhuma decisao: o voo em piloto automatico e o
    /// relogio da tela. Os pontos **nao** se movem aqui -- eles nao perseguem a
    /// simulacao, sao a escolha do jogador, e um painel aberto por cima nao
    /// reparte energia por conta propria.
    void acompanhar(Context& ctx, float dt) override;
    void desenhar(Context& ctx, float alpha) override;

    /// O conves fica visivel atras do painel, como no diagnostico e na bancada.
    bool bloqueiaRender() const override { return false; }

private:
    /// Os quatro sistemas, na ordem em que aparecem na tela e em que cima e baixo
    /// os percorrem.
    /// O turbo vem logo abaixo do motor porque e a segunda metade da mesma
    /// decisao: um da a velocidade que se mantem, o outro a que se puxa por
    /// alguns segundos.
    enum Sistema { kMotor = 0, kTurbo = 1, kSensor = 2, kCasco = 3, kSistemas = 4 };

    /// A celula de um ponto na fileira, e o vao entre duas.
    static constexpr float kLarguraPonto = 16.0f;
    static constexpr float kAlturaPonto = 12.0f;
    static constexpr float kVaoPonto = 4.0f;

    /// Com que rapidez o realce de um movimento (ou de uma recusa) se apaga.
    static constexpr float kDecaimentoRealce = 3.4f;

    /// Os pontos do sistema escolhido, para ler e escrever sem um switch em
    /// cada uso.
    int pontosDe(Sistema sistema) const;
    /// Tenta gravar uma reparticao com `pontos` no sistema escolhido; devolve
    /// se a nave aceitou. A soma e conferida no Flight.
    bool definirPontos(Sistema sistema, int pontos);
    /// Tira um ponto do sistema escolhido e devolve a reserva.
    void recolher(Context& ctx);
    /// Puxa um ponto da reserva para o sistema escolhido.
    void distribuir(Context& ctx);
    /// O movimento recusado: nada muda, e o painel pisca em vermelho.
    void recusar(Context& ctx);

    Flight& voo_;

    /// Qual sistema as setas de cima e de baixo estao mexendo.
    Sistema escolhido_{kMotor};

    float realceTroca_{0.0f};
    float realceRecusa_{0.0f};
    float tempo_{0.0f};

    /// Mesma trava da InteriorScene, pelo mesmo motivo: dois passos fixos podem
    /// cair no mesmo quadro e pediriam duas vezes a vista externa.
    bool entregouADestruicao_{false};

    Audio::SomId somMover_{0};
    Audio::SomId somPonto_{0};
    Audio::SomId somRecusa_{0};
    Audio::SomId somVoltar_{0};
};

}  // namespace jogo
