#include "scenes/PauseScene.hpp"

#include "core/App.hpp"
#include "gfx/BitmapFont.hpp"
#include "gfx/Draw.hpp"
#include "input/Input.hpp"
#include "scenes/Instrucoes.hpp"

namespace jogo {

void PauseScene::aoEntrar(Context& ctx) {
    somVoltar_ = ctx.audio.carregar("audio/back.wav");
    // Pausado, o mundo para inteiro: o ambiente do lado de fora e a sirene do
    // casco sao loops que tocariam a pausa toda. Suspender o dispositivo, e nao
    // parar as vozes, e o que faz o som voltar de onde estava quando a partida
    // voltar -- inclusive a fase do loop, que nao pode ter emenda.
    ctx.audio.suspender();
}

void PauseScene::aoSair(Context& ctx) {
    // O `back` pedido em atualizar() foi enfileirado com o dispositivo parado; a
    // pilha so aplica a saida no fim do quadro, entao ele toca aqui, ja no ar.
    ctx.audio.retomar();
}

void PauseScene::atualizar(Context& ctx, float dt) {
    tempo_ += dt;

    if (ctx.input.acaoPressionada(Acao::Pausar) || ctx.input.acaoPressionada(Acao::Confirmar)) {
        ctx.audio.tocar(somVoltar_);
        ctx.cenas.desempilhar();
        return;
    }
    if (ctx.input.teclaPressionada(SDL_SCANCODE_M)) {
        ctx.audio.tocar(somVoltar_);
        // Sai da pausa e da partida, voltando ao menu que ficou na base da pilha.
        ctx.cenas.desempilhar();
        ctx.cenas.desempilhar();
    }
}

void PauseScene::desenhar(Context& ctx, float /*alpha*/) {
    const SDL_FRect tela{0.0f, 0.0f, static_cast<float>(App::kLarguraLogica),
                         static_cast<float>(App::kAlturaLogica)};
    draw::retanguloTela(ctx.renderer, tela, SDL_Color{8, 10, 16, 170});

    const float meio = static_cast<float>(App::kLarguraLogica) * 0.5f;
    const float linha = ctx.fonte.alturaLinha(1.0f);

    // Titulo, as duas saidas da pausa e a lista de controles, centrados como um
    // conjunto: as medidas saem da fonte e da altura do proprio bloco, e nao de
    // constantes, senao acrescentar uma linha a lista empurra o resto para fora
    // da tela sem ninguem perceber (ja aconteceu no diagnostico).
    const float alturaTitulo = ctx.fonte.alturaLinha(3.0f);
    const float alturaBloco =
        alturaTitulo + linha * 1.4f + linha * 2.0f + linha * 2.2f + instrucoes::altura(ctx);
    float y = (static_cast<float>(App::kAlturaLogica) - alturaBloco) * 0.5f;

    ctx.fonte.desenharCentralizado(ctx.renderer, "PAUSADO", meio, y,
                                   SDL_Color{255, 255, 255, 255}, 3.0f);
    y += alturaTitulo + linha * 1.4f;
    ctx.fonte.desenharCentralizado(ctx.renderer, "Esc ou Start: continuar", meio, y,
                                   SDL_Color{186, 196, 210, 255}, 1.0f);
    y += linha;
    ctx.fonte.desenharCentralizado(ctx.renderer, "M: voltar ao menu", meio, y,
                                   SDL_Color{186, 196, 210, 255}, 1.0f);
    y += linha * 2.2f;

    // O outro lugar em que o jogo esta parado. Aqui a lista serve a quem ja
    // esta voando e esqueceu um atalho, e por isso ela e a mesma do menu, e nao
    // uma versao resumida: quem pausa para conferir precisa da lista inteira.
    instrucoes::desenhar(ctx, meio, y);
}

}  // namespace jogo
