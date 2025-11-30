 import java.util.Arrays;
import processing.serial.*;
//import java.util.PVector;
public class Graph extends Axes {
  //======USER EDITABLE VARIABL ES=========================
  //======================================================
  Serial s;

  //private int thickness = 3;
  private color[] rgb = {color(0, 0, 255)}; //point colors
  private int colorPointer = 0;
  private Float[][] data= new Float[500000][2]; //val,seconds
  private int dataSize = 0;
  private PVector[] pointShift = new PVector[500000]; //val,seconds
  private int pointShiftSize = 0;
  private char[] sep;
  private long start =0;
  private long pauseTime =0;
  private float radius;
  private int debugInTracker =0;
  private int pointer; //for closest point to mouse
  //if we zoom out, graph +
  //for every value the graph shows

  private boolean debug= false;
  private boolean debug2 = false; //see serial
  public Graph(int[] og,
               int[] inputTickDist,
               float[] scale,
               Serial serial,
               float size,
               int[] d,
               char[] end,
               color[] keys) {
    super(og, inputTickDist, d, scale);
    s=serial;
    radius = size;
    sep = end;
    rgb=keys;
  }
  public boolean read() { //read Serial +add (((((data))))) if available
    String buffer = "";
    String temp="";
    boolean stay = true;
    if (debug)  print("read, ");
    if (s.available()> 0) {
      while (s.available()> 0) {
       // if (debug) println("There is serial data");
        char c= s.readChar();
        for (char a: sep) { //fails if end signals are contained within each other
          if (c==a) {
            stay =false;
            break;
          }
        }
        println("buffer: "+ buffer);

        if (!stay) {
          stay=true;
          temp=buffer.trim();
          buffer="";
          if (temp.length()>0) {
            float val= (float)Double.parseDouble(temp);
            println("# "+val);
            if (pointShiftSize == 0) {
              start= millis();
              data[dataSize]=(new Float[] {0.0, val});
            } else {
              data[dataSize] = new Float[] {(millis()-start)/1000.0, val};
            }
            dataSize++;
          }
        } else {
          buffer+=c;
        }
      }
      if (dataSize-pointShiftSize> 0) {
        Float[] atemp;
          int lowestP = pointShiftSize;
        for (int i = dataSize-lowestP; i>0; i--) {
          atemp = data[lowestP-1+i];
                      if (debug) println("data:" +Arrays.toString(data[i]));
          float xShift =atemp[0]/(super.scale[0]/super.c_tickDist[0]); //val2/(val1/dist1) = dist2
          float yShift = atemp[1]/(super.scale[1]/super.c_tickDist[1]);
          pointShift[pointShiftSize] = (new PVector(xShift, -yShift));//accoutned for Prcoesisng world graph
          pointShiftSize++;
        }
       if(debug2) p();
      
        return true;
      }
    }
    return false;
  }
  public void plot() { 
    //data();
    //graphs line and connects to previous one, no clear screen
    if (debug)    print("plot, ");
    ///===
    colorPointer = dataSize%rgb.length;
    strokeWeight(3);
    for (int f=rgb.length-1; f>=0; f--) {
       stroke(rgb[f]);
      beginShape(POINTS);
      if (debug)    print("plot2 ");
      for (int k=(dataSize-f)/rgb.length-1; k>=0; k--) {
        if(debug2){
          print("r: " + red(rgb[f])+", g: "+green(rgb[f])+", b: "+blue(rgb[f])+" \n");
        println("datalen:" +dataSize+", ");
        println("k:" + k+", ");
        println("rgb:" + rgb.length+", ");
        println("f: " + f+", ");
        println("product: "+ (k*rgb.length+f)+", \n");
      }
       // k*rgb.length+f = index that accounts color f and i taken ever rgb.length
        if(pointShift[k*rgb.length+f].x*super.dilation[0]<super.dimensions[0] &&
           pointShift[k*rgb.length+f].y*super.dilation[1]<super.dimensions[1] &&
           pointShift[k*rgb.length+f].y*super.dilation[1]>-super.dimensions[1]){
           vertex(pointShift[k*rgb.length+f].x*super.dilation[0]+super.origin[0],pointShift[k*rgb.length+f].y*super.dilation[1]+super.origin[1]);

         }
       
      }
      // if(debug)    print("plot2.75 ");
      endShape(CLOSE);
      //noStroke();
      //fill(rgb[f]);
      //ellipseMode(RADIUS);
      //for(int k= ((((((data))))).size()-f/rgb.length)-1; k>=0 ;k-=50){
      //  ellipse(pointShift[k].x*super.dilation[0]+super.origin[0],pointShift[k].y*super.dilation[1]+super.origin[1],radius,radius);
      //} if(debug)    print("plot3 ");
    }
    //==
   /* if ((((((data))))).size()>0) {
      // strokeWeight(thickness);
      //   line(origin[0],,,);
    }
    */
}

  public Serial getSerial() {
    return this.s;
  }
  public void paws() {
    super.pause = !super.pause;
    if(super.pause){
      pauseTime = millis();
    }else{
      start += millis()-pauseTime;
      pauseTime =0;
      stroke(255,0,0);
      strokeWeight(1);
      line(pointShift[pointShiftSize-1].x*super.dilation[0]+origin[0],origin[1]-40.0,pointShift[pointShiftSize-1].x*super.dilation[0]+origin[0],origin[1]+40.0);
    }
  }
  public boolean getStatus() { //if pause get the status
    if (super.pause) {
      textSize(20);
      line(mouseX, super.origin[1]-super.dimensions[1], mouseX, origin[1]+super.dimensions[1]);
      this.plot();
      float relativeX = (mouseX-super.origin[0])/super.dilation[0]; //relative mouseX in dilation
      if (pointShift[pointer].x>relativeX) { //check right of line
        for (int i=min(200,dataSize-pointer-1); i>0; i--) {
          if (abs(relativeX-pointShift[pointer].x) > abs(relativeX-pointShift[i].x)) {
            pointer=i;
          } else {
            break;
          }
        }
        //print(mouse+", " + pointShift[(i)+"\n");
        noStroke();
        fill(0, 255, 255);
        rectMode(CORNER);
        rect(width-180, 0, width, 70);
        fill(0, 0, 255);
        text("X val: "+data[pointer][0], width-150, 30);
        text("Y val: "+data[pointer][1], width-150, 60);
        text(""+relativeX, width-150,85);
      } else {
        for (int i=min(200, dataSize-pointer-1); i>0; i--) {
          if (abs(relativeX-pointShift[pointer].x) > abs(relativeX-pointShift[i].x)) {
            pointer=i;
          } else {
            break;
          }
        }
        //print(mouse+", " + pointShift[(i)+"\n");
        noStroke();
        fill(0, 255, 255);
        rectMode(CORNER);
        rect(width-180, 0, width, 70);
        fill(0, 0, 255);
        text("X val: "+data[pointer][0], width-150, 30);
        text("Y val: "+data[pointer][1], width-150, 60);
        text(""+relativeX, width-150,85);
      }
    }
    return super.pause;
  }
  public void data(){  
    println();
    print("data("+dataSize+"): ");
    for(int i = 0;i<dataSize;i++){
      print(Arrays.toString(data[i])+", ");
    }
    println();
     print("shiftPoitnSize("+pointShiftSize+"): ");
    for(int i = 0;i<pointShiftSize;i++){
      print((pointShift[i])+", ");
    }
  }
  public void p() {
    if (true) {
      p(10);
      int i = dataSize-1;
      if (i>0) {
        int k = width-150;
        int h = height-100;
        int m = 20;
        if(data[i][1]>1.1){
             println(i);
           }
           
        text("data["+i+"]="+Arrays.toString(data[i])+"",k,h);
        text("dataLocation: "+ pointShift[i]+", ",k,h+m);
        text("Dp len: "+pointShiftSize+", ",k,h+2*m);
        text("dataLen: "+dataSize+", ",k,h+3*m);
      }
    }
  }
  public void p(int a){
    if(debugInTracker ==0){
      debugInTracker =a;
      for (int i=a;i>=0 && dataSize-1-i>=0;i--){
        print("[i" + (dataSize-1-i)+"," +data[dataSize-1-i][1]+").");
      }
      println();
    }else{
      debugInTracker--;
    }
  }
}
