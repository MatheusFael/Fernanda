#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define GRID_COLS 30
#define GRID_ROWS 20
#define CELL_SIZE 24
#define STATUS_HEIGHT 72
#define WINDOW_WIDTH (GRID_COLS * CELL_SIZE)
#define WINDOW_HEIGHT (GRID_ROWS * CELL_SIZE + STATUS_HEIGHT)
#define MAX_SNAKE (GRID_COLS * GRID_ROWS)
#define PLAYER_NAME_MAX 19

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point body[MAX_SNAKE];
    int length;
    int dirX;
    int dirY;
    Point food;
    int score;
} Snake;

typedef enum {
    SCREEN_MENU = 0,
    SCREEN_NAME_INPUT,
    SCREEN_PLAYING,
    SCREEN_RECORDS,
    SCREEN_HELP,
    SCREEN_GAME_OVER
} Screen;

typedef struct {
    HWND hwnd;
    HFONT titleFont;
    HFONT textFont;
    HFONT smallFont;

    Screen screen;
    int menuIndex;

    Snake snake;

    char playerName[PLAYER_NAME_MAX + 1];
    int playerNameLen;

    char recordsText[4096];
} AppState;

static AppState gApp;

static COLORREF rgb(int r, int g, int b)
{
    return RGB(r, g, b);
}

static void fill_rect(HDC hdc, int left, int top, int right, int bottom, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    RECT rc = { left, top, right, bottom };
    FillRect(hdc, &rc, brush);
    DeleteObject(brush);
}

static void draw_text_center(HDC hdc, const char *text, RECT rc, HFONT font, COLORREF color)
{
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
}

static int snake_contains(const Snake *snake, int x, int y)
{
    int i;
    for (i = 0; i < snake->length; i++) {
        if (snake->body[i].x == x && snake->body[i].y == y) {
            return 1;
        }
    }
    return 0;
}

static void spawn_food(void)
{
    int x;
    int y;

    do {
        x = rand() % GRID_COLS;
        y = rand() % GRID_ROWS;
    } while (snake_contains(&gApp.snake, x, y));

    gApp.snake.food.x = x;
    gApp.snake.food.y = y;
}

static void load_records(void)
{
    FILE *arquivo = fopen("recordes.txt", "r");
    size_t len = 0;

    gApp.recordsText[0] = '\0';

    if (arquivo == NULL) {
        strcpy(gApp.recordsText, "Nenhum recorde registrado ainda.");
        return;
    }

    while (!feof(arquivo) && len + 2 < sizeof(gApp.recordsText)) {
        int c = fgetc(arquivo);
        if (c == EOF) {
            break;
        }

        gApp.recordsText[len++] = (char)c;
        gApp.recordsText[len] = '\0';
    }

    fclose(arquivo);

    if (len == 0) {
        strcpy(gApp.recordsText, "Nenhum recorde registrado ainda.");
    }
}

static void save_record(int score)
{
    FILE *arquivo = fopen("recordes.txt", "a");

    if (arquivo == NULL) {
        return;
    }

    fprintf(arquivo, "%03dP - %s\n", score, gApp.playerName);
    fclose(arquivo);
}

static void reset_snake(void)
{
    int centerX = GRID_COLS / 2;
    int centerY = GRID_ROWS / 2;
    int i;

    gApp.snake.length = 4;
    gApp.snake.dirX = 1;
    gApp.snake.dirY = 0;
    gApp.snake.score = 0;

    for (i = 0; i < gApp.snake.length; i++) {
        gApp.snake.body[i].x = centerX - i;
        gApp.snake.body[i].y = centerY;
    }

    spawn_food();
}

static void start_game(void)
{
    reset_snake();
    gApp.screen = SCREEN_PLAYING;
}

static void finish_game(void)
{
    save_record(gApp.snake.score);
    load_records();
    gApp.screen = SCREEN_GAME_OVER;
}

static void update_game(void)
{
    int i;
    Point newHead;

    if (gApp.screen != SCREEN_PLAYING) {
        return;
    }

    newHead.x = gApp.snake.body[0].x + gApp.snake.dirX;
    newHead.y = gApp.snake.body[0].y + gApp.snake.dirY;

    if (newHead.x < 0 || newHead.y < 0 || newHead.x >= GRID_COLS || newHead.y >= GRID_ROWS) {
        finish_game();
        return;
    }

    for (i = 0; i < gApp.snake.length; i++) {
        if (gApp.snake.body[i].x == newHead.x && gApp.snake.body[i].y == newHead.y) {
            finish_game();
            return;
        }
    }

    for (i = gApp.snake.length; i > 0; i--) {
        gApp.snake.body[i] = gApp.snake.body[i - 1];
    }

    gApp.snake.body[0] = newHead;

    if (newHead.x == gApp.snake.food.x && newHead.y == gApp.snake.food.y) {
        if (gApp.snake.length < MAX_SNAKE - 1) {
            gApp.snake.length++;
        }
        gApp.snake.score++;
        spawn_food();
    }
}

static void draw_menu(HDC hdc)
{
    static const char *menu[] = {
        "Jogar",
        "Ver recordes",
        "Como jogar",
        "Sair"
    };

    RECT titleRect = { 0, 30, WINDOW_WIDTH, 100 };
    int i;

    draw_text_center(hdc, "FERNANDA GAME SNAKE", titleRect, gApp.titleFont, rgb(248, 242, 220));

    for (i = 0; i < 4; i++) {
        RECT itemRect = { WINDOW_WIDTH / 2 - 180, 140 + (i * 64), WINDOW_WIDTH / 2 + 180, 190 + (i * 64) };
        COLORREF boxColor = (gApp.menuIndex == i) ? rgb(45, 160, 88) : rgb(29, 48, 45);
        COLORREF textColor = (gApp.menuIndex == i) ? rgb(12, 21, 18) : rgb(212, 240, 224);

        fill_rect(hdc, itemRect.left, itemRect.top, itemRect.right, itemRect.bottom, boxColor);
        draw_text_center(hdc, menu[i], itemRect, gApp.textFont, textColor);
    }

    {
        RECT footerRect = { 0, WINDOW_HEIGHT - 38, WINDOW_WIDTH, WINDOW_HEIGHT - 8 };
        draw_text_center(hdc, "Setas para navegar | Enter para selecionar", footerRect, gApp.smallFont, rgb(193, 212, 204));
    }
}

static void draw_name_input(HDC hdc)
{
    RECT titleRect = { 0, 70, WINDOW_WIDTH, 130 };
    RECT labelRect = { 0, 180, WINDOW_WIDTH, 220 };
    RECT nameBoxRect = { WINDOW_WIDTH / 2 - 220, 230, WINDOW_WIDTH / 2 + 220, 290 };
    RECT hintRect = { 0, 315, WINDOW_WIDTH, 350 };

    draw_text_center(hdc, "Digite seu nome", titleRect, gApp.titleFont, rgb(248, 242, 220));
    draw_text_center(hdc, "O recorde sera salvo com este nome:", labelRect, gApp.textFont, rgb(220, 238, 232));

    fill_rect(hdc, nameBoxRect.left, nameBoxRect.top, nameBoxRect.right, nameBoxRect.bottom, rgb(24, 46, 62));

    {
        char buffer[PLAYER_NAME_MAX + 4];
        snprintf(buffer, sizeof(buffer), "%s_", gApp.playerNameLen > 0 ? gApp.playerName : "");
        draw_text_center(hdc, buffer, nameBoxRect, gApp.textFont, rgb(234, 252, 255));
    }

    draw_text_center(hdc, "Enter para iniciar | Esc para voltar", hintRect, gApp.smallFont, rgb(193, 212, 204));
}

static void draw_playfield(HDC hdc)
{
    int i;
    int x;
    int y;

    fill_rect(hdc, 0, 0, WINDOW_WIDTH, GRID_ROWS * CELL_SIZE, rgb(18, 38, 31));

    for (y = 0; y < GRID_ROWS; y++) {
        for (x = 0; x < GRID_COLS; x++) {
            RECT cell = {
                x * CELL_SIZE,
                y * CELL_SIZE,
                (x + 1) * CELL_SIZE,
                (y + 1) * CELL_SIZE
            };

            if ((x + y) % 2 == 0) {
                fill_rect(hdc, cell.left, cell.top, cell.right, cell.bottom, rgb(21, 50, 39));
            } else {
                fill_rect(hdc, cell.left, cell.top, cell.right, cell.bottom, rgb(18, 44, 35));
            }
        }
    }

    {
        RECT foodRect = {
            gApp.snake.food.x * CELL_SIZE + 4,
            gApp.snake.food.y * CELL_SIZE + 4,
            (gApp.snake.food.x + 1) * CELL_SIZE - 4,
            (gApp.snake.food.y + 1) * CELL_SIZE - 4
        };

        HBRUSH redBrush = CreateSolidBrush(rgb(221, 66, 60));
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, redBrush);
        HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        Ellipse(hdc, foodRect.left, foodRect.top, foodRect.right, foodRect.bottom);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(redBrush);
    }

    for (i = gApp.snake.length - 1; i >= 0; i--) {
        RECT snakeRect = {
            gApp.snake.body[i].x * CELL_SIZE + 2,
            gApp.snake.body[i].y * CELL_SIZE + 2,
            (gApp.snake.body[i].x + 1) * CELL_SIZE - 2,
            (gApp.snake.body[i].y + 1) * CELL_SIZE - 2
        };

        COLORREF color = (i == 0) ? rgb(121, 236, 138) : rgb(66, 190, 95);
        fill_rect(hdc, snakeRect.left, snakeRect.top, snakeRect.right, snakeRect.bottom, color);
    }
}

static void draw_status_bar(HDC hdc)
{
    RECT statusRect = { 0, GRID_ROWS * CELL_SIZE, WINDOW_WIDTH, WINDOW_HEIGHT };
    RECT scoreRect = { 18, GRID_ROWS * CELL_SIZE + 12, WINDOW_WIDTH - 18, GRID_ROWS * CELL_SIZE + 38 };
    RECT infoRect = { 18, GRID_ROWS * CELL_SIZE + 38, WINDOW_WIDTH - 18, WINDOW_HEIGHT - 8 };
    char scoreText[128];

    fill_rect(hdc, statusRect.left, statusRect.top, statusRect.right, statusRect.bottom, rgb(14, 24, 33));

    snprintf(scoreText, sizeof(scoreText), "Jogador: %s   |   Pontos: %d", gApp.playerName, gApp.snake.score);

    draw_text_center(hdc, scoreText, scoreRect, gApp.textFont, rgb(234, 252, 255));
    draw_text_center(hdc, "Setas movem a cobrinha | Esc volta ao menu", infoRect, gApp.smallFont, rgb(193, 212, 204));
}

static void draw_records(HDC hdc)
{
    RECT titleRect = { 0, 24, WINDOW_WIDTH, 80 };
    RECT recordsRect = { 70, 110, WINDOW_WIDTH - 70, WINDOW_HEIGHT - 80 };
    RECT hintRect = { 0, WINDOW_HEIGHT - 48, WINDOW_WIDTH, WINDOW_HEIGHT - 12 };

    draw_text_center(hdc, "Recordes", titleRect, gApp.titleFont, rgb(248, 242, 220));

    fill_rect(hdc, recordsRect.left, recordsRect.top, recordsRect.right, recordsRect.bottom, rgb(24, 46, 62));

    {
        RECT txtRect = { recordsRect.left + 18, recordsRect.top + 14, recordsRect.right - 18, recordsRect.bottom - 14 };
        HFONT oldFont = (HFONT)SelectObject(hdc, gApp.textFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, rgb(226, 246, 252));
        DrawTextA(hdc, gApp.recordsText, -1, &txtRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
        SelectObject(hdc, oldFont);
    }

    draw_text_center(hdc, "Esc ou Enter para voltar", hintRect, gApp.smallFont, rgb(193, 212, 204));
}

static void draw_help(HDC hdc)
{
    const char *helpText =
        "1. Use as setas para mover a cobrinha.\n"
        "2. Coma os pontos vermelhos para crescer.\n"
        "3. Nao bata nas paredes nem no proprio corpo.\n"
        "4. Seu placar e salvo quando acabar.\n\n"
        "Esc volta ao menu principal.";

    RECT titleRect = { 0, 40, WINDOW_WIDTH, 92 };
    RECT helpRect = { 70, 120, WINDOW_WIDTH - 70, WINDOW_HEIGHT - 70 };

    draw_text_center(hdc, "Como jogar", titleRect, gApp.titleFont, rgb(248, 242, 220));
    fill_rect(hdc, helpRect.left, helpRect.top, helpRect.right, helpRect.bottom, rgb(24, 46, 62));

    {
        RECT txtRect = { helpRect.left + 18, helpRect.top + 16, helpRect.right - 18, helpRect.bottom - 16 };
        HFONT oldFont = (HFONT)SelectObject(hdc, gApp.textFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, rgb(226, 246, 252));
        DrawTextA(hdc, helpText, -1, &txtRect, DT_LEFT | DT_TOP | DT_WORDBREAK);
        SelectObject(hdc, oldFont);
    }
}

static void draw_game_over(HDC hdc)
{
    RECT titleRect = { 0, 80, WINDOW_WIDTH, 132 };
    RECT boxRect = { WINDOW_WIDTH / 2 - 250, 170, WINDOW_WIDTH / 2 + 250, 360 };
    RECT hintRect = { 0, 390, WINDOW_WIDTH, 430 };
    char scoreText[128];

    draw_text_center(hdc, "GAME OVER", titleRect, gApp.titleFont, rgb(248, 122, 110));
    fill_rect(hdc, boxRect.left, boxRect.top, boxRect.right, boxRect.bottom, rgb(24, 46, 62));

    snprintf(scoreText, sizeof(scoreText), "%s terminou com %d pontos.", gApp.playerName, gApp.snake.score);

    {
        RECT txtRect = { boxRect.left + 24, boxRect.top + 30, boxRect.right - 24, boxRect.bottom - 24 };
        HFONT oldFont = (HFONT)SelectObject(hdc, gApp.textFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, rgb(240, 251, 255));
        DrawTextA(hdc, scoreText, -1, &txtRect, DT_CENTER | DT_TOP | DT_WORDBREAK);
        DrawTextA(hdc, "Enter para voltar ao menu", -1, &txtRect, DT_CENTER | DT_BOTTOM | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
    }

    draw_text_center(hdc, "Recorde salvo automaticamente", hintRect, gApp.smallFont, rgb(193, 212, 204));
}

static void render(HDC hdc)
{
    fill_rect(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, rgb(11, 21, 30));

    if (gApp.screen == SCREEN_MENU) {
        draw_menu(hdc);
    } else if (gApp.screen == SCREEN_NAME_INPUT) {
        draw_name_input(hdc);
    } else if (gApp.screen == SCREEN_PLAYING) {
        draw_playfield(hdc);
        draw_status_bar(hdc);
    } else if (gApp.screen == SCREEN_RECORDS) {
        draw_records(hdc);
    } else if (gApp.screen == SCREEN_HELP) {
        draw_help(hdc);
    } else if (gApp.screen == SCREEN_GAME_OVER) {
        draw_game_over(hdc);
    }
}

static void set_direction(int dx, int dy)
{
    if (gApp.screen != SCREEN_PLAYING) {
        return;
    }

    if (gApp.snake.length > 1 && gApp.snake.dirX == -dx && gApp.snake.dirY == -dy) {
        return;
    }

    gApp.snake.dirX = dx;
    gApp.snake.dirY = dy;
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        gApp.hwnd = hwnd;
        gApp.screen = SCREEN_MENU;
        gApp.menuIndex = 0;
        gApp.playerName[0] = '\0';
        gApp.playerNameLen = 0;
        load_records();

        gApp.titleFont = CreateFontA(42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");

        gApp.textFont = CreateFontA(26, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");

        gApp.smallFont = CreateFontA(19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");

        SetTimer(hwnd, 1, 110, NULL);
        return 0;

    case WM_TIMER:
        update_game();
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_CHAR:
        if (gApp.screen == SCREEN_NAME_INPUT) {
            char c = (char)wParam;

            if (c >= 32 && c <= 126 && gApp.playerNameLen < PLAYER_NAME_MAX) {
                gApp.playerName[gApp.playerNameLen++] = c;
                gApp.playerName[gApp.playerNameLen] = '\0';
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;

    case WM_KEYDOWN:
        if (gApp.screen == SCREEN_MENU) {
            if (wParam == VK_UP) {
                if (gApp.menuIndex > 0) {
                    gApp.menuIndex--;
                }
            } else if (wParam == VK_DOWN) {
                if (gApp.menuIndex < 3) {
                    gApp.menuIndex++;
                }
            } else if (wParam == VK_RETURN) {
                if (gApp.menuIndex == 0) {
                    gApp.screen = SCREEN_NAME_INPUT;
                    gApp.playerName[0] = '\0';
                    gApp.playerNameLen = 0;
                } else if (gApp.menuIndex == 1) {
                    load_records();
                    gApp.screen = SCREEN_RECORDS;
                } else if (gApp.menuIndex == 2) {
                    gApp.screen = SCREEN_HELP;
                } else {
                    PostQuitMessage(0);
                }
            }
        } else if (gApp.screen == SCREEN_NAME_INPUT) {
            if (wParam == VK_BACK) {
                if (gApp.playerNameLen > 0) {
                    gApp.playerNameLen--;
                    gApp.playerName[gApp.playerNameLen] = '\0';
                }
            } else if (wParam == VK_RETURN) {
                if (gApp.playerNameLen > 0) {
                    start_game();
                }
            } else if (wParam == VK_ESCAPE) {
                gApp.screen = SCREEN_MENU;
            }
        } else if (gApp.screen == SCREEN_PLAYING) {
            if (wParam == VK_UP) {
                set_direction(0, -1);
            } else if (wParam == VK_DOWN) {
                set_direction(0, 1);
            } else if (wParam == VK_LEFT) {
                set_direction(-1, 0);
            } else if (wParam == VK_RIGHT) {
                set_direction(1, 0);
            } else if (wParam == VK_ESCAPE) {
                gApp.screen = SCREEN_MENU;
            }
        } else if (gApp.screen == SCREEN_RECORDS || gApp.screen == SCREEN_HELP || gApp.screen == SCREEN_GAME_OVER) {
            if (wParam == VK_ESCAPE || wParam == VK_RETURN) {
                gApp.screen = SCREEN_MENU;
            }
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBmp = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        render(memDC);
        BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (gApp.titleFont != NULL) {
            DeleteObject(gApp.titleFont);
        }
        if (gApp.textFont != NULL) {
            DeleteObject(gApp.textFont);
        }
        if (gApp.smallFont != NULL) {
            DeleteObject(gApp.smallFont);
        }
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int main(void)
{
    WNDCLASSA wc;
    MSG msg;
    HWND hwnd;

    srand((unsigned int)time(NULL));

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "SnakeWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Falha ao registrar janela.", "Erro", MB_ICONERROR | MB_OK);
        return 1;
    }

    hwnd = CreateWindowExA(
        0,
        "SnakeWindowClass",
        "Fernanda Game Snake",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WINDOW_WIDTH + 16,
        WINDOW_HEIGHT + 39,
        NULL,
        NULL,
        wc.hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBoxA(NULL, "Falha ao criar janela.", "Erro", MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
