#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"

#define max(x, y) ((x)>(y)?(x):(y))

#define THICK 5

SDL_Window* w;
SDL_Renderer* r;

typedef enum {
  SYMB,
  APPL,
  DEFN
} expr_type;

typedef struct expr_t {
  expr_type t;
  union {
    char name;
    struct{
      struct expr_t* fun;
      struct expr_t* par;
    };
    struct{
      char arg;
      struct expr_t* bod;
    };
  };
} expr_t;

expr_t* make_expr(){
  return (expr_t*)malloc(sizeof(expr_t));
}

void free_expr(expr_t* e){
  switch(e->t){
    case SYMB:
      break;
    case APPL:
      free_expr(e->fun);
      free_expr(e->par);
      break;
    case DEFN:
      free_expr(e->bod);
      break;
  }
  free(e);
}

expr_t* copy_expr(expr_t* e){
  expr_t* out = make_expr();
  out->t = e->t;
  switch(e->t){
    case SYMB:
      out->name = e->name;
      break;
    case APPL:
      out->fun = copy_expr(e->fun);
      out->par = copy_expr(e->par);
      break;
    case DEFN:
      out->arg = e->arg;
      out->bod = copy_expr(e->bod);
      break;
  }
  return out;
}

void skip_whitespace(char** s){
  while(**s && (**s == ' ' || **s == '\t' || **s == '\n')){
    (*s)++;
  }
}

expr_t* parse_expr(char** s);

expr_t* parse_expr_part(char** s){
  skip_whitespace(s);
  if(!**s || **s == ')'){
    return NULL;
  }

  if(**s == '('){
    (*s)++;
    expr_t* out = parse_expr(s);
    skip_whitespace(s);
    if(!**s || **s != ')') goto FAIL;
    (*s)++;
    return out;
  }

  expr_t* out = make_expr();

  if(**s == '\\'){
    (*s)++; skip_whitespace(s);
    if(!**s) goto FAIL;
    out->t = DEFN;
    out->arg = **s;

    (*s)++; skip_whitespace(s);
    if(!**s || **s != '.') goto FAIL;

    (*s)++; skip_whitespace(s);
    out->bod = parse_expr(s);
    return out;
  }

  out->t = SYMB;
  out->name = **s;
  (*s)++;
  return out;
FAIL:
  free(out);
  return NULL;
}

expr_t* parse_expr(char** s){
  expr_t* l = parse_expr_part(s);
  if(!l) return NULL;

  expr_t* r;
  while((r = parse_expr_part(s))){
    expr_t* new = make_expr();

    new->t = APPL;
    new->fun = l;
    new->par = r;
    l = new;
  }

  return l;
}

expr_t* substitute(expr_t* e, char n, expr_t* s){
  if(e->t == SYMB){
    if(e->name == n){
      return copy_expr(s);
    }
    return copy_expr(e);
  }else if(e->t == APPL){
    expr_t* out = make_expr();
    *out = *e;
    out->fun = substitute(e->fun, n, s);
    out->par = substitute(e->par, n, s);
    return out;
  }else if(e->t == DEFN){
    if(e->arg == n){
      return copy_expr(e);
    }
    expr_t* out = make_expr();
    *out = *e;
    out->bod = substitute(e->bod, n, s);
    return out;
  }
  return NULL;
}

void beta_reduce(expr_t* e){
  if(e->t != APPL || e->fun->t != DEFN){
    return;
  }

  expr_t* en = substitute(e->fun->bod, e->fun->arg, e->par);
  free_expr(e->fun);
  free_expr(e->par);
  *e = *en;
  free(en);
}

int greedy_eval(expr_t* e){
  if(e->t == SYMB){
    return 0;
  }else if(e->t == APPL){
    if(e->fun->t == DEFN){
      beta_reduce(e);
      return 1;
    }
    if(greedy_eval(e->fun)) return 1;
    if(greedy_eval(e->par)) return 1;
  }else{
    if(greedy_eval(e->bod)) return 1;
  }
  return 0;
}

int is_normal_form(expr_t* e){
  if(e->t == SYMB) return 1;
  if(e->t == APPL){
    if(e->fun->t == DEFN) return 0;
    return is_normal_form(e->fun) && is_normal_form(e->par);
  }
  if(e->t == DEFN) return is_normal_form(e->bod);
  return 1;
}

typedef struct {
  char* buf;
  size_t size;
  int pos;
} strbuf_t;

static void sb_append_char(strbuf_t* sb, char c){
  if(sb->pos < sb->size-1){
    sb->buf[sb->pos++] = c;
  }
}

static void sb_expr(strbuf_t* sb, expr_t* e){
  if(e->t == SYMB){
    sb_append_char(sb, e->name);
  }else if(e->t == APPL){
    sb_append_char(sb, '(');
    sb_expr(sb, e->fun);
    sb_append_char(sb, ' ');
    sb_expr(sb, e->par);
    sb_append_char(sb, ')');
  }else{
    sb_append_char(sb, '\\');
    sb_append_char(sb, e->arg);
    sb_append_char(sb, '.');
    sb_expr(sb, e->bod);
  }
}

void expr_to_string(expr_t* e, char* buf, size_t size){
  strbuf_t sb = {buf, size, 0};
  sb_expr(&sb, e);
  sb_append_char(&sb, '\0');
}

void print_expr(expr_t e){
  char buf[4096];
  expr_to_string(&e, buf, sizeof(buf));
  printf("%s", buf);
}

typedef struct stack_t {
  void* d;
  struct stack_t* n;
}* stack_t;

void push_stack(stack_t* s, void* d){
  stack_t new = (stack_t)malloc(sizeof(struct stack_t));
  new->d = d;
  new->n = *s;
  *s = new;
}

void* pop_stack(stack_t* s){
  if(*s == NULL){
    return NULL;
  }

  stack_t r = *s;
  *s = (*s)->n;
  return r->d;
}

void draw_char(int x, int y, char c, int scale){
  unsigned char ch = (unsigned char)c;
  if(ch < FONT_FIRST || ch > FONT_LAST) ch = '?';
  const uint8_t* glyph = font_data[ch];
  for(int row = 0; row < FONT_ROWS; row++){
    for(int col = 0; col < FONT_COLS; col++){
      if(glyph[row] & (1 << col)){
        SDL_Rect rect = {x + col*scale, y + row*scale, scale, scale};
        SDL_RenderFillRect(r, &rect);
      }
    }
  }
}

void draw_string(int x, int y, const char* s, int scale, int wrap_width){
  int cx = x, cy = y;
  int cw = FONT_COLS*scale + 1;
  int ch = FONT_ROWS*scale + 2;

  while(*s){
    if(*s == '\n'){
      cx = x;
      cy += ch;
      s++;
      continue;
    }
    if(wrap_width > 0 && cx + cw > x + wrap_width){
      cx = x;
      cy += ch;
    }
    draw_char(cx, cy, *s, scale);
    cx += cw;
    s++;
  }
}

SDL_Point draw_expr_(expr_t e, int sx, int sy, stack_t* s){
  typedef struct {
    char c;
    int h;
  } height_t;

  if(e.t == SYMB){
    stack_t t = *s;
    while(t != NULL && ((height_t*)t->d)->c != e.name){
      t = t->n;
    }

    if(t == NULL){
      printf("found undefined symbol: %c\n", e.name);
    }else{
      int h = ((height_t*)t->d)->h;
      SDL_Rect rc;
      rc.x = sx+THICK;
      rc.w = THICK;
      rc.y = h;
      rc.h = sy-h+THICK;
      SDL_RenderFillRect(r, &rc);
    }

    return (SDL_Point){sx+3*THICK, sy};
  }else if(e.t == APPL){
    SDL_Point end1 = draw_expr_(*e.fun, sx, sy, s);
    SDL_Point end2 = draw_expr_(*e.par, end1.x+THICK, sy, s);

    int h = max(end1.y, end2.y);

    SDL_Rect rc;
    rc.x = sx+THICK;
    rc.w = THICK;
    rc.y = sy;
    rc.h = h-sy+2*THICK;
    SDL_RenderFillRect(r, &rc);

    rc.x = end1.x+2*THICK;
    rc.h -= THICK;
    SDL_RenderFillRect(r, &rc);

    rc.x = sx+THICK;
    rc.w = end1.x-sx+THICK;
    rc.y = h;
    rc.h = THICK;
    SDL_RenderFillRect(r, &rc);

    return (SDL_Point){end2.x, h+2*THICK};
  }else{
    height_t h;
    h.c = e.arg;
    h.h = sy;
    push_stack(s, &h);
    SDL_Point end = draw_expr_(*e.bod, sx, sy+2*THICK, s);
    pop_stack(s);

    SDL_Rect rc;
    rc.x = sx;
    rc.w = end.x-sx;
    rc.y = sy;
    rc.h = THICK;
    SDL_RenderFillRect(r, &rc);

    return end;
  }
}

void draw_expr(expr_t e, int x, int y){
  stack_t s = NULL;
  draw_expr_(e, x, y, &s);
}

int main(){
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(400, 400, 0, &w, &r);

  char* fib = "\\n.\\f.n(\\c.\\a.\\b.c b(\\x.a (b x)))(\\x.\\y.x)(\\x.x)f";
  char* omega = "(\\x.xx)(\\x.xx)";
  char* trivial = "(\\y.(\\x.x)y)";
  char* ycomb = "\\f.(\\x.x x)(\\x.f(x x))";
  char* oneplustwo = "(\\a.\\b.(\\f.\\x.(af(bfx))))(\\f.\\x.(fx))(\\f.\\x.f(fx))";
  char* twotimestwo = "(\\a.\\b.(\\f.\\x.(a(bf)x)))(\\f.\\x.f(fx))(\\f.\\x.f(fx))";

  char** s = &oneplustwo;
  expr_t* exp = parse_expr(s);
  print_expr(*exp);
  printf("\n");

  int steps = 0;
  char expr_text[4096];
  int quit = 0;
  while(!quit){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
      if(e.type == SDL_QUIT){
        quit = 1;
      }else if(e.type == SDL_KEYDOWN){
        if(greedy_eval(exp)){
          steps++;
          print_expr(*exp); printf("\n");
        }
      }
    }
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderClear(r);

    char status[64];
    int len = snprintf(status, sizeof(status), "Steps: %d", steps);
    if(is_normal_form(exp)){
      snprintf(status + len, sizeof(status) - len, "  [NORMAL FORM]");
    }
    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    draw_string(5, 5, status, 2, 0);

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    draw_expr(*exp, 20, 40);

    expr_to_string(exp, expr_text, sizeof(expr_text));
    draw_string(5, 320, expr_text, 2, 390);

    SDL_RenderPresent(r);
    SDL_Delay(16);
  }
  return 0;
}
