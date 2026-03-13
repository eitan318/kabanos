// AI generated source code for editor
//
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <termios.h>
#include <unistd.h>

/* ───────────── constants ───────────── */
#define MAX_LINES 65536
#define MAX_LINE_LEN 4096
#define UNDO_DEPTH 100
#define TAB_STOP 8
#define VERSION "0.1"

/* ───────────── types ───────────── */
typedef struct {
  char **lines;
  int count;
} Buffer;

typedef struct {
  Buffer buf;
  int cx, cy;         /* cursor col, row (0-based) */
  int rx;             /* rendered x (accounting for tabs) */
  int rowoff, coloff; /* scroll offsets */
  int screenrows, screencols;
  int mode; /* 0=normal 1=insert 2=command */
  char filename[256];
  int dirty;
  char status[256];
  char cmdbuf[256]; /* : command accumulation */
  int cmdlen;
  /* undo */
  Buffer undo_stack[UNDO_DEPTH];
  int undo_top;
  /* search */
  char search[256];
  int search_dir; /* 1 = forward, -1 = backward */
  /* yank */
  char *yank_line;
} Editor;

/* ───────────── globals ───────────── */
static Editor E;
// static struct termios orig_termios;

/* ───────────── terminal ───────────── */
static void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
  perror(s);
  exit(1);
}

// static void disable_raw(void) {
//   tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
//   write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
//   write(STDOUT_FILENO, "\x1b[?25h", 6);
// }
// static void enable_raw(void) {
//   if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
//     die("tcgetattr");
//   atexit(disable_raw);
//   struct termios raw = orig_termios;
//   raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
//   raw.c_oflag &= ~(OPOST);
//   raw.c_cflag |= (CS8);
//   raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
//   raw.c_cc[VMIN] = 0;
//   raw.c_cc[VTIME] = 1;
//   if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
//     die("tcsetattr");
// }
//
/* ───────────── key reading ───────────── */
enum {
  KEY_ESC = 27,
  KEY_UP = 1000,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,
  KEY_DEL,
  KEY_HOME,
  KEY_END,
  KEY_PGUP,
  KEY_PGDN,
  KEY_BACKSPACE = 127,
  KEY_CTRL_H = 8
};

static int read_key(void) {
  char c;
  int n;
  while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
    if (n == -1)
      die("read");
  }
  if (c == '\x1b') {
    char seq[4] = {0};
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return KEY_ESC;
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return KEY_ESC;
    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        read(STDIN_FILENO, &seq[2], 1);
        if (seq[2] == '~') {
          switch (seq[1]) {
          case '1':
            return KEY_HOME;
          case '3':
            return KEY_DEL;
          case '4':
            return KEY_END;
          case '5':
            return KEY_PGUP;
          case '6':
            return KEY_PGDN;
          case '7':
            return KEY_HOME;
          case '8':
            return KEY_END;
          }
        }
      } else {
        switch (seq[1]) {
        case 'A':
          return KEY_UP;
        case 'B':
          return KEY_DOWN;
        case 'C':
          return KEY_RIGHT;
        case 'D':
          return KEY_LEFT;
        case 'H':
          return KEY_HOME;
        case 'F':
          return KEY_END;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
      case 'H':
        return KEY_HOME;
      case 'F':
        return KEY_END;
      }
    }
    return KEY_ESC;
  }
  return (unsigned char)c;
}

/* ───────────── buffer helpers ───────────── */
static Buffer buf_clone(Buffer *b) {
  Buffer n;
  n.count = b->count;
  n.lines = malloc(sizeof(char *) * (b->count + 1));
  for (int i = 0; i < b->count; i++)
    n.lines[i] = strdup(b->lines[i]);
  return n;
}
static void buf_free(Buffer *b) {
  if (!b->lines)
    return;
  for (int i = 0; i < b->count; i++)
    free(b->lines[i]);
  free(b->lines);
  b->lines = NULL;
  b->count = 0;
}
static void buf_insert_line(Buffer *b, int at, const char *txt) {
  b->lines = realloc(b->lines, sizeof(char *) * (b->count + 1));
  memmove(&b->lines[at + 1], &b->lines[at], sizeof(char *) * (b->count - at));
  b->lines[at] = strdup(txt);
  b->count++;
}
static void buf_delete_line(Buffer *b, int at) {
  free(b->lines[at]);
  memmove(&b->lines[at], &b->lines[at + 1],
          sizeof(char *) * (b->count - at - 1));
  b->count--;
}
static void buf_set_line(Buffer *b, int at, const char *txt) {
  free(b->lines[at]);
  b->lines[at] = strdup(txt);
}

/* ───────────── undo ───────────── */
static void undo_push(void) {
  int top = E.undo_top;
  buf_free(&E.undo_stack[top % UNDO_DEPTH]);
  E.undo_stack[top % UNDO_DEPTH] = buf_clone(&E.buf);
  E.undo_top++;
}
static void undo_pop(void) {
  if (E.undo_top == 0) {
    snprintf(E.status, sizeof E.status, "Already at oldest change");
    return;
  }
  E.undo_top--;
  buf_free(&E.buf);
  E.buf = buf_clone(&E.undo_stack[E.undo_top % UNDO_DEPTH]);
  if (E.cy >= E.buf.count)
    E.cy = E.buf.count - 1;
  if (E.cy < 0)
    E.cy = 0;
  E.dirty = 1;
  snprintf(E.status, sizeof E.status, "Undo");
}

/* ───────────── file i/o ───────────── */
static void file_open(const char *fname) {
  strncpy(E.filename, fname, 255);
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    /* new file */
    buf_insert_line(&E.buf, 0, "");
    return;
  }
  char line[MAX_LINE_LEN];
  while (fgets(line, sizeof line, fp)) {
    int len = strlen(line);
    if (len && line[len - 1] == '\n')
      line[--len] = '\0';
    if (len && line[len - 1] == '\r')
      line[--len] = '\0';
    buf_insert_line(&E.buf, E.buf.count, line);
  }
  fclose(fp);
  if (E.buf.count == 0)
    buf_insert_line(&E.buf, 0, "");
}
static int file_save(void) {
  if (!E.filename[0]) {
    snprintf(E.status, sizeof E.status, "No filename (use :w <file>)");
    return 0;
  }
  FILE *fp = fopen(E.filename, "w");
  if (!fp) {
    snprintf(E.status, sizeof E.status, "Can't write: %s", E.filename);
    return 0;
  }
  for (int i = 0; i < E.buf.count; i++)
    fprintf(fp, "%s\n", E.buf.lines[i]);
  fclose(fp);
  E.dirty = 0;
  snprintf(E.status, sizeof E.status, "\"%s\" %dL written", E.filename,
           E.buf.count);
  return 1;
}

/* ───────────── cursor helpers ───────────── */
static int cur_line_len(void) {
  if (E.cy >= E.buf.count)
    return 0;
  return (int)strlen(E.buf.lines[E.cy]);
}
static void clamp_cx(void) {
  int max = cur_line_len();
  if (E.mode != 1 && max > 0)
    max--; /* normal: 0..len-1 */
  if (max < 0)
    max = 0;
  if (E.cx > max)
    E.cx = max;
  if (E.cx < 0)
    E.cx = 0;
}

/* rendered x (tabs) */
static int cx_to_rx(const char *row, int cx) {
  int rx = 0;
  for (int i = 0; i < cx; i++) {
    if (row[i] == '\t')
      rx += TAB_STOP - (rx % TAB_STOP);
    else
      rx++;
  }
  return rx;
}

/* ───────────── screen output ───────────── */
#define AB_INIT                                                                \
  { NULL, 0 }
typedef struct {
  char *b;
  int len;
} Abuf;
static void ab_append(Abuf *ab, const char *s, int len) {
  ab->b = realloc(ab->b, ab->len + len);
  memcpy(ab->b + ab->len, s, len);
  ab->len += len;
}
static void ab_appendf(Abuf *ab, const char *fmt, ...) {
  char tmp[512];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
  va_end(ap);
  ab_append(ab, tmp, n);
}
static void ab_free(Abuf *ab) { free(ab->b); }

static void scroll(void) {
  /* rx */
  if (E.cy < E.buf.count)
    E.rx = cx_to_rx(E.buf.lines[E.cy], E.cx);
  else
    E.rx = 0;

  if (E.cy < E.rowoff)
    E.rowoff = E.cy;
  if (E.cy >= E.rowoff + E.screenrows)
    E.rowoff = E.cy - E.screenrows + 1;
  if (E.rx < E.coloff)
    E.coloff = E.rx;
  if (E.rx >= E.coloff + E.screencols)
    E.coloff = E.rx - E.screencols + 1;
}

static void draw_rows(Abuf *ab) {
  int numw = 4; /* width of line number column */
  for (int y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.buf.count) {
      /* draw ~ lines */
      if (E.buf.count == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int wlen = snprintf(welcome, sizeof welcome,
                            "mivim -- version %s  (stdlib only)", VERSION);
        if (wlen > E.screencols)
          wlen = E.screencols;
        int pad = (E.screencols - wlen) / 2;
        ab_appendf(ab, "\x1b[90m%*d \x1b[0m", numw, 0);
        while (pad-- > 1)
          ab_append(ab, " ", 1);
        ab_append(ab, welcome, wlen);
      } else {
        ab_appendf(ab, "\x1b[90m%*s \x1b[0m~", numw, "");
      }
    } else {
      /* line number */
      int rel = filerow - E.cy;
      if (rel == 0)
        ab_appendf(ab, "\x1b[33m%*d \x1b[0m", numw, filerow + 1);
      else
        ab_appendf(ab, "\x1b[90m%*d \x1b[0m", numw, abs(rel));

      /* line content */
      const char *line = E.buf.lines[filerow];
      int len = strlen(line);
      int drawn = 0, col = 0;
      for (int i = 0; i < len && drawn < E.screencols; i++) {
        if (line[i] == '\t') {
          int spaces = TAB_STOP - (col % TAB_STOP);
          while (spaces-- && drawn < E.screencols) {
            if (col >= E.coloff) {
              ab_append(ab, " ", 1);
              drawn++;
            }
            col++;
          }
        } else {
          if (col >= E.coloff) {
            ab_append(ab, &line[i], 1);
            drawn++;
          }
          col++;
        }
      }
    }
    ab_append(ab, "\x1b[K\r\n", 5);
  }
}

static void draw_status(Abuf *ab) {
  ab_append(ab, "\x1b[7m", 4); /* reverse video */
  char left[128], right[32];
  const char *mode_str = (E.mode == 1)   ? "INSERT"
                         : (E.mode == 2) ? "COMMAND"
                                         : "NORMAL";
  int llen =
      snprintf(left, sizeof left, " %s | %.40s%s ", mode_str,
               E.filename[0] ? E.filename : "[No Name]", E.dirty ? " [+]" : "");
  int rlen = snprintf(right, sizeof right, " %d/%d:%d ", E.cy + 1, E.buf.count,
                      E.cx + 1);
  if (llen > E.screencols)
    llen = E.screencols;
  ab_append(ab, left, llen);
  int pad = E.screencols - llen - rlen;
  while (pad-- > 0)
    ab_append(ab, " ", 1);
  if (llen + rlen <= E.screencols)
    ab_append(ab, right, rlen);
  ab_append(ab, "\x1b[m\r\n", 5);
}

static void draw_message(Abuf *ab) {
  ab_append(ab, "\x1b[K", 3);
  if (E.mode == 2) {
    /* show command buffer */
    char tmp[260];
    snprintf(tmp, sizeof tmp, ":%s", E.cmdbuf);
    ab_append(ab, tmp, strlen(tmp));
  } else {
    int len = strlen(E.status);
    if (len > E.screencols)
      len = E.screencols;
    ab_append(ab, E.status, len);
  }
}

static void refresh_screen(void) {
  scroll();
  Abuf ab = AB_INIT;
  ab_append(&ab, "\x1b[?25l\x1b[H", 9);
  draw_rows(&ab);
  draw_status(&ab);
  draw_message(&ab);

  /* position cursor */
  int numw = 5; /* numw cols + space */
  int screen_cy = E.cy - E.rowoff + 1;
  int screen_cx = E.rx - E.coloff + numw + 1;
  if (E.mode == 2) {
    ab_appendf(&ab, "\x1b[%d;%dH", E.screenrows + 2, E.cmdlen + 2);
  } else {
    ab_appendf(&ab, "\x1b[%d;%dH", screen_cy, screen_cx);
  }
  ab_append(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}

/* ───────────── editing ops ───────────── */
static void insert_char(int c) {
  if (E.cy >= E.buf.count) {
    buf_insert_line(&E.buf, E.buf.count, "");
  }
  char *line = E.buf.lines[E.cy];
  int len = strlen(line);
  if (E.cx > len)
    E.cx = len;
  char *newline = malloc(len + 2);
  memcpy(newline, line, E.cx);
  newline[E.cx] = (char)c;
  memcpy(newline + E.cx + 1, line + E.cx, len - E.cx);
  newline[len + 1] = '\0';
  free(E.buf.lines[E.cy]);
  E.buf.lines[E.cy] = newline;
  E.cx++;
  E.dirty = 1;
}

static void insert_newline(void) {
  char *line = E.buf.lines[E.cy];
  int len = strlen(line);
  char *rest = strdup(line + E.cx);
  char *first = malloc(E.cx + 1);
  memcpy(first, line, E.cx);
  first[E.cx] = '\0';
  buf_set_line(&E.buf, E.cy, first);
  free(first);
  buf_insert_line(&E.buf, E.cy + 1, rest);
  free(rest);
  E.cy++;
  E.cx = 0;
  E.dirty = 1;
}

static void delete_char_at_cursor(void) {
  if (E.cy >= E.buf.count)
    return;
  char *line = E.buf.lines[E.cy];
  int len = strlen(line);
  if (E.cx >= len) {
    /* join with next line */
    if (E.cy + 1 < E.buf.count) {
      int nlen = len + strlen(E.buf.lines[E.cy + 1]);
      char *joined = malloc(nlen + 1);
      strcpy(joined, line);
      strcat(joined, E.buf.lines[E.cy + 1]);
      buf_set_line(&E.buf, E.cy, joined);
      free(joined);
      buf_delete_line(&E.buf, E.cy + 1);
      E.dirty = 1;
    }
    return;
  }
  memmove(line + E.cx, line + E.cx + 1, len - E.cx);
  E.dirty = 1;
}

static void backspace(void) {
  if (E.cx == 0 && E.cy == 0)
    return;
  if (E.cx > 0) {
    char *line = E.buf.lines[E.cy];
    int len = strlen(line);
    memmove(line + E.cx - 1, line + E.cx, len - E.cx + 1);
    E.cx--;
    E.dirty = 1;
  } else {
    /* join with previous line */
    int prev_len = strlen(E.buf.lines[E.cy - 1]);
    int cur_len = strlen(E.buf.lines[E.cy]);
    char *joined = malloc(prev_len + cur_len + 1);
    strcpy(joined, E.buf.lines[E.cy - 1]);
    strcat(joined, E.buf.lines[E.cy]);
    buf_set_line(&E.buf, E.cy - 1, joined);
    free(joined);
    buf_delete_line(&E.buf, E.cy);
    E.cy--;
    E.cx = prev_len;
    E.dirty = 1;
  }
}

/* ───────────── search ───────────── */
static void do_search(int dir) {
  if (!E.search[0])
    return;
  int start = E.cy;
  for (int i = 0; i < E.buf.count; i++) {
    int row = (start + dir * (i + 1) + E.buf.count * 10) % E.buf.count;
    char *p = strstr(E.buf.lines[row], E.search);
    if (p) {
      E.cy = row;
      E.cx = p - E.buf.lines[row];
      E.rowoff = E.cy - E.screenrows / 2;
      if (E.rowoff < 0)
        E.rowoff = 0;
      snprintf(E.status, sizeof E.status, "/%s", E.search);
      return;
    }
  }
  snprintf(E.status, sizeof E.status, "Pattern not found: %s", E.search);
}

/* ───────────── command mode ───────────── */
static void exec_command(void) {
  char *cmd = E.cmdbuf;
  E.mode = 0;

  /* :q :q! :wq :w :w filename :set ... */
  if (strcmp(cmd, "q") == 0) {
    if (E.dirty) {
      snprintf(E.status, sizeof E.status,
               "No write since last change (use :q! to force)");
      return;
    }
    exit(0);
  } else if (strcmp(cmd, "q!") == 0) {
    exit(0);
  } else if (strcmp(cmd, "wq") == 0 || strcmp(cmd, "x") == 0) {
    if (file_save())
      exit(0);
  } else if (strncmp(cmd, "w ", 2) == 0) {
    strncpy(E.filename, cmd + 2, 255);
    file_save();
  } else if (strcmp(cmd, "w") == 0) {
    file_save();
  } else if (strncmp(cmd, "e ", 2) == 0) {
    /* re-open file */
    buf_free(&E.buf);
    E.buf.lines = malloc(sizeof(char *));
    E.buf.count = 0;
    file_open(cmd + 2);
    E.cx = E.cy = 0;
    E.dirty = 0;
  } else if (strncmp(cmd, "/", 1) == 0) {
    strncpy(E.search, cmd + 1, 255);
    E.search_dir = 1;
    do_search(1);
    return;
  } else {
    /* try line number */
    int lineno = atoi(cmd);
    if (lineno > 0) {
      E.cy = lineno - 1;
      if (E.cy >= E.buf.count)
        E.cy = E.buf.count - 1;
      E.cx = 0;
      snprintf(E.status, sizeof E.status, "");
      return;
    }
    snprintf(E.status, sizeof E.status, "Unknown command: %s", cmd);
  }
}

/* ───────────── normal mode ops ───────────── */
static int g_pending = 0; /* for 'gg' */
static int d_pending = 0; /* for 'dd' */
static int y_pending = 0; /* for 'yy' */
static int count_buf = 0; /* numeric prefix */

static void normal_key(int c) {
  /* numeric prefix */
  if (c >= '1' && c <= '9' && !g_pending && !d_pending && !y_pending) {
    count_buf = count_buf * 10 + (c - '0');
    return;
  }
  if (c == '0' && count_buf > 0) {
    count_buf = count_buf * 10;
    return;
  }
  int rep = count_buf > 0 ? count_buf : 1;
  count_buf = 0;

  /* movement */
  if (c == 'h' || c == KEY_LEFT) {
    for (int i = 0; i < rep; i++) {
      if (E.cx > 0)
        E.cx--;
    }
  } else if (c == 'l' || c == KEY_RIGHT) {
    for (int i = 0; i < rep; i++) {
      int max = cur_line_len() - 1;
      if (max < 0)
        max = 0;
      if (E.cx < max)
        E.cx++;
    }
  } else if (c == 'j' || c == KEY_DOWN) {
    for (int i = 0; i < rep; i++) {
      if (E.cy < E.buf.count - 1)
        E.cy++;
    }
    clamp_cx();
  } else if (c == 'k' || c == KEY_UP) {
    for (int i = 0; i < rep; i++) {
      if (E.cy > 0)
        E.cy--;
    }
    clamp_cx();
  } else if (c == '0' || c == KEY_HOME) {
    E.cx = 0;
  } else if (c == '$' || c == KEY_END) {
    int len = cur_line_len();
    E.cx = len > 0 ? len - 1 : 0;
  } else if (c == 'w') {
    /* word forward */
    for (int r = 0; r < rep; r++) {
      char *line = E.buf.lines[E.cy];
      int len = strlen(line);
      int x = E.cx;
      while (x < len && !isspace((unsigned char)line[x]))
        x++;
      while (x < len && isspace((unsigned char)line[x]))
        x++;
      if (x >= len && E.cy + 1 < E.buf.count) {
        E.cy++;
        x = 0;
      }
      E.cx = x;
    }
    clamp_cx();
  } else if (c == 'b') {
    /* word backward */
    for (int r = 0; r < rep; r++) {
      char *line = E.buf.lines[E.cy];
      int x = E.cx;
      if (x == 0 && E.cy > 0) {
        E.cy--;
        x = strlen(E.buf.lines[E.cy]);
      }
      if (x > 0)
        x--;
      while (x > 0 && isspace((unsigned char)line[x]))
        x--;
      while (x > 0 && !isspace((unsigned char)line[x - 1]))
        x--;
      E.cx = x;
    }
  } else if (c == 'G') {
    E.cy = rep > 1 ? rep - 1 : E.buf.count - 1;
    if (E.cy >= E.buf.count)
      E.cy = E.buf.count - 1;
    E.cx = 0;
  } else if (c == 'g') {
    if (g_pending) {
      E.cy = 0;
      E.cx = 0;
      g_pending = 0;
    } else
      g_pending = 1;
    return;
  } else if (c == KEY_PGDN) {
    E.cy += E.screenrows;
    if (E.cy >= E.buf.count)
      E.cy = E.buf.count - 1;
    clamp_cx();
  } else if (c == KEY_PGUP) {
    E.cy -= E.screenrows;
    if (E.cy < 0)
      E.cy = 0;
    clamp_cx();

    /* enter insert mode */
  } else if (c == 'i') {
    E.mode = 1;
  } else if (c == 'I') {
    E.cx = 0;
    E.mode = 1;
  } else if (c == 'a') {
    int len = cur_line_len();
    if (E.cx < len)
      E.cx++;
    E.mode = 1;
  } else if (c == 'A') {
    E.cx = cur_line_len();
    E.mode = 1;
  } else if (c == 'o') {
    undo_push();
    E.cx = cur_line_len();
    buf_insert_line(&E.buf, E.cy + 1, "");
    E.cy++;
    E.cx = 0;
    E.mode = 1;
    E.dirty = 1;
  } else if (c == 'O') {
    undo_push();
    buf_insert_line(&E.buf, E.cy, "");
    E.cx = 0;
    E.mode = 1;
    E.dirty = 1;

    /* delete */
  } else if (c == 'x') {
    undo_push();
    for (int r = 0; r < rep; r++)
      delete_char_at_cursor();
    clamp_cx();
  } else if (c == 'd') {
    if (d_pending) {
      /* dd */
      undo_push();
      for (int r = 0; r < rep; r++) {
        if (E.yank_line)
          free(E.yank_line);
        E.yank_line = strdup(E.buf.lines[E.cy]);
        buf_delete_line(&E.buf, E.cy);
        if (E.buf.count == 0)
          buf_insert_line(&E.buf, 0, "");
        if (E.cy >= E.buf.count)
          E.cy = E.buf.count - 1;
      }
      E.dirty = 1;
      d_pending = 0;
    } else {
      d_pending = 1;
      return;
    }
  } else if (c == 'D') {
    undo_push();
    char *line = E.buf.lines[E.cy];
    line[E.cx] = '\0';
    E.dirty = 1;

    /* yank */
  } else if (c == 'y') {
    if (y_pending) {
      if (E.yank_line)
        free(E.yank_line);
      E.yank_line = strdup(E.buf.lines[E.cy]);
      snprintf(E.status, sizeof E.status, "1 line yanked");
      y_pending = 0;
    } else {
      y_pending = 1;
      return;
    }
    /* paste */
  } else if (c == 'p') {
    if (E.yank_line) {
      undo_push();
      buf_insert_line(&E.buf, E.cy + 1, E.yank_line);
      E.cy++;
      E.cx = 0;
      E.dirty = 1;
    }
  } else if (c == 'P') {
    if (E.yank_line) {
      undo_push();
      buf_insert_line(&E.buf, E.cy, E.yank_line);
      E.cx = 0;
      E.dirty = 1;
    }

    /* undo */
  } else if (c == 'u') {
    undo_pop();

    /* r - replace single char */
  } else if (c == 'r') {
    int nc = read_key();
    if (nc != KEY_ESC) {
      undo_push();
      char *line = E.buf.lines[E.cy];
      int len = strlen(line);
      if (E.cx < len) {
        line[E.cx] = nc;
        E.dirty = 1;
      }
    }

    /* ~ toggle case */
  } else if (c == '~') {
    char *line = E.buf.lines[E.cy];
    int len = strlen(line);
    if (E.cx < len) {
      undo_push();
      line[E.cx] = isupper((unsigned char)line[E.cx])
                       ? tolower((unsigned char)line[E.cx])
                       : toupper((unsigned char)line[E.cx]);
      if (E.cx < len - 1)
        E.cx++;
      E.dirty = 1;
    }

    /* search */
  } else if (c == '/') {
    E.mode = 2;
    E.cmdbuf[0] = '/';
    E.cmdbuf[1] = '\0';
    E.cmdlen = 1;
    return;
  } else if (c == 'n') {
    do_search(E.search_dir);
  } else if (c == 'N') {
    do_search(-E.search_dir);

    /* colon command */
  } else if (c == ':') {
    E.mode = 2;
    E.cmdbuf[0] = '\0';
    E.cmdlen = 0;
    snprintf(E.status, sizeof E.status, "");
    return;

    /* ctrl-f / ctrl-b scroll */
  } else if (c == 6) { /* ctrl-f */
    E.cy += E.screenrows;
    if (E.cy >= E.buf.count)
      E.cy = E.buf.count - 1;
    clamp_cx();
  } else if (c == 2) { /* ctrl-b */
    E.cy -= E.screenrows;
    if (E.cy < 0)
      E.cy = 0;
    clamp_cx();
  } else if (c == 'J') { /* join lines */
    if (E.cy + 1 < E.buf.count) {
      undo_push();
      int l1 = strlen(E.buf.lines[E.cy]);
      int l2 = strlen(E.buf.lines[E.cy + 1]);
      char *joined = malloc(l1 + l2 + 2);
      strcpy(joined, E.buf.lines[E.cy]);
      if (l2) {
        joined[l1] = ' ';
        strcpy(joined + l1 + 1, E.buf.lines[E.cy + 1]);
      }
      buf_set_line(&E.buf, E.cy, joined);
      free(joined);
      buf_delete_line(&E.buf, E.cy + 1);
      E.cx = l1;
      E.dirty = 1;
    }
  }

  g_pending = 0;
  d_pending = 0;
  y_pending = 0;
  clamp_cx();
}

/* ───────────── main loop ───────────── */
static void process_key(void) {
  int c = read_key();

  if (E.mode == 0) { /* normal */
    if (c == KEY_ESC) {
      g_pending = d_pending = y_pending = count_buf = 0;
      E.status[0] = 0;
      return;
    }
    normal_key(c);

  } else if (E.mode == 1) { /* insert */
    if (c == KEY_ESC) {
      if (E.cx > 0)
        E.cx--;
      E.mode = 0;
      clamp_cx();
    } else if (c == KEY_BACKSPACE || c == KEY_CTRL_H || c == 127) {
      backspace();
    } else if (c == KEY_DEL) {
      delete_char_at_cursor();
    } else if (c == '\r' || c == '\n') {
      undo_push();
      insert_newline();
    } else if (c == KEY_UP) {
      E.cy--;
      if (E.cy < 0)
        E.cy = 0;
      clamp_cx();
    } else if (c == KEY_DOWN) {
      E.cy++;
      if (E.cy >= E.buf.count)
        E.cy = E.buf.count - 1;
      clamp_cx();
    } else if (c == KEY_LEFT) {
      if (E.cx > 0)
        E.cx--;
    } else if (c == KEY_RIGHT) {
      if (E.cx < cur_line_len())
        E.cx++;
    } else if (c == KEY_HOME) {
      E.cx = 0;
    } else if (c == KEY_END) {
      E.cx = cur_line_len();
    } else if (!iscntrl(c)) {
      if (E.cx == 0 && cur_line_len() == 0)
        undo_push();
      insert_char(c);
    }

  } else if (E.mode == 2) { /* command */
    if (c == KEY_ESC) {
      E.mode = 0;
      E.cmdbuf[0] = '\0';
      E.cmdlen = 0;
    } else if (c == '\r' || c == '\n') {
      exec_command();
    } else if ((c == KEY_BACKSPACE || c == 127) && E.cmdlen > 0) {
      E.cmdbuf[--E.cmdlen] = '\0';
      if (E.cmdlen == 0) {
        E.mode = 0;
      }
    } else if (!iscntrl(c) && E.cmdlen < 255) {
      E.cmdbuf[E.cmdlen++] = (char)c;
      E.cmdbuf[E.cmdlen] = '\0';
    }
  }
}

/* ───────────── init ───────────── */
static void init(void) {
  E.cx = E.cy = E.rx = 0;
  E.rowoff = E.coloff = 0;
  E.mode = 0;
  E.dirty = 0;
  E.filename[0] = '\0';
  E.status[0] = '\0';
  E.cmdbuf[0] = '\0';
  E.cmdlen = 0;
  E.undo_top = 0;
  E.yank_line = NULL;
  E.search[0] = '\0';
  E.search_dir = 1;
  E.buf.lines = malloc(sizeof(char *));
  E.buf.count = 0;
  memset(E.undo_stack, 0, sizeof E.undo_stack);
  E.screenrows = 26;
  E.screencols = 80;
  E.screenrows -= 2; /* status + message rows */
}

int main(int argc, char *argv[]) {
  init();
  // enable_raw();
  if (argc >= 2)
    file_open(argv[1]);
  else
    buf_insert_line(&E.buf, 0, "");

  snprintf(E.status, sizeof E.status,
           "mivim ready  --  :w save  :q quit  i insert  ESC normal");

  while (1) {
    refresh_screen();
    process_key();
  }
  return 0;
}
