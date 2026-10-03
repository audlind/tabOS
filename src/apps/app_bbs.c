#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "os.h"
#include "audio.h"

#define MAX_TERM_COLS 100
#define MAX_TERM_ROWS 40

/* Nettverkstilstander */
typedef enum {
    CONN_STATE_DISCONNECTED,
    CONN_STATE_CONNECTING,
    CONN_STATE_CONNECTED,
    CONN_STATE_ERROR,
    CONN_STATE_DEMO
} ConnState;

static ConnState conn_state = CONN_STATE_DISCONNECTED;
static int sock_fd = -1;
static char status_text[64] = "Frakoblet";
static uint32_t conn_timer = 0;

/* Terminal buffer */
static char term_grid[MAX_TERM_ROWS][MAX_TERM_COLS];
static uint8_t term_fg[MAX_TERM_ROWS][MAX_TERM_COLS];
static uint8_t term_bg[MAX_TERM_ROWS][MAX_TERM_COLS];

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t cur_fg = ANSI_LIGHT_GREEN;
static uint8_t cur_bg = ANSI_BLACK;

/* ANSI Parser tilstand */
typedef enum {
    PARSE_NORMAL,
    PARSE_ESCAPE,
    PARSE_CSI
} ParseState;

static ParseState parse_state = PARSE_NORMAL;
static char csi_params[32];
static int csi_param_len = 0;

/* Telnet IAC tilstand */
typedef enum {
    TELNET_NORMAL,
    TELNET_IAC,
    TELNET_DO,
    TELNET_DONT,
    TELNET_WILL,
    TELNET_WONT,
    TELNET_SB
} TelnetState;

static TelnetState telnet_state = TELNET_NORMAL;

/* Touch tastatur tilstand */
static int pressed_key_id = -1;
static bool shift_active = false;

/* Prototype funksjoner */
static void bbs_connect(void);
static void bbs_disconnect(void);
static void bbs_send_data(const char *data, size_t len);
static void bbs_term_clear(void);
static void bbs_term_putc(char c);
static void bbs_term_puts(const char *s);
static void bbs_handle_csi(char cmd);
static void bbs_start_demo(void);

static void bbs_start(void) {
    bbs_term_clear();
    bbs_term_puts("\033[36m============================================================\r\n");
    bbs_term_puts("      tabOS CYBERDECK BBS TERMINAL - TELEHACK.COM:23       \r\n");
    bbs_term_puts("============================================================\033[0m\r\n");
    bbs_term_puts("Kobler til telehack.com over port 23...\r\n");
    
    bbs_connect();
}

static void bbs_exit(void) {
    bbs_disconnect();
}

static void bbs_term_clear(void) {
    for (int r = 0; r < MAX_TERM_ROWS; r++) {
        for (int c = 0; c < MAX_TERM_COLS; c++) {
            term_grid[r][c] = ' ';
            term_fg[r][c] = cur_fg;
            term_bg[r][c] = cur_bg;
        }
    }
    cursor_x = 0;
    cursor_y = 0;
}

static void bbs_scroll_up(int viewport_rows) {
    for (int r = 0; r < viewport_rows - 1; r++) {
        for (int c = 0; c < MAX_TERM_COLS; c++) {
            term_grid[r][c] = term_grid[r + 1][c];
            term_fg[r][c]   = term_fg[r + 1][c];
            term_bg[r][c]   = term_bg[r + 1][c];
        }
    }
    for (int c = 0; c < MAX_TERM_COLS; c++) {
        term_grid[viewport_rows - 1][c] = ' ';
        term_fg[viewport_rows - 1][c]   = cur_fg;
        term_bg[viewport_rows - 1][c]   = cur_bg;
    }
    cursor_y = viewport_rows - 1;
}

static void bbs_handle_csi(char cmd) {
    csi_params[csi_param_len] = '\0';
    int p1 = 0, p2 = 0;
    sscanf(csi_params, "%d;%d", &p1, &p2);

    int viewport_rows = (display_get_orientation() == ORIENTATION_PORTRAIT) ? 32 : 20;
    int viewport_cols = (display_get_orientation() == ORIENTATION_PORTRAIT) ? 60 : 100;

    switch (cmd) {
        case 'm': { /* SGR Colors */
            char *token = strtok(csi_params, ";");
            if (!token) {
                cur_fg = ANSI_LIGHT_GREEN;
                cur_bg = ANSI_BLACK;
                break;
            }
            while (token) {
                int code = atoi(token);
                if (code == 0) {
                    cur_fg = ANSI_LIGHT_GREEN;
                    cur_bg = ANSI_BLACK;
                } else if (code == 1) {
                    /* Bold/Bright */
                    if (cur_fg < 8) cur_fg += 8;
                } else if (code >= 30 && code <= 37) {
                    static const uint8_t ansi_to_tabos[8] = {
                        ANSI_BLACK, ANSI_RED, ANSI_GREEN, ANSI_BROWN,
                        ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN, ANSI_LIGHT_GRAY
                    };
                    cur_fg = ansi_to_tabos[code - 30];
                } else if (code == 39) {
                    cur_fg = ANSI_LIGHT_GREEN;
                } else if (code >= 40 && code <= 47) {
                    static const uint8_t ansi_bg_to_tabos[8] = {
                        ANSI_BLACK, ANSI_RED, ANSI_GREEN, ANSI_BROWN,
                        ANSI_BLUE, ANSI_MAGENTA, ANSI_CYAN, ANSI_LIGHT_GRAY
                    };
                    cur_bg = ansi_bg_to_tabos[code - 40];
                } else if (code == 49) {
                    cur_bg = ANSI_BLACK;
                } else if (code >= 90 && code <= 97) {
                    static const uint8_t ansi_bright_to_tabos[8] = {
                        ANSI_DARK_GRAY, ANSI_LIGHT_RED, ANSI_LIGHT_GREEN, ANSI_YELLOW,
                        ANSI_LIGHT_BLUE, ANSI_LIGHT_MAGENTA, ANSI_LIGHT_CYAN, ANSI_WHITE
                    };
                    cur_fg = ansi_bright_to_tabos[code - 90];
                }
                token = strtok(NULL, ";");
            }
            break;
        }
        case 'H':
        case 'f': { /* Cursor position */
            if (p1 < 1) p1 = 1;
            if (p2 < 1) p2 = 1;
            cursor_y = p1 - 1;
            cursor_x = p2 - 1;
            if (cursor_y >= viewport_rows) cursor_y = viewport_rows - 1;
            if (cursor_x >= viewport_cols) cursor_x = viewport_cols - 1;
            break;
        }
        case 'J': { /* Clear display */
            if (p1 == 2) {
                bbs_term_clear();
            }
            break;
        }
        case 'K': { /* Clear in line */
            for (int c = cursor_x; c < viewport_cols; c++) {
                term_grid[cursor_y][c] = ' ';
                term_fg[cursor_y][c] = cur_fg;
                term_bg[cursor_y][c] = cur_bg;
            }
            break;
        }
        case 'A': { /* Cursor Up */
            int n = (p1 > 0) ? p1 : 1;
            cursor_y -= n;
            if (cursor_y < 0) cursor_y = 0;
            break;
        }
        case 'B': { /* Cursor Down */
            int n = (p1 > 0) ? p1 : 1;
            cursor_y += n;
            if (cursor_y >= viewport_rows) cursor_y = viewport_rows - 1;
            break;
        }
        case 'C': { /* Cursor Forward */
            int n = (p1 > 0) ? p1 : 1;
            cursor_x += n;
            if (cursor_x >= viewport_cols) cursor_x = viewport_cols - 1;
            break;
        }
        case 'D': { /* Cursor Back */
            int n = (p1 > 0) ? p1 : 1;
            cursor_x -= n;
            if (cursor_x < 0) cursor_x = 0;
            break;
        }
    }
}

static void bbs_term_putc(char c) {
    int viewport_rows = (display_get_orientation() == ORIENTATION_PORTRAIT) ? 32 : 20;
    int viewport_cols = (display_get_orientation() == ORIENTATION_PORTRAIT) ? 60 : 100;

    /* ANSI Parser */
    if (parse_state == PARSE_NORMAL) {
        if (c == 0x1B) {
            parse_state = PARSE_ESCAPE;
            return;
        }
    } else if (parse_state == PARSE_ESCAPE) {
        if (c == '[') {
            parse_state = PARSE_CSI;
            csi_param_len = 0;
            return;
        } else {
            parse_state = PARSE_NORMAL;
        }
    } else if (parse_state == PARSE_CSI) {
        if ((c >= '0' && c <= '9') || c == ';') {
            if (csi_param_len < 31) {
                csi_params[csi_param_len++] = c;
            }
            return;
        } else {
            bbs_handle_csi(c);
            parse_state = PARSE_NORMAL;
            return;
        }
    }

    /* Kontrolltegn */
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= viewport_rows) {
            bbs_scroll_up(viewport_rows);
        }
        return;
    }
    if (c == '\b' || c == 0x7F) {
        if (cursor_x > 0) cursor_x--;
        return;
    }
    if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
        if (cursor_x >= viewport_cols) {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= viewport_rows) {
                bbs_scroll_up(viewport_rows);
            }
        }
        return;
    }
    if (c == '\a') {
        audio_play(SOUND_START);
        return;
    }

    /* Utskrift av synlig tegn */
    if ((uint8_t)c >= 32) {
        if (cursor_x >= viewport_cols) {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= viewport_rows) {
                bbs_scroll_up(viewport_rows);
            }
        }
        term_grid[cursor_y][cursor_x] = c;
        term_fg[cursor_y][cursor_x] = cur_fg;
        term_bg[cursor_y][cursor_x] = cur_bg;
        cursor_x++;
    }
}

static void bbs_term_puts(const char *s) {
    while (*s) {
        bbs_term_putc(*s++);
    }
}

/* Telnet IAC protokollbehandling */
static void bbs_process_incoming_byte(uint8_t b) {
    if (telnet_state == TELNET_NORMAL) {
        if (b == 0xFF) {
            telnet_state = TELNET_IAC;
        } else {
            bbs_term_putc((char)b);
        }
    } else if (telnet_state == TELNET_IAC) {
        if (b == 0xFD) { /* DO */
            telnet_state = TELNET_DO;
        } else if (b == 0xFE) { /* DONT */
            telnet_state = TELNET_DONT;
        } else if (b == 0xFB) { /* WILL */
            telnet_state = TELNET_WILL;
        } else if (b == 0xFC) { /* WONT */
            telnet_state = TELNET_WONT;
        } else if (b == 0xFA) { /* Subnegotiation */
            telnet_state = TELNET_SB;
        } else if (b == 0xFF) { /* Literal 0xFF */
            bbs_term_putc((char)0xFF);
            telnet_state = TELNET_NORMAL;
        } else {
            telnet_state = TELNET_NORMAL;
        }
    } else if (telnet_state == TELNET_DO) {
        /* Server ber oss om noe. Svar WONT med mindre det er SGA (3) */
        uint8_t reply[3] = {0xFF, (b == 3) ? (uint8_t)0xFB : (uint8_t)0xFC, b};
        bbs_send_data((const char *)reply, 3);
        telnet_state = TELNET_NORMAL;
    } else if (telnet_state == TELNET_DONT) {
        telnet_state = TELNET_NORMAL;
    } else if (telnet_state == TELNET_WILL) {
        /* Server tilbyr noe. Svar DO hvis SGA (3) eller ECHO (1) */
        uint8_t reply[3] = {0xFF, (b == 3 || b == 1) ? (uint8_t)0xFD : (uint8_t)0xFE, b};
        bbs_send_data((const char *)reply, 3);
        telnet_state = TELNET_NORMAL;
    } else if (telnet_state == TELNET_WONT) {
        telnet_state = TELNET_NORMAL;
    } else if (telnet_state == TELNET_SB) {
        if (b == 0xF0) { /* SE: Subnegotiation End */
            telnet_state = TELNET_NORMAL;
        }
    }
}

static void bbs_connect(void) {
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }

    bbs_term_clear();
    bbs_term_puts("\033[36m============================================================\r\n");
    bbs_term_puts("      tabOS CYBERDECK BBS TERMINAL - TELEHACK.COM:23       \r\n");
    bbs_term_puts("============================================================\033[0m\r\n");
    bbs_term_puts("Kobler til telehack.com over port 23...\r\n");

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        conn_state = CONN_STATE_ERROR;
        snprintf(status_text, sizeof(status_text), "Socket feil: %s", strerror(errno));
        bbs_start_demo();
        return;
    }

    /* Sett non-blocking */
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(23);

    /* Telehack IP: 64.13.139.230 */
    struct hostent *he = gethostbyname("telehack.com");
    if (he && he->h_addr_list && he->h_addr_list[0]) {
        memcpy(&saddr.sin_addr, he->h_addr_list[0], sizeof(struct in_addr));
    } else {
        /* Fallback til direkte IP dersom DNS mangler */
        inet_pton(AF_INET, "64.13.139.230", &saddr.sin_addr);
    }

    conn_state = CONN_STATE_CONNECTING;
    conn_timer = 0;
    snprintf(status_text, sizeof(status_text), "Kobler til telehack.com...");

    int res = connect(sock_fd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (res == 0) {
        conn_state = CONN_STATE_CONNECTED;
        snprintf(status_text, sizeof(status_text), "ONLINE (telehack.com:23)");
        audio_play(SOUND_START);
        /* Send innledende telnet handshake */
        uint8_t init_telnet[] = { 0xFF, 0xFD, 0x03, '\r', '\n' };
        bbs_send_data((const char *)init_telnet, sizeof(init_telnet));
    } else if (errno == EINPROGRESS) {
        /* Venter i non-blocking mode */
    } else {
        conn_state = CONN_STATE_ERROR;
        snprintf(status_text, sizeof(status_text), "Tilkoblingsfeil: %s", strerror(errno));
        bbs_start_demo();
    }
}

static void bbs_disconnect(void) {
    if (sock_fd >= 0) {
        close(sock_fd);
        sock_fd = -1;
    }
    conn_state = CONN_STATE_DISCONNECTED;
    snprintf(status_text, sizeof(status_text), "Frakoblet");
}

static void bbs_send_data(const char *data, size_t len) {
    if (conn_state == CONN_STATE_CONNECTED && sock_fd >= 0) {
        send(sock_fd, data, len, 0);
    } else if (conn_state == CONN_STATE_DEMO) {
        /* Ekkolodd i lokal demo-modus */
        for (size_t i = 0; i < len; i++) {
            if (data[i] == '\r') {
                bbs_term_puts("\r\n. Command? (Prøv f.eks: help, starwars, zork, weather, eliza)\r\n> ");
            } else if (data[i] == 0x03) {
                bbs_term_puts("^C\r\n> ");
            } else if (data[i] == 0x08 || data[i] == 0x7F) {
                bbs_term_putc('\b');
            } else {
                bbs_term_putc(data[i]);
            }
        }
    }
}

static void bbs_start_demo(void) {
    conn_state = CONN_STATE_DEMO;
    snprintf(status_text, sizeof(status_text), "LOKAL DEMO (Koble til WiFi for ekte BBS)");
    bbs_term_puts("\r\n\033[33m[!] Nettverk ikke tilgjengelig - starter lokal Telehack simulator!\033[0m\r\n");
    bbs_term_puts("Connected to TELEHACK port 167 (tabOS offline mode)\r\n");
    bbs_term_puts("There are 26657 hosts on the network.\r\n");
    bbs_term_puts("May the command line live forever.\r\n\r\n");
    bbs_term_puts("Command, one of: help, zork, starwars, eliza, cowsay, weather, newuser\r\n");
    bbs_term_puts("Type NEWUSER to create an account. Press Ctrl-C to interrupt.\r\n");
    bbs_term_puts(". \r\n> ");
}

static bool bbs_update(uint32_t delta_ms) {
    bool dirty = false;

    if (conn_state == CONN_STATE_CONNECTING) {
        conn_timer += delta_ms;
        if (conn_timer > 12000) { /* 12 sekunder timeout */
            conn_state = CONN_STATE_ERROR;
            snprintf(status_text, sizeof(status_text), "Tidsavbrudd: WiFi ikke tilkoblet");
            bbs_start_demo();
            dirty = true;
        } else {
            /* Sjekk om tilkoblingen er etablert */
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(sock_fd, &wfds);
            struct timeval tv = {0, 0};
            int sel = select(sock_fd + 1, NULL, &wfds, NULL, &tv);
            if (sel > 0) {
                int err = 0;
                socklen_t len = sizeof(err);
                getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (err == 0) {
                    conn_state = CONN_STATE_CONNECTED;
                    snprintf(status_text, sizeof(status_text), "ONLINE (telehack.com:23)");
                    audio_play(SOUND_START);
                    bbs_term_puts("\r\n\033[32m[*] Tilkoblet TELEHACK!\033[0m\r\n\r\n");
                    uint8_t init_telnet[] = { 0xFF, 0xFD, 0x03, '\r', '\n' };
                    bbs_send_data((const char *)init_telnet, sizeof(init_telnet));
                    dirty = true;
                } else {
                    conn_state = CONN_STATE_ERROR;
                    snprintf(status_text, sizeof(status_text), "Tilkoblingsfeil: %s", strerror(err));
                    bbs_start_demo();
                    dirty = true;
                }
            }
        }
    }

    if (conn_state == CONN_STATE_CONNECTED && sock_fd >= 0) {
        uint8_t buf[256];
        ssize_t bytes = recv(sock_fd, buf, sizeof(buf), MSG_DONTWAIT);
        if (bytes > 0) {
            for (ssize_t i = 0; i < bytes; i++) {
                bbs_process_incoming_byte(buf[i]);
            }
            dirty = true;
        } else if (bytes == 0) {
            /* Server lukket forbindelsen */
            bbs_disconnect();
            snprintf(status_text, sizeof(status_text), "Forbindelse lukket av tjener");
            bbs_term_puts("\r\n\033[31m[!] Telehack koblet fra.\033[0m\r\n");
            dirty = true;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                bbs_disconnect();
                snprintf(status_text, sizeof(status_text), "Nettverksfeil: %s", strerror(errno));
                dirty = true;
            }
        }
    }

    return dirty;
}

/* Helper for å tegne tast */
static void draw_bbs_key(int col, int row, int w, int h, const char *label, bool pressed, AnsiColor fg, AnsiColor bg) {
    AnsiColor final_fg = pressed ? ANSI_BLACK : fg;
    AnsiColor final_bg = pressed ? ANSI_LIGHT_CYAN : bg;
    AnsiColor border_col = pressed ? ANSI_YELLOW : ANSI_DARK_GRAY;

    int c2 = col + w - 1;
    int r2 = row + h - 1;

    display_put_cell(col, row, 0xDA, border_col, final_bg);
    display_put_cell(c2,  row, 0xBF, border_col, final_bg);
    display_put_cell(col, r2,  0xC0, border_col, final_bg);
    display_put_cell(c2,  r2,  0xD9, border_col, final_bg);

    for (int c = col + 1; c < c2; c++) {
        display_put_cell(c, row, 0xC4, border_col, final_bg);
        display_put_cell(c, r2,  0xC4, border_col, final_bg);
    }
    for (int r = row + 1; r < r2; r++) {
        display_put_cell(col, r, 0xB3, border_col, final_bg);
        display_put_cell(c2,  r, 0xB3, border_col, final_bg);
        for (int c = col + 1; c < c2; c++) {
            display_put_cell(c, r, ' ', final_fg, final_bg);
        }
    }

    int label_r = row + (h - 1) / 2;
    int label_c = col + (w - (int)strlen(label)) / 2;
    display_draw_string(label_c, label_r, label, final_fg, final_bg);
}

static void bbs_render_portrait(void) {
    display_clear(ANSI_BLACK);

    /* 1. Header (Rad 0..1) */
    display_draw_box(0, 0, 60, 2, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(1, 0, "TELEHACK BBS", ANSI_WHITE, ANSI_BLUE);
    
    char status_sub[32];
    snprintf(status_sub, sizeof(status_sub), "[%s]", (conn_state == CONN_STATE_CONNECTED) ? "ONLINE" : (conn_state == CONN_STATE_CONNECTING) ? "KOBLER..." : "DEMO");
    display_draw_string(15, 0, status_sub, (conn_state == CONN_STATE_CONNECTED) ? ANSI_LIGHT_GREEN : ANSI_YELLOW, ANSI_BLUE);

    display_draw_button(46, 0, 13, "[X RETUR]", ANSI_WHITE, ANSI_RED, (pressed_key_id == 99));

    /* 2. Terminal Visningsvindu (Rad 2..33 = 32 rader x 60 kolonner) */
    for (int r = 0; r < 32; r++) {
        for (int c = 0; c < 60; c++) {
            char glyph = term_grid[r][c];
            uint8_t fg = term_fg[r][c];
            uint8_t bg = term_bg[r][c];

            /* Tegn blinkende / synlig markør */
            if (r == cursor_y && c == cursor_x) {
                glyph = 0xDB; /* Full solid blokk */
                fg = ANSI_LIGHT_GREEN;
                bg = ANSI_BLACK;
            }
            display_put_cell(c, r + 2, (uint8_t)glyph, fg, bg);
        }
    }

    /* 3. Hurtigmakroer / BBS Kommandoer (Rad 34) */
    display_draw_button(0,  34, 10, "[HELP]",    ANSI_WHITE, ANSI_BLUE,    (pressed_key_id == 101));
    display_draw_button(10, 34, 10, "[ZORK]",    ANSI_WHITE, ANSI_BLUE,    (pressed_key_id == 102));
    display_draw_button(20, 34, 12, "[STARWARS]",ANSI_WHITE, ANSI_BLUE,    (pressed_key_id == 103));
    display_draw_button(32, 34, 11, "[WEATHER]", ANSI_WHITE, ANSI_BLUE,    (pressed_key_id == 104));
    display_draw_button(43, 34, 9,  "[ELIZA]",   ANSI_WHITE, ANSI_BLUE,    (pressed_key_id == 105));
    display_draw_button(52, 34, 8,  "[USER]",    ANSI_WHITE, ANSI_MAGENTA, (pressed_key_id == 106));

    /* 4. Touch-tastatur (Rad 35..49 = 15 rader) */
    int kw = 5;
    int kh = 3;

    /* Rad 1: Tall 1..0, ?, / (Rad 35..37) */
    const char *r1[12] = {"1","2","3","4","5","6","7","8","9","0","?","/"};
    for (int i = 0; i < 12; i++) {
        draw_bbs_key(i * kw, 35, kw, kh, r1[i], (pressed_key_id == i + 1), ANSI_WHITE, ANSI_DARK_GRAY);
    }

    /* Rad 2: QWERTY (Rad 38..40) */
    const char *r2[10] = {"Q","W","E","R","T","Y","U","I","O","P"};
    for (int i = 0; i < 10; i++) {
        draw_bbs_key(5 + i * kw, 38, kw, kh, r2[i], (pressed_key_id == i + 20), ANSI_LIGHT_CYAN, ANSI_DARK_GRAY);
    }

    /* Rad 3: ASDFGHJKL . (Rad 41..43) */
    const char *r3[10] = {"A","S","D","F","G","H","J","K","L","."};
    for (int i = 0; i < 10; i++) {
        draw_bbs_key(5 + i * kw, 41, kw, kh, r3[i], (pressed_key_id == i + 40), ANSI_LIGHT_GREEN, ANSI_DARK_GRAY);
    }

    /* Rad 4: CTRL-C, ZXCVBNM, BACK (Rad 44..46) */
    draw_bbs_key(0, 44, 8, kh, "^C", (pressed_key_id == 60), ANSI_WHITE, ANSI_RED);
    const char *r4[7] = {"Z","X","C","V","B","N","M"};
    for (int i = 0; i < 7; i++) {
        draw_bbs_key(8 + i * 6, 44, 6, kh, r4[i], (pressed_key_id == i + 61), ANSI_YELLOW, ANSI_DARK_GRAY);
    }
    draw_bbs_key(50, 44, 10, kh, "SLETT", (pressed_key_id == 70), ANSI_WHITE, ANSI_RED);

    /* Rad 5: MELLOMROM, ENTER, RECONNECT (Rad 47..49) */
    draw_bbs_key(0, 47, 12, kh, "KOBL-TIL", (pressed_key_id == 80), ANSI_WHITE, ANSI_BLUE);
    draw_bbs_key(13, 47, 30, kh, "MELLOMROM", (pressed_key_id == 81), ANSI_WHITE, ANSI_DARK_GRAY);
    draw_bbs_key(44, 47, 16, kh, "ENTER", (pressed_key_id == 82), ANSI_WHITE, ANSI_GREEN);
}

static void bbs_render_landscape(void) {
    display_clear(ANSI_BLACK);

    /* 1. Header (Rad 0..1) */
    display_draw_box(0, 0, 100, 2, "", ANSI_WHITE, ANSI_BLUE, ANSI_YELLOW);
    display_draw_string(2, 0, "TELEHACK ONLINE BBS TERMINAL (telehack.com:23)", ANSI_WHITE, ANSI_BLUE);
    
    char status_sub[48];
    snprintf(status_sub, sizeof(status_sub), "STATUS: %s", status_text);
    display_draw_string(52, 0, status_sub, ANSI_YELLOW, ANSI_BLUE);
    display_draw_button(86, 0, 13, "[X RETUR]", ANSI_WHITE, ANSI_RED, (pressed_key_id == 99));

    /* 2. Terminal Visningsvindu (Rad 2..20 = 19 rader x 100 kolonner) */
    for (int r = 0; r < 19; r++) {
        for (int c = 0; c < 100; c++) {
            char glyph = term_grid[r][c];
            uint8_t fg = term_fg[r][c];
            uint8_t bg = term_bg[r][c];

            if (r == cursor_y && c == cursor_x) {
                glyph = 0xDB;
                fg = ANSI_LIGHT_GREEN;
                bg = ANSI_BLACK;
            }
            display_put_cell(c, r + 2, (uint8_t)glyph, fg, bg);
        }
    }

    /* 3. Hurtigknapper (Rad 21) */
    display_draw_button(2,  21, 12, "[HELP]",    ANSI_WHITE, ANSI_BLUE, (pressed_key_id == 101));
    display_draw_button(15, 21, 12, "[ZORK]",    ANSI_WHITE, ANSI_BLUE, (pressed_key_id == 102));
    display_draw_button(28, 21, 14, "[STARWARS]",ANSI_WHITE, ANSI_BLUE, (pressed_key_id == 103));
    display_draw_button(43, 21, 13, "[WEATHER]", ANSI_WHITE, ANSI_BLUE, (pressed_key_id == 104));
    display_draw_button(57, 21, 12, "[ELIZA]",   ANSI_WHITE, ANSI_BLUE, (pressed_key_id == 105));
    display_draw_button(70, 21, 14, "[NEWUSER]", ANSI_WHITE, ANSI_MAGENTA, (pressed_key_id == 106));
    display_draw_button(85, 21, 13, "[KOBLE TIL]", ANSI_WHITE, ANSI_CYAN, (pressed_key_id == 80));

    /* 4. Tastatur i bunn (Rad 22..29) */
    /* Rad 1: Tall 1..0, ? (Rad 22..24) */
    for (int i = 0; i < 10; i++) {
        char buf[2] = {'0' + (i + 1) % 10, '\0'};
        draw_bbs_key(5 + i * 9, 22, 8, 3, buf, (pressed_key_id == i + 1), ANSI_WHITE, ANSI_DARK_GRAY);
    }

    /* Rad 2: QWERTY (Rad 25..27) */
    const char *r2[10] = {"Q","W","E","R","T","Y","U","I","O","P"};
    for (int i = 0; i < 10; i++) {
        draw_bbs_key(5 + i * 9, 25, 8, 3, r2[i], (pressed_key_id == i + 20), ANSI_LIGHT_CYAN, ANSI_DARK_GRAY);
    }

    /* Rad 3: Kontroller (Rad 28..29) */
    display_draw_button(2,  28, 14, "[Ctrl-C]", ANSI_WHITE, ANSI_RED, (pressed_key_id == 60));
    display_draw_button(18, 28, 16, "[SLETT]", ANSI_WHITE, ANSI_RED, (pressed_key_id == 70));
    display_draw_button(36, 28, 36, "[ MELLOMROM ]", ANSI_WHITE, ANSI_DARK_GRAY, (pressed_key_id == 81));
    display_draw_button(74, 28, 24, "[ ENTER ]", ANSI_WHITE, ANSI_GREEN, (pressed_key_id == 82));
}

static void bbs_render(void) {
    if (display_get_orientation() == ORIENTATION_PORTRAIT) {
        bbs_render_portrait();
    } else {
        bbs_render_landscape();
    }
}

static void bbs_touch(const TouchEvent *t) {
    ScreenOrientation orient = display_get_orientation();

    if (orient == ORIENTATION_PORTRAIT) {
        if (t->is_down) {
            /* Retur */
            if (input_hit_box(t, 46, 0, 13, 2)) {
                pressed_key_id = 99;
                return;
            }

            /* Hurtigmakroer rad 34 */
            if (input_hit_box(t, 0,  34, 10, 1)) { pressed_key_id = 101; return; }
            if (input_hit_box(t, 10, 34, 10, 1)) { pressed_key_id = 102; return; }
            if (input_hit_box(t, 20, 34, 12, 1)) { pressed_key_id = 103; return; }
            if (input_hit_box(t, 32, 34, 11, 1)) { pressed_key_id = 104; return; }
            if (input_hit_box(t, 43, 34, 9,  1)) { pressed_key_id = 105; return; }
            if (input_hit_box(t, 52, 34, 8,  1)) { pressed_key_id = 106; return; }

            /* Tastatur Rad 1: Tall 1..0, ?, / (Rad 35..37) */
            if (t->grid_row >= 35 && t->grid_row <= 37) {
                int idx = t->grid_col / 5;
                if (idx >= 0 && idx < 12) {
                    pressed_key_id = idx + 1;
                    return;
                }
            }

            /* Tastatur Rad 2: QWERTY (Rad 38..40) */
            if (t->grid_row >= 38 && t->grid_row <= 40) {
                int idx = (t->grid_col - 5) / 5;
                if (idx >= 0 && idx < 10) {
                    pressed_key_id = idx + 20;
                    return;
                }
            }

            /* Tastatur Rad 3: ASDFGHJKL . (Rad 41..43) */
            if (t->grid_row >= 41 && t->grid_row <= 43) {
                int idx = (t->grid_col - 5) / 5;
                if (idx >= 0 && idx < 10) {
                    pressed_key_id = idx + 40;
                    return;
                }
            }

            /* Tastatur Rad 4: CTRL-C, ZXCVBNM, BACK (Rad 44..46) */
            if (t->grid_row >= 44 && t->grid_row <= 46) {
                if (t->grid_col < 8) {
                    pressed_key_id = 60; /* Ctrl-C */
                    return;
                } else if (t->grid_col >= 50) {
                    pressed_key_id = 70; /* Slett */
                    return;
                } else {
                    int idx = (t->grid_col - 8) / 6;
                    if (idx >= 0 && idx < 7) {
                        pressed_key_id = idx + 61;
                        return;
                    }
                }
            }

            /* Tastatur Rad 5: KOBL-TIL, MELLOMROM, ENTER (Rad 47..49) */
            if (t->grid_row >= 47 && t->grid_row <= 49) {
                if (t->grid_col < 13) {
                    pressed_key_id = 80; /* Koble til */
                    return;
                } else if (t->grid_col < 44) {
                    pressed_key_id = 81; /* Mellomrom */
                    return;
                } else {
                    pressed_key_id = 82; /* Enter */
                    return;
                }
            }
        } else if (t->just_up) {
            int key = pressed_key_id;
            pressed_key_id = -1;

            if (key == 99) {
                os_switch_app(&app_launcher);
                return;
            }

            /* Makroer */
            if (key == 101) { bbs_send_data("help\r", 5); return; }
            if (key == 102) { bbs_send_data("zork\r", 5); return; }
            if (key == 103) { bbs_send_data("starwars\r", 9); return; }
            if (key == 104) { bbs_send_data("weather\r", 8); return; }
            if (key == 105) { bbs_send_data("eliza\r", 6); return; }
            if (key == 106) { bbs_send_data("newuser\r", 8); return; }

            /* Tall 1..0, ?, / */
            if (key >= 1 && key <= 12) {
                const char nums[] = "1234567890?/";
                char c = nums[key - 1];
                bbs_send_data(&c, 1);
                return;
            }

            /* QWERTY */
            if (key >= 20 && key <= 29) {
                const char *r2 = "qwertyuiop";
                char c = r2[key - 20];
                bbs_send_data(&c, 1);
                return;
            }

            /* ASDFGHJKL . */
            if (key >= 40 && key <= 49) {
                const char *r3 = "asdfghjkl.";
                char c = r3[key - 40];
                bbs_send_data(&c, 1);
                return;
            }

            /* ZXCVBNM */
            if (key >= 61 && key <= 67) {
                const char *r4 = "zxcvbnm";
                char c = r4[key - 61];
                bbs_send_data(&c, 1);
                return;
            }

            /* Kontroller */
            if (key == 60) { /* Ctrl-C */
                char c = 0x03;
                bbs_send_data(&c, 1);
                return;
            }
            if (key == 70) { /* Slett / Backspace */
                char c = 0x08;
                bbs_send_data(&c, 1);
                return;
            }
            if (key == 80) { /* Koble til */
                bbs_connect();
                return;
            }
            if (key == 81) { /* Space */
                char c = ' ';
                bbs_send_data(&c, 1);
                return;
            }
            if (key == 82) { /* Enter */
                char c = '\r';
                bbs_send_data(&c, 1);
                return;
            }
        }
    } else {
        /* Landskap touch */
        if (t->is_down) {
            if (input_hit_box(t, 86, 0, 13, 2)) { pressed_key_id = 99; return; }

            if (input_hit_box(t, 2,  21, 12, 1)) { pressed_key_id = 101; return; }
            if (input_hit_box(t, 15, 21, 12, 1)) { pressed_key_id = 102; return; }
            if (input_hit_box(t, 28, 21, 14, 1)) { pressed_key_id = 103; return; }
            if (input_hit_box(t, 43, 21, 13, 1)) { pressed_key_id = 104; return; }
            if (input_hit_box(t, 57, 21, 12, 1)) { pressed_key_id = 105; return; }
            if (input_hit_box(t, 70, 21, 14, 1)) { pressed_key_id = 106; return; }
            if (input_hit_box(t, 85, 21, 13, 1)) { pressed_key_id = 80;  return; }

            if (input_hit_box(t, 2,  28, 14, 2)) { pressed_key_id = 60; return; }
            if (input_hit_box(t, 18, 28, 16, 2)) { pressed_key_id = 70; return; }
            if (input_hit_box(t, 36, 28, 36, 2)) { pressed_key_id = 81; return; }
            if (input_hit_box(t, 74, 28, 24, 2)) { pressed_key_id = 82; return; }
        } else if (t->just_up) {
            int key = pressed_key_id;
            pressed_key_id = -1;

            if (key == 99) { os_switch_app(&app_launcher); return; }
            if (key == 101) { bbs_send_data("help\r", 5); return; }
            if (key == 102) { bbs_send_data("zork\r", 5); return; }
            if (key == 103) { bbs_send_data("starwars\r", 9); return; }
            if (key == 104) { bbs_send_data("weather\r", 8); return; }
            if (key == 105) { bbs_send_data("eliza\r", 6); return; }
            if (key == 106) { bbs_send_data("newuser\r", 8); return; }
            if (key == 80)  { bbs_connect(); return; }
            if (key == 60)  { char c = 0x03; bbs_send_data(&c, 1); return; }
            if (key == 70)  { char c = 0x08; bbs_send_data(&c, 1); return; }
            if (key == 81)  { char c = ' '; bbs_send_data(&c, 1); return; }
            if (key == 82)  { char c = '\r'; bbs_send_data(&c, 1); return; }
        }
    }
}

App app_bbs = {
    .name = "Telehack BBS",
    .title = "Telehack Online Terminal",
    .on_start = bbs_start,
    .on_update = bbs_update,
    .on_render = bbs_render,
    .on_touch = bbs_touch,
    .on_resize = NULL,
    .on_exit = bbs_exit
};
