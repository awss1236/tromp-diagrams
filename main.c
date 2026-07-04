#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#define max(x, y) ((x)>(y)?(x):(y))

#define THICK 10

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

int main(){
  SDL_Init(SDL_INIT_VIDEO);
  SDL_CreateWindowAndRenderer(400, 400, 0, &w, &r);

  expr x = (expr){SYMB, .name='x'};
  expr a = (expr){SYMB, .name='a'};
  expr b = (expr){SYMB, .name='b'};
  expr c = (expr){SYMB, .name='c'};
  expr f = (expr){SYMB, .name='f'};
  expr n = (expr){SYMB, .name='n'};
  expr id = (expr){DEFN, .arg='x', .bod=&x};
  expr ki = (expr){DEFN, .arg='y', .bod=&x};
  expr k = (expr){DEFN, .arg='x', .bod=&ki};
  expr cb = (expr){APPL, .fun=&c, .par=&b};
  expr bx = (expr){APPL, .fun=&b, .par=&x};
  expr abx = (expr){APPL, .fun=&a, .par=&bx};
  expr in = (expr){DEFN, .arg='x', .bod=&abx};
  expr cbt = (expr){APPL, .fun=&cb, .par=&in};
  expr db = (expr){DEFN, .arg='b', .bod=&cbt};
  expr da = (expr){DEFN, .arg='a', .bod=&db};
  expr dc = (expr){DEFN, .arg='c', .bod=&da};
  expr ncab = (expr){APPL, .fun=&n, .par=&dc};
  expr l = (expr){APPL, .fun=&ncab, .par=&k};
  expr fst = (expr){APPL, .fun=&l, .par=&id};
  expr bo = (expr){APPL, .fun=&fst, .par=&f};
  expr df = (expr){DEFN, .arg='f', .bod=&bo};
  expr dn = (expr){DEFN, .arg='n', .bod=&df};
  print_expr(dn);
  printf("\n");

  int quit = 0;
  while(!quit){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
      if(e.type == SDL_QUIT){
        quit = 1;
      }
    }
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderClear(r);

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    draw_expr(dn, 20, 20);

    SDL_RenderPresent(r);
    SDL_Delay(16);
  }
  return 0;
}
