
#include "message.h"
#include "font.h"
#include "sdlutil.h"
#include "draw.h"
#include "util.h"
#include "chars.h"

struct mreal : public message {

  static mreal * create();
  virtual void destroy();

  /*  enter: true
     escape: false */
  virtual bool ask(char * actualchar = 0, string charspec = "");

  virtual void draw();
  virtual void screenresize();
  virtual ~mreal();

  SDL_Surface * alpharect;
  bool loop (char * actualchar, string charspec);

  int nlines;
  int posx;
};

mreal * mreal::create() {
  mreal * pp = new mreal();
  pp->below = 0;
  pp->alpharect = 0;
  pp->posx = 0;
  pp->nlines = 0;
  return pp;
}

void mreal::destroy() {
  if (alpharect) SDL_FreeSurface(alpharect);
  delete this;
}

mreal::~mreal() {}
message::~message() {}
message * message::create() {
  return mreal::create();
}

bool mreal::ask(char * actualchar, string charspec) {

  /* find longest line */
  int ll = 0;
  { string titlen = title + "\n";
  int cl = 0;
  for(unsigned int i = 0; i < titlen.length(); i ++) {
    if (titlen[i] == '\n') {
      ll = util::maximum(fon->sizex(titlen.substr(cl, i - cl)), ll);
      cl = i;
    } else if (titlen[cl] == '\n') cl = i;
  }
  }

  int w = 
    (2 * fon->width) +
    util::maximum(ll,
		  fon->sizex("ESCAPE: ") +
		  util::maximum(fon->sizex(ok),
				fon->sizex(cancel)));

  nlines = font::lines(title);

  int h;
  if (cancel == "") {
    h = ((3 + nlines) * fon->height);
  } else {
    h = ((4 + nlines) * fon->height);
  }

  /* now center */
  posx = (screen->w - w) / 2;
  
  /* and y, if desired */
  if (posy < 0) {
    posy = (screen->h - h) / 2;
  }

  /* XXX make these values setable */
  /* create alpha rectangle */
  alpharect = sdlutil::makealpharect(w, h, 90, 90, 90, 200);

  sdlutil::outline(alpharect, 2, 36, 36, 36, 200);

  return loop(actualchar, charspec);
}

void mreal::screenresize() {
  if (below) below->screenresize();
}

void mreal::draw() {

  /* clear back */
  if (!below) {
    sdlutil::clearsurface(screen, BGCOLOR);
  } else {
    below->draw();
  }

  /* draw alpha-transparent box */
  SDL_Rect dest;

  dest.x = posx;
  dest.y = posy;

  SDL_BlitSurface(alpharect, 0, screen, &dest);

  /* draw text */
  fon->drawlines(posx + fon->width, posy + fon->height, title);
  fon->draw(posx + fon->width, posy + ((1 + nlines) * fon->height),
	    (string)YELLOW "ENTER" POP ":  " + ok);
  if (cancel != "") 
    fon->draw(posx + fon->width, posy + ((2 + nlines) * fon->height),
	      (string)YELLOW "ESCAPE" POP ": " + cancel);
}

bool mreal::loop(char * actualchar, string charspec) {

  draw();
  SDL_Flip(screen);

  SDL_Event e;

  while ( SDL_WaitEvent(&e) >= 0 ) {

    int key;

    switch(e.type) {
    case SDL_QUIT:
      return false; /* XXX ? */
    case SDL_MOUSEBUTTONDOWN: {
      SDL_MouseButtonEvent * em = (SDL_MouseButtonEvent*)&e;

      if (em->button == SDL_BUTTON_LEFT) {
	/* allow a click within the entire box if
	   there is no cancel; otherwise, require clicking
	   on the text itself. */
	
 	int x = em->x;
	int y = em->y;

	if (cancel == "") {
	  if (x > posx &&
	      x < (posx + alpharect->w) &&
	      y >= posy &&
	      y < (posy + alpharect->h)) return true;
	} else {
	  if (x > posx &&
	      x < (posx + alpharect->w)) {

	    /* which did we click on? */
	    if (y > posy + ((1 + nlines) * fon->height) &&
		y < posy + ((2 + nlines) * fon->height)) return true;

	    if (y > posy + ((2 + nlines) * fon->height) &&
		y < posy + ((3 + nlines) * fon->height)) return false;

	  }
	}
	/* out of area; flash screen? */
	
      }
    }
    case SDL_KEYDOWN:
      key = e.key.keysym.sym;
      if (actualchar) {
	  int uc = e.key.keysym.unicode;
	  if ((uc & ~0x7F) == 0 && uc >= ' ') {
	    *actualchar = (char)uc;
	    /* non-ascii */
	  } else *actualchar = 0;
      }
      switch(key) {
      case SDLK_ESCAPE:
	return false;
      case SDLK_RETURN:
	return true;

      default:;
	/* is this a key in our range? */
	if (actualchar && util::matchspec(charspec, *actualchar)) {
	  return false; /* wasn't 'enter' */
	} 
	/* XXX else might flash screen or something */
      }
      break;
    case SDL_VIDEORESIZE: {
      SDL_ResizeEvent * eventp = (SDL_ResizeEvent*)&e;
      screen = sdlutil::makescreen(eventp->w, 
				   eventp->h);
      screenresize();
      draw();
      SDL_Flip(screen);
      break;
    }
    case SDL_VIDEOEXPOSE:
      draw();
      SDL_Flip(screen);
      break;
    default: break;
    }
  }
  return false; /* XXX ??? */
}

