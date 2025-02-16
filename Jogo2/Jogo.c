#include <allegro5/allegro5.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

// Definindo constantes
#define TAM_TILE 36
#define C_MAPA 20
#define L_MAPA 20
#define colisao_1 1
#define colisao_2 3


typedef struct Node {
    int mapa[L_MAPA][C_MAPA];  // Mapa de tiles
    struct Node* proximo;      // Ponteiro para o próximo nó
} Node;

typedef struct Inimigo{
    int x,y;
    int altura, largura;
    int dx, dy;
    int velocidade;
    int contador_frames;
}Inimigo;

typedef struct Jogo {
    ALLEGRO_DISPLAY* disp;
    ALLEGRO_TIMER* timer;
    ALLEGRO_EVENT_QUEUE* queue;
    ALLEGRO_FONT* font;
    ALLEGRO_BITMAP* image;
    ALLEGRO_BITMAP* grama;
    ALLEGRO_BITMAP* arbusto;
    ALLEGRO_BITMAP* flor;
    ALLEGRO_BITMAP* pedra;
    ALLEGRO_BITMAP* portal;
    ALLEGRO_BITMAP* objetivo;
    ALLEGRO_BITMAP* sprite_inimigo;
    bool keys[ALLEGRO_KEY_MAX];
    Node* mapas;  // Lista encadeada de mapas
    Node* mapa_atual;  // Ponteiro para o mapa atual
    Inimigo inimigos[12]; // Inimigos
    int num_inimigos;
    int x, y;
    int si;
    int flags;
    int n;
    int l_sprite, a_sprite;
    bool redraw;
} Jogo;

void adicionar_mapa(Node** lista, const char* nome_arquivo);
void destruir_lista(Node* lista);
void desenha_mapa(int mapa[L_MAPA][C_MAPA], ALLEGRO_BITMAP* grama, ALLEGRO_BITMAP* arbusto, ALLEGRO_BITMAP* flor, ALLEGRO_BITMAP* pedra, ALLEGRO_BITMAP* portal, ALLEGRO_BITMAP* objetivo);
void movimentacao(bool keys[], int* x, int* y, int* si, int largura_sprite, int altura_sprite, int mapa[L_MAPA][C_MAPA]);
Inimigo criar_inimigo(int x, int y, int altura, int largura,int velocidade);
void desenha_inimigo(Inimigo *Inimigo, ALLEGRO_BITMAP* sprite);
void atualizar_inimigo(Inimigo *inimigo, int mapa[L_MAPA][C_MAPA]);
bool colidiu_com_personagem(int x1, int y1, int largura1, int altura1, int x2, int y2, int largura2, int altura2);
void tela_game_over(Jogo *J);
void tela_win(Jogo* J);

Jogo* novo_jogo() {
    Jogo* J = (Jogo*)malloc(sizeof(Jogo));
    assert(J != NULL);

    al_init();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();
    al_install_keyboard();

    J->timer = al_create_timer(1.0 / 100.0);
    J->queue = al_create_event_queue();
    J->disp = al_create_display(720, 720);
    J->font = al_load_font("Roboto-Regular.ttf", 60, 0);
    assert(J->font != NULL);

    J->image = al_load_bitmap("girl.png");
    J->grama = al_load_bitmap("grama.png");
    J->arbusto = al_load_bitmap("arbusto.png");
    J->flor = al_load_bitmap("flor.png");
    J->pedra = al_load_bitmap("pedra.png");
    J->portal = al_load_bitmap("portal.png");
    J->objetivo = al_load_bitmap("objetivo.png");
    J->sprite_inimigo = al_load_bitmap("ghostdog.png");

    assert(J->image && J->grama && J->arbusto && J->flor && J->pedra && J->portal && J->sprite_inimigo);

    // Inicializa a lista de mapas e carrega os arquivos
    J->mapas = NULL;
    adicionar_mapa(&J->mapas, "mapa1.txt");
    adicionar_mapa(&J->mapas, "mapa2.txt");
    adicionar_mapa(&J->mapas, "mapa3.txt");
    adicionar_mapa(&J->mapas, "mapa4.txt");
    adicionar_mapa(&J->mapas, "mapa5.txt");

    J->mapa_atual = J->mapas;  // O primeiro mapa da lista será o atual
    J->x = 55;
    J->y = 55;
    J->si = 0;
    J->flags = 0;
    J->n = 0;
    J->l_sprite = 64;
    J->a_sprite = 64;

    J->num_inimigos = 4;
    for (int i = 0; i < J->num_inimigos; i++) {
        J->inimigos[i] = criar_inimigo(360, 360, 64, 64, 1);
    }

    J->redraw = true;

    for (int i = 0; i < ALLEGRO_KEY_MAX; i++) {
        J->keys[i] = false;
    }

    al_register_event_source(J->queue, al_get_keyboard_event_source());
    al_register_event_source(J->queue, al_get_display_event_source(J->disp));
    al_register_event_source(J->queue, al_get_timer_event_source(J->timer));


    al_start_timer(J->timer);

    return J;
}


void finalizar_jogo(Jogo* J) {
    al_destroy_bitmap(J->image);
    al_destroy_bitmap(J->grama);
    al_destroy_bitmap(J->arbusto);
    al_destroy_bitmap(J->flor);
    al_destroy_bitmap(J->pedra);
    al_destroy_bitmap(J->portal);
    al_destroy_bitmap(J->sprite_inimigo);
    al_destroy_bitmap(J->objetivo);

    al_destroy_timer(J->timer);
    al_destroy_event_queue(J->queue);
    al_destroy_display(J->disp);
    al_destroy_font(J->font);
    destruir_lista(J->mapas);


    free(J);
}


bool jogo_rodando(Jogo* J) {
    ALLEGRO_EVENT event;
    al_wait_for_event(J->queue, &event);

    if (event.type == ALLEGRO_EVENT_TIMER) {
        J->redraw = true;
    } else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
        J->keys[event.keyboard.keycode] = true;
    } else if (event.type == ALLEGRO_EVENT_KEY_UP) {
        J->keys[event.keyboard.keycode] = false;
    } else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE || J->keys[ALLEGRO_KEY_ESCAPE]) {
        return false;
    }
    return true;
}

void atualizar_jogo(Jogo* J) {
    if (J->redraw && al_is_event_queue_empty(J->queue)) {
        al_clear_to_color(al_map_rgb(0, 0, 0));

        desenha_mapa(J->mapa_atual->mapa, J->grama, J->arbusto, J->flor, J->pedra, J->portal, J->objetivo);

        for (int i = 0; i < J->num_inimigos; i++) {
            atualizar_inimigo(&J->inimigos[i], J->mapa_atual->mapa);
            desenha_inimigo(&J->inimigos[i], J->sprite_inimigo);
            if (colidiu_com_personagem(J->x, J->y, J->l_sprite, J->a_sprite, J->inimigos[i].x, J->inimigos[i].y, J->inimigos[i].largura, J->inimigos[i].altura)) {
                tela_game_over(J);
                finalizar_jogo(J);
            }
        }

        if (J->keys[ALLEGRO_KEY_UP] || J->keys[ALLEGRO_KEY_DOWN] || J->keys[ALLEGRO_KEY_LEFT] || J->keys[ALLEGRO_KEY_RIGHT]) {
            J->n++;
            movimentacao(J->keys, &J->x, &J->y, &J->si, J->l_sprite, J->a_sprite, J->mapa_atual->mapa);
            if (J->n % 12 == 0) {
                J->si = (J->si + 1) % 4;
            }
        }

        int col_tile = (J->x + J->l_sprite / 2) / TAM_TILE;
        int lin_tile = (J->y + J->a_sprite / 2) / TAM_TILE;

        // Verificar se o personagem está no tile 4 (portal)
        if (J->mapa_atual->mapa[lin_tile][col_tile] == 4) {
                // Avança para o próximo mapa
                J->mapa_atual = J->mapa_atual->proximo;

                for (int i = J->num_inimigos; i < J->num_inimigos + 2; i++) {
                    J->inimigos[i] = criar_inimigo(360, 360, 64, 64, 1);
                }
                J->num_inimigos += 2;

                J->x = 55;
                J->y = 55;
        }
        else if (J->mapa_atual->proximo == NULL && J->mapa_atual->mapa[lin_tile][col_tile] == 5) {
                tela_win(J);
                finalizar_jogo(J);
        }

        int lin = J->si / 4;
        int col = J->si % 4;

        al_draw_bitmap_region(J->image, col * J->l_sprite, lin * J->a_sprite, J->l_sprite, J->a_sprite, J->x, J->y, J->flags);
        al_flip_display();
        J->redraw = false;
    }
}


bool pode_mover(int x, int y, int largura_sprite, int altura_sprite, int dx, int dy, int mapa[L_MAPA][C_MAPA]) {
    int novo_x = x + dx + largura_sprite/2;
    int novo_y = y + dy + altura_sprite/2;
    int col = novo_x / TAM_TILE;
    int lin = novo_y / TAM_TILE;

    // Verifica se o tile na nova posição é uma parede (arbusto ou pedra)
    if (mapa[lin][col] == colisao_1 || mapa[lin][col] == colisao_2) {
        return false;
    }

    return true;
}

// Função para carregar o mapa em lista encadeada a partir de um arquivo
void carrega_mapa_lista(const char* nome_arquivo, Node* lista) {
    FILE* file = fopen(nome_arquivo, "r");
    assert(file != NULL);

    // Carrega o mapa no primeiro nó da lista
    for (int i = 0; i < L_MAPA; i++) {
        for (int j = 0; j < C_MAPA; j++) {
            fscanf(file, "%d", &lista->mapa[i][j]);
        }
    }

    fclose(file);
}

// Função para realizar a movimentação do personagem com base nas teclas pressionadas
void movimentacao(bool keys[], int* x, int* y, int* si, int largura_sprite, int altura_sprite, int mapa[L_MAPA][C_MAPA]) {
    // Só realiza a movimentação se houver uma tecla pressionada
    if (keys[ALLEGRO_KEY_RIGHT] && *x + 2 + largura_sprite <= 720 && pode_mover(*x, *y, largura_sprite, altura_sprite, 24, 0, mapa)) {
        *x += 2;
    }
    if (keys[ALLEGRO_KEY_LEFT] && (*x - 2 >= 0) && pode_mover(*x, *y, largura_sprite, altura_sprite, -24, 0, mapa)) {
        *x -= 2;
    }
    if (keys[ALLEGRO_KEY_DOWN] && *y + 2 + altura_sprite <= 720 && pode_mover(*x, *y, largura_sprite, altura_sprite, 0, 24, mapa)) {
        *y += 2;
    }
    if (keys[ALLEGRO_KEY_UP] && (*y - 2 >= 0) && pode_mover(*x, *y, largura_sprite, altura_sprite, 0, -24, mapa)) {
        *y -= 2;
    }

    // Atualiza o índice do sprite com base na direção do movimento
    if (keys[ALLEGRO_KEY_UP]) {
        *si = 12 + (*si % 4);
    } if (keys[ALLEGRO_KEY_DOWN]) {
        *si = (*si % 4);
    } if (keys[ALLEGRO_KEY_LEFT]) {
        *si = 4 + (*si % 4);
    } if (keys[ALLEGRO_KEY_RIGHT]) {
        *si = 8 + (*si % 4);
    }
}

// Função para desenhar o mapa na tela, usando diferentes tiles para diferentes valores
void desenha_mapa(int mapa[L_MAPA][C_MAPA], ALLEGRO_BITMAP* grama, ALLEGRO_BITMAP* arbusto, ALLEGRO_BITMAP* flor, ALLEGRO_BITMAP* pedra, ALLEGRO_BITMAP* portal, ALLEGRO_BITMAP* objetivo) {
    ALLEGRO_BITMAP* tile = NULL;
    for (int i = 0; i < L_MAPA; i++) {
        for (int j = 0; j < C_MAPA; j++) {
            // Verifica o tipo do tile e seleciona a imagem correspondente
            switch (mapa[i][j]) {
                case 0: tile = grama; break;
                case 1: tile = arbusto; break;
                case 2: tile = flor; break;
                case 3: tile = pedra; break;
                case 4: tile = portal; break;
                case 5: tile = objetivo; break;
                default: tile = NULL; break;
            }
            // Se o tile não for nulo, desenha na posição correspondente
            if (tile) {
                al_draw_bitmap(tile, j * TAM_TILE, i * TAM_TILE, 0);
            }
        }
    }
}

Node* new_node() {
    Node* n = (Node*)malloc(sizeof(Node));
    assert(n != NULL);
    n->proximo = NULL;
    return n;
}

void adicionar_mapa(Node** lista, const char* nome_arquivo) {
    Node* novo = new_node();
    carrega_mapa_lista(nome_arquivo, novo);

    if (*lista == NULL) {
        *lista = novo;
    } else {
        Node* temp = *lista;
        while (temp->proximo != NULL) {
            temp = temp->proximo;
        }
        temp->proximo = novo;
    }
}

void destruir_lista(Node* lista) {
    Node* temp;
    while (lista != NULL) {
        temp = lista;
        lista = lista->proximo;
        free(temp);
    }
}

void atualizar_inimigo(Inimigo* inimigo, int mapa[L_MAPA][C_MAPA]) {

    if (inimigo->contador_frames % 60 == 0) {
        inimigo->dx = (rand() % 3) - 1;
        inimigo->dy = (rand() % 3) - 1;
        if (inimigo->dx == 0 && inimigo->dy == 0) {
            inimigo->dx = 1;
        }
    }
    inimigo->contador_frames++;

    int proximo_x = inimigo->x + inimigo->dx * inimigo->velocidade;
    int proximo_y = inimigo->y + inimigo->dy * inimigo->velocidade;

    if (proximo_x >= 0 && proximo_x + inimigo->largura <= C_MAPA * TAM_TILE) {
        inimigo->x = proximo_x;
    }
    if (proximo_y >= 0 && proximo_y + inimigo->altura <= L_MAPA * TAM_TILE) {
        inimigo->y = proximo_y;
    }
}

bool colidiu_com_personagem(int x1, int y1, int largura1, int altura1, int x2, int y2, int largura2, int altura2) {
    return x1 < x2 + largura2/2 && x1 + largura1/2 > x2 && y1 < y2 + altura2/2 && y1 + altura1/2 > y2;
}

void desenha_inimigo(Inimigo* inimigo, ALLEGRO_BITMAP *sprite_inimigo) {
    al_draw_bitmap_region(sprite_inimigo, inimigo->dx, inimigo->dy, inimigo->largura, inimigo->altura, inimigo->x, inimigo->y, 0);
}

Inimigo criar_inimigo(int x, int y, int altura, int largura, int velocidade){
    Inimigo inimigo;
    inimigo.x = x;
    inimigo.y = y;
    inimigo.largura = largura;
    inimigo.altura = altura;
    inimigo.dx = 1;
    inimigo.dy = 0;
    inimigo.velocidade = velocidade;
    inimigo.contador_frames = 0;
    return inimigo;
}

void tela_win(Jogo* J) {
    al_clear_to_color(al_map_rgb(0, 255, 0));

    al_draw_text(J->font, al_map_rgb(255, 255, 255), 720 / 2, 720 / 2 - 30, ALLEGRO_ALIGN_CENTRE, "Você venceu!");

    al_flip_display();
    J->redraw = false;
    al_rest(3.0);
}

void tela_game_over(Jogo* J) {
    al_clear_to_color(al_map_rgb(255, 0, 0));

    al_draw_text(J->font, al_map_rgb(255, 255, 255), 360, 360 - 30, ALLEGRO_ALIGN_CENTRE, "Game Over!");

    al_flip_display();
    J->redraw = false;
    al_rest(3.0);
}






































































































































































































