/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <shape.h>
#include <abCircle.h>
#include <p2switches.h>

#define GREEN_LED BIT6
#define RED_LED BIT0


AbRect paddle = {abRectGetBounds, abRectCheck, {10,2}};
AbRect rect0 = {abRectGetBounds, abRectCheck, {10,4}};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 5, screenHeight/2 - 10}
};

Layer ballLayer = {(AbShape *)&circle3, {screenWidth/2, screenHeight/2}, {0,0}, {0,0}, COLOR_VIOLET, 0};
Layer fieldLayer = {(AbShape *) &fieldOutline, {screenWidth/2, screenHeight/2}, {0,0}, {0,0}, COLOR_BLUE, &ballLayer};
Layer paddleLayer = {(AbShape *)&paddle, {screenWidth/2, screenHeight-15}, {0,0}, {0,0}, COLOR_BLUE, &fieldLayer};

Layer layer9 = {(AbShape *)&rect0, {35, 18}, {0,0}, {0,0}, COLOR_BLUE, &paddleLayer};
Layer layer8 = {(AbShape *)&rect0, {60, 18}, {0,0}, {0,0}, COLOR_RED, &layer9};
Layer layer7 = {(AbShape *)&rect0, {85, 18}, {0,0}, {0,0}, COLOR_GREEN, &layer8};
Layer layer6 = {(AbShape *)&rect0, {25, 28}, {0,0}, {0,0}, COLOR_ORANGE, &layer7};
Layer layer5 = {(AbShape *)&rect0, {50, 28}, {0,0}, {0,0}, COLOR_VIOLET, &layer6};
Layer layer4 = {(AbShape *)&rect0, {75, 28}, {0,0}, {0,0}, COLOR_BLUE, &layer5};
Layer layer3 = {(AbShape *)&rect0, {100,28}, {0,0}, {0,0}, COLOR_RED, &layer4};
Layer layer2 = {(AbShape *)&rect0, {35, 38}, {0,0}, {0,0}, COLOR_GREEN, &layer3};
Layer layer1 = {(AbShape *)&rect0, {60, 38}, {0,0}, {0,0}, COLOR_ORANGE, &layer2};
Layer layer0 = {(AbShape *)&rect0, {85, 38}, {0,0}, {0,0}, COLOR_VIOLET, &layer1};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 *velocity;
  struct MovLayer_s *next;
} MovLayer;

Vec2 ballV = {4, 3};
Vec2 paddleV = {0, 0};
MovLayer ml1 = {&paddleLayer, &paddleV, 0};
MovLayer ml0 = {&ballLayer, &ballV, &ml1}; 

movLayerDraw(MovLayer *movLayers, Layer *layers){
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */

  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence){
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity->axes[axis] = -ml->velocity->axes[axis];
	newPos.axes[axis] += (2*velocity);
      }	/**< if outside of fence */
    } /**< for axis */
    ml->layer->posNext = newPos;
  } /**< for ml */
}

void switchHandler(u_int switches){
  if(!(switches&1))
    paddleV.axes[0] = -4;
  else if(!(switches&8))
    paddleV.axes[0] = 4;
  else
    paddleV.axes[0] = 0;

    
}

void checkCollision(Layer *ballLayer, Layer *l){
  Vec2 newPos;
  Region shapeBoundary, ballBoundary;
  abShapeGetBounds(ballLayer->abShape, &ballLayer->pos, &ballBoundary);

  int i;
  for(i=0; i<10; i++){
    abShapeGetBounds(l->abShape, &l->pos, &shapeBoundary);
    l = l->next;
  }
}

u_int bgColor = COLOR_BLACK;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main(){
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  
  layerInit(&layer0);
  layerDraw(&layer0);

  drawString5x7(40,2, "Score: ", COLOR_GREEN, COLOR_BLACK);
  drawString5x7(77,2, "0", COLOR_GREEN, COLOR_BLACK);

  layerGetBounds(&fieldLayer, &fieldFence);

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &layer0);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler(){
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 15) {
    switchHandler(p2sw_read());
    mlAdvance(&ml0, &fieldFence);
    //checkCollision(&ballLayer, &layer0);
    redrawScreen = 1;
    count = 0;
  } 
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
