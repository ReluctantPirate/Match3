enum blinkStates {INERT, MATCH_MADE, DISSOLVING, EXPLODE, BEAM, BUCKET, RAINBOW, RESOLVE};
byte faceStates[6] = {INERT, INERT, INERT, INERT, INERT, INERT};
byte nextState = INERT;
byte specialState = INERT;

enum blinkColor {RD, YW, GR, BL, PR};
byte colorHues[5] = {0, 50, 75, 100, 200};
byte blinkColor = RD;
byte previousColor = RD;

Timer dissolveTimer;
#define DISSOLVE_TIME 1500

void setup() {
  randomize();
  blinkColor = random(4);
}

void loop() {

  //run loops
  FOREACH_FACE(f) {
    switch (faceStates[f]) {
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
        //explodeLoop();
        break;
      case BEAM:
        //beamLoop();
        break;
      case BUCKET:
        //bucketLoop();
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
  FOREACH_FACE(f) {
    byte sendData = (faceStates[f] << 3) + (blinkColor);
    setValueSentOnFace(sendData, f);
  }

  //temp display
  switch (faceStates[0]) {
    case INERT:
    case RAINBOW:
      inertDisplay();
      break;
    case MATCH_MADE:
      setColor(WHITE);
      break;
    case DISSOLVING:
      dissolveDisplay();
      break;
  }
}

////DISPLAY FUNCTIONS
void dissolveDisplay() {
  if (dissolveTimer.getRemaining() > DISSOLVE_TIME / 2) {//first half

    byte dissolveBrightness = map(dissolveTimer.getRemaining() - (DISSOLVE_TIME / 2), 0, DISSOLVE_TIME / 2, 0, 255);
    setColor(makeColorHSB(colorHues[previousColor], 255, dissolveBrightness));

  } else {//second half

    byte dissolveBrightness = map(dissolveTimer.getRemaining(), 0, DISSOLVE_TIME / 2, 0, 255);
    setColor(makeColorHSB(colorHues[blinkColor], 255, 255 - dissolveBrightness));

  }
}

void inertDisplay() {
  switch (specialState) {
    case INERT:
      setColor(makeColorHSB(colorHues[blinkColor], 255, 255));
      break;
    case RAINBOW:
      FOREACH_FACE(f) {
        setColorOnFace(makeColorHSB(colorHues[random(4)], 255, 255), f);
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


  if (sameColorNeighbors >= 2) {
    setFullState(MATCH_MADE);
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

byte matchesMade = 0;
#define MATCH_GOAL 4

void createNewBlink() {

  //always become a new color
  previousColor = blinkColor;
  blinkColor = (blinkColor + random(3) + 1) % 5;

  if (specialState == INERT) {//this is a regular blink becoming a new blink

    matchesMade++;
    if (matchesMade >= MATCH_GOAL) {//normal blink, may upgrade
      byte whichSpecial = random(3);
      switch (whichSpecial) {
        case 0://a rainbow piece!
          nextState = RAINBOW;
          specialState = RAINBOW;
          break;
        case 1:
          nextState = INERT;
          specialState = BUCKET;
          break;
        case 2:
          nextState = INERT;
          specialState = BEAM;
          break;
        case 3:
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

  }

}

void setFullState(byte state) {
  FOREACH_FACE(f) {
    faceStates[f] = state;
  }
}

byte getNeighborState(byte data) {
  return ((data >> 3) & 7);
}

byte getNeighborColor(byte data) {
  return (data & 7);
}
