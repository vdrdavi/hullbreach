#pragma once

#include <SDL3/SDL.h>

namespace jogo {

struct Context;

/// O bloco "COMO JOGAR", desenhado igual no menu e na pausa.
///
/// Os controles saiam antes em tarjas no rodape de cada tela do jogo -- uma no
/// conves, outra na cabine, outra em cada painel. Cada uma so sabia da propria
/// tela, entao ninguem via a lista inteira em lugar nenhum, e as tarjas ficavam
/// na frente justo enquanto se joga, que e quando nao se le nada.
///
/// Aqui elas viram uma lista so, em dois lugares onde o jogo esta parado e ha
/// tempo de ler: o menu, antes de comecar, e a pausa, no meio da partida.
///
/// Por isso este arquivo existe em vez de o texto estar nas duas cenas: duas
/// copias de uma lista de controles divergem no primeiro atalho que mudar, e a
/// que diverge e sempre a que o jogador esta lendo.
namespace instrucoes {

/// Altura que o bloco ocupa, para quem precisa fechar o layout antes de
/// desenhar.
float altura(const Context& ctx);

/// Desenha o bloco centrado em `x`, do topo `y` para baixo. As linhas mudam com
/// o gamepad ligado, como as tarjas mudavam.
void desenhar(Context& ctx, float x, float y);

}  // namespace instrucoes
}  // namespace jogo
