#include "scenes/StatusScene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "core/App.hpp"
#include "gfx/BitmapFont.hpp"
#include "gfx/Draw.hpp"
#include "input/Input.hpp"
#include "scenes/FlightScene.hpp"

namespace jogo {
namespace {

constexpr SDL_Color kCorVeu{6, 9, 16, 190};
constexpr SDL_Color kCorVidro{10, 18, 30, 235};
constexpr SDL_Color kCorBorda{60, 110, 150, 255};
constexpr SDL_Color kCorTitulo{150, 230, 255, 255};
constexpr SDL_Color kCorApagada{92, 110, 130, 255};
constexpr SDL_Color kCorTrilho{14, 26, 40, 255};
constexpr SDL_Color kCorPerda{235, 110, 105, 255};
/// O turbo tem cor propria, e fria: ele nao e uma faixa do casco vista de outro
/// jeito, e sim outro recurso, com outro relogio -- o casco so cai, o tanque
/// sobe e desce. Duas cores para nao se lerem como um medidor partido em dois.
constexpr SDL_Color kCorTurbo{130, 225, 255, 255};
/// O motor trancado esperando esfriar. E o mesmo ambar do casco AVARIADO, de
/// proposito: nos dois casos a nave nao esta quebrada, esta pior do que deveria
/// e volta ao normal -- e o painel nao pode ter dois vocabularios para isso.
constexpr SDL_Color kCorTurboQuente{245, 190, 110, 255};

/// Faixas do casco: a cor e a palavra saem da mesma fronteira, para o texto
/// nunca dizer "integro" sobre uma barra ja alaranjada. A fronteira do critico
/// e a do Flight, a mesma que liga a sirene e a luz de emergencia -- o
/// mostrador nao pode dizer AVARIADO com o alarme tocando.
struct Faixa {
    SDL_Color cor;
    const char* palavra;
};

Faixa faixaDo(float casco) {
    if (casco > 0.6f) {
        return Faixa{SDL_Color{120, 220, 150, 255}, "INTEGRO"};
    }
    if (casco > Flight::kCascoCritico) {
        return Faixa{SDL_Color{245, 190, 110, 255}, "AVARIADO"};
    }
    return Faixa{kCorPerda, "CRITICO"};
}

/// Suavizacao exponencial estavel em passo fixo.
float aproximar(float atual, float alvo, float taxa, float dt) {
    return atual + (alvo - atual) * (1.0f - std::exp(-taxa * dt));
}

}  // namespace

StatusScene::StatusScene(Flight& voo) : voo_(voo) {}

void StatusScene::aoEntrar(Context& ctx) {
    somVoltar_ = ctx.audio.carregar("audio/back.wav");
    // O mostrador abre no valor real: a viagem ja aconteceu, e uma varredura do
    // zero ate o casco atual seria enfeite fingindo ser medicao.
    ponteiro_ = voo_.casco();
}

void StatusScene::atualizar(Context& ctx, float dt) {
    // O passo do voo vem antes da saida, como no conves: fechar o painel nao
    // pode custar um passo a viagem. E o mesmo passo que esta cena da quando um
    // painel se abre por cima dela, entao vem de la inteiro.
    acompanhar(ctx, dt);

    // O mostrador chegou a zero: nao ha diagnostico a fazer em uma nave que
    // acabou de se romper. Esta cena se troca pela vista externa -- trocar, e
    // nao empilhar, deixa a pilha igualzinha a dos outros caminhos ate o fim
    // (MenuScene > InteriorScene > FlightScene).
    if (voo_.destruida() && !entregouADestruicao_) {
        entregouADestruicao_ = true;
        ctx.cenas.substituir(std::make_unique<FlightScene>(voo_));
        return;
    }
    if (entregouADestruicao_) {
        return;
    }

    if (ctx.input.acaoPressionada(Acao::Voltar) || ctx.input.acaoPressionada(Acao::Pausar) ||
        ctx.input.acaoPressionada(Acao::Diagnostico)) {
        ctx.audio.tocar(somVoltar_);
        ctx.cenas.desempilhar();
    }
}

void StatusScene::acompanhar(Context& ctx, float dt) {
    tempo_ += dt;

    // O diagnostico nao interrompe a viagem: le o casco de uma nave que segue
    // voando sozinha -- inclusive contra a proxima pedra.
    voo_.atualizar(ctx, dt, Flight::Comando{});

    ponteiro_ = aproximar(ponteiro_, voo_.casco(), kTaxaPonteiro, dt);
    // A perseguicao exponencial chega perto e nunca encosta; sem este encaixe
    // sobraria para sempre uma lasca de vermelho de menos de um pixel na barra.
    if (std::fabs(ponteiro_ - voo_.casco()) < 0.001f) {
        ponteiro_ = voo_.casco();
    }
}

void StatusScene::desenhar(Context& ctx, float /*alpha*/) {
    const float larguraTela = static_cast<float>(App::kLarguraLogica);
    const float alturaTela = static_cast<float>(App::kAlturaLogica);
    const float meio = larguraTela * 0.5f;

    draw::retanguloTela(ctx.renderer, SDL_FRect{0.0f, 0.0f, larguraTela, alturaTela}, kCorVeu);

    // A moldura do painel. As medidas verticais saem da altura da linha da
    // fonte, e nao de constantes: trocar a fonte muda a metrica da celula. A
    // altura e a soma do que vai dentro, na mesma ordem do desenho abaixo (se
    // mexer em um, mexa no outro), mais a margem de cima e a de baixo.
    const float linha = ctx.fonte.alturaLinha(1.0f);
    const float margem = 14.0f;
    const float alturaConteudo = linha * 2.0f + linha * 1.8f + kAlturaBarra + linha +
                                 ctx.fonte.alturaLinha(2.0f) + 4.0f + linha * 1.6f + linha +
                                 linha * 1.8f + kAlturaBarraTurbo + linha + linha * 1.4f;
    // A moldura se centra sozinha em vez de comecar num y fixo. Com um medidor
    // so ela cabia em qualquer lugar; com dois, o literal que havia aqui jogou
    // conteudo para fora da tela -- e jogaria de novo no proximo mostrador.
    const float alturaVidro = alturaConteudo + margem * 2.0f;
    const SDL_FRect vidro{meio - 190.0f, (alturaTela - alturaVidro) * 0.5f, 380.0f, alturaVidro};
    draw::retanguloTela(ctx.renderer, vidro, kCorVidro);
    draw::retanguloTela(ctx.renderer, vidro, kCorBorda, false);

    float y = vidro.y + margem;
    ctx.fonte.desenharCentralizado(ctx.renderer, "DIAGNOSTICO DA NAVE", meio, y, kCorTitulo, 1.0f);
    y += linha * 2.0f;

    // A barra: trilho, o casco que restou e -- entre ele e o ponteiro, que
    // ainda esta descendo -- o pedaco que a ultima rocha levou. A cor e a
    // palavra saem do ponteiro, e nao do casco, para nao contradizerem o numero
    // enquanto a queda esta sendo mostrada.
    const Faixa faixa = faixaDo(ponteiro_);
    const SDL_FRect trilho{vidro.x + 22.0f, y + linha * 1.8f, vidro.w - 44.0f, kAlturaBarra};
    // Os dois medidores se anunciam do mesmo jeito. Sem o rotulo, a barra de
    // cima seria "a barra" e a de baixo "a outra": o painel deixou de ser sobre
    // uma coisa so, e a tela tem de dizer isso antes de o jogador perguntar.
    ctx.fonte.desenhar(ctx.renderer, "CASCO", trilho.x, y, kCorApagada, 1.0f);
    y = trilho.y;
    draw::retanguloTela(ctx.renderer, trilho, kCorTrilho);
    draw::retanguloTela(
        ctx.renderer,
        SDL_FRect{trilho.x, trilho.y, trilho.w * std::max(voo_.casco(), ponteiro_), trilho.h},
        kCorPerda);
    draw::retanguloTela(ctx.renderer,
                        SDL_FRect{trilho.x, trilho.y, trilho.w * voo_.casco(), trilho.h},
                        faixa.cor);
    draw::retanguloTela(ctx.renderer, trilho, kCorBorda, false);
    y += trilho.h + linha;

    // O numero acompanha o ponteiro, nao o casco: e o mesmo movimento da barra
    // dito em digitos, e assim os dois nunca se contradizem no meio da queda.
    char leitura[32];
    std::snprintf(leitura, sizeof(leitura), "%d%%",
                  static_cast<int>(ponteiro_ * 100.0f + 0.5f));
    ctx.fonte.desenharCentralizado(ctx.renderer, leitura, meio, y, faixa.cor, 2.0f);
    y += ctx.fonte.alturaLinha(2.0f) + 4.0f;

    // Enquanto o baque decai, a palavra pisca: o mostrador reage a batida que
    // acabou de acontecer sem que a cena precise de um temporizador proprio.
    const bool piscando = voo_.batida() > 0.0f && std::sin(tempo_ * 26.0f) < 0.0f;
    ctx.fonte.desenharCentralizado(ctx.renderer, faixa.palavra, meio, y,
                                   piscando ? kCorApagada : faixa.cor, 1.0f);
    y += linha * 1.6f;

    // O pedaco que cada rocha leva deixou de ser fixo -- ele sai da blindagem, e
    // quem a reparte e o painel de energia. Dizer "um pedaco" aqui esconderia
    // justamente o que o jogador acabou de escolher. Quantas rochas ainda cabem
    // e conta dele: o mostrador da o custo e o que sobrou, nao a conclusao.
    char custo[64];
    std::snprintf(custo, sizeof(custo), "cada rocha custa %d%% do casco",
                  static_cast<int>(voo_.danoPorBatida() * 100.0f + 0.5f));
    ctx.fonte.desenharCentralizado(ctx.renderer, custo, meio, y, kCorApagada, 1.0f);
    y += linha * 1.8f;

    // O tanque de turbo. Ele mora aqui, e nao na cabine, e isso e a regra do
    // recurso e nao uma escolha de layout: saber quanto resta custa largar os
    // controles e atravessar a nave, exatamente o pedagio que o casco ja cobra.
    // Na cabine o piloto sabe **que** o motor superaqueceu (a HUD diz); quanto
    // falta para religar, so aqui.
    //
    // E o mostrador sobe na frente de quem esta lendo: a nave continua voando
    // com o painel aberto e, no piloto automatico, com o motor fechado -- entao
    // o tanque se recompoe justamente enquanto se olha para ele.
    const bool quente = voo_.superaquecido();
    const SDL_Color corTurbo = quente ? kCorTurboQuente : kCorTurbo;

    ctx.fonte.desenhar(ctx.renderer, "TURBO", trilho.x, y, kCorApagada, 1.0f);
    char tanque[32];
    std::snprintf(tanque, sizeof(tanque), "%.1f s",
                  static_cast<double>(voo_.reservaTurbo() * voo_.segundosDeTurbo()));
    const SDL_FPoint medidaTanque = ctx.fonte.medir(tanque, 1.0f);
    ctx.fonte.desenhar(ctx.renderer, tanque, trilho.x + trilho.w - medidaTanque.x, y, corTurbo,
                       1.0f);
    y += linha * 1.8f;

    const SDL_FRect trilhoTurbo{trilho.x, y, trilho.w, kAlturaBarraTurbo};
    draw::retanguloTela(ctx.renderer, trilhoTurbo, kCorTrilho);
    draw::retanguloTela(
        ctx.renderer,
        SDL_FRect{trilhoTurbo.x, trilhoTurbo.y, trilhoTurbo.w * voo_.reservaTurbo(),
                  trilhoTurbo.h},
        corTurbo);
    // Uma marca por segundo, para o medidor dizer em que unidade ele fala: o
    // numero ao lado esta em segundos, entao meia barra se le como "metade dos
    // segundos que ali dizem" sem ninguem precisar contar. Quantas divisoes ha e
    // o ponto de energia do turbo que decide -- de duas a nove --, e e por isso
    // que a barra **muda de escala** quando se reparte energia: ela nao mede
    // carga, mede tempo de motor aberto.
    const float segundos = voo_.segundosDeTurbo();
    for (int marca = 1; marca < static_cast<int>(segundos); ++marca) {
        const float fracao = static_cast<float>(marca) / segundos;
        // A primeira marca e o limiar do religamento, e com o motor quente ela
        // deixa de ser escala e vira **alvo**: acesa, ela mostra onde a barra
        // precisa chegar. E por isso que o limiar vale uma divisao inteira --
        // assim a regra cabe no desenho que a barra ja tinha.
        const bool limiar = quente && marca == 1;
        draw::retanguloTela(ctx.renderer,
                            SDL_FRect{trilhoTurbo.x + trilhoTurbo.w * fracao, trilhoTurbo.y,
                                      limiar ? 2.0f : 1.0f, trilhoTurbo.h},
                            limiar ? kCorTurboQuente : kCorVidro);
    }
    draw::retanguloTela(ctx.renderer, trilhoTurbo, kCorBorda, false);
    y += trilhoTurbo.h + linha;

    // Como o tanque volta, dito onde ele e lido. O numero sai da constante do
    // Flight, e nao de um literal: quem mexer no equilibrio nao pode deixar esta
    // linha mentindo. E curta porque a moldura tem 380 px logicos e a fonte, 8
    // por celula: a frase que ja esteve aqui tinha 47 caracteres e vazava pelos
    // dois lados.
    char recarga[72];
    if (quente) {
        std::snprintf(recarga, sizeof(recarga), "SUPERAQUECIDO: religa na primeira marca");
    } else {
        std::snprintf(recarga, sizeof(recarga), "recarrega sozinho: %.0f s do vazio ao cheio",
                      static_cast<double>(Flight::kSegundosParaEncher));
    }
    ctx.fonte.desenharCentralizado(ctx.renderer, recarga, meio, y,
                                   quente ? kCorTurboQuente : kCorApagada, 1.0f);

}

}  // namespace jogo
