#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define max(x, y) ((x)>(y)?(x):(y))

#define THICK 5

SDL_Window* w;
SDL_Renderer* r;

typedef enum {
  SYMB,
  APPL,
  DEFN
} expr_type;

typedef struct expr {
  expr_type t;
  union {
    char name;
    struct{
      struct expr* fun;
      struct expr* par;
    };
    struct{
      char arg;
      struct expr* bod;
    };
  };
} expr;

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
  if(s == NULL){
    return NULL;
  }

  stack_t r = *s;
  *s = (*s)->n;
  return r->d;
}

void print_expr(expr e){
  if(e.t == SYMB){
    printf("%c", e.name);
  }else if(e.t == APPL){
    printf("(");
    print_expr(*e.fun);
    printf(" ");
    print_expr(*e.par);
    printf(")");
  }else{
    printf("\\");
    printf("%c", e.arg);
    printf(".");
    print_expr(*e.bod);
  }
}

SDL_Point draw_expr_(expr e, int sx, int sy, stack_t* s){
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

void draw_expr(expr e, int x, int y){
  stack_t s = NULL;
  draw_expr_(e, x, y, &s);
}

void skip_whitespace(char** s){
  while(**s && (**s == ' ' || **s == '\t' || **s == '\n')){
    (*s)++;
  }
}

expr* make_expr(){
  return (expr*)malloc(sizeof(expr));
}

expr* copy_expr(expr* e){
  expr* out = make_expr();
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

void free_expr(expr* e){
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

expr* parse_expr(char** s);

expr* parse_expr_part(char** s){
  expr* out = make_expr();
  skip_whitespace(s);
  if(!**s){
    goto FAIL;
  }

  if(**s == '('){
    (*s)++;
    out = parse_expr(s);
    skip_whitespace(s);
    if(!**s || **s != ')') goto FAIL;
    (*s)++;
    return out;
  }

  if(**s == ')'){
    goto FAIL;
  }

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

expr* parse_expr(char** s){
  expr* l = parse_expr_part(s);
  if(!l) return NULL;

  expr* r;
  while((r = parse_expr_part(s))){
    expr* new = make_expr();

    new->t = APPL;
    new->fun = l;
    new->par = r;
    l = new;
  }

  return l;
}

expr* substitute(expr* e, char n, expr* s){
  if(e->t == SYMB){
    if(e->name == n){
      return copy_expr(s);
    }
    return copy_expr(e);
  }else if(e->t == APPL){
    expr* out = make_expr();
    *out = *e;
    out->fun = substitute(e->fun, n, s);
    out->par = substitute(e->par, n, s);
    return out;
  }else if(e->t == DEFN){
    if(e->arg == n){
      return copy_expr(e);
    }
    expr* out = make_expr();
    *out = *e;
    out->bod = substitute(e->bod, n, s);
    return out;
  }
  return NULL;
}

void beta_reduce(expr* e){
  if(e->t != APPL || e->fun->t != DEFN){
    return;
  }

  expr* en = substitute(e->fun->bod, e->fun->arg, e->par);
  free_expr(e->fun);
  free_expr(e->par); // don't free e itself because we assign to it!
  *e = *en;
  free(en);
}

int greedy_eval(expr* e){
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

int main(){
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(400, 400, 0, &w, &r);

  char* fib = "\\n.\\f.n(\\c.\\a.\\b.c b(\\x.a (b x)))(\\x.\\y.x)(\\x.x)f";
  char* omega = "(\\x.xx)(\\x.xx)";
  char* trivial = "(\\y.(\\x.x)y)";
  char* ycomb = "\\f.(\\x.x x)(\\x.f(x x))";
  char* oneplustwo = "(\\a.\\b.(\\f.\\x.(af(bfx))))(\\f.\\x.(fx))(\\f.\\x.f(fx))";
  char* twotimestwo = "(\\a.\\b.(\\f.\\x.(a(bf)x)))(\\f.\\x.f(fx))(\\f.\\x.f(fx))";

  char** s = &twotimestwo;
  expr* exp = parse_expr(s);
  print_expr(*exp);
  printf("\n");

  int quit = 0;
  while(!quit){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
      if(e.type == SDL_QUIT){
        quit = 1;
      }else if(e.type == SDL_KEYDOWN){
        greedy_eval(exp);
        print_expr(*exp); printf("\n");
      }
    }
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderClear(r);

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    draw_expr(*exp, 20, 20);

    SDL_RenderPresent(r);
    SDL_Delay(16);
  }
  return 0;
}
