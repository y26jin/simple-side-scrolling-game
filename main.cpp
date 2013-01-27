#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sstream>
#include <iostream>
#include <list>
#include <cstdlib>
#include <sys/time.h>
#include <cmath>

using namespace std;

const int Border = 5;
const int BufferSize = 10;
const int FPS = 30;
const int SequenceSize = 1000;
int brickWidth = 20, brickHeight = 20;
int numBuildingPerLine = 20, numBuildingPerCol = 25;
int currentX[SequenceSize];
int brickSeq[SequenceSize];
// where the targets are locate:
int isTarget[SequenceSize];// 0 is normal building, 1 is target
int totalScore = 0;
int firstTime = 1;
int isBackward = 0;
int isPaused = 0;
int menuOption[3] = {1, 0, 0};
int cursor = 0;// indicate where the cursor is when we are hover over on the menu
int level = 1;
string levelPlusscore, scoreString, levelString;

// Info need to draw on the window
// basic properties of a window
struct XInfo {
  Display *display;
  int screen;
  Window window;
  GC gc[3];// graphic context options
  Pixmap pixmap;
  int width;
  int height;
};

// prompt error message when error occurs
void errorMessage(string str){
  cerr << str << endl;
  exit(0);
}

class Displayable{
public:
  virtual void paint(XInfo &xinfo) = 0;
};

list<Displayable *> dList;

// For testing purposes, i would assume the helicopter is just an oval
class Chopper : public Displayable{
public:
  virtual void paint(XInfo &xinfo){
    counter++;
    if(Collision(xinfo, getX(), getY()) == 0){
      XFillArc(xinfo.display, xinfo.pixmap, xinfo.gc[0], getX(), getY(), diameter, diameter, 0, 360*64);

      if(isPaused == 1){
	// draw splash screen
	levelPlusscore = "Your current score is ";
	stringstream ss;
	ss<<level;
	ss>>levelString;
	stringstream sss;
	sss<<totalScore;
	sss>>scoreString;
	levelPlusscore += scoreString;
	levelPlusscore += " and your level is ";
	levelPlusscore += levelString;

	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3, "Yuhang Jin ID 20368414", 22);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3 + 20, levelPlusscore.c_str(), levelPlusscore.length());
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3 + 40, "This is a simple side scrolling game.", 37);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3 + 60, "UP: pull the chopper up", 23);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3 + 80, "DOWN: lower the chopper", 23);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3 + 100, "LEFT: go backward", 17);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3 + 120, "RIGHT: go forward", 17);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[0], xinfo.width/3, xinfo.height/3 + 140, "------------------------------", 30);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[menuOption[0]], xinfo.width/3, xinfo.height/3 + 160, "Resume", 6);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[menuOption[1]], xinfo.width/3, xinfo.height/3 + 180, "Level UP", 8);
	XDrawImageString(xinfo.display, xinfo.pixmap, xinfo.gc[menuOption[2]], xinfo.width/3, xinfo.height/3 + 200, "Level DOWN", 10);
      }

      if(currentBomb > 0){
	int i;
	for(i=0;i<currentBomb;i++){
	  if(state[i] == 1 && isPaused == 0){
	    if(Collision(xinfo, dropX[i]+4, dropY[i]+5) == 0){
	      dropX[i] = dropX[i] + 4;
	      dropY[i] = dropY[i] + 5;
	      XFillArc(xinfo.display, xinfo.pixmap, xinfo.gc[0], dropX[i], dropY[i], diameter/2, diameter/2, 0, 360*64);
	    }
	    else {
	      state[i] = 4;
	      // bomb effects
	      if(isTarget[i] == 1){
		totalScore = totalScore + 10;
	      }
	    }
	  }
	  else if(state[i] == 2 && isPaused == 0){
	    if(Collision(xinfo, dropX[i]+2, dropY[i]+5) == 0){
	      dropX[i] = dropX[i] + 2;
	      dropY[i] = dropY[i] + 5;
	      XFillArc(xinfo.display, xinfo.pixmap, xinfo.gc[0], dropX[i], dropY[i], diameter/2, diameter/2, 0, 360*64);
	    }
	    else {
	      state[i] = 4;
	      // bomb effects
	      if(isTarget[i] == 1){
		totalScore = totalScore + 10;
	      }
	    }
	  }
	}
      }
    }
    else errorMessage("Booom!");
  }
  
  void move(XInfo &xinfo, int state){// state[0,1,2,3] -> move[left, right, up, down]
    XWindowAttributes currentWinInfo;
    XGetWindowAttributes(xinfo.display, xinfo.window, &currentWinInfo);
    unsigned int currentHeight = currentWinInfo.height;
    unsigned int currentWidth = currentWinInfo.width;

    if(state == 0 && x>0 && isHitEdge == false){
      x = x - direction;
      isBackward = 1;
    }
    else if(state == 1 && x<currentWidth && isHitEdge == false){
      x = x + direction;
      isBackward = 0;
    }
    else if(state == 2 && y<currentHeight && isHitEdge == false){
      y = y + direction;
      isBackward = 0;
    }
    else if(state == 3 && y>0 && isHitEdge == false){
      y = y - direction;
      isBackward = 0;
    }
  }
  void dropBomb(XInfo &xinfo){
    if(state[currentBomb] == 0){
      dropX[currentBomb] = getX();
      dropY[currentBomb] = getY();

      if(isBackward == 0) state[currentBomb] = 1;
      else state[currentBomb] = 2;
    }
    currentBomb++;
  }

  void menu(XInfo &xinfo){
    strcpy(name, "Yuhang Jin ID 20368414");
    name[25]='\0';
  }

  int findX(int *current, int length, int x){
    int temp = (x - x%brickWidth)/brickWidth;
    while(current[temp] < x) temp++;

    while(current[temp] > x) temp--;

    return temp;
  }

  int Collision(XInfo &xinfo, int x, int y){
    int index = findX(currentX, SequenceSize, x);
    if(brickSeq[index] >= 0){
      int top = xinfo.height - brickHeight*brickSeq[index];
      if(isTarget[index] == 0){
	if(y < top) return 0;
	else return 1;
      }
      else{
	if(x >= currentX[index] && x <= currentX[index] + brickWidth/2){
	  top = top - brickHeight/2;
	  if(y < top) return 0;
	  else return 1;
	}
	else if(x > currentX[index] + brickWidth/2){
	  if(y < top) return 0;
	  else return 1;
	}
      }
    }
    else{
      int bottom = brickHeight*(-brickSeq[index]);
      if(y > bottom) return 0;//no collision
      else return 1;// collision
    }
  }
  
  int getX(){return x;}
  int getY(){return y;}

  Chopper(int x, int y, int diameter):x(x), y(y), diameter(diameter){
    direction = 4;
    int i;
    for(i=0;i<SequenceSize;i++){
      state[i] = 0;
    }
    currentBomb = 0;
    counter = 0;
  }

private:
  int x;
  int y;
  int diameter;
  int direction;
  bool isHitEdge;
  int counter;
  int currentBomb;// the index of the latest dropped bomb
  char name[128];
  // reserve bomb properties
  // if a bomb is dropped, start updating the cooresponding coordinates
  // otherwise don't do anything
  int dropX[SequenceSize];
  int dropY[SequenceSize];
  int state[SequenceSize];// 0 stands for not dropped, 1 stands for normal dropping, 2 stands for backward dropping, 3 stands for exploded
};



// obstacles and walls
// each building's xy coordinates should be recorded and so do the width and height

class Building : public Displayable{
public:
  virtual void paint(XInfo &xinfo){
    int i, j, tempX = xinfo.width, tempY = xinfo.height;

    brickWidth = 20 + (tempX - 400) / numBuildingPerLine;
    brickHeight = brickWidth;

    if(firstTime == 1){
      for(i=0;i<SequenceSize;i++){
	currentX[i] = i*(tempX/numBuildingPerLine);
      }
      firstTime = 0;
    }

    for(i=0;i<SequenceSize;i++){
      if(brickSeq[i] > 0){
	for(j=0;j<brickSeq[i];j++){ 
	  XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[0], currentX[i], tempY-brickHeight*(brickSeq[i]-j), brickWidth, brickHeight);
	}
	if(i%(random()%25+2) == 0){
	  isTarget[i] = 1;
	  //	  XFillArc(xinfo.display, xinfo.pixmap, xinfo.gc[0], currentX[i], tempY-brickHeight*brickSeq[i]-brickHeight/2, brickWidth/2, brickHeight/2, 0, 360*64);
	}
      }
      else if(brickSeq[i] < 0){
	for(j=0;j< -brickSeq[i];j++){
	  XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[0], currentX[i], j*brickHeight, brickWidth, brickHeight);
	}
      }
      if(isTarget[i] == 1){
	XFillArc(xinfo.display, xinfo.pixmap, xinfo.gc[0], currentX[i], tempY-brickHeight*brickSeq[i]-brickHeight/2, brickWidth/2, brickHeight/2, 0, 360*64);
      } 
      
    }

  }
  
  // generate the number of bricks and x coordinates assoicated with bricks
  void seqGen(){
    int i, tempX = 10;

    for(i=0;i<SequenceSize;i++){
      brickSeq[i] = -random()%10 + 5;
      if(brickSeq[i] == 0) brickSeq[i] = 1;
      
      isTarget[i] = 0;
    }
  }

  void scroll(XInfo &xinfo){
    int i;
    for(i=0;i<SequenceSize;i++){
      currentX[i] = currentX[i] - 4*level;
    }
  }

  Building(int x, int y):x(x), y(y) {
    seqGen();
  }

private:
  //  int brickSeq[SequenceSize]; // if the element is > 0, generate that number of bricks on the ground, otherwise generate that many from the ceilings
  // int currentX[SequenceSize]; // record current x coordinates of buildings
  int x;
  int y;
};

// there comes a bunch of componenets that are supposed to be displayed

Chopper helicopter(0, 200, 40);
//Bomb bomb(helicopter.getX(), helicopter.getY(), 20);
Building building(0, 50);

// Initialize X and create a window
void initX(int argc, char *argv[], XInfo &xinfo){
  XSizeHints hints;
  unsigned long white, black;

  xinfo.display = XOpenDisplay("");
  if(!xinfo.display){
    errorMessage("Can't open display");
  }
  
  xinfo.screen = DefaultScreen(xinfo.display);
  
  white = XWhitePixel(xinfo.display, xinfo.screen);
  black = XBlackPixel(xinfo.display, xinfo.screen);
  
  hints.x = 100;
  hints.y = 100;
  hints.width = 400;
  hints.height = 500;
  hints.flags = PPosition | PSize;

  xinfo.window = XCreateSimpleWindow(xinfo.display,
				     DefaultRootWindow(xinfo.display),
				     hints.x, hints.y,
				     hints.width, hints.height,
				     Border,
				     black,
				     white 
				     );
  
  XSetStandardProperties(xinfo.display,
			 xinfo.window,
			 "main", // window's title
			 "Animate", // icon's title
			 None, // icon pixmap
			 argv,argc,// app command line arguments
			 &hints);// size hints for the windowd

  // create graphic contexts
  int i = 0;
  xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
  XSetForeground(xinfo.display,
		 xinfo.gc[i],
		 BlackPixel(xinfo.display, xinfo.screen)
		 );
  XSetBackground(xinfo.display,
		 xinfo.gc[i],
		 WhitePixel(xinfo.display, xinfo.screen)
		 );
  XSetFillStyle(xinfo.display, xinfo.gc[i], FillSolid);
  XSetLineAttributes(xinfo.display, 
		     xinfo.gc[i], 
		     1, 
		     LineSolid, 
		     CapButt, 
		     JoinRound);
  // reverse video...
  // chopper uses the same sets of graphic contexts
  i = 1;
  xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
  XSetForeground(xinfo.display, 
		 xinfo.gc[i],
		 WhitePixel(xinfo.display, xinfo.screen));
  XSetBackground(xinfo.display,
		 xinfo.gc[i],
		 BlackPixel(xinfo.display, xinfo.screen));
  XSetFillStyle(xinfo.display, xinfo.gc[i], FillSolid);
  XSetLineAttributes(xinfo.display,
		     xinfo.gc[i],
		     1,
		     LineSolid,
		     CapButt,
		     JoinRound);

  // save it up displaying fonts
  i = 2;
  xinfo.gc[i] = XCreateGC(xinfo.display, xinfo.window, 0, 0);
  XSetForeground(xinfo.display,
		 xinfo.gc[i],
		 WhitePixel(xinfo.display, xinfo.screen));
  XSetBackground(xinfo.display,
		 xinfo.gc[i],
		 BlackPixel(xinfo.display, xinfo.screen));
  XSetFillStyle(xinfo.display, xinfo.gc[i], FillSolid);
  XSetLineAttributes(xinfo.display,
		     xinfo.gc[i],
		     1,
		     LineSolid,
		     CapButt,
		     JoinRound);

  int depth = DefaultDepth(xinfo.display, DefaultScreen(xinfo.display));
  xinfo.pixmap = XCreatePixmap(xinfo.display,
			       xinfo.window,
			       hints.width,
			       hints.height,
			       depth);
  xinfo.width = hints.width;
  xinfo.height = hints.height;

  XSelectInput(xinfo.display,
	       xinfo.window,
	       ButtonPressMask | KeyPressMask | PointerMotionMask 
	       | EnterWindowMask | LeaveWindowMask );
  XMapRaised(xinfo.display, xinfo.window);
  XFlush(xinfo.display);
  sleep(2); // let server get set up before pushing drawing commands
}


// repaint all the components in the display list
void repaint(XInfo &xinfo){
  list<Displayable *>::const_iterator begin = dList.begin();
  list<Displayable *>::const_iterator end = dList.end();

  XWindowAttributes windowInfo;
  XGetWindowAttributes(xinfo.display, xinfo.window, &windowInfo);
  unsigned int height = windowInfo.height;
  unsigned int width = windowInfo.width;

  XFreePixmap(xinfo.display, xinfo.pixmap);
  int depth = DefaultDepth(xinfo.display, DefaultScreen(xinfo.display));
  xinfo.pixmap = XCreatePixmap(xinfo.display, xinfo.window, width, height, depth);
  xinfo.width = width;
  xinfo.height = height;


  XClearWindow( xinfo.display, xinfo.window );

  XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[1], 0, 0, width, height);
  // startre painting each element within the display list
  while( begin != end ) {
    Displayable *d = *begin;
    d->paint(xinfo);
    begin++;
  }
  
  XCopyArea(xinfo.display, xinfo.pixmap, xinfo.window, xinfo.gc[1], 
	    0, 0, width, height, 0, 0);
  XFlush( xinfo.display );
}

void handleKeyPress(XInfo &xinfo, XEvent &event){
  // when arrows pressed, move respectively
  // use passive key event grab paradigm
  // no multi key support
  int key = XLookupKeysym((XKeyEvent*)&event,
			0); // what does index do here?

  int code = XKeysymToKeycode(xinfo.display, key);

  if(code == 20){ // key 'Q'
    // shortcut to quit the game
    exit(0);
  }
  else if(code == 13){ // key 'g/G' drop the bomb
    helicopter.dropBomb(xinfo);
  }
  else if(code == 11){ // key 'f/F' bring up the splash screen
    if(isPaused == 0){
      helicopter.menu(xinfo);
      // lock the screen
      isPaused = 1;
    }
    else if(isPaused == 1) isPaused = 0;
    
  }
  // when helicopter moves, also update the coordinates for bomb
  else if(code == 131){ // key 'left arrow'
    isBackward = 1;
    helicopter.move(xinfo, 0);
  }
  else if(code == 132){// key 'right arrow'
    isBackward = 0;
    helicopter.move(xinfo, 1);
  }
  else if(code == 133){ // key 'down'
    if(isPaused == 0){
      helicopter.move(xinfo, 2);
    }
    else{
      menuOption[cursor] = 0;
      if(cursor <= 1) cursor++;
      else cursor = 0;

      menuOption[cursor] = 1;
    }
  }
  else if(code == 134){ // key 'up'
    if(isPaused == 0){
      helicopter.move(xinfo, 3);
    }
    else{
      menuOption[cursor] = 0;
      if(cursor >= 1) cursor--;
      else cursor = 2;

      menuOption[cursor] = 1;
    }
  }
  else if(code == 44){
    if(isPaused == 1){
      if(cursor == 0) isPaused = 0;
      else if(cursor == 1) {
	level++;
	cursor = 0;
	menuOption[0] = 1;
	menuOption[1] = 0;
	menuOption[2] = 0;
	isPaused = 0;
      }
      else if(cursor == 2) {
	if(level > 1) level--;
	else level = 1;

	cursor = 0;
	menuOption[0] = 1;
	menuOption[1] = 0;
	menuOption[2] = 0;
	isPaused = 0;
      }
    }
  }

}

void handleAnimation(XInfo &xinfo){
  if(isPaused == 0){
    building.scroll(xinfo);
  }

}

void handleResize(XInfo &xinfo, XEvent &event){
  XResizeRequestEvent *rre = (XResizeRequestEvent *)&event;

  XFreePixmap(xinfo.display, xinfo.pixmap);
  int depth = DefaultDepth(xinfo.display, DefaultScreen(xinfo.display));
   xinfo.pixmap = XCreatePixmap(xinfo.display, xinfo.window, rre->width, rre->height, depth);
  xinfo.width = rre->width;
  xinfo.height = rre->height;
  
  XRectangle      clip_rect;
  clip_rect.x = 0;
  clip_rect.y = 0;
  clip_rect.width = xinfo.width;
  clip_rect.height = xinfo.height;
  XSetClipRectangles(xinfo.display, xinfo.gc[0], 0, 0, &clip_rect, 1, Unsorted);
}

unsigned long now(){
  timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 100000 + tv.tv_usec;
}

void eventLoop(XInfo &xinfo){
  dList.push_front(&helicopter);
  dList.push_front(&building);
  
  // also need to push other components to the display list

  XEvent event;
  unsigned long lastRepaint = 0;
  int inside = 0;

  while(true){
    if(XPending(xinfo.display) > 0){
      XNextEvent(xinfo.display, &event);

      switch(event.type){
      case ResizeRequest:
	handleResize(xinfo, event);
	break;
      
      case KeyPress:
	handleKeyPress(xinfo, event);
	break;
      }

    }

    unsigned long end = now();
    if(end - lastRepaint > 1000000/FPS){
	handleAnimation(xinfo);
	repaint(xinfo);
	lastRepaint = now();

    }
    else if(XPending(xinfo.display) == 0){
      usleep(1000000/FPS - (end - lastRepaint));
    }

  }
}


int main(int argc, char *argv[]){
  XInfo xinfo;
  
  initX(argc, argv, xinfo);
  eventLoop(xinfo);
  XCloseDisplay(xinfo.display);
}
