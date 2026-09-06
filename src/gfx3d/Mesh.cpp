#include "gfx3d/Mesh.hpp"

#include <algorithm>
#include <cmath>
#include <map>

#include "core/Aleatorio.hpp"

namespace jogo {
namespace {

constexpr SDL_FColor kCasco{0.46f, 0.56f, 0.72f, 1.0f};
constexpr SDL_FColor kCascoEscuro{0.18f, 0.22f, 0.34f, 1.0f};
constexpr SDL_FColor kCabine{0.30f, 0.95f, 1.00f, 1.0f};
constexpr SDL_FColor kMotor{1.0f, 0.55f, 0.25f, 1.0f};

}  // namespace

void orientarFacesParaFora(Mesh& malha) {
    if (malha.vertices.empty()) {
        return;
    }

    Vec3 centro{};
    for (const Vec3& v : malha.vertices) {
        centro += v;
    }
    centro = centro * (1.0f / static_cast<float>(malha.vertices.size()));

    for (Mesh::Face& face : malha.faces) {
        const Vec3& a = malha.vertices[static_cast<std::size_t>(face.a)];
        const Vec3& b = malha.vertices[static_cast<std::size_t>(face.b)];
        const Vec3& c = malha.vertices[static_cast<std::size_t>(face.c)];

        const Vec3 normal = cross(b - a, c - a);
        const Vec3 centroDaFace = (a + b + c) * (1.0f / 3.0f);
        if (dot(normal, centroDaFace - centro) < 0.0f) {
            std::swap(face.b, face.c);
        }
    }
}

Mesh criarNaveLowPoly() {
    Mesh nave;
    // O nariz fica em -Z: a mesma convencao de "frente" usada por Mat3.
    nave.vertices = {
        Vec3{0.0f, 0.00f, -2.60f},    // 0 nariz
        Vec3{-1.70f, -0.10f, 1.10f},  // 1 ponta da asa esquerda
        Vec3{1.70f, -0.10f, 1.10f},   // 2 ponta da asa direita
        Vec3{0.0f, 0.60f, 0.40f},     // 3 dorso
        Vec3{0.0f, -0.42f, 0.30f},    // 4 ventre
        Vec3{0.0f, 0.12f, 1.55f},     // 5 cauda
        Vec3{-0.42f, 0.30f, -0.55f},  // 6 cabine esquerda
        Vec3{0.42f, 0.30f, -0.55f},   // 7 cabine direita
    };

    nave.faces = {
        // dorso, com a cabine destacada perto do nariz
        {0, 6, 3, kCasco},
        {0, 3, 7, kCasco},
        {0, 7, 6, kCabine},
        {6, 1, 3, kCasco},
        {7, 3, 2, kCasco},
        // ventre
        {0, 4, 1, kCascoEscuro},
        {0, 2, 4, kCascoEscuro},
        {1, 4, 5, kCascoEscuro},
        {2, 5, 4, kCascoEscuro},
        // traseira: os motores
        {1, 5, 3, kCasco},
        {2, 3, 5, kCasco},
        {3, 5, 4, kMotor},
    };

    orientarFacesParaFora(nave);
    return nave;
}

namespace {

/// Os doze vertices e as vinte faces do icosaedro, que as duas rochas partilham.
Mesh icosaedro() {
    constexpr float t = 1.618034f;
    Mesh malha;
    malha.vertices = {
        Vec3{-1.0f, t, 0.0f},  Vec3{1.0f, t, 0.0f},  Vec3{-1.0f, -t, 0.0f},
        Vec3{1.0f, -t, 0.0f},  Vec3{0.0f, -1.0f, t}, Vec3{0.0f, 1.0f, t},
        Vec3{0.0f, -1.0f, -t}, Vec3{0.0f, 1.0f, -t}, Vec3{t, 0.0f, -1.0f},
        Vec3{t, 0.0f, 1.0f},   Vec3{-t, 0.0f, -1.0f}, Vec3{-t, 0.0f, 1.0f},
    };
    malha.faces = {
        {0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
        {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
        {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1},
    };
    return malha;
}

/// Parte cada face em quatro, com os novos vertices empurrados para a esfera.
/// Os pontos medios sao guardados por aresta: sem isso o mesmo ponto nasceria
/// duas vezes, e o amassado seguinte o moveria de um jeito em cada face --
/// abrindo fendas na superficie.
void subdividir(Mesh& malha) {
    std::map<std::pair<int, int>, int> meios;
    const auto meio = [&](int a, int b) {
        const auto chave = std::minmax(a, b);
        const auto achado = meios.find({chave.first, chave.second});
        if (achado != meios.end()) {
            return achado->second;
        }
        const int indice = static_cast<int>(malha.vertices.size());
        malha.vertices.push_back(normalizar(malha.vertices[static_cast<std::size_t>(a)] +
                                            malha.vertices[static_cast<std::size_t>(b)]));
        meios.emplace(std::pair<int, int>{chave.first, chave.second}, indice);
        return indice;
    };

    std::vector<Mesh::Face> novas;
    novas.reserve(malha.faces.size() * 4);
    for (const Mesh::Face& f : malha.faces) {
        const int ab = meio(f.a, f.b);
        const int bc = meio(f.b, f.c);
        const int ca = meio(f.c, f.a);
        novas.push_back({f.a, ab, ca, f.cor});
        novas.push_back({f.b, bc, ab, f.cor});
        novas.push_back({f.c, ca, bc, f.cor});
        novas.push_back({ab, bc, ca, f.cor});
    }
    malha.faces = std::move(novas);
}

/// Amassa ao longo do proprio raio e normaliza pelo maior, que e o que faz a
/// escala de desenho valer como raio de colisao.
void amassar(Mesh& malha, Aleatorio& rng, float minimo, float maximo) {
    float maior = 0.0f;
    for (Vec3& v : malha.vertices) {
        v = normalizar(v) * rng.entre(minimo, maximo);
        maior = std::max(maior, comprimento(v));
    }
    for (Vec3& v : malha.vertices) {
        v = v * (1.0f / maior);
    }
}

/// Cinza terroso variando por face: sem textura, e o que tira a rocha da
/// aparencia de solido chapado.
void pintarComoRocha(Mesh& malha, Aleatorio& rng, float claro) {
    for (Mesh::Face& face : malha.faces) {
        const float tom = rng.entre(0.26f, 0.44f) * claro;
        face.cor = SDL_FColor{tom * 1.10f, tom * 1.00f, tom * 0.86f, 1.0f};
    }
}

}  // namespace

/// Uma protuberancia (ou uma cova, com amplitude negativa) larga o bastante
/// para mudar a silhueta.
struct Bossa {
    Vec3 eixo;
    float amplitude;
    float largura;
};

Mesh criarMonolitoLowPoly(Uint32 semente) {
    Mesh rocha = icosaedro();
    Aleatorio rng(semente);
    // Subdividir **antes** de deformar: os pontos medios nascem na esfera e a
    // deformacao desloca todos juntos, entao a superficie continua fechada.
    subdividir(rocha);

    // Poucas bossas largas, e nao um sorteio por vertice. Amassar vertice a
    // vertice -- o que a rocha comum faz -- da ruido de **alta frequencia**:
    // de perto parece granulado, e de longe a silhueta continua sendo um
    // circulo. O que muda a forma vista e feicao do tamanho da propria rocha,
    // e e isso que estas sao.
    Bossa bossas[5];
    for (Bossa& b : bossas) {
        b.eixo = normalizar(Vec3{rng.entre(-1.0f, 1.0f), rng.entre(-1.0f, 1.0f),
                                 rng.entre(-1.0f, 1.0f)});
        // Amplitude moderada, e o alongamento por eixo e que faz o trabalho
        // pesado da silhueta. E uma divisao de tarefas que sai da geometria:
        // um elipsoide cabe bem numa esfera media -- os raios dele variam pouco
        // e suavemente --, enquanto uma cova funda muda o raio num ponto so e e
        // exatamente o que uma esfera nao consegue representar. Bossa demais
        // gastava o orcamento de erro sem mudar o contorno.
        b.amplitude = rng.entre(-0.2f, 0.2f);
        // O expoente estreita a bossa: 1 cobre um hemisferio inteiro, 5 e quase
        // um calo. Sorteado, cada rocha tem umas largas e outras localizadas.
        b.largura = rng.entre(1.0f, 5.0f);
    }
    // Escalas por eixo, e o eixo curto e **escolhido**, nao sorteado junto com
    // os outros. Sorteando os tres na mesma faixa, saem tres parecidos com
    // frequencia e a rocha volta a ser uma bola -- foi o que aconteceu na
    // primeira tentativa, com duas das cinco em 1,05 de alongamento, mais
    // redondas que as pedras pequenas. Escolhido, toda malha tem uma direcao
    // visivelmente mais curta que as outras, e o alongamento nunca fica abaixo
    // de 1,25.
    float eixos[3] = {1.0f, 1.0f, 1.0f};
    const int curto = static_cast<int>(rng.proximo() % 3u);
    eixos[curto] = rng.entre(0.64f, 0.80f);
    const int medio = (curto + 1 + static_cast<int>(rng.proximo() % 2u)) % 3;
    eixos[medio] = rng.entre(0.82f, 1.0f);
    const Vec3 proporcao{eixos[0], eixos[1], eixos[2]};

    for (Vec3& v : rocha.vertices) {
        const Vec3 direcao = normalizar(v);
        float raio = 1.0f;
        for (const Bossa& b : bossas) {
            const float alinhamento = std::max(0.0f, dot(direcao, b.eixo));
            raio += b.amplitude * std::pow(alinhamento, b.largura);
        }
        v = Vec3{direcao.x * proporcao.x, direcao.y * proporcao.y, direcao.z * proporcao.z} *
            raio;
    }

    // Normaliza pelo vertice mais distante, que e a escala de desenho, e mede o
    // raio **medio** da superficie: e ele que vira o colisor. A esfera passa
    // pelo meio da forma, entao o erro fica dos dois lados -- atravessa-se um
    // naco de ponta antes de bater, e bate-se um naco antes de encostar num
    // vale -- em vez de todo ele de um lado so. Sem isto a forma teria de caber
    // na esfera circunscrita, e caber nela e ser redonda.
    float maior = 0.0f;
    for (const Vec3& v : rocha.vertices) {
        maior = std::max(maior, comprimento(v));
    }
    float soma = 0.0f;
    for (Vec3& v : rocha.vertices) {
        v = v * (1.0f / maior);
        soma += comprimento(v);
    }
    rocha.raioColisao = soma / static_cast<float>(rocha.vertices.size());

    // Um tom mais claro que o das pequenas: a rocha que nao se desvia por
    // manobra tem de ser reconhecida como outra coisa antes de estar perto.
    pintarComoRocha(rocha, rng, 1.35f);
    orientarFacesParaFora(rocha);
    return rocha;
}

Mesh criarAsteroideLowPoly(Uint32 semente) {
    // Icosaedro: 12 vertices e 20 faces, o menor solido que ainda passa por
    // rocha depois de amassado.
    //
    // Fica com `raioColisao` em 1, a esfera circunscrita, e nao com o raio medio
    // que o monolito usa. Nao e esquecimento: nesta escala a folga vale menos de
    // quatro unidades, e este e o colisor com que a dificuldade inteira do jogo
    // foi medida -- trocar por um menor tiraria quase 40% da secao de choque de
    // todas as rochas do campo de uma vez.
    Mesh rocha = icosaedro();
    Aleatorio rng(semente);
    amassar(rocha, rng, 0.62f, 1.10f);
    pintarComoRocha(rocha, rng, 1.0f);
    orientarFacesParaFora(rocha);
    return rocha;
}

}  // namespace jogo
