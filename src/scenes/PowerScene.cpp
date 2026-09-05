#include "scenes/PowerScene.hpp"

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
constexpr SDL_Color kCorTexto{198, 226, 245, 255};
constexpr SDL_Color kCorApagada{92, 110, 130, 255};
constexpr SDL_Color kCorTrilho{14, 26, 40, 255};
constexpr SDL_Color kCorPerda{235, 110, 105, 255};
constexpr SDL_Color kCorAtencao{245, 190, 110, 255};
constexpr SDL_Color kCorFolga{120, 220, 150, 255};

/// Uma cor por sistema, e as mesmas em toda a tela: o ponto que sai do motor e
/// entra no sensor muda de cor no caminho, e e assim que se ve que ele andou.
constexpr SDL_Color kCorMotor{245, 175, 95, 255};
constexpr SDL_Color kCorSensor{125, 210, 255, 255};
constexpr SDL_Color kCorCasco{130, 220, 155, 255};
constexpr SDL_Color kCorReserva{150, 168, 190, 255};

/// Onde cada coluna da fileira comeca, medido da borda esquerda do vidro.
constexpr float kColunaSeta = 12.0f;
constexpr float kColunaNome = 26.0f;
constexpr float kColunaPontos = 108.0f;
constexpr float kColunaEfeito = 196.0f;

}  // namespace

PowerScene::PowerScene(Flight& voo) : voo_(voo) {}

void PowerScene::aoEntrar(Context& ctx) {
    somMover_ = ctx.audio.carregar("audio/blip.wav");
    somPonto_ = ctx.audio.carregar("audio/confirm.wav");
    somRecusa_ = ctx.audio.carregar("audio/falha.wav");
    somVoltar_ = ctx.audio.carregar("audio/back.wav");
}

int PowerScene::pontosDe(Sistema sistema) const {
    const Flight::Reparticao& energia = voo_.energia();
    switch (sistema) {
        case kMotor:
            return energia.motor;
        case kSensor:
            return energia.sensor;
        case kCasco:
            return energia.casco;
        default:
            break;
    }
    return 0;
}

bool PowerScene::definirPontos(Sistema sistema, int pontos) {
    Flight::Reparticao nova = voo_.energia();
    switch (sistema) {
        case kMotor:
            nova.motor = pontos;
            break;
        case kSensor:
            nova.sensor = pontos;
            break;
        case kCasco:
            nova.casco = pontos;
            break;
        default:
            return false;
    }
    return voo_.repartirEnergia(nova);
}

void PowerScene::recolher(Context& ctx) {
    // Quem recusa e a nave: aqui nao se sabe onde fica o minimo, so se pede.
    if (!definirPontos(escolhido_, pontosDe(escolhido_) - 1)) {
        recusar(ctx);
        return;
    }
    realceTroca_ = 1.0f;
    ctx.audio.tocar(somPonto_);
}

void PowerScene::distribuir(Context& ctx) {
    // Sem reserva a soma estoura e a nave recusa, do mesmo jeito que recusaria
    // um quinto ponto num sistema so: um caminho de recusa, nao dois.
    if (!definirPontos(escolhido_, pontosDe(escolhido_) + 1)) {
        recusar(ctx);
        return;
    }
    realceTroca_ = 1.0f;
    ctx.audio.tocar(somPonto_);
}

void PowerScene::recusar(Context& ctx) {
    realceRecusa_ = 1.0f;
    ctx.audio.tocar(somRecusa_);
}

void PowerScene::atualizar(Context& ctx, float dt) {
    // O passo do voo vem antes de qualquer saida, como no conves e na bancada:
    // fechar o painel nao pode custar um passo a viagem.
    acompanhar(ctx, dt);

    // O casco cedeu com o piloto repartindo energia. Nao ha o que repartir numa
    // nave que acabou de se abrir: esta cena se troca pela vista externa --
    // trocar, e nao empilhar, deixa a pilha igual a dos outros caminhos ate o
    // fim (MenuScene > InteriorScene > FlightScene).
    if (voo_.destruida() && !entregouADestruicao_) {
        entregouADestruicao_ = true;
        ctx.cenas.substituir(std::make_unique<FlightScene>(voo_));
        return;
    }
    if (entregouADestruicao_) {
        return;
    }

    if (ctx.input.acaoPressionada(Acao::Voltar) || ctx.input.acaoPressionada(Acao::Pausar) ||
        ctx.input.acaoPressionada(Acao::Interagir)) {
        ctx.audio.tocar(somVoltar_);
        ctx.cenas.desempilhar();
        return;
    }

    realceTroca_ = std::max(0.0f, realceTroca_ - kDecaimentoRealce * dt);
    realceRecusa_ = std::max(0.0f, realceRecusa_ - kDecaimentoRealce * dt);

    // Cima e baixo escolhem o sistema, na direcao em que as fileiras estao
    // empilhadas, e dao a volta: com tres fileiras, chegar na ultima por cima e
    // mais curto do que atravessar as tres.
    if (ctx.input.acaoPressionada(Acao::Cima)) {
        escolhido_ = static_cast<Sistema>((escolhido_ + kSistemas - 1) % kSistemas);
        ctx.audio.tocar(somMover_);
    } else if (ctx.input.acaoPressionada(Acao::Baixo)) {
        escolhido_ = static_cast<Sistema>((escolhido_ + 1) % kSistemas);
        ctx.audio.tocar(somMover_);
    }

    // As laterais movem o ponto entre o sistema e a reserva, na direcao em que a
    // fileira de pontos cresce: direita enche, esquerda esvazia. Nao existe um
    // atalho que tire de um sistema e ponha no outro -- a energia passa pela
    // reserva a vista, e e isso que mostra que ela e conservada em vez de
    // aparecer.
    if (ctx.input.acaoPressionada(Acao::Direita)) {
        distribuir(ctx);
    } else if (ctx.input.acaoPressionada(Acao::Esquerda)) {
        recolher(ctx);
    }
}

void PowerScene::acompanhar(Context& ctx, float dt) {
    tempo_ += dt;

    // Repartir energia nao interrompe a viagem: mexe-se no painel de uma nave
    // que segue voando sozinha -- inclusive contra a proxima pedra.
    voo_.atualizar(ctx, dt, Flight::Comando{});
}

void PowerScene::desenhar(Context& ctx, float /*alpha*/) {
    const float larguraTela = static_cast<float>(App::kLarguraLogica);
    const float alturaTela = static_cast<float>(App::kAlturaLogica);
    const float meio = larguraTela * 0.5f;

    draw::retanguloTela(ctx.renderer, SDL_FRect{0.0f, 0.0f, larguraTela, alturaTela}, kCorVeu);

    // Como nos outros paineis, as medidas verticais saem da altura da linha da
    // fonte: a altura do vidro e a soma do que vai dentro, na mesma ordem do
    // desenho abaixo (se mexer em um, mexa no outro), mais as duas margens.
    const float linha = ctx.fonte.alturaLinha(1.0f);
    const float margem = 12.0f;
    const float fileira = kAlturaPonto + 10.0f;
    const float alturaConteudo = linha * 2.2f + fileira * 3.0f + 8.0f + fileira + 8.0f +
                                 ctx.fonte.alturaLinha(2.0f) + 4.0f + linha;
    const SDL_FRect vidro{meio - 190.0f, 62.0f, 380.0f, alturaConteudo + margem * 2.0f};
    draw::retanguloTela(ctx.renderer, vidro, kCorVidro);
    draw::retanguloTela(ctx.renderer, vidro, draw::misturar(kCorBorda, kCorPerda, realceRecusa_),
                        false);

    float y = vidro.y + margem;
    ctx.fonte.desenharCentralizado(ctx.renderer, "REPARTICAO DE ENERGIA", meio, y, kCorTitulo,
                                   1.0f);
    y += linha * 2.2f;

    // Uma fileira: a seta de quem esta escolhido, o nome do sistema, os quatro
    // lugares de ponto e o que a energia comprou ali. `pontos` negativo e a
    // reserva, que nao e um sistema e nao se escolhe.
    const auto desenharFileira = [&](int sistema, const char* nome, SDL_Color cor, int pontos,
                                     int lugares, const char* efeito, SDL_Color corEfeito,
                                     float topo) {
        const bool ativo = sistema >= 0 && static_cast<Sistema>(sistema) == escolhido_;
        const float yTexto = topo + (fileira - linha) * 0.5f;

        if (ativo) {
            ctx.fonte.desenhar(ctx.renderer, ">", vidro.x + kColunaSeta, yTexto, cor, 1.0f);
        }
        ctx.fonte.desenhar(ctx.renderer, nome, vidro.x + kColunaNome, yTexto,
                           ativo ? cor : kCorTexto, 1.0f);

        const float yPonto = topo + (fileira - kAlturaPonto) * 0.5f;
        for (int i = 0; i < lugares; ++i) {
            const SDL_FRect celula{vidro.x + kColunaPontos +
                                       static_cast<float>(i) * (kLarguraPonto + kVaoPonto),
                                   yPonto, kLarguraPonto, kAlturaPonto};
            draw::retanguloTela(ctx.renderer, celula, kCorTrilho);
            if (i < pontos) {
                // O ponto que acabou de se mexer clareia junto com a fileira
                // dele: sem isso, quatro celulas iguais nao dizem qual mudou.
                draw::retanguloTela(
                    ctx.renderer, celula,
                    ativo ? draw::misturar(cor, SDL_Color{255, 255, 255, 255}, realceTroca_ * 0.6f)
                          : cor);
            }
            draw::retanguloTela(ctx.renderer, celula, ativo ? cor : kCorApagada, false);
        }

        if (efeito != nullptr) {
            ctx.fonte.desenhar(ctx.renderer, efeito, vidro.x + kColunaEfeito, yTexto, corEfeito,
                               1.0f);
        }
    };

    const Flight::Reparticao& energia = voo_.energia();
    char efeito[48];

    std::snprintf(efeito, sizeof(efeito), "CRUZEIRO %.0f u/s",
                  static_cast<double>(Flight::velocidadeDeCruzeiroDe(energia.motor)));
    desenharFileira(kMotor, "MOTOR", kCorMotor, energia.motor, Flight::kPontoMaximo, efeito,
                    kCorApagada, y);
    y += fileira;

    std::snprintf(efeito, sizeof(efeito), "ALCANCE %.0f u",
                  static_cast<double>(Flight::alcanceDoSensorDe(energia.sensor)));
    desenharFileira(kSensor, "SENSOR", kCorSensor, energia.sensor, Flight::kPontoMaximo, efeito,
                    kCorApagada, y);
    y += fileira;

    std::snprintf(efeito, sizeof(efeito), "AGUENTA %d ROCHAS",
                  Flight::batidasSuportadasDe(energia.casco));
    desenharFileira(kCasco, "CASCO", kCorCasco, energia.casco, Flight::kPontoMaximo, efeito,
                    kCorApagada, y);
    y += fileira + 8.0f;

    // A reserva. Com o minimo de um ponto em cada sistema nunca sobram mais de
    // tres, e por isso ela tem tres lugares e nao seis.
    const int reserva = voo_.reserva();
    desenharFileira(-1, "RESERVA", kCorReserva, reserva, Flight::kPontosDeEnergia - kSistemas,
                    reserva > 0 ? "ENERGIA PARADA" : nullptr, kCorAtencao, y);
    y += fileira + 8.0f;

    // O numero que resume a troca. Fica grande e sozinho porque e o unico que o
    // jogador precisa levar daqui: as tres fileiras acima sao a conta, e esta
    // linha e o resultado dela.
    const float aviso = Flight::segundosDeAvisoDe(energia);
    const int centesimos = static_cast<int>(aviso * 100.0f + 0.5f);
    SDL_Color corAviso = kCorTexto;
    if (aviso >= 0.85f) {
        corAviso = kCorFolga;
    } else if (aviso < 0.42f) {
        corAviso = kCorPerda;
    } else if (aviso < 0.60f) {
        corAviso = kCorAtencao;
    }
    char leitura[32];
    std::snprintf(leitura, sizeof(leitura), "AVISO %d,%02d s", centesimos / 100, centesimos % 100);
    ctx.fonte.desenharCentralizado(ctx.renderer, leitura, meio, y, corAviso, 2.0f);
    y += ctx.fonte.alturaLinha(2.0f) + 4.0f;

    ctx.fonte.desenharCentralizado(ctx.renderer, "o tempo entre a rocha surgir e o casco", meio, y,
                                   kCorApagada, 1.0f);

    // A dica nomeia os dois eixos separados: qual seta faz o que nao e obvio
    // antes de experimentar, e experimentar aqui custa uma travessia do conves.
    const char* dica = ctx.input.temGamepad()
                           ? "direcional: sistema e energia   B ou X: voltar ao conves"
                           : "cima/baixo: sistema   esq/dir: energia   Esc: voltar";
    ctx.fonte.desenharCentralizado(ctx.renderer, dica, meio, vidro.y + vidro.h + 12.0f, kCorTexto,
                                   1.0f);
}

}  // namespace jogo
