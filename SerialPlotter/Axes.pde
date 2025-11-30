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
   private int[] tickSize={2,16}; //in vertical orientation
   private color labelColor = color(100,100,255); //color of the numbers
   private color axisColor = color(255,0,0); //color of the axes, in rgb
   private int thickness = 3; //how thick you want the axes to be
   private int dividers =1; //number of horizontal lines each y tick space is divided into
   //======================================================
   public boolean pause=false; 
   //public static int maxVal =;
   public float[] dilation = {1.5,1.5};
   public boolean[] dilationDir = {true,true}; //Stores which Axes are allowed to Dilate
   
   private float[] c_tickDist =new float[2]; //tick distance
   private float[] tickDist=new float[2];
   private float[] scale; // Numerical starting value of the scale, default
   
   private int chartWidth;  //graph dimension
   private int chartHeight;  //graph dimension
   private int x0; // in pixel dist (ref to PWORLD)
   private float[] ogs = {.1,0};   //Stores where the origin is shifted
   
   public Axes(int origin_X, int originY,
               int chart_width, int chart_height,
               int[] inputTickDist,float[] s){
     scale =s;
     x0 = origin_X;
     y0= origin_Y;
     chartWidth= chart_width;
     chartHeight= chart_height;
     c_tickDist[0] = inputTickDist[0];
     c_tickDist[1] = inputTickDist[1];
     tickDist[0] = inputTickDist[0];
     tickDist[1] = inputTickDist[1];
     dimensions = dimen;
   }
   
   public void render(){ //begisn by drawing axis, call during setup
   int a=5;//collapseMultiple;
     int b = 25;//const dist for pwoers (actual dist)
     float multipely= pow(a,ceil(log(b/tickDist[1])/log((float)a))); //take smaller exp (when tick shorten) 
     //not multiplied into tickDist or scale
     float multipelx= pow(a,ceil(log(b/tickDist[0])/log((float)a))); //take smaller exp (when tick shorten)
      //multiplxy not factored into scale/ticks
     //multiple witch ticks live
     
     drawMainAxes();
     drawTicksX(multipelx);
     drawTicksY(multipely);
     
     textSize(labelSize+5);
     text(axesName[0],x0+chartWidth/2,y0+60);
     fill(labelColor);
     textSize(10);
     text("Axes Scroll Enabled? x,y: ("+dilationDir[0]+"," + dilationDir[1]+")", x0+20,y0-chartHeight-10);

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
   
   //=========================== WRAPPED FUCNITONS ====================================
  public void drawMainAxes(){
     if(debug)    print("axes Render, ");
     stroke(axisColor);
     strokeWeight(thickness);
     rectMode(CENTER);
     fill(axisColor);
     
     //DRAW AXES AND INTERSECTION
     line(x0,y0,x0+chartWidth, y0); //x-axis
     line(x0,y0,x0, y0-chartHeight); //y-axis
     line(x0,y0,x0, y0+chartHeight); //-y-axis
     
  }
  
  public void drawTicksY(float numUnscaledTicksY){
       /*
     unit_distance from height midpoint to bottom bound: chartHeight/2 * scale[1]/3tickDist[1]
     unit_distance from true origin to height midpoint: originShift[1]
           difference = number unit from bottom bound to true origin
            number of shown (aka multipley) ticks before true origin: take % (multipely * tickDistance)
            Final val: For loop instance count: (include minor gridlines): 
            
           (chartHeight/2 * scale[1]/tickDist[1] - ogs[1]) /
           (multipely * tickDist[1] / dividers) +
           
          (chartHeight/2 * scale[1]/tickDist[1] + ogs[1]) /
           (multipely * tickDist[1] / dividers)            +1;
            
            2: when making tickines check (i % <minor gridline count> == #inor_gridliens before tick), 
            assume instance count starts at 1 and not 0
      */
    int totalTicksShown = 
              (int)((chartHeight/2 * scale[1]/tickDist[1] - ogs[1]) /
               (numUnscaledTicksY * scale[1] / dividers)) +
              (int)((chartHeight/2 * scale[1]/tickDist[1] + ogs[1]) /
               (numUnscaledTicksY * scale[1] / dividers))  +1;
           
     for(int i = totalTicksShown;i>0;i--){
       int newIterator = i-
                         ((int)((dimensions```[1]/2 * scale[1]/tickDist[1] + 
                           ogs[1]) /
                           (numUnscaledTicksY * scale[1] / dividers))
                           + 1); 
       /*
       (int)((chartHeight/2 * scale[1]/tickDist[1] + 
               ogs[1]) /
               (numUnscaledTicksY * tickDist[1] / dividers))
       check (i % <minor gridline count> == #inor_gridliens before tick)
       
       TICK PLACEMENT
       y0- ogs1[]*conversion_to_pixel +  
       [(int)((chartHeight/2 + ogs[1]*tickDist[1]/scale[1]) /
           (numUnscaledTicksY* tickDist[1] / dividers))*(numUnscaledTicksY* tickDist[1] / dividers]  
       */
       fill(axisColor);
       println(numUnscaledTicksY);
       rect(x0,
           y0- ogs[1]*tickDist[1]/scale[1] +  
           (
             (int)((chartHeight/2 + ogs[1]*tickDist[1]/scale[1]) /
             (numUnscaledTicksY* tickDist[1] / dividers))
             * (numUnscaledTicksY* tickDist[1] / dividers)
           )
           + i * numUnscaledTicksY *tickDist[1]/dividers
           ,
           tickSize[1],tickSize[0]);
           
           
           
           
       //Y - Axis Labels|
       int[] cset = {#FF00EF,#CFFF04,#39FF14, #4166F5, #9D00FF, #FFFFFF};
       //int marker= 0;
       for(int marker = 0; marker <6; marker++){
         fill(cset[marker]);
         rect(x0, 
 //START OF TICK POSITION CODE 
           y0 - marker*tickDist[1]/scale[1] +
             (
               (int)((200 + ogs[1]*tickDist[1]/scale[1]) / //difference in space btwn orgin
               (numUnscaledTicksY* tickDist[1] / dividers))
               * (numUnscaledTicksY* tickDist[1] / dividers)
             )
             + marker * numUnscaledTicksY *tickDist[1]/dividers
  //END OF TICK POSITION CODE
         ,2*tickSize[1],2*tickSize[0]);
       }
       //MAKING THE LABELS (NUMBER SCALE)
       fill(labelColor);
       text(String.valueOf((i/dividers - 1
                             -(int)((chartHeight/2 * scale[1]/tickDist[1] + 
                              ogs[1]) /
                              (numUnscaledTicksY * scale[1]))
                            )
                          *scale[1] *numUnscaledTicksY
                          ),
            x0-15,
            ((i/dividers - 1)*tickDist[1]/scale[1] //<- divide by num divders cuz tick
               +(int)((chartHeight/2 + 
                      tickDist[1]/scale[1] * ogs[1]) /
                (numUnscaledTicksY * tickDist[1]))
             )
            *scale[1]
            );
     }
     
        strokeWeight(1);  //y axis alignment lines
     for(int i=(int)(chartHeight*dividers/tickDist[1]/numUnscaledTicksY);i>=0;i--){
       line(x0,y0-i*tickDist[1]*numUnscaledTicksY/dividers,x0+chartWidth,y0-i*tickDist[1]*numUnscaledTicksY/dividers);
       line(x0,y0+i*tickDist[1]*numUnscaledTicksY/dividers,x0+chartWidth,y0+i*tickDist[1]*numUnscaledTicksY/dividers);
     }
  }
  
  public void drawLabelsY(float numUnscaledTicksY){
    
  }
  
  public void drawTicksX(float numUnscaledTicksX){      
    
      //X AXIS TICKS + LABELS
     strokeWeight(0);
     textSize(labelSize);
     for(int i=(int)(chartWidth/tickDist[0]/numUnscaledTicksX);i>=0;i--){ //x axis ticks + label
      if(debug&& i==0) println((int)(chartWidth/tickDist[0]/numUnscaledTicksX));
       rectMode(CORNER);  
       fill(labelColor);
       if(i%2==0){
         text(String.valueOf(scale[0]*i*numUnscaledTicksX), 
                             x0+i*tickDist[0]*numUnscaledTicksX-(labelSize/2.5)*tickSize[0],
                             y0+3*tickSize[1]);
       } else {
         text(String.valueOf(scale[0]*i*numUnscaledTicksX), 
                             x0+i*tickDist[0]*numUnscaledTicksX-(labelSize/2.5)*tickSize[0],
                             y0+4.6*tickSize[1]);
       }
       fill(axisColor);
       rectMode(CENTER);
       rect(x0+i*tickDist[0]*numUnscaledTicksX,y0,tickSize[0],tickSize[1]);
     }
  }
}
