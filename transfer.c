/*
 * ============================================================
 *  DOBBLE LÓGICO — versão C (ncurses)
 *  Conversão do jogo JS para terminal interativo.
 *
 *  Compilação:
 *      gcc dobble_logico.c -o dobble_logico -lncurses -lm
 *
 *  Jogador 1 : W/S para navegar, ESPAÇO para confirmar
 *  Jogador 2 : Setas para navegar, ENTER para confirmar
 * ============================================================
 */

#include <PDCurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* ============================================================
   CONSTANTES
   ============================================================ */
#define MAX_FORMULAS    5          /* máximo de fórmulas por carta */
#define MAX_RODADAS     10         /* máximo de rodadas no array   */
#define MAX_STR         32         /* comprimento máximo de fórmula */
#define TEMPO_INICIAL   300        /* 5 minutos em segundos        */

/* Cores (pares ncurses) */
#define COR_TITULO      1
#define COR_J1          2
#define COR_J2          3
#define COR_CENTRAL     4
#define COR_ACERTO      5
#define COR_ERRO        6
#define COR_TEMPO       7
#define COR_NORMAL      8
#define COR_SEL_J1      9
#define COR_SEL_J2      10
#define COR_MATCH       11

/* ============================================================
   ESTRUTURAS DE DADOS
   ============================================================ */

typedef struct {
    char cartaCentral[MAX_FORMULAS][MAX_STR];
    char cartaJ1     [MAX_FORMULAS][MAX_STR];
    char cartaJ2     [MAX_FORMULAS][MAX_STR];
    int  respostaCorretaJ1;
    int  respostaCorretaJ2;
    int  indiceCentralJ1;
    int  indiceCentralJ2;
    char explicacaoJ1[128];
    char explicacaoJ2[128];
    char regraJ1     [64];
    char regraJ2     [64];
} Rodada;

/* ============================================================
   ESTADO GLOBAL
   ============================================================ */

static Rodada rodadas[MAX_RODADAS];
static int    totalRodadas  = 0;
static int    rodadaAtual   = 0;

static int pontosJ1 = 0, pontosJ2 = 0;
static int acertosJ1 = 0, acertosJ2 = 0;
static int errosJ1  = 0, errosJ2  = 0;

static int selJ1 = 0;   /* índice selecionado pelo J1 */
static int selJ2 = 0;   /* índice selecionado pelo J2 */

static int tempoRestante = TEMPO_INICIAL;
static int jogoAtivo     = 0;
static int aguardando    = 0;   /* bloqueia input durante feedback */

/* Janelas ncurses */
static WINDOW *winJ1      = NULL;
static WINDOW *winCentral = NULL;
static WINDOW *winJ2      = NULL;
static WINDOW *winPlacar  = NULL;
static WINDOW *winFeed    = NULL;
static WINDOW *winTimer   = NULL;

/* ============================================================
   1. DADOS DAS RODADAS (demo embutido)
      Espelha gerarRodadasDemo() do JS original.
   ============================================================ */

static void carregarRodadasDemo(void) {

    /* ---------- Rodada 0 ---------- */
    Rodada *r = &rodadas[0];
    strcpy(r->cartaCentral[0], "~(p^q)");
    strcpy(r->cartaCentral[1], "~~p");
    strcpy(r->cartaCentral[2], "q^p");
    strcpy(r->cartaCentral[3], "p->q");
    strcpy(r->cartaCentral[4], "pv(p^q)");

    strcpy(r->cartaJ1[0], "p^p");
    strcpy(r->cartaJ1[1], "qvp");
    strcpy(r->cartaJ1[2], "pv(p^q)");
    strcpy(r->cartaJ1[3], "~pv~q");
    strcpy(r->cartaJ1[4], "~~p");

    strcpy(r->cartaJ2[0], "q^p");
    strcpy(r->cartaJ2[1], "[p^q]^(q^p)");
    strcpy(r->cartaJ2[2], "~p^~q");
    strcpy(r->cartaJ2[3], "p");
    strcpy(r->cartaJ2[4], "~pvq");

    r->respostaCorretaJ1 = 4;
    r->respostaCorretaJ2 = 0;
    r->indiceCentralJ1   = 1;
    r->indiceCentralJ2   = 2;
    strcpy(r->explicacaoJ1, "Dupla negacao: ~~p = p.");
    strcpy(r->explicacaoJ2, "Comutatividade: q^p = p^q.");
    strcpy(r->regraJ1, "Dupla negacao");
    strcpy(r->regraJ2, "Comutatividade");

    /* ---------- Rodada 1 ---------- */
    r = &rodadas[1];
    strcpy(r->cartaCentral[0], "pv(p^q)");
    strcpy(r->cartaCentral[1], "~(p^q)");
    strcpy(r->cartaCentral[2], "p");
    strcpy(r->cartaCentral[3], "~pvq");
    strcpy(r->cartaCentral[4], "q->p");

    strcpy(r->cartaJ1[0], "pv(p^q)");
    strcpy(r->cartaJ1[1], "~(p^q)");
    strcpy(r->cartaJ1[2], "p");
    strcpy(r->cartaJ1[3], "~pvq");
    strcpy(r->cartaJ1[4], "q->p");

    strcpy(r->cartaJ2[0], "~pvq");
    strcpy(r->cartaJ2[1], "~(p^q)");
    strcpy(r->cartaJ2[2], "p^q");
    strcpy(r->cartaJ2[3], "~qvp");
    strcpy(r->cartaJ2[4], "p<->q");

    r->respostaCorretaJ1 = 0;
    r->respostaCorretaJ2 = 1;
    r->indiceCentralJ1   = 0;
    r->indiceCentralJ2   = 1;
    strcpy(r->explicacaoJ1, "Absorcao: pv(p^q) = p.");
    strcpy(r->explicacaoJ2, "De Morgan: ~(p^q) = ~pv~q.");
    strcpy(r->regraJ1, "Absorcao");
    strcpy(r->regraJ2, "De Morgan");

    /* ---------- Rodada 2 ---------- */
    r = &rodadas[2];
    strcpy(r->cartaCentral[0], "p->q");
    strcpy(r->cartaCentral[1], "~p^~q");
    strcpy(r->cartaCentral[2], "p^p");
    strcpy(r->cartaCentral[3], "pvq");
    strcpy(r->cartaCentral[4], "~(pvq)");

    strcpy(r->cartaJ1[0], "p->q");
    strcpy(r->cartaJ1[1], "~qv~p");
    strcpy(r->cartaJ1[2], "p^p");
    strcpy(r->cartaJ1[3], "~~p");
    strcpy(r->cartaJ1[4], "q^p");

    strcpy(r->cartaJ2[0], "~pvq");
    strcpy(r->cartaJ2[1], "~p^~q");
    strcpy(r->cartaJ2[2], "qvp");
    strcpy(r->cartaJ2[3], "p<->q");
    strcpy(r->cartaJ2[4], "~p^~q");

    r->respostaCorretaJ1 = 0;
    r->respostaCorretaJ2 = 1;
    r->indiceCentralJ1   = 0;
    r->indiceCentralJ2   = 4;
    strcpy(r->explicacaoJ1, "Implicacao: p->q = ~pvq.");
    strcpy(r->explicacaoJ2, "De Morgan: ~(pvq) = ~p^~q.");
    strcpy(r->regraJ1, "Implicacao");
    strcpy(r->regraJ2, "De Morgan");

    /* ---------- Rodada 3 ---------- */
    r = &rodadas[3];
    strcpy(r->cartaCentral[0], "~(pvq)");
    strcpy(r->cartaCentral[1], "~p^~q");
    strcpy(r->cartaCentral[2], "p<->q");
    strcpy(r->cartaCentral[3], "q^p");
    strcpy(r->cartaCentral[4], "p");

    strcpy(r->cartaJ1[0], "~p^~q");
    strcpy(r->cartaJ1[1], "pvq");
    strcpy(r->cartaJ1[2], "p<->q");
    strcpy(r->cartaJ1[3], "~~p");
    strcpy(r->cartaJ1[4], "q->p");

    strcpy(r->cartaJ2[0], "p<->q");
    strcpy(r->cartaJ2[1], "~(pvq)");
    strcpy(r->cartaJ2[2], "pv(p^q)");
    strcpy(r->cartaJ2[3], "~pvq");
    strcpy(r->cartaJ2[4], "q^p");

    r->respostaCorretaJ1 = 0;
    r->respostaCorretaJ2 = 1;
    r->indiceCentralJ1   = 1;
    r->indiceCentralJ2   = 0;
    strcpy(r->explicacaoJ1, "De Morgan: ~(pvq) = ~p^~q.");
    strcpy(r->explicacaoJ2, "De Morgan: ~(pvq) = ~p^~q.");
    strcpy(r->regraJ1, "De Morgan");
    strcpy(r->regraJ2, "De Morgan");

    totalRodadas = 4;
}

/* ============================================================
   2. INICIALIZAÇÃO DAS JANELAS
   ============================================================ */

static void iniciarCores(void) {
    start_color();
    use_default_colors();
    init_pair(COR_TITULO,   COLOR_YELLOW,  COLOR_BLACK);
    init_pair(COR_J1,       COLOR_CYAN,    COLOR_BLACK);
    init_pair(COR_J2,       COLOR_MAGENTA, COLOR_BLACK);
    init_pair(COR_CENTRAL,  COLOR_WHITE,   COLOR_BLACK);
    init_pair(COR_ACERTO,   COLOR_GREEN,   COLOR_BLACK);
    init_pair(COR_ERRO,     COLOR_RED,     COLOR_BLACK);
    init_pair(COR_TEMPO,    COLOR_RED,     COLOR_BLACK);
    init_pair(COR_NORMAL,   COLOR_WHITE,   COLOR_BLACK);
    init_pair(COR_SEL_J1,   COLOR_BLACK,   COLOR_CYAN);
    init_pair(COR_SEL_J2,   COLOR_BLACK,   COLOR_MAGENTA);
    init_pair(COR_MATCH,    COLOR_BLACK,   COLOR_YELLOW);
}

/*
 * Layout do terminal (80x24 mínimo):
 *
 *  ┌─ PLACAR / TIMER ──────────────────────────────────────┐  linha 0-1
 *  ├─ J1 (12 colunas) ─┬─ CENTRAL (32) ─┬─ J2 (12 cols) ─┤  linhas 2-16
 *  ├─ FEEDBACK ────────────────────────────────────────────┤  linhas 17-22
 *  └───────────────────────────────────────────────────────┘
 */
static void criarJanelas(void) {
    int linhas, colunas;
    getmaxyx(stdscr, linhas, colunas);

    int altoCarta  = 16;
    int largoJ     = (colunas - 34) / 2;   /* largura de cada carta do jogador */
    int largoC     = 34;                    /* largura da carta central          */
    int colJ2      = largoJ + largoC;
    int linFeed    = 2 + altoCarta;
    int altoFeed   = linhas - linFeed - 1;

    winPlacar  = newwin(2,      colunas,  0,      0);
    winJ1      = newwin(altoCarta, largoJ,   2,      0);
    winCentral = newwin(altoCarta, largoC,   2,      largoJ);
    winJ2      = newwin(altoCarta, largoJ,   2,      colJ2);
    winFeed    = newwin(altoFeed,  colunas,  linFeed, 0);
    winTimer   = NULL; /* embutido no placar */

    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);   /* getch() não bloqueante */
    noecho();
    curs_set(0);
}

/* ============================================================
   3. DESENHO DAS CARTAS
   ============================================================ */

/* Desenha a borda e título de uma janela de carta */
static void desenharBordaCarta(WINDOW *win, const char *titulo, int corPar) {
    wattron(win, COLOR_PAIR(corPar) | A_BOLD);
    box(win, 0, 0);
    int largura = getmaxx(win);
    int titLen  = (int)strlen(titulo);
    mvwprintw(win, 0, (largura - titLen) / 2, "%s", titulo);
    wattroff(win, COLOR_PAIR(corPar) | A_BOLD);
}

/*
 * Distribui as 5 fórmulas em posições fixas dentro da janela,
 * simulando o layout "losango" do JS.
 *
 * Posições relativas (linha, coluna) para uma carta de 16x? :
 *   0 → topo-esq    (2, 2)
 *   1 → topo-dir    (2, largura-12)
 *   2 → centro      (7, meio)
 *   3 → baixo-esq   (12, 2)
 *   4 → baixo-dir   (12, largura-12)
 */
static void posFormula(WINDOW *win, int idx, int *linOut, int *colOut) {
    int largura = getmaxx(win);
    int meio    = largura / 2 - 5;
    int direita = largura - 12;

    switch (idx) {
        case 0: *linOut = 2;  *colOut = 2;       break;
        case 1: *linOut = 2;  *colOut = direita;  break;
        case 2: *linOut = 7;  *colOut = meio;     break;
        case 3: *linOut = 12; *colOut = 2;        break;
        case 4: *linOut = 12; *colOut = direita;  break;
        default:*linOut = 2;  *colOut = 2;        break;
    }
}

/*
 * Renderiza uma carta completa.
 *  formulas[] : array de strings com as fórmulas
 *  n          : número de fórmulas
 *  selIdx     : índice selecionado (-1 = nenhum, -2 = acerto/match)
 *  corNormal  : par de cor da bolinha não selecionada
 *  corSel     : par de cor quando selecionada
 */
static void renderizarCarta(WINDOW *win, char formulas[][MAX_STR], int n,
                            int selIdx, int matchIdx,
                            int corNormal, int corSel) {
    werase(win);
    int largura = getmaxx(win);

    for (int i = 0; i < n && i < MAX_FORMULAS; i++) {
        int lin, col;
        posFormula(win, i, &lin, &col);

        /* Escolhe cor */
        int par;
        attr_t atributo = A_BOLD;
        if (i == matchIdx) {
            par       = COR_MATCH;
            atributo |= A_REVERSE;
        } else if (i == selIdx) {
            par = corSel;
        } else {
            par = corNormal;
        }

        /* Caixa ao redor da fórmula */
        int flen = (int)strlen(formulas[i]);
        int boxW = flen + 4;
        if (col + boxW > largura - 1) col = largura - 1 - boxW;
        if (col < 1) col = 1;

        wattron(win, COLOR_PAIR(par) | atributo);
        /* Linha superior da caixa */
        mvwprintw(win, lin,     col, "[");
        mvwprintw(win, lin,     col + 1, " %s ", formulas[i]);
        mvwprintw(win, lin,     col + flen + 3, "]");
        wattroff(win, COLOR_PAIR(par) | atributo);
    }
}

/* ============================================================
   4. DESENHO DO PLACAR E TIMER
   ============================================================ */

static void desenharPlacar(void) {
    int largura = getmaxx(winPlacar);
    werase(winPlacar);

    /* Título */
    wattron(winPlacar, COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvwprintw(winPlacar, 0, largura/2 - 8, "  DOBBLE LOGICO  ");
    wattroff(winPlacar, COLOR_PAIR(COR_TITULO) | A_BOLD);

    /* Jogador 1 */
    wattron(winPlacar, COLOR_PAIR(COR_J1) | A_BOLD);
    mvwprintw(winPlacar, 1, 2, "J1: %d pts", pontosJ1);
    wattroff(winPlacar, COLOR_PAIR(COR_J1) | A_BOLD);

    /* Rodada */
    wattron(winPlacar, COLOR_PAIR(COR_NORMAL));
    mvwprintw(winPlacar, 1, largura/2 - 6,
              "Rodada %d/%d", rodadaAtual + 1, totalRodadas);
    wattroff(winPlacar, COLOR_PAIR(COR_NORMAL));

    /* Timer */
    int min = tempoRestante / 60;
    int seg = tempoRestante % 60;
    int corT = (tempoRestante <= 30) ? COR_TEMPO : COR_NORMAL;
    wattron(winPlacar, COLOR_PAIR(corT) | A_BOLD);
    mvwprintw(winPlacar, 0, largura - 12, "  %02d:%02d  ", min, seg);
    wattroff(winPlacar, COLOR_PAIR(corT) | A_BOLD);

    /* Jogador 2 */
    wattron(winPlacar, COLOR_PAIR(COR_J2) | A_BOLD);
    mvwprintw(winPlacar, 1, largura - 14, "J2: %d pts", pontosJ2);
    wattroff(winPlacar, COLOR_PAIR(COR_J2) | A_BOLD);

    wrefresh(winPlacar);
}

/* ============================================================
   5. PAINEL DE FEEDBACK
   ============================================================ */

static void mostrarFeedback(int acerto, const char *msg,
                            const char *exp, const char *regra) {
    int largura = getmaxx(winFeed);
    werase(winFeed);

    int corPar = acerto ? COR_ACERTO : COR_ERRO;
    wattron(winFeed, COLOR_PAIR(corPar) | A_BOLD);
    box(winFeed, 0, 0);
    int mlen = (int)strlen(msg);
    mvwprintw(winFeed, 1, (largura - mlen) / 2, "%s", msg);
    wattroff(winFeed, COLOR_PAIR(corPar) | A_BOLD);

    if (exp && exp[0]) {
        wattron(winFeed, COLOR_PAIR(COR_NORMAL));
        mvwprintw(winFeed, 2, 2, "Explicacao: %s", exp);
        wattroff(winFeed, COLOR_PAIR(COR_NORMAL));
    }
    if (regra && regra[0]) {
        wattron(winFeed, COLOR_PAIR(COR_TITULO));
        mvwprintw(winFeed, 3, 2, "Regra: %s", regra);
        wattroff(winFeed, COLOR_PAIR(COR_TITULO));
    }

    wrefresh(winFeed);
}

static void feedbackPadrao(void) {
    int largura = getmaxx(winFeed);
    werase(winFeed);
    wattron(winFeed, COLOR_PAIR(COR_NORMAL));
    box(winFeed, 0, 0);
    const char *msg = "ENCONTRE O PAR LOGICAMENTE EQUIVALENTE!";
    mvwprintw(winFeed, 1, (largura - (int)strlen(msg)) / 2, "%s", msg);

    /* Controles */
    mvwprintw(winFeed, 3, 2,
              "J1: [W/S] ou [A/D] navega  |  [ESPACO] confirma");
    mvwprintw(winFeed, 4, 2,
              "J2: [Setas] navega          |  [ENTER]  confirma");
    wattroff(winFeed, COLOR_PAIR(COR_NORMAL));
    wrefresh(winFeed);
}

/* ============================================================
   6. RENDERIZAR RODADA COMPLETA
   ============================================================ */

/* Conta fórmulas não-vazias */
static int contarFormulas(char formulas[][MAX_STR]) {
    int n = 0;
    for (int i = 0; i < MAX_FORMULAS; i++)
        if (formulas[i][0] != '\0') n++;
    return n;
}

static void renderizarRodada(void) {
    if (rodadaAtual >= totalRodadas) return;

    Rodada *r = &rodadas[rodadaAtual];
    selJ1 = 0;
    selJ2 = 0;
    aguardando = 0;

    int nJ1 = contarFormulas(r->cartaJ1);
    int nJ2 = contarFormulas(r->cartaJ2);
    int nC  = contarFormulas(r->cartaCentral);

    desenharPlacar();

    /* Cartas dos jogadores */
    renderizarCarta(winJ1, r->cartaJ1, nJ1,
                    selJ1, -1, COR_J1, COR_SEL_J1);
    desenharBordaCarta(winJ1, " JOGADOR 1 [WASD+ESP] ", COR_J1);
    wrefresh(winJ1);

    renderizarCarta(winCentral, r->cartaCentral, nC,
                    -1, -1, COR_CENTRAL, COR_CENTRAL);
    desenharBordaCarta(winCentral, " CARTA CENTRAL ", COR_CENTRAL);
    wrefresh(winCentral);

    renderizarCarta(winJ2, r->cartaJ2, nJ2,
                    selJ2, -1, COR_J2, COR_SEL_J2);
    desenharBordaCarta(winJ2, " JOGADOR 2 [SETAS+ENT] ", COR_J2);
    wrefresh(winJ2);

    feedbackPadrao();
}

/* ============================================================
   7. VERIFICAÇÃO E PONTUAÇÃO  (espelha confirmarSelecao do JS)
   ============================================================ */

static void atualizarCartaJogador(int jogador) {
    Rodada *r = &rodadas[rodadaAtual];
    int nJ = contarFormulas(jogador == 1 ? r->cartaJ1 : r->cartaJ2);
    if (jogador == 1) {
        renderizarCarta(winJ1, r->cartaJ1, nJ,
                        selJ1, -1, COR_J1, COR_SEL_J1);
        desenharBordaCarta(winJ1, " JOGADOR 1 [WASD+ESP] ", COR_J1);
        wrefresh(winJ1);
    } else {
        renderizarCarta(winJ2, r->cartaJ2, nJ,
                        selJ2, -1, COR_J2, COR_SEL_J2);
        desenharBordaCarta(winJ2, " JOGADOR 2 [SETAS+ENT] ", COR_J2);
        wrefresh(winJ2);
    }
}

static void destacarCentral(Rodada *r, int jogador) {
    int idxC = (jogador == 1) ? r->indiceCentralJ1 : r->indiceCentralJ2;
    int nC   = contarFormulas(r->cartaCentral);
    renderizarCarta(winCentral, r->cartaCentral, nC,
                    -1, idxC, COR_CENTRAL, COR_CENTRAL);
    desenharBordaCarta(winCentral, " CARTA CENTRAL ", COR_CENTRAL);
    wrefresh(winCentral);
}

static void confirmarSelecao(int jogador) {
    if (!jogoAtivo || aguardando) return;

    Rodada *r       = &rodadas[rodadaAtual];
    int sel         = (jogador == 1) ? selJ1 : selJ2;
    int correta     = (jogador == 1) ? r->respostaCorretaJ1 : r->respostaCorretaJ2;
    const char *exp = (jogador == 1) ? r->explicacaoJ1 : r->explicacaoJ2;
    const char *reg = (jogador == 1) ? r->regraJ1 : r->regraJ2;

    int acertou = (sel == correta);

    if (acertou) {
        /* Bônus por rapidez: se ainda estiver próximo do início */
        int bonus = (tempoRestante > 295) ? 5 : 0;
        int ganho = 10 + bonus;

        if (jogador == 1) { pontosJ1 += ganho; acertosJ1++; }
        else              { pontosJ2 += ganho; acertosJ2++; }

        desenharPlacar();
        destacarCentral(r, jogador);

        char msg[64];
        snprintf(msg, sizeof(msg),
                 "*** JOGADOR %d ACERTOU! +%d pontos ***", jogador, ganho);
        mostrarFeedback(1, msg, exp, reg);

        aguardando = 1;

        /* Pausa de 2,5 s (ncurses: nodelay ativo, usamos sleep) */
        napms(2500);

        rodadaAtual++;
        if (rodadaAtual >= totalRodadas) {
            jogoAtivo = 0;
        } else {
            renderizarRodada();
        }

    } else {
        /* Erro */
        int penalidade = 5;
        if (jogador == 1) { pontosJ1 = (pontosJ1 >= penalidade) ? pontosJ1 - penalidade : 0; errosJ1++; }
        else              { pontosJ2 = (pontosJ2 >= penalidade) ? pontosJ2 - penalidade : 0; errosJ2++; }

        desenharPlacar();

        char msg[64];
        snprintf(msg, sizeof(msg),
                 "!!! ERRO J%d: -5 pontos. Observe: %s", jogador, reg);
        mostrarFeedback(0, msg, "Tente novamente!", "");

        napms(1500);
        feedbackPadrao();
    }

    aguardando = 0;
}

/* ============================================================
   8. TELA INICIAL
   ============================================================ */

static void telaInicio(void) {
    clear();
    int lin, col;
    getmaxyx(stdscr, lin, col);

    attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvprintw(lin/2 - 5, col/2 - 16,
             "  *** DOBBLE LOGICO — Logica Proposicional ***  ");
    attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

    attron(COLOR_PAIR(COR_NORMAL));
    mvprintw(lin/2 - 3, col/2 - 20,
             "Encontre a formula equivalente na sua carta");
    mvprintw(lin/2 - 2, col/2 - 20,
             "que corresponda a formula destacada na carta central.");
    mvprintw(lin/2,     col/2 - 18,
             "Jogador 1 :  W/S ou A/D  para navegar,  ESPACO para confirmar");
    mvprintw(lin/2 + 1, col/2 - 18,
             "Jogador 2 :  Setas       para navegar,  ENTER  para confirmar");
    mvprintw(lin/2 + 3, col/2 - 8,
             "Acerto: +10 pts  |  Erro: -5 pts");
    mvprintw(lin/2 + 5, col/2 - 10,
             "Pressione ENTER para comecar...");
    attroff(COLOR_PAIR(COR_NORMAL));

    refresh();
    nodelay(stdscr, FALSE);
    int ch;
    while ((ch = getch()) != '\n' && ch != KEY_ENTER && ch != ' ');
    nodelay(stdscr, TRUE);
}

/* ============================================================
   9. TELA FINAL  (espelha encerrarJogo do JS)
   ============================================================ */

static const char *categoria(int pts) {
    if (pts <= 30)  return "Iniciante em Logica";
    if (pts <= 70)  return "Aprendiz Proposicional";
    if (pts <= 120) return "Mestre das Equivalencias";
    return "Especialista em Logica";
}

static void telaFinal(void) {
    clear();
    int lin, col;
    getmaxyx(stdscr, lin, col);

    /* Vencedor */
    const char *vencedor;
    if      (pontosJ1 > pontosJ2) vencedor = "*** JOGADOR 1 VENCEU! ***";
    else if (pontosJ2 > pontosJ1) vencedor = "*** JOGADOR 2 VENCEU! ***";
    else                          vencedor = "***  EMPATE! Os dois sao logicos!  ***";

    attron(COLOR_PAIR(COR_TITULO) | A_BOLD);
    mvprintw(2, (col - (int)strlen(vencedor)) / 2, "%s", vencedor);
    attroff(COLOR_PAIR(COR_TITULO) | A_BOLD);

    /* Placar detalhado */
    attron(COLOR_PAIR(COR_J1) | A_BOLD);
    mvprintw(5,  4, "=== JOGADOR 1 ===");
    attroff(COLOR_PAIR(COR_J1) | A_BOLD);
    attron(COLOR_PAIR(COR_J1));
    mvprintw(6,  4, "Pontos  : %d", pontosJ1);
    mvprintw(7,  4, "Acertos : %d", acertosJ1);
    mvprintw(8,  4, "Erros   : %d", errosJ1);
    mvprintw(9,  4, "Nivel   : %s", categoria(pontosJ1));
    attroff(COLOR_PAIR(COR_J1));

    attron(COLOR_PAIR(COR_J2) | A_BOLD);
    mvprintw(5,  col - 24, "=== JOGADOR 2 ===");
    attroff(COLOR_PAIR(COR_J2) | A_BOLD);
    attron(COLOR_PAIR(COR_J2));
    mvprintw(6,  col - 24, "Pontos  : %d", pontosJ2);
    mvprintw(7,  col - 24, "Acertos : %d", acertosJ2);
    mvprintw(8,  col - 24, "Erros   : %d", errosJ2);
    mvprintw(9,  col - 24, "Nivel   : %s", categoria(pontosJ2));
    attroff(COLOR_PAIR(COR_J2));

    attron(COLOR_PAIR(COR_ACERTO) | A_BOLD);
    mvprintw(12, (col - 24) / 2, "A logica venceu!  =)");
    attroff(COLOR_PAIR(COR_ACERTO) | A_BOLD);

    attron(COLOR_PAIR(COR_NORMAL));
    mvprintw(lin - 2, (col - 34) / 2,
             "Pressione R para jogar de novo ou Q para sair");
    attroff(COLOR_PAIR(COR_NORMAL));

    refresh();
}

/* ============================================================
   10. LOOP PRINCIPAL
   ============================================================ */

static void iniciarJogo(void) {
    /* Reset de estado (espelha iniciarJogo do JS) */
    rodadaAtual = 0;
    pontosJ1 = pontosJ2 = 0;
    acertosJ1 = acertosJ2 = errosJ1 = errosJ2 = 0;
    selJ1 = selJ2 = 0;
    tempoRestante = TEMPO_INICIAL;
    jogoAtivo = 1;
    aguardando = 0;

    clear();
    refresh();
    renderizarRodada();
}

int main(void) {
    /* Inicializa ncurses */
    initscr();
    iniciarCores();
    criarJanelas();

    /* Carrega dados */
    carregarRodadasDemo();

    telaInicio();

    /* Loop de partidas */
    int continuar = 1;
    while (continuar) {
        iniciarJogo();

        /* Timestamp para controle do timer (1 segundo) */
        time_t ultimo = time(NULL);

        /* Loop de jogo */
        while (jogoAtivo) {
            /* Cronômetro */
            time_t agora = time(NULL);
            if (agora != ultimo) {
                ultimo = agora;
                tempoRestante--;
                desenharPlacar();
                if (tempoRestante <= 0) {
                    jogoAtivo = 0;
                    break;
                }
            }

            /* Leitura de tecla (não bloqueante) */
            int ch = getch();
            if (ch == ERR) {
                napms(50);
                continue;
            }

            Rodada *r   = &rodadas[rodadaAtual];
            int nJ1 = contarFormulas(r->cartaJ1);
            int nJ2 = contarFormulas(r->cartaJ2);

            /* ---- Jogador 1: WASD + Espaço ---- */
            switch (ch) {
                case 'w': case 'W':
                case 'a': case 'A':
                    if (!aguardando) {
                        selJ1 = (selJ1 - 1 + nJ1) % nJ1;
                        atualizarCartaJogador(1);
                    }
                    break;
                case 's': case 'S':
                case 'd': case 'D':
                    if (!aguardando) {
                        selJ1 = (selJ1 + 1) % nJ1;
                        atualizarCartaJogador(1);
                    }
                    break;
                case ' ':
                    confirmarSelecao(1);
                    break;

                /* ---- Jogador 2: Setas + Enter ---- */
                case KEY_UP:
                case KEY_LEFT:
                    if (!aguardando) {
                        selJ2 = (selJ2 - 1 + nJ2) % nJ2;
                        atualizarCartaJogador(2);
                    }
                    break;
                case KEY_DOWN:
                case KEY_RIGHT:
                    if (!aguardando) {
                        selJ2 = (selJ2 + 1) % nJ2;
                        atualizarCartaJogador(2);
                    }
                    break;
                case '\n': case KEY_ENTER:
                    confirmarSelecao(2);
                    break;

                /* Sair a qualquer momento */
                case 'q': case 'Q':
                    jogoAtivo = 0;
                    continuar = 0;
                    break;
            }
        }

        /* Tela final */
        if (continuar) {
            telaFinal();
            nodelay(stdscr, FALSE);
            int ch2;
            while (1) {
                ch2 = getch();
                if (ch2 == 'r' || ch2 == 'R') break;
                if (ch2 == 'q' || ch2 == 'Q') { continuar = 0; break; }
            }
            nodelay(stdscr, TRUE);
        }
    }

    /* Cleanup ncurses */
    delwin(winJ1);
    delwin(winJ2);
    delwin(winCentral);
    delwin(winPlacar);
    delwin(winFeed);
    endwin();

    return 0;
}
