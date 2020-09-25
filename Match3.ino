enum blinkStates {INERT, MATCH_MADE, DISSOLVING, EXPLODE, BEAM, BUCKET, RAINBOW, RESOLVE};
byte signalState = INERT;
byte nextState = INERT;
byte specialState = INERT;
bool wasActivated = false;

#define NUM_COLORS 6
byte colorHues[6] = {0, 50, 75, 100, 200, 235};
byte blinkColor;
byte previousColor;

Timer dissolveTimer;
#define DISSOLVE_TIME 1500

byte matchesMade = 0;
#define MATCH_GOAL 7
Timer bubbleTimer;
byte bubbleFace;
#define BUBBLE_TIME 300
#define BUBBLE_WAIT_MIN 300
#define BUBBLE_WAIT_MAX 2000

void setup() {
  randomize();
  blinkColor = random(NUM_COLORS - 1);
}

void loop() {

  //run loops
  FOREACH_FACE(f) {
    switch (signalState) {
      case INERT:
        inertLoop();
        break;
      case MATCH_MADE:
        matchmadeLoop();
        break;
      case DISSOLVING:
        dissolvingLoop();
        break;
      case EXPLODE:
        explodeLoop();
        break;
      case BEAM:
        //beamLoop();
        break;
      case BUCKET:
        bucketLoop();
        break;
      case RAINBOW:
        rainbowLoop();
        break;
      case RESOLVE:
        resolveLoop();
        break;
    }
  }

  //send data
  byte sendData = (signalState << 3) + (blinkColor);
  setValueSentOnAllFaces(sendData);

  //temp display
  switch (signalState) {
    case INERT:
    case RAINBOW:
    case MATCH_MADE:
      inertDisplay();
      break;
    case DISSOLVING:
      dissolveDisplay();
      break;
  }

  //dump button presses
  buttonSingleClicked();
}

////DISPLAY FUNCTIONS
void dissolveDisplay() {
  if (dissolveTimer.getRemaining() > DISSOLVE_TIME / 2) {//first half

    byte dissolveBrightness = map(dissolveTimer.getRemaining() - (DISSOLVE_TIME / 2), 0, DISSOLVE_TIME / 2, 0, 255);
    setColor(makeColorHSB(colorHues[previousColor], 255, dissolveBrightness));

  } else {//second half

    byte dissolveBrightness = map(dissolveTimer.getRemaining(), 0, DISSOLVE_TIME / 2, 0, 255);

    if (specialState == RAINBOW) {
      FOREACH_FACE(f) {
        setColorOnFace(makeColorHSB(colorHues[f], 255, 255 - dissolveBrightness), f);
      }
    } else {
      setColor(makeColorHSB(colorHues[blinkColor], 255, 255 - dissolveBrightness));
    }

  }
}

void inertDisplay() {
  switch (specialState) {
    case INERT:
      setColor(makeColorHSB(colorHues[blinkColor], 255, 255));
      break;
    case RAINBOW:
      FOREACH_FACE(f) {
        setColorOnFace(makeColorHSB(colorHues[f], 255, 255), f);
      }
      break;
    case EXPLODE:
      setColor(makeColorHSB(colorHues[blinkColor], 255, random(255)));
      break;
    case BEAM:
      setColor(makeColorHSB(colorHues[blinkColor], 255, 255));
      setColorOnFace(WHITE, 0);
      setColorOnFace(WHITE, 2);
      setColorOnFace(WHITE, 4);
      break;
    case BUCKET:
      setColor(OFF);
      setColorOnFace(makeColorHSB(colorHues[blinkColor], 255, 255), random(2));
      setColorOnFace(makeColorHSB(colorHues[blinkColor], 255, 255), 3);
      setColorOnFace(makeColorHSB(colorHues[blinkColor], 255, 255), 4);
      setColorOnFace(makeColorHSB(colorHues[blinkColor], 255, 255), 5);
      break;
  }

  //do bubbles if it's bubble time
  if (specialState == INERT) {
    if (bubbleTimer.isExpired()) {//the timer is over, just reset
      //what's our current wait time?
      bubbleTimer.set(map(MATCH_GOAL - matchesMade, 0, MATCH_GOAL, BUBBLE_WAIT_MIN, BUBBLE_WAIT_MAX));
      bubbleFace = (bubbleFace + random(4) + 1) % 6;//choose a new bubble face
    } else if (bubbleTimer.getRemaining() <= BUBBLE_TIME) {//ooh, we are actively bubbling right now
      //how bubbly are we right now?
      byte bubbleSat = map(BUBBLE_TIME - bubbleTimer.getRemaining(), 0, BUBBLE_TIME, 100, 255);
      setColorOnFace(makeColorHSB(colorHues[blinkColor], bubbleSat, 255), bubbleFace);
    }
  }

}

void inertLoop() {
  //search my neighbors, see if I can make a match
  byte sameColorNeighbors = 0;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      //check to see if it's INERT
      if (getNeighborColor(neighborData) == blinkColor || getNeighborState(neighborData) == RAINBOW) {//ok, this is our color
        if (getNeighborState(neighborData) == INERT || getNeighborState(neighborData) == RAINBOW) {//this one is inert, but could still help the matches
          sameColorNeighbors++;
        } else if (getNeighborState(neighborData) == MATCH_MADE) {//this on is in matchmade!
          sameColorNeighbors += 2;//just automatically get us high enough
        }
      }
    }
  }

  //search my neighbors to see if there are special things (EXPLODE or BUCKET)
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getNeighborState(neighborData) == EXPLODE) {
        //so this makes us into a matching color thing
        signalState = MATCH_MADE;
        blinkColor = getNeighborColor(neighborData);
      } else if (getNeighborState(neighborData) == BUCKET) {
        signalState = BUCKET;
        blinkColor = getNeighborColor(neighborData);
      }
    }
  }


  if (sameColorNeighbors >= 2) {
    setFullState(MATCH_MADE);
  }

  //listen for button clicks
  if (buttonSingleClicked()) {
    switch (specialState) {
      case BEAM:
        setFullState(EXPLODE);
        break;
      case EXPLODE:
        setFullState(EXPLODE);
        wasActivated = true;
        break;
      case BUCKET:
        setFullState(BUCKET);
        break;
    }
  }
}

void rainbowLoop() {
  byte neighborColorSets[5] = {0, 0, 0, 0, 0};

  //all I gotta do is look for neighbors that are matching
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getNeighborState(neighborData) == MATCH_MADE) {//this on is in matchmade!
        setFullState(MATCH_MADE);//go into matching
        blinkColor = getNeighborColor(neighborData);//change color to this match color
      } else {//no match made, but track their colors

      }
    }
  }
}

void matchmadeLoop() {
  //so in here, we listen to make sure no same-color neighbor is waiting
  bool foundUnmatchedNeighbors = false;
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getNeighborState(neighborData) == INERT && getNeighborColor(neighborData) == blinkColor) {
        //this neighbor is my color and inert - we need to wait to dissolve
        foundUnmatchedNeighbors = true;
      } else if (getNeighborState(neighborData) == RAINBOW) {
        foundUnmatchedNeighbors = true;
      }
    }
  }

  if (foundUnmatchedNeighbors == false) {
    setFullState(DISSOLVING);
    createNewBlink();
    dissolveTimer.set(DISSOLVE_TIME);
  }
}

void dissolvingLoop() {
  if (dissolveTimer.isExpired()) {
    setFullState(nextState);
    //TODO: THIS IS WHERE SPECIALS SHOW UP
  }
}

void resolveLoop() {

}

void explodeLoop() {
  //so in here all we do is make sure all of our neighbors are in MATCH_MADE
  bool hasInertNeighbors = false;

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getNeighborState(neighborData) == INERT) {
        hasInertNeighbors = true;
      }
    }
  }

  if (hasInertNeighbors == false) {
    signalState = MATCH_MADE;
  }
}

void bucketLoop() {

}

void createNewBlink() {

  //always become a new color
  previousColor = blinkColor;
  blinkColor = (blinkColor + random(NUM_COLORS - 2) + 1) % NUM_COLORS;

  if (specialState == INERT) {//this is a regular blink becoming a new blink

    matchesMade++;
    if (matchesMade >= MATCH_GOAL) {//normal blink, may upgrade
      byte whichSpecial = random(1);
      switch (whichSpecial) {
        case 0://a rainbow piece!
          nextState = RAINBOW;
          specialState = RAINBOW;
          break;
        case 1:
          nextState = INERT;
          specialState = EXPLODE;
          break;
      }
    } else {
      nextState = INERT;
      specialState = INERT;
    }

  } else if (specialState == RAINBOW) {

    //this is a rainbow blink becoming normal again
    matchesMade = 0;
    nextState = INERT;
    specialState = INERT;

  } else {

    //special blink, just needs a new color
    if (wasActivated) {
      matchesMade = 0;
      nextState = INERT;
      specialState = INERT;
    }

  }

}

void setFullState(byte state) {
  signalState = state;
}

byte getNeighborState(byte data) {
  return ((data >> 3) & 7);
}

byte getNeighborColor(byte data) {
  return (data & 7);
}
