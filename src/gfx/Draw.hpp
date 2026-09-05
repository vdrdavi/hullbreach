#pragma once

#include <SDL3/SDL.h>

#include "gfx/Camera.hpp"
#include "gfx/Sprite.hpp"

namespace jogo::draw {

/// Desenha um sprite posicionado em coordenadas de mundo, atraves da camera.
void sprite(SDL_Renderer* renderer, const Camera& camera, const Sprite& sprite,
            SDL_FPoint posicaoMundo);

/// Desenha um sprite direto em coordenadas de tela (HUD, menus).
void spriteTela(SDL_Renderer* renderer, const Sprite& sprite, SDL_FPoint posicaoTela,
                float escala = 1.0f);

void retanguloMundo(SDL_Renderer* renderer, const Camera& camera, const SDL_FRect& retangulo,
                    SDL_Color cor, bool preenchido = true);

void retanguloTela(SDL_Renderer* renderer, const SDL_FRect& retangulo, SDL_Color cor,
                   bool preenchido = true);

void linhaMundo(SDL_Renderer* renderer, const Camera& camera, SDL_FPoint a, SDL_FPoint b,
                SDL_Color cor);

/// Mistura duas cores canal a canal: t=0 devolve `a`, t=1 devolve `b`. E como
/// os paineis realcam o que acabou de acontecer -- a zona que acendeu no
/// acerto, a borda que piscou no erro -- sem uma segunda cor constante para
/// cada realce.
SDL_Color misturar(SDL_Color a, SDL_Color b, float t);

/// Brilho radial aditivo em coordenadas de tela: um leque de triangulos que sai
/// da cor no centro e chega transparente na borda. E o que faz as luzes do 3D
/// (o escapamento do motor, as faiscas dos destrocos) somarem com o fundo em
/// vez de recorta-lo, sem textura nem shader.
void brilhoAditivo(SDL_Renderer* renderer, SDL_FPoint centro, float raio, SDL_FColor cor);

}  // namespace jogo::draw
