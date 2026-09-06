#include "gfx3d/Mesh.hpp"

#include <algorithm>
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

Mesh criarMonolitoLowPoly(Uint32 semente) {
    Mesh rocha = icosaedro();
    Aleatorio rng(semente);
    // Subdividir **antes** de amassar: os pontos medios nascem na esfera e o
    // amassado desloca todos juntos, entao a superficie continua fechada.
    subdividir(rocha);
    // Amassado de leve, entre 0,88 e 1,0: e o que mantem a superficie perto da
    // esfera de colisao. Com a faixa da rocha comum, um monolito bateria muito
    // antes de encostar.
    amassar(rocha, rng, 0.88f, 1.0f);
    // Um tom mais claro que o das pequenas: a rocha que nao se desvia por
    // manobra tem de ser reconhecida como outra coisa antes de estar perto.
    pintarComoRocha(rocha, rng, 1.35f);
    orientarFacesParaFora(rocha);
    return rocha;
}

Mesh criarAsteroideLowPoly(Uint32 semente) {
    // Icosaedro: 12 vertices e 20 faces, o menor solido que ainda passa por
    // rocha depois de amassado.
    Mesh rocha = icosaedro();
    Aleatorio rng(semente);
    amassar(rocha, rng, 0.62f, 1.10f);
    pintarComoRocha(rocha, rng, 1.0f);
    orientarFacesParaFora(rocha);
    return rocha;
}

}  // namespace jogo
