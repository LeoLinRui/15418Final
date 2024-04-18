//https://www.youtube.com/watch?v=IKB1hWWedMk
//https://github.com/CodingTrain/website/blob/master/CodingChallenges/CC_011_PerlinNoiseTerrain/P5/sketch.js

var cols, rows;
var scl = 30;
var w = 3000;
var h = 2000;

var terrain = [];

function setup() {
  createCanvas(windowWidth, windowHeight, WEBGL);
  smooth();
  
  cols = w / scl;
  rows = h/ scl;

  for (var x = 0; x < cols; x++) {
    terrain[x] = [];
    for (var y = 0; y < rows; y++) {
      terrain[x][y] = 0; //specify a default value for now
    }
  }
}

function windowResized() {
  resizeCanvas(windowWidth, windowHeight);
}

function draw() {
  yoff = 0;
  for (var y = 0; y < rows; y++) {
    var xoff = 0;
    for (var x = 0; x < cols; x++) {
      terrain[x][y] = map(noise(millis() / 10000 + xoff, yoff), 0, 1, -200, 200);
      xoff += 0.1;
    }
    yoff += 0.1;
  }


  background(0, 0, 0);

  var locX = mouseX - height / 2;
  var locY = mouseY - width / 2;

  noFill();
  strokeWeight(2);
  stroke(81, 52, 72);

  translate(-w/2, -h/2);

  for (var y = 0; y < rows-1; y++) {
    beginShape(TRIANGLE_STRIP);
    for (var x = 0; x < cols; x++) {
      vertex(x*scl, y*scl, terrain[x][y]);
      vertex(x*scl, (y+1)*scl, terrain[x][y+1]);
    }
    endShape();
  }
}