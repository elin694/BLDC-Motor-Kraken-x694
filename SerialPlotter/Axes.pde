import java.util.Arrays;
import java.util.ArrayList;
import processing.serial.*;
public class Axes{
   private final float minTickDist = 2E-44;
   private final float maxTickDist = (2E38);
   private boolean debug = false;
   private String[] axesName = {"Time in seconds","var"}; 
   //private String[] mode = {"autoScroll", "fixedMotion"};
   //======USER EDITABLE VARIABLES=========================
   //Aesthetic Variables
   private int labelSize =13; //Size of Label text
   private color labelColor = color(100,100,255); //color of the numbers
   private color axisColor = color(255,0,0); //color of the axes, in rgb
   private int thickness = 3; //how thick you want the axes to be
   private int[] tickSize={2,8}; //in vertical orientation
   
   private int dividers =3; //number of horizontal lines each y tick space is divided into
   //======================================================
   public boolean pause=false; 
   //public static int maxVal =;
   public float[] dilation = {1.5,1.5};
   public boolean[] dilationDir = {true,true};
   //Stores which Axes are allowed to scale and Dilate
   
   private float[] c_tickDist =new float[2]; //tick distance
   private float[] tickDist=new float[2];
   private float[] scale; // Numerical initial value of the scale
   private int[] dimensions = new int[2];
   private int[] origin = {0,0};
   private int[] ogs = {0,0};   //Stores where the origin is shifted
   
   public Axes(int[] a, int[] inputTickDist,int[] dimen,float[] s){
     scale =s;
     origin[0] = a[0];
     origin[1]= a[1];
     c_tickDist[0] = inputTickDist[0];
     c_tickDist[1] = inputTickDist[1];
     tickDist[0] = inputTickDist[0];
     tickDist[1] = inputTickDist[1];
     dimensions = dimen;
     fill(0);
     ellipse(origin[0],origin[1],10,10);
   }
   
   public void render(){ //begisn by drawing axis, call during setup
     int a=5;//collapseMultiple;
     int b = 25;//const dist for pwoers (actual dist)
     float multipely= pow(a,ceil(log(b/tickDist[1])/log((float)a))); //take smaller exp (when tick shorten) 
     //not accounted in mult
     float multipelx= pow(a,ceil(log(b/tickDist[0])/log((float)a))); //take smaller exp (when tick shorten)
      //multiplxy not factored into scale/ticks
     //multiple witch ticks live
     
   if(debug)    print("axes Render, ");
     stroke(axisColor);
     strokeWeight(thickness);
     rectMode(CENTER);
     fill(axisColor);
     //x then y axis ticks
     line(origin[0],origin[1],origin[0]+dimensions[0], origin[1]); //x-axis
     line(origin[0],origin[1],origin[0], origin[1]-dimensions[1]); //y-axis
     line(origin[0],origin[1],origin[0], origin[1]+dimensions[1]); //-y-axis
     
      //X AXIS TICKS + LABELS
     strokeWeight(0);
     textSize(labelSize);
     for(int i=(int)(dimensions[0]/tickDist[0]/multipelx);i>=0;i--){ //x axis ticks + label
    if(debug&& i==0) println((int)(dimensions[0]/tickDist[0]/multipelx));
       rectMode(CORNER);
       fill(labelColor);
       if(i%2==0){
         text(String.valueOf(scale[0]*i*multipelx), origin[0]+i*tickDist[0]*multipelx-(labelSize/2.5)*tickSize[0], origin[1]+3*tickSize[1]);
       } else {
         text(String.valueOf(scale[0]*i*multipelx), origin[0]+i*tickDist[0]*multipelx-(labelSize/2.5)*tickSize[0], origin[1]+4.6*tickSize[1]);
       }
       fill(axisColor);
       rectMode(CENTER);
       rect(origin[0]+i*tickDist[0]*multipelx,origin[1],tickSize[0],tickSize[1]);
     }
     
     
     //starting y points in pxl:
     //Y AXIS TICKS
     float starty = dimensions[1] + (
           multipely*tickDist[1] + (
            -tickDist[1]*ogs[1]/scale[1] - dimensions[1] //pxl- arrow
           ) % multipely*tickDist[1]) %  multipely*tickDist[1]; //pxl-negative
           println("Starty pxl offset from ai (no tick): " + -starty);
           
     for(int i=(int)((starty+dimensions[1])/tickDist[1] / multipely);i>0;i--){
       float newi = i+((-starty-ogs[1])/multipely/tickDist[1]);//label onlys
              println(newi);
       fill(axisColor);
       rect(origin[0],origin[1]+starty-i*tickDist[1]*multipely,tickSize[1],tickSize[0]);
       //Y - Axis Labels
       fill(labelColor);
       text(String.valueOf(scale[1]*newi*multipely), origin[0]-10*tickSize[1],
         origin[1]+starty-i*tickDist[1]*multipely+2*tickSize[0]);
     }
     println("\n");
     strokeWeight(1);  //y axis alignment lines
     for(int i=(int)(dimensions[1]*dividers/tickDist[1]/multipely);i>=0;i--){
       line(origin[0],origin[1]-i*tickDist[1]*multipely/dividers,origin[0]+dimensions[0],origin[1]-i*tickDist[1]*multipely/dividers);
       line(origin[0],origin[1]+i*tickDist[1]*multipely/dividers,origin[0]+dimensions[0],origin[1]+i*tickDist[1]*multipely/dividers);
     }
     textSize(labelSize+5);
     text(axesName[0],origin[0]+dimensions[0]/2,origin[1]+60);
     fill(labelColor);
     textSize(10);
     text("Axes Scroll Enabled? x,y: ("+dilationDir[0]+"," + dilationDir[1]+")", origin[0]+20,origin[1]-dimensions[1]-10);
  }
   
  public void changeTickDist(float scrolls){ //begisn by drawing axis, call during setup
    float newFactor = (1-scrolls/100);
     if(dilationDir[0]){
       dilation[0]*= newFactor;
       if(c_tickDist[0]*dilation[0]>maxTickDist){
         dilation[0]= maxTickDist/c_tickDist[0];
       }else if(c_tickDist[0]*dilation[0]<minTickDist){
         dilation[0]= minTickDist/c_tickDist[0];
       }else{
       }
       tickDist[0]= c_tickDist[0]*dilation[0];
     }

     if(dilationDir[1]){
       dilation[1]*= newFactor;
       
       if(c_tickDist[1]*dilation[1]>maxTickDist){
         dilation[1]= maxTickDist/c_tickDist[1];
       }else if(c_tickDist[1]*dilation[1]<minTickDist){
         dilation[1]= minTickDist/c_tickDist[1];
       }else{
       }
       tickDist[1]= c_tickDist[1]*dilation[1];     }
     render();
   }
   public boolean dilationState(int i){
    this.dilationDir[i] = !this.dilationDir[i]; 
    return this.dilationDir[i]; 
    
   }
   /*
   public float getScaleX(){
     return scale[0];
   }
   public float getScaleY(){
     return scale[1];
   }
   public float getTickDistX(){
     return tickDist[0];
   }x
   public float getTickDistY(){
     return tickDist[1];
   }
   */
}
