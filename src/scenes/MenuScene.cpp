#include "scenes/MenuScene.hpp"

#include <algorithm>
#include <cmath>

#include "core/Aleatorio.hpp"
#include "core/App.hpp"
#include "gfx/BitmapFont.hpp"
#include "gfx/Draw.hpp"
#include "input/Input.hpp"
#include "scenes/InteriorScene.hpp"

namespace jogo {
namespace {

constexpr SDL_Color kCorTitulo{240, 216, 120, 255};
constexpr SDL_Color kCorItem{176, 186, 200, 255};
constexpr SDL_Color kCorItemAtivo{255, 255, 255, 255};
constexpr SDL_Color kCorRodape{110, 120, 138, 255};

// O logotipo: "HULL" no aco frio da nave intacta, "BREACH" no ambar do alarme,
// e entre as duas silabas a fenda que da nome ao jogo.
constexpr float kEscalaTitulo = 4.0f;
constexpr float kFolgaFenda = 6.0f;  // vao entre HULL e BREACH onde a fenda corre
constexpr SDL_Color kCorTituloHull{150, 170, 198, 255};
constexpr SDL_Color kCorTituloBreach{232, 96, 74, 255};
constexpr SDL_Color kCorFenda{255, 202, 128, 255};
constexpr SDL_Color kCorSombraTitulo{8, 8, 14, 255};

// Zigue-zague fixo da fenda, em pixels logicos de deslocamento horizontal; um
// valor por dente, de cima para baixo.
constexpr float kZigueFenda[] = {0.0f,  4.0f, -3.0f, 6.0f, -5.0f,
                                 2.0f, -4.0f,  5.0f, -2.0f, 3.0f};
constexpr int kDentesFenda = static_cast<int>(sizeof(kZigueFenda) / sizeof(kZigueFenda[0]));

// Campo de estrelas do fundo. Semente fixa: o ceu do menu e sempre o mesmo.
constexpr int kQuantidadeEstrelas = 220;
constexpr Uint32 kSementeCampo = 0x5D3A17F1u;
constexpr int kFaixasFundo = 24;  // faixas do gradiente vertical
constexpr float kDerivaEstelar = 5.0f;  // px/s da camada mais proxima
constexpr SDL_Color kCorFundoTopo{8, 10, 20, 255};
constexpr SDL_Color kCorFundoBase{3, 4, 9, 255};

bool telaCheia(const Context& ctx) {
    SDL_Window* janela = ctx.app.janela();
    return janela != nullptr && (SDL_GetWindowFlags(janela) & SDL_WINDOW_FULLSCREEN) != 0;
}

}  // namespace

std::string MenuScene::rotulo(const Context& ctx, Opcao opcao) {
    switch (opcao) {
        case Opcao::Jogar:
            return "Jogar";
        case Opcao::Volume:
            // Arredonda para o inteiro mais proximo: com o passo de 5% o texto
            // anda de 5 em 5 sem casa decimal aparecendo por erro de float.
            return "Volume: " + std::to_string(static_cast<int>(ctx.app.volume() * 100.0f +
                                                               0.5f)) +
                   "%";
        case Opcao::TelaCheia:
            return telaCheia(ctx) ? "Tela cheia: sim" : "Tela cheia: nao";
        case Opcao::Sair:
        case Opcao::Contagem:
            break;
    }
    return "Sair";
}

bool MenuScene::ajustar(Context& ctx, Opcao opcao, int passo) {
    switch (opcao) {
        case Opcao::Volume: {
            const float antes = ctx.app.volume();
            const float agora =
                std::clamp(antes + static_cast<float>(passo) * kPassoVolume, 0.0f, 1.0f);
            if (agora == antes) {
                return false;  // ja esta no fim da faixa: nada muda, nada soa
            }
            ctx.app.definirVolume(agora);
            return true;
        }
        case Opcao::TelaCheia:
            ctx.app.alternarTelaCheia();
            return true;
        case Opcao::Jogar:
        case Opcao::Sair:
        case Opcao::Contagem:
            return false;
    }
    return false;
}

void MenuScene::aoEntrar(Context& ctx) {
    somMover_ = ctx.audio.carregar("audio/blip.wav");
    somConfirmar_ = ctx.audio.carregar("audio/confirm.wav");
    gerarCampoEstelar();
}

void MenuScene::gerarCampoEstelar() {
    Aleatorio rng(kSementeCampo);
    estrelas_.clear();
    estrelas_.reserve(static_cast<std::size_t>(kQuantidadeEstrelas));
    for (int i = 0; i < kQuantidadeEstrelas; ++i) {
        Estrela e;
        e.x = rng.entre(0.0f, static_cast<float>(App::kLarguraLogica));
        e.y = rng.entre(0.0f, static_cast<float>(App::kAlturaLogica));
        // Profundidade sorteada ao quadrado: muitas estrelas fracas e distantes,
        // poucas grandes e proximas -- o mesmo truque da deriva das rochas, que
        // impede o fundo de virar um enxame homogeneo.
        const float perto = rng.unitario() * rng.unitario();
        e.parallax = 0.2f + perto * 0.85f;
        e.tamanho = 1.0f + perto * 1.7f;
        e.brilho = 0.28f + perto * 0.62f + rng.unitario() * 0.10f;
        e.fase = rng.entre(0.0f, 6.2831853f);
        e.velCintilar = rng.entre(0.4f, 2.4f);
        // Um fio de azul nas comuns, um fio de ambar nas raras.
        e.cor = draw::misturar(SDL_Color{198, 214, 255, 255}, SDL_Color{255, 238, 210, 255},
                               rng.unitario() * rng.unitario());
        estrelas_.push_back(e);
    }
}

void MenuScene::desenharFundo(Context& ctx) {
    const float largura = static_cast<float>(App::kLarguraLogica);
    const float altura = static_cast<float>(App::kAlturaLogica);

    // Gradiente vertical em poucas faixas: quase preto no pe, um respiro de azul
    // no topo, atras do logotipo.
    const float alturaFaixa = altura / static_cast<float>(kFaixasFundo);
    for (int i = 0; i < kFaixasFundo; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kFaixasFundo - 1);
        draw::retanguloTela(ctx.renderer,
                            SDL_FRect{0.0f, static_cast<float>(i) * alturaFaixa, largura,
                                      alturaFaixa + 1.0f},
                            draw::misturar(kCorFundoTopo, kCorFundoBase, t));
    }

    for (const Estrela& e : estrelas_) {
        // Deriva horizontal com wrap; a camada proxima anda mais que a distante.
        float x = std::fmod(e.x - tempo_ * kDerivaEstelar * e.parallax, largura);
        if (x < 0.0f) {
            x += largura;
        }
        const float cintila = 0.55f + 0.45f * std::sin(tempo_ * e.velCintilar + e.fase);
        const float b = std::clamp(e.brilho * cintila, 0.0f, 1.0f);
        const SDL_Color cor{e.cor.r, e.cor.g, e.cor.b, static_cast<Uint8>(b * 255.0f)};
        draw::retanguloTela(ctx.renderer, SDL_FRect{x, e.y, e.tamanho, e.tamanho}, cor);

        // As poucas grandes ganham um halo aditivo de leve.
        if (e.tamanho > 2.2f) {
            draw::brilhoAditivo(
                ctx.renderer, SDL_FPoint{x + e.tamanho * 0.5f, e.y + e.tamanho * 0.5f},
                e.tamanho * 2.6f,
                SDL_FColor{static_cast<float>(e.cor.r) / 255.0f,
                           static_cast<float>(e.cor.g) / 255.0f,
                           static_cast<float>(e.cor.b) / 255.0f, 0.08f + 0.06f * cintila});
        }
    }
}

void MenuScene::atualizar(Context& ctx, float dt) {
    tempo_ += dt;

    const int total = static_cast<int>(Opcao::Contagem);
    if (ctx.input.acaoPressionada(Acao::Cima)) {
        selecao_ = (selecao_ + total - 1) % total;
        ctx.audio.tocar(somMover_);
    }
    if (ctx.input.acaoPressionada(Acao::Baixo)) {
        selecao_ = (selecao_ + 1) % total;
        ctx.audio.tocar(somMover_);
    }

    // O blip toca depois da mudanca de volume, e por isso ja sai no volume novo:
    // o som e a propria previa do ajuste.
    const int passo = (ctx.input.acaoPressionada(Acao::Direita) ? 1 : 0) -
                      (ctx.input.acaoPressionada(Acao::Esquerda) ? 1 : 0);
    if (passo != 0 && ajustar(ctx, static_cast<Opcao>(selecao_), passo)) {
        ctx.audio.tocar(somMover_);
    }

    if (ctx.input.acaoPressionada(Acao::Confirmar)) {
        ctx.audio.tocar(somConfirmar_);
        switch (static_cast<Opcao>(selecao_)) {
            case Opcao::Jogar:
                ctx.cenas.empilhar(std::make_unique<InteriorScene>());
                break;
            case Opcao::Volume:
            case Opcao::TelaCheia:
                ajustar(ctx, static_cast<Opcao>(selecao_), 1);
                break;
            case Opcao::Sair:
            case Opcao::Contagem:
                ctx.app.sair();
                break;
        }
    }

    if (ctx.input.acaoPressionada(Acao::Voltar)) {
        ctx.app.sair();
    }
}

void MenuScene::desenharTitulo(Context& ctx, float meio) {
    // Pulso lento para o brilho respirar; tremor rapido para a fenda faiscar.
    const float pulso = 0.5f + 0.5f * std::sin(tempo_ * 2.0f);
    const float tremor = std::sin(tempo_ * 13.0f) * 1.2f;

    const float larguraHull = ctx.fonte.medir("HULL", kEscalaTitulo).x;
    const float larguraBreach = ctx.fonte.medir("BREACH", kEscalaTitulo).x;
    const float larguraTotal = larguraHull + kFolgaFenda + larguraBreach;
    const float xInicio = meio - larguraTotal * 0.5f;
    const float xBreach = xInicio + larguraHull + kFolgaFenda;
    const float yTitulo = 46.0f;
    const float alturaTitulo = ctx.fonte.alturaLinha(kEscalaTitulo);

    // Fulgor aditivo por tras da palavra inteira, alaranjado como o alarme.
    draw::brilhoAditivo(ctx.renderer, SDL_FPoint{meio, yTitulo + alturaTitulo * 0.5f}, 210.0f,
                        SDL_FColor{0.95f, 0.45f, 0.22f, 0.14f + 0.05f * pulso});

    // Sombra dura deslocada, para a palavra descolar do fundo.
    ctx.fonte.desenhar(ctx.renderer, "HULL", xInicio + 3.0f, yTitulo + 4.0f, kCorSombraTitulo,
                       kEscalaTitulo);
    ctx.fonte.desenhar(ctx.renderer, "BREACH", xBreach + 3.0f, yTitulo + 4.0f, kCorSombraTitulo,
                       kEscalaTitulo);

    ctx.fonte.desenhar(ctx.renderer, "HULL", xInicio, yTitulo, kCorTituloHull, kEscalaTitulo);
    const SDL_Color corBreach =
        draw::misturar(kCorTituloBreach, SDL_Color{255, 150, 120, 255}, 0.35f * pulso);
    ctx.fonte.desenhar(ctx.renderer, "BREACH", xBreach, yTitulo, corBreach, kEscalaTitulo);

    // A fenda: tracos curtos empilhados seguindo kZigueFenda, com um ponto de
    // brilho em cada dente. E o "breach" do nome desenhado -- o casco partindo
    // entre as duas silabas.
    const float xFenda = xInicio + larguraHull + kFolgaFenda * 0.5f;
    const float yTopo = yTitulo - 6.0f;
    const float yBase = yTitulo + alturaTitulo + 6.0f;
    const float alturaDente = (yBase - yTopo) / static_cast<float>(kDentesFenda);
    for (int i = 0; i < kDentesFenda; ++i) {
        const float y = yTopo + static_cast<float>(i) * alturaDente;
        const float x = xFenda + kZigueFenda[i] + tremor;
        draw::retanguloTela(ctx.renderer, SDL_FRect{x - 1.5f, y, 3.0f, alturaDente + 1.0f},
                            kCorFenda);
        draw::brilhoAditivo(ctx.renderer, SDL_FPoint{x, y}, 13.0f,
                            SDL_FColor{1.0f, 0.75f, 0.4f, 0.45f + 0.2f * pulso});
    }
}

void MenuScene::desenhar(Context& ctx, float /*alpha*/) {
    const float meio = static_cast<float>(App::kLarguraLogica) * 0.5f;
    const std::size_t itens = static_cast<std::size_t>(Opcao::Contagem);

    desenharFundo(ctx);
    desenharTitulo(ctx, meio);

    // O espacamento sai da altura de linha da fonte, para o layout acompanhar
    // uma eventual troca do atlas. O bloco de itens fica centrado na faixa que
    // sobra abaixo do logotipo -- a posicao vem das alturas da fonte, nunca de
    // uma constante, senao trocar o atlas desalinha tudo.
    const float espacoItem = ctx.fonte.alturaLinha(2.0f) + 10.0f;
    const float alturaBloco =
        espacoItem * static_cast<float>(itens - 1) + ctx.fonte.alturaLinha(2.0f);
    const float yTopoFaixa = 46.0f + ctx.fonte.alturaLinha(kEscalaTitulo) + 40.0f;
    const float yFimFaixa = static_cast<float>(App::kAlturaLogica) - 48.0f;
    const float yPrimeiroItem = yTopoFaixa + (yFimFaixa - yTopoFaixa - alturaBloco) * 0.5f;

    for (std::size_t i = 0; i < itens; ++i) {
        const bool ativo = static_cast<int>(i) == selecao_;
        const float y = yPrimeiroItem + static_cast<float>(i) * espacoItem;
        const std::string texto = rotulo(ctx, static_cast<Opcao>(i));
        ctx.fonte.desenharCentralizado(ctx.renderer, texto, meio, y,
                                       ativo ? kCorItemAtivo : kCorItem, ativo ? 2.0f : 1.5f);
        if (ativo) {
            // Cursor pulsante a esquerda do item selecionado.
            const float pulso = 3.0f * std::sin(tempo_ * 6.0f);
            const float largura = ctx.fonte.medir(texto, 2.0f).x;
            ctx.fonte.desenhar(ctx.renderer, ">", meio - largura * 0.5f - 32.0f + pulso, y,
                               kCorTitulo, 2.0f);
        }
    }

    const char* dica = ctx.input.temGamepad()
                           ? "direcional: navegar e ajustar   A: confirmar   B: sair"
                           : "setas ou WASD: navegar e ajustar   Enter: confirmar   Esc: sair";
    ctx.fonte.desenharCentralizado(ctx.renderer, dica, meio,
                                   static_cast<float>(App::kAlturaLogica) - 34.0f, kCorRodape,
                                   1.0f);
}

}  // namespace jogo
