
import processing.serial.*;
int frame =50;
//=========USER MODIFIABLE VARIABLES=========
final int PORT_NUMBER= 3; 
// port ur thing is connected to, check the serial monitor print when u first run it
// for which port ur using. Doesn't work when other programs llike Arduino ide are using the
// same port
boolean useComicSans = true; //use comic sans?
color bkgd =color(0); //background color
char x_axis= 'r'; //key to start/stop x axis dilation
char y_axis= 't'; //key to start/stop y axis dilation
float dotSize = 3; // data point size(radius i believe)
int worldx = 800;
int worldy = 500; 
char[] splitChar = {'\n',','}; //single characters only

int baudRate = 460800; //speed at which serial sends data (ex Serial.begin(115200) has baud rate 115200)
int[] origin = new int[] {2*frame,(worldy-frame)/2}; // where the origin is placed on t
int[] dimension = new int[] {worldx-4*frame,200};    // rectangle size that bounds graph lines 
int[] user_defTickDistbyPixel =new int[] {50,50}; // no idea what this is 
float[] scale =  {2f,2f}; //scale of the xy axis labels, here it is 2.5 and 1.5
color[] colorArr = {#7ff4ff};
//===========================================
// Keys: 1
// Description: tracks 1 variable with a plot. graph starts the instant serial data
// has been recieved. Data to be plotted must be sent with  1 and only one \n to be plotted.
// tasks to be done
// - add lsit of acceptable data separator charaters
// - remove all data that isn't valid for float
// - hoepfully desmos pan screen like thingy

PFont comicSansBold;
Graph g1;
Serial serial1;
void setup() {
  //fullScreen();
  size(900,800);
  noSmooth();
  //printArray(PFont.list());
  if(useComicSans){
    comicSansBold=createFont("ComicSansMS-BOLD",32); //48
    textFont(comicSansBold);
  }
  background(bkgd);
  printArray(Serial.list());
  serial1 = new Serial(this, Serial.list()[PORT_NUMBER], baudRate);
  g1 = new Graph(origin, user_defTickDistbyPixel,scale,serial1,dotSize,dimension,splitChar,colorArr);
  g1.render(); 
 delay(1000);
frameRate =60;
}

void draw() {
    background(bkgd);
    g1.render();
    
  if(!g1.getStatus()){
    g1.read();
    g1.plot();
  }else{
   
  }
}void mouseWheel(MouseEvent event){
  float e=event.getCount();
  g1.changeTickDist(e);
 //--> update axes--> updte graph
}
void keyPressed(){
  if (key == 49){
    g1.paws();
  }
  if(key == x_axis){
   g1.dilationState(0); 
    //stop x dilation
    //only y dilation
  }else if(key== y_axis){
    g1.dilationState(1); 
    //stop y dilation 
    //only x dilation
  }
}
