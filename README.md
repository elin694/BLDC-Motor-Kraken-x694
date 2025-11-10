# In Progress :)
Hello! In this repository, I have saved my in progress work of my BLDC Motor project! Other relevant links below:  
Google Doc where I document most of my progress and plan out my design:
https://docs.google.com/document/d/19WLVvq-dfDWW1GdS-zNIwnykFYo5D6YfktGFYB5Ziv4/edit?usp=sharing.  

>Google Sheets (which I use to do most of the electronic calculations): https://docs.google.com/spreadsheets/d/1zoV-iSvWJgggNpqaQPVjK8pLd_YnIrjemZmvVzTJ-E0/edit?usp=sharing.  

>Onshape CAD model (public): https://cad.onshape.com/documents/441ee0228635d6e7f63cc692/w/1e3ba592a198b8a0d94a5698/e/7b7b7b2b2a7df3ccfb7c86bf?renderMode=0&uiState=69111e453288d4b486afb941

#Goal
The Goal of this project is to use as many parts that I have scavenged and lying around to make a mock-Kraken X60, a popular FIRST Robotics Competition (FRC) motor. That involves:
- A 28 gauge enameled copper wire from BingoTech
- 10mmx5mmx40mm neodymium 52 magnets
- 3D printed parts (from the NYC First Stem Center)
- Parts from the ELEGOO Arduino Starter Kit
- and many more.
While I tried my best to make my part as close as possible to real motor (in size and shape), there were certain limitations that forced me to bend the rules a little- I didn't have a spline  XS shaft, nor could I have manufactured one (I couldn't find any scrap Kraken X60's either), so I replaced it with a 1/2" hex shaft.
>Ultimately, the goal is to make a really cheap version of the actual motor that is as accurate as possible so I can confuse my friends, but also as a gift/homage to team 694.

Designing the part:
As per the Dunning-Kruger effect, I originally thought this was going to be an easy project-as seen in the images below (I underestimate wire space and how much I needed, tolerance, wiring, and magnet size).

I initially started out with an out-runner rotor design because I wanted to maximize the number of magnets I can put inside, thinking that it will increase my motor torque. That clearly wasn't right.

<img src="readMeImages/2026%20bldc%20v3.png" alt="bldc v3" height="300"><img src="readMeImages/2026%20bldc%20v4.png" alt="bldc v4" height="300">

^^(version 3 and version 4)
- You can see that after I printed this version and tested it, I realized that a measly 300mA and few strands of wire wasn't going to provide enough magnetic force to attract the rotor. I decided to make slots in the next version so I could put in steel or iron to increase core strength.

<img src="readMeImages/2026%20bldc%20v7_1.png" alt="bldc v7_1" height="300"><img src="readMeImages/2026%20bldc%20v7_3.png" alt="bloc v7_3" height="300">

^^(version 7_1 and version 7_3)
- I was going to fill the gaps with ferromagnetic metal, but I didn't have any solid piece on hand. Somehow I thought the mandrels of steel rivets that the STEM center provided would do magic, but after more testing, it didn't work. I realized that and redesigned to maximized the amount of wires that I loop around. 
<img src="readMeImages/2026%20bldc%20v18.png" alt="bldc v18" height="300">

- After I while, I settled on using the AS5600 magnetic sensor for FOC feedback. However, I hadn't design my motor with concealing this piece in mind, and I realized I had to do another redesign. The bigger problem was that packaging was going to get challenging on another level, and I was already pushing the thinness of the PETG stator to the limits. Ultimately I switched to an in runner rotor design. Ironically, later on through accidental research, I would discover that the Kraken x60 itself is an out runner motor. However, since only external appearance and functionality mattered to me, I continued my project.

<img src="readMeImages/2026%20bldc%20v26.png" alt="bldc v26" height="300">
<img src="readMeImages/2026%20bldc%20v26%20exploded.png" alt="bldc v26 exploded view" height="300">

After printing many case designs, v27 is my current one.
left to right: V1, v3, v4, v7_3, v18, v26, current one
<img src="readMeImages/2026%20bldc%20parts.jpg" alt="grid of 3d printed prototype parts " height="500">

