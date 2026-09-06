#pragma once

#include <SDL3/SDL.h>

#include "audio/Audio.hpp"
#include "scene/Scene.hpp"
#include "sim/Flight.hpp"

namespace jogo {

/// A outra opcao do painel: o diagnostico da nave, aberto com Q no console e
/// fechado com Esc. Mostra os dois recursos da viagem -- a integridade do casco,
/// que cai a cada rocha, e o tanque de turbo, que so sobe raspando nelas.
///
/// O medidor de turbo mora aqui, e longe da cabine, e isso e regra do recurso:
/// saber quanto resta custa largar os controles e atravessar a nave, o mesmo
/// pedagio que o casco sempre cobrou.
///
/// Como a FlightScene, esta cena nao e dona do voo -- guarda uma referencia
/// para o Flight da InteriorScene, que sempre sobrevive a ela (a pilha so
/// desempilha do topo). E, como ela, bloqueia o update da cena de baixo e passa
/// a ser quem chama Flight::atualizar: a nave continua voando em piloto
/// automatico enquanto o painel esta aberto, ainda um passo por passo fixo, e
/// uma batida faz o mostrador cair na frente de quem esta lendo.
class StatusScene : public Scene {
public:
    explicit StatusScene(Flight& voo);

    void aoEntrar(Context& ctx) override;
    void atualizar(Context& ctx, float dt) override;
    /// O passo desta cena sem nenhuma decisao: o voo em piloto automatico e o
    /// ponteiro perseguindo o casco. E o que ela faz por si mesma a cada passo,
    /// e o que continua fazendo quando um painel a congela por cima.
    void acompanhar(Context& ctx, float dt) override;
    void desenhar(Context& ctx, float alpha) override;

    /// O conves fica visivel atras do painel, como na pausa.
    bool bloqueiaRender() const override { return false; }

private:
    static constexpr float kAlturaBarra = 16.0f;
    /// A barra do turbo e mais baixa que a do casco de proposito: as duas sao
    /// medidores da mesma nave, mas perder o casco acaba a viagem e ficar sem
    /// turbo so a deixa lenta. A hierarquia da tela tem de dizer isso.
    static constexpr float kAlturaBarraTurbo = 10.0f;
    /// Com que taxa o ponteiro persegue o casco: lento o bastante para a queda
    /// ser vista, rapido o bastante para nao atrasar a leitura.
    static constexpr float kTaxaPonteiro = 5.0f;  // 1/s

    /// A nave diagnosticada; vive na cena de baixo.
    Flight& voo_;

    /// Mesma trava da InteriorScene: dois passos no mesmo quadro nao podem
    /// pedir duas vezes a vista externa.
    bool entregouADestruicao_{false};

    /// O que o mostrador exibe, perseguindo voo_.casco(). A diferenca entre os
    /// dois e o pedaco que acabou de ser arrancado, e e desenhada em vermelho.
    float ponteiro_{1.0f};
    float tempo_{0.0f};

    Audio::SomId somVoltar_{0};
};

}  // namespace jogo
