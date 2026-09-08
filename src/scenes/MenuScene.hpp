#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "audio/Audio.hpp"
#include "scene/Scene.hpp"

namespace jogo {

/// Menu inicial: navegacao por teclado/gamepad, som nas transicoes e entrada
/// para a partida. Tambem e onde ficam as preferencias -- volume e tela cheia
/// --, que o App grava em disco e reencontra na proxima execucao.
class MenuScene : public Scene {
public:
    void aoEntrar(Context& ctx) override;
    void atualizar(Context& ctx, float dt) override;
    void desenhar(Context& ctx, float alpha) override;

private:
    enum class Opcao { Jogar, Volume, TelaCheia, Sair, Contagem };

    /// Passo do volume por toque; 20 toques atravessam a faixa inteira.
    static constexpr float kPassoVolume = 0.05f;

    /// O rotulo mostra o valor da preferencia, entao nao pode ser constante.
    static std::string rotulo(const Context& ctx, Opcao opcao);
    /// Esquerda/direita sobre a opcao selecionada. Devolve true se mudou algo.
    static bool ajustar(Context& ctx, Opcao opcao, int passo);

    /// Desenha o logotipo HULLBREACH no topo: fulgor, sombra dura, "HULL" no aco
    /// e "BREACH" no ambar, com a fenda faiscando entre as silabas.
    void desenharTitulo(Context& ctx, float meio);

    /// Gradiente vertical mais o campo de estrelas com deriva e cintilar.
    void desenharFundo(Context& ctx);

    /// Espalha `estrelas_` a partir de uma semente fixa: sempre o mesmo ceu.
    void gerarCampoEstelar();

    /// Uma estrela do fundo. Posicao em coordenadas logicas; `parallax` faz a
    /// camada proxima derivar mais que a distante.
    struct Estrela {
        float x{0.0f};
        float y{0.0f};
        float tamanho{1.0f};
        float brilho{1.0f};
        float fase{0.0f};
        float velCintilar{1.0f};
        float parallax{1.0f};
        SDL_Color cor{255, 255, 255, 255};
    };

    int selecao_{0};
    float tempo_{0.0f};
    std::vector<Estrela> estrelas_;
    Audio::SomId somMover_{0};
    Audio::SomId somConfirmar_{0};
};

}  // namespace jogo
