# Kraken x694 BLDC Motor! (In Progress) :)

Hello! In this repository, I show my process and progress on my BLDC Motor project! Other relevant links below:    
>Google Sheets (which I use to do most of the electronic calculations): https://docs.google.com/spreadsheets/d/1zoV-iSvWJgggNpqaQPVjK8pLd_YnIrjemZmvVzTJ-E0/edit?usp=sharing.    
>  
>Onshape CAD model (public): https://cad.onshape.com/documents/441ee0228635d6e7f63cc692/w/1e3ba592a198b8a0d94a5698/e/7b7b7b2b2a7df3ccfb7c86bf.   
>  
>Videos from testing: https://drive.google.com/drive/folders/19Vp0OEzmQrTeg8EhbQTLMmb9ZPl5efjz?usp=sharing

## File Explanation

>SerialPlotter Folder: java code that I am working simultaneously to create a mimic- oscilloscope with another ESP32. I want to see what is happening to my electrical components and the really cool sinusoidal graphs that a motor can generate. It will run on Processing.  
>bldc-falstad.txt: up-to-date text file containing the main circuit design and exported from falstad.com. bldc-falstad-v2.txt and bldc-falstad-v1.txt are out-dated versions.  
>boostConv.txt: up-to-date text file containing the subcircuit design for my boost converter.  
>inductanceTest.txt: text file containing the schematic I used to test my motor phases.

# Acknowledgements

I used a lot of resources, including but not limited to: Aaron Damer's Transistor playlist (to learn AC signal analysis and Transistor configurations), MIT's OpenCourseWare, specifically 6.002 taught by Anant Agarwal (to learn basic electronics) and 6.622 taught by David Perrault (I learned the theory behind Power electronics and buck, boost, and buck boost converters), as well as my BWSI teaching Assistants (specifically Srikrishna for teaching MOSFET and Semiconductor theory), and ChatGPT, which I used as a search engine and consultant to critique my schematic and electronic designs.

# Goal

The Goal of this project is to use as many parts that I have scavenged and lying around to make a mock-Kraken X60, a popular FIRST Robotics Competition (FRC) motor. That involves:

* A 28 gauge enameled copper wire from BingoTech  
* 10mmx5mmx40mm neodymium 52 magnets  
* 3D printed parts (from the NYC First Stem Center)  
* Parts from the ELEGOO Arduino Starter Kit  
* and many more. While I tried my best to make my part as close as possible to real motor (in size and shape), there were certain limitations that forced me to bend the rules a little- I didn't have a spline XS shaft, nor could I have manufactured one (I couldn't find any scrap Kraken X60's either), so I replaced it with a 1/2" hex shaft.

Ultimately, the goal is to make a really cheap version of the actual motor that is as accurate as possible so I can confuse my friends, but also as a gift/homage to team 694.

# Designing the part:

As per the Dunning-Kruger effect, I originally thought this was going to be an easy project-as seen in the images below (I underestimate wire space and how much I needed, tolerance, wiring, and magnet size).  
I initially started out with an out-runner rotor design because I wanted to maximize the number of magnets I can put inside, thinking that it will increase my motor torque. That clearly wasn't right.  
<img src="readMeImages/2026%20bldc%20v3.png" alt="bldc v3" height="300"><img src="readMeImages/2026%20bldc%20v4.png" alt="bldc v4" height="300">

^^(version 3 and version 4)

* You can see that after I printed this version and tested it, I realized that a measly 300mA and few strands of wire wasn't going to provide enough magnetic force to attract the rotor. I decided to make slots in the next version so I could put in steel or iron to increase core strength.

<img src="readMeImages/2026%20bldc%20v7_1.png" alt="bldc v7_1" height="300"><img src="readMeImages/2026%20bldc%20v7_3.png" alt="bloc v7_3" height="300">

^^(version 7_1 and version 7_3)

* I was going to fill the gaps with ferromagnetic metal, but I didn't have any solid or strongly ferromagnetic pieces on hand. Somehow I thought the mandrels of steel rivets that the STEM center provided would do magic, so inside this version of the stator, I created large slots for the mandrels in the stator case. But, after running 200mA of current through a test coil wound around each stator slot, it still didn’t have a strong attraction to the N52 magnets, despite how I pack as many turns of wire around the slot as I could per phase. If the core material barely helped, my best bet was to  redesign the stator to maximize the amount of wires that I can loop around.

<img src="readMeImages/2026%20bldc%20v18.png" alt="bldc v18" height="300">

* After a while, I settled on using the AS5600 magnetic sensor for FOC feedback. However, I hadn't designed my motor with this piece in mind, and I realized I had to do another redesign. The bigger problem was that packaging was going to get challenging on another level, and I was already pushing the thinness of the PETG stator to the limits. Ultimately I switched to an in runner rotor design. Ironically, later on through accidental research, I would discover that the Kraken x60 itself is an out runner motor. However, since only external appearance and functionality mattered to me, I continued my project.
 
<img src="readMeImages/2026%20bldc%20v26.png" alt="bldc v26" height="300">  
<img src="readMeImages/2026%20bldc%20v26%20exploded.png" alt="bldc v26 exploded view" height="300">

After printing many case designs, v27 is my current one.

For each major stator iteration, I wrapped as much wire as I could fit per-phase on each stator slot, and “shorted” both ends to the Arduino kit’s 5V Power board module, providing around 200mA current, and testing the magnetic force. It was through this testing that I realized that I underestimated the space that the phase coils took up. I ended up pushing the limit on the smallest stator thickness, with my final print being 2-3 walls thick.

# Assembly

Because I chose a very small wire diameter, I had to compensate with a larger length, and so I needed around 170 feet for each phase. This meant wiring would be more tedious, error prone, and time consuming. I 3D printed a jig to measure 1 foot, and simply wrapped the 28 gauge wire along it, measuring around 165 turns, and carefully removing it.

Making scratches to the enamel was inevitable, despite my caution. To prevent phases from shorting with itself, I wrapped electrical tape over exposed copper (the windings won't reach the point of melting the tape). I also used a saltwater pinhole test to find less visible scratches for some phases   
![][image8]

I faced smaller issues with tolerance and fitting, as I attempted to squeeze every millimeter of space between the rotor magnets and stator so I could magnify the magnetic force when coils were activated. I had sanded down both the rotor and stator until the rolling friction of the bearing and drag were the only sources of friction.

This was the interior of the final product after months of design and 3D printing. The red PETG stator, chosen for its higher temperature resistance than PLA, held the windings, and the magnets were attached onto the rotor by pressfitting and with the double sided tape that came with the magnets I bought.  
![][image9]

left to right: V1, v3, v4, v7_3, v18, v26, current motor  
<img src="readMeImages/2026%20bldc%20parts.jpg" alt="grid of 3d printed motor prototype parts " height="500">

# Electronics:

Up to this point, I was focused only on learning the internals of BLDC motors by making one myself, and I had made one! I knew I needed a motor controller, but spared looking for one until it was finished. However, as I looked online, their prices were rocket high— reliable electric speed Controller ones came at around $30, $40, even $70 dollars for 1! It cost as much as my 3D printed motor I made, and it was even double that of my arduino starter kit! With these price tags, I wanted to get more than a simple motor controller; I should learn something long, lasting, and meaningful that’ll be of service to me in future. It was then I knew I’d make one, one that had much more value than a simple controller.

Luckily, my electronics journey had already started (around November of 2024). I’d always wanted to learn something new with the starter kit my parents purchased for me. I watched lessons and the playlist series of ELEC 110 and 220, taught by Joe Gryniuk at Lake Washington Technical College, and followed along with the corresponding textbook “Introduction to Electronics“ on train rides and spare time. 

## Breadboard Prototypes

I started by finding values to represent each motor coil in a circuit.  
First, I measured the linear resistance of each inductor using the ohm-meter on my MM450.  
![][image11]

Next, I wanted a good estimate of my motor phase inductances. Despite not having access to an inductance-measuring tool, I eventually figured out a viable formula, using the AC-signal analysis tools I learned  from Aaron’s Damer transistors playlist, as well as the node methods from 6.002 lectures by Anant Agarwhal.  
![][image12]![][image13]  
This was the circuit schematic for my voltage AC-source, shown on falstad.com.  
![][image14]

In the DC Darlington model above,  I used ESP32's DAC pins to generate a pseudo-sine wave to mimic an AC signal, which I fed into a Darlington amplifier to create an AC- voltage source. Using definitions and formulas for impedance, I determined an estimate of the phase inductances.  
![][image15]

These phases and 20mΩ current sense shunt resistors comprised of my “Motor phases” subcircuit in Falstad.com  
![][image16]

## Motor circuit:

![][image17]

My motor controller will use a 3 half-bridge configuration, one half-bridge for each phase. The power bus will be connected to a boost converter, taking the 12V battery input and producing 24V for the gates. Shunt current sense resistors in-line with motor phases are paired with current sense amplifiers to measure the current. Another buck-converter supplied the logic voltage of 5V, which will be used to power the ESP-33. The AS5600 magnetic encoder communicates with the ESP-32 using I2C. To reduce costs, I had asked my robotics coach for permission to bring broken FRC motor controllers home, which I disassembled to salvage PSMN1R0-30YLD enhancement-mode mosfets for my half bridges. 

I wanted to design my own boost and buck converter module as well, after being inspired from watching MIT’s Open Courseware 6.022 Power electronics series; however, I struggled to implement a feedback system for a stable output voltage.  
[basic 555 astable implementation for a boost converter]  
![][image18]

My thought process was this: a logic or feedback system could be made either by programming a microcontroller to take input and give output, or it could automatically be regulated by hardware. I didn’t want to use multiple microcontrollers, and I thought creating feedback systems with hardware was more elegant than programming, so I attempted to manipulate the voltage of the CTRL pin on the 555 timer (used in an astable output configuration). the core Integrated chip(IC) that provided the necessary switching logic. I first attempted to look for a mathematical relationship between the duty cycle and the CTRL pin voltage.  
 ![][image19]  
![][image20]  
![][image21]

Graphing showed me a direct (almost linear) relationship between the duty cycle and switching frequency, which I hoped I could utilize through some feedback network. However, it had many problems. Falstad showed that the circuit didn’t work as I predicted: adjustments would often change the 555 switching duty cycle and switching frequency. I spent multiple days attempting to solve this problem, but I realized if I continued, my project’s pacing would be slow, and so I decided to buy the converters online- I had learned my lesson of getting too stuck on the details before, from the long time I spent on designing motor cases, and the boost converter wasn’t my biggest priority.

![][image22]  
![][image23]

This was my final breadboard prototype circuit schematic. The main difference between this and my intended PCB design was that I made bootstrap circuits for my high-side MOSFET to use instead of the gate drivers that I lacked. I built this circuit, ran it, and was able to reach motor speeds up to 800rpm using 6-step commutation (see video).  
![][image24]  
[op-amps to represent Current Sense Amplifiers temporarily removed]

# Motor Characteristics

From my tests, my motor coils always struggled with generating a magnetic field. I did my share of research, both from my physics regents classes and online searching, and I learned about B = µ₀(N/ℓ)I. I had an air core for my solenoid, and my number of turns and wire length was already determined by my 4oz supply of wire. So, I had to increase my voltage.

After consideration, I decided to use a motor bus voltage of 23-24V maximum, as that would put my phase current at around 1A (if 2 phases were enabled); However, that would only occur when the motor stalls. In normal operation, I would put the motor current to .8A, as I was worried about motor temperature, and that was a safe limit that online sources and ChatGPT had suggested. 57 degrees was the temperature the PLA casing would start degrading or soften.  
![][image25]

## Designing around ESP32

From watching motor control algorithms, such as Field-Oriented Control (FOC) by Texas Instruments and also Janteen Lee’s series on FOC, I learned I needed shunts for phase current sensing. I will use 2 shunts to determine the current of 2 phases, and use Kirchoff's Current Law to determine the third phase current.

While most modern BLDC motors run with 20kHz PWM frequency, I chose to use 16kHz because 1) it was barely within my audio hearing range, and 2) I wanted to use ESP32’s ADC to read the measurements. For my first PCb, I’ve decided to sample with the ADC at 16kHz as well, so that I my measurements are always timed with the gate switching Using 16kHz allows me to attempt higher sampling frequencies in the future, such as 64kHz across 2 channels, totaling to 128kHz, which was about the limit of Espressif’s recommended ESP32 ADC reading speed.

After my circuit was finished, I recreated the schematic on EasyEDA and added more suitable components. While I haven’t wired the traces on the PCB, I have settled on a preferred component placement to minimize inductance and cross-talk across signals on my PCB. Below are the footprints as well as a 3D-model rendition of my current part placement.

While it is still a work-in progress, I’m excited for its completion!  
![][image26]  
![][image27]
