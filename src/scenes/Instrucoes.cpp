#include "scenes/Instrucoes.hpp"

#include <algorithm>

#include "core/Context.hpp"
#include "gfx/BitmapFont.hpp"
#include "input/Input.hpp"

namespace jogo {
namespace instrucoes {
namespace {

constexpr SDL_Color kCorTitulo{240, 216, 120, 255};
constexpr SDL_Color kCorLugar{110, 120, 138, 255};
constexpr SDL_Color kCorTexto{186, 196, 210, 255};

/// Uma linha da lista: **onde** se esta e o que se pode fazer ali.
///
/// A lista e organizada por lugar da nave, e nao por tecla, porque e assim que
/// a duvida aparece -- ninguem pergunta "o que o E faz", pergunta "estou no
/// conves, e agora". A ultima linha e a excecao, e nomeia a tecla, porque ela e
/// a que faz coisas diferentes em cada lugar.
struct Linha {
    const char* lugar;
    const char* teclado;
    const char* gamepad;
};

constexpr Linha kLinhas[] = {
    {"CONVES", "WASD andar   E usar o movel a frente",
     "analogico andar   X usar o movel a frente"},
    {"PAINEL", "Q diagnostico da nave   R repartir energia",
     "Y diagnostico da nave   RB repartir energia"},
    {"CABINE", "WASD pilotar   Espaco turbo", "analogico pilotar   A turbo"},
    {"BANCADA", "Espaco solda com o ponteiro dentro da zona",
     "A solda com o ponteiro dentro da zona"},
    {"VOLTAR", "Esc fecha a tela; no conves, pausa a partida",
     "B fecha a tela; Start pausa a partida"},
};

constexpr float kEspacoTitulo = 1.8f;  // alturas de linha, do titulo ao corpo

const char* corpoDe(const Context& ctx, const Linha& linha) {
    return ctx.input.temGamepad() ? linha.gamepad : linha.teclado;
}

/// Onde a coluna dos controles comeca, medida do maior nome de lugar. Sai da
/// fonte e nao de uma constante: trocar o atlas muda a metrica da celula, e o
/// bloco continua alinhado (veja BitmapFont).
float recuo(const Context& ctx) {
    float maior = 0.0f;
    for (const Linha& linha : kLinhas) {
        maior = std::max(maior, ctx.fonte.medir(linha.lugar, 1.0f).x);
    }
    return maior + ctx.fonte.medir("  ", 1.0f).x;
}

}  // namespace

float altura(const Context& ctx) {
    const float linha = ctx.fonte.alturaLinha(1.0f);
    return linha * kEspacoTitulo + linha * static_cast<float>(std::size(kLinhas));
}

void desenhar(Context& ctx, float x, float y) {
    const float alturaLinha = ctx.fonte.alturaLinha(1.0f);
    const float coluna = recuo(ctx);

    // A largura do bloco e a da linha mais larga: o conjunto e que se centra em
    // `x`, e nao cada linha, senao a coluna dos lugares serrilharia.
    float largura = 0.0f;
    for (const Linha& linha : kLinhas) {
        largura = std::max(largura, coluna + ctx.fonte.medir(corpoDe(ctx, linha), 1.0f).x);
    }
    const float esquerda = x - largura * 0.5f;

    ctx.fonte.desenharCentralizado(ctx.renderer, "COMO JOGAR", x, y, kCorTitulo, 1.0f);
    y += alturaLinha * kEspacoTitulo;

    for (const Linha& linha : kLinhas) {
        ctx.fonte.desenhar(ctx.renderer, linha.lugar, esquerda, y, kCorLugar, 1.0f);
        ctx.fonte.desenhar(ctx.renderer, corpoDe(ctx, linha), esquerda + coluna, y, kCorTexto,
                           1.0f);
        y += alturaLinha;
    }
}

}  // namespace instrucoes
}  // namespace jogo
