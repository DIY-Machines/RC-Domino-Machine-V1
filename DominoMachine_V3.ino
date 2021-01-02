#include <Servo.h>
#include <LiquidCrystal.h>

/*******************************************************
V3 

DIY Machines project - Domino laying machine.

Instructional build video: https://youtu.be/QsAplPLtriw 
Help and FAQs: https://www.diymachines.co.uk/rc-domino-laying-machine

www.youtube.com/c/diymachines

********************************************************/

// These are the variables that you may need to adjust when configuring your machine

const int carouselStackHeight = 22; //how many dominoes are in a single fully stocked vertical stack on the carousel. The machine I built actaully carries 19 in a stack. I added another three to create a firebreak every 19 dominoes.
const int carouselQtyStacks = 7; //how many of the vertical stacks are on the carousel. Once all have been depleted the machine will prompt for a refill.

const int amountToTurn = 40; //How extreme should the steering angle be? Can be a value between 1 and 90. Too high and the servo will continously strain itself.
const int carouselServoNeutral = 91; //Continuous rotation servos should be in neutral at 90. You may need to tweak this up or down if you find your servo turns when it should not be.
const int dcMotorSpeed = 120; // Adjust the speed/power of the DC motor which in turn effects the travel speed and time allowed for domino to fall before being ejected. I have built several all of these machine and they have varied between 90 to 120.

// You should get Auth Token in the Blynk App when configuring for Bluetooth control
// Go to the Project Settings (nut icon)
char auth[] = "MESOKvi72SGmw5OlxhIkjmz7OEHvhYt_";




LiquidCrystal lcd(8, 9, 4, 5, 6, 7); // select the pins used on the LCD panel

Servo steeringServo;  // create a servo object to control a servo used for steering
Servo carouselServo;  // create a servo object to control a servo used for rotating the carousel

//for controlling the L298N and attached DC motor
const int enA = 3;
const int in1 = A1;
const int in2 = A5;

//pins used for the two contact switches
const int carouselSwitch = A2;
const int ejectorSwitch = A3;


int remainingInCurrentStack = carouselStackHeight;
int remainingInCarousel = carouselStackHeight * carouselQtyStacks;
int carouselSwitchState;
int ejectorSwitchState;
boolean firebreakMode = false;
boolean bluetoothControlled = false;


/* Blynk Things begin
---------------------------------------------------------------------
 */

int blynkDirectionOfTravel = 1;

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <BlynkSimpleSerialBLE.h>

/* Blynk Things end
---------------------------------------------------------------------
 */

 
// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
/* if (adc_key_in < 50)   return btnRIGHT;
 if (adc_key_in < 250)  return btnUP;
 if (adc_key_in < 450)  return btnDOWN;
 if (adc_key_in < 650)  return btnLEFT;
 if (adc_key_in < 850)  return btnSELECT;
*/
 // For V1.0 comment the other threshold and use the one below:

 if (adc_key_in < 50)   return btnRIGHT;
 if (adc_key_in < 195)  return btnUP;
 if (adc_key_in < 380)  return btnDOWN;
 if (adc_key_in < 555)  return btnLEFT;
 if (adc_key_in < 790)  return btnSELECT;


 return btnNONE;  // when all others fail, return this...
}

void setup()
{
 Serial.begin(9600);

 pinMode(carouselSwitch, INPUT_PULLUP);
 pinMode(ejectorSwitch, INPUT_PULLUP);
  
 lcd.begin(16, 2);              // start the display library
 lcd.setCursor(0,0);

 steeringServo.attach(11);  // attaches the servo on pin D11 to the servo object
 carouselServo.attach(2);  // attaches the servo on pin D2 to the servo object
 carouselServo.write(carouselServoNeutral); // seems to stop judderiness which appear before it's first issued 'position'
 steeringServo.write(90); // set the steering servo to it's central forward facing position


 //setup motor control pins as outputs
 pinMode(enA, OUTPUT);
 pinMode(in1, OUTPUT);
 pinMode(in2, OUTPUT);
 // and then disable the DC motor
 digitalWrite(in1, LOW);
 digitalWrite(in2, LOW);


  printToLCD("Remove carousel","");
  delay(800);
  printToLCD("Is it removed?","Press any button");
  
  lcd_key = read_LCD_buttons();  // read the buttons
    while (lcd_key == btnNONE){
      lcd_key = read_LCD_buttons();  // keep reading the buttons
    }
     if (lcd_key != btnNONE)               // when any button is pressed, perforom one revolution of main DC drive
     {
        layDomino(1, 'F');
     }
  printToLCD("Choose control:","1=IDE 2=BT");
  lcd_key = read_LCD_buttons();  // read the buttons
    while (lcd_key == btnNONE){
      lcd_key = read_LCD_buttons();  // read the buttons
    }
     if (lcd_key == btnLEFT)               // depending on which button was pushed, we perform an action
     {
        printToLCD("Choose your code:","1 or 2?");
        delay(400);
     }
     if (lcd_key == btnRIGHT)               
     {
        bluetoothControl();
     }
     
}

void loop()
{

 lcd.setCursor(0,1);            // move to the begining of the second line
 lcd_key = read_LCD_buttons();  // read the buttons

 switch (lcd_key)               // depending on which button was pushed, we perform an action
 {
   case btnRIGHT:   //this is actually labelled as number two on our 3D printed projects
     {
      startProgramme();
      machineMove("F-Straight",12);
      machineMove("F-Straight",12);
      endProgramme();
     break;
     }
   case btnLEFT:   //this is actually labelled as number one on our 3D printed projects
     {
      startProgramme();
      machineMove("F-Straight",1);
      endProgramme();
     break;
     }
   case btnUP:   //this is labelled with an upwards arrow on our 3D printed projects
     {
      startProgramme();
      machineMove("F-Straight",4);
      machineMove("F-Right",4);
      machineMove("F-Left",4);
      machineMove("B-Straight",4);
      endProgramme();
     break;
     }
   case btnDOWN:   //this is labelled with a downwards arrow on our 3D printed projects
     {
     break;
     }
   case btnSELECT:  //this is actually labelled with a tick on our 3D printed projects
     {
     moveCarousel(1);
     break;
     }
     case btnNONE:
     {
     break;
     }
 }

}


void machineMove(String directionToTravel, int numberOfTurns){

  while (numberOfTurns > 0){
    //Serial.print("Number of turns remaining for this drawing instruction entry ");
    //Serial.println(numberOfTurns);

    
    countDominos();

      // now we dispense dominoes below after checking if we have any left above
      
      if (directionToTravel == "F-Right"){
      updateScreen("Forward right",numberOfTurns);
      steeringServo.write(90 - amountToTurn);
      layDomino(1, 'F');
      
    } else if (directionToTravel == "F-Left"){
      updateScreen("Forward left",numberOfTurns);
      steeringServo.write(90 + amountToTurn);
      layDomino(1, 'F');
      
    }else if (directionToTravel == "F-Straight"){
      updateScreen("Forward",numberOfTurns);
      steeringServo.write(90);
      layDomino(1, 'F');
      
    }else if (directionToTravel == "Forward"){   //just drive forward without updating steering angle
      updateScreen("Forward",numberOfTurns);
      layDomino(1, 'F');
      
    }else if (directionToTravel == "B-Right"){
      updateScreen("Reverse right",numberOfTurns);
      steeringServo.write(90 - amountToTurn);
      layDomino(1, 'B');
      
    }else if (directionToTravel == "B-Left"){
      updateScreen("Reverse left",numberOfTurns);
      steeringServo.write(90 + amountToTurn);
      layDomino(1, 'B');
      
    } else if (directionToTravel == "B-Straight"){
      updateScreen("Reverse",numberOfTurns);
      steeringServo.write(90);
      layDomino(1, 'B');
    }
    else if (directionToTravel == "Backward"){  //just drive backwards without updating steering angle
      updateScreen("Reverse",numberOfTurns);
      layDomino(1, 'B');
    }
    numberOfTurns--;
  }
  //delay(900); 
}

void countDominos(){
  if (firebreakMode == false){
    remainingInCurrentStack = --remainingInCurrentStack;
    remainingInCarousel = --remainingInCarousel;
  }
  //Serial.print("Number of dominos remaining in current stack ");
  //Serial.println(remainingInCurrentStack);
  //Serial.print("Number of dominos remaining in this carousel ");
  //Serial.println(remainingInCarousel);
  //Serial.println();
  Blynk.virtualWrite(V5, remainingInCarousel);
  
  if (remainingInCarousel == 0){
    printToLCD("Carousel empty","refill / replace");
    Blynk.virtualWrite(V4, 255);
    Blynk.setProperty(V4, "color", "#ebb607"); 
    //Serial.println("Need to refill the carousel or replace it with a ready loaded done. Press OK button when remedied....");
    delay(2000);
    printToLCD("When done press","key to continue");
    lcd_key = read_LCD_buttons();  // read the buttons
    while (lcd_key == btnNONE){
      lcd_key = read_LCD_buttons();  // read the buttons
    }
    printToLCD("Continuing...","");
    Blynk.virtualWrite(V4, 0);
    moveCarousel(1);
    delay(1000);
    remainingInCurrentStack = carouselStackHeight;
    remainingInCarousel = carouselStackHeight * carouselQtyStacks;
  }
  if (remainingInCurrentStack == 0){
    moveCarousel(1);
    remainingInCurrentStack = carouselStackHeight;
  }
}

void updateScreen(String directionTravelling, int remainingTurns){
    lcd.clear();
    lcd.setCursor(0,0);
    //Serial.println(directionTravelling);
    lcd.print(directionTravelling);
    lcd.setCursor(0,1);
    if (bluetoothControlled == false){
      lcd.print(remainingTurns);
      lcd.print(" remaining."); 
    }
    
}


void layDomino(int remainingDominoes, char directionOfTravel){
   ejectorSwitchState = digitalRead(ejectorSwitch);
     while (ejectorSwitchState == HIGH){
      moveMachine(10,directionOfTravel);
      ejectorSwitchState = digitalRead(ejectorSwitch);
     }
      moveMachine(300,directionOfTravel);
}


void moveMachine(int distance, char directionOfTravel){
  analogWrite(enA, dcMotorSpeed);
    if (directionOfTravel == 'F'){
      digitalWrite(in1, HIGH);
     digitalWrite(in2, LOW);
    } else {
      digitalWrite(in1, LOW);
     digitalWrite(in2, HIGH);
    }
     delay(distance);
     digitalWrite(in1, LOW);
     digitalWrite(in2, LOW);
     carouselServo.write(carouselServoNeutral);  //to keep main carosuel servo steady
}


void moveCarousel(int numberOfTurns){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Spinning");
  lcd.setCursor(0,1);
  lcd.print("carousel...");
  //Serial.println("Spinnning carousel");
  int i = numberOfTurns;
    while (i > 0) {
       carouselSwitchState = digitalRead(carouselSwitch);
       while (carouselSwitchState == HIGH){ //not engaged
        //Serial.println("Switch high");
          carouselServo.write(105);
          carouselSwitchState = digitalRead(carouselSwitch);
        }
       while (carouselSwitchState == LOW){  //engaged
        //Serial.println("Switch low");
          carouselServo.write(105);
          carouselSwitchState = digitalRead(carouselSwitch);
        }
        carouselServo.write(carouselServoNeutral);
        if (i == 1){
          bumpCarousel(170);
        } 
      i--;
    }
}

void bumpCarousel(int bumpSize){
  delay(100);
  carouselServo.write(78);
  delay(bumpSize); //bump
  carouselServo.write(carouselServoNeutral);
}

void firebreak(bool toggle){
  if (toggle == true){
    firebreakMode = true;
    printToLCD("Moving to fire","break position.");
      //Serial.println("Fire break mode is enabled");

      carouselSwitchState = digitalRead(carouselSwitch);
      while (carouselSwitchState == HIGH){  //not engaged
        //Serial.println("Switch high");
        carouselServo.write(100);
        carouselSwitchState = digitalRead(carouselSwitch);
      }
      carouselServo.write(carouselServoNeutral);
      carouselServo.write(80);
      delay(110);
      carouselServo.write(carouselServoNeutral);
      
  }
  else{
    firebreakMode = false;
    //Serial.println("Fire break mode is disabled");
    moveCarousel(7);
  }
  
}


void startProgramme(){
    printToLCD("Play programme:","");
    delay(500);
    moveCarousel(1);
}

void endProgramme(){
    printToLCD("Programme","completed.");
}

void bluetoothControl(){
  bluetoothControlled = true;
  //Serial.println("Controlling with bluetooth");
  printToLCD("Connect with the","Blynk app..");
  //SwSerial.begin(9600);
  Blynk.begin(Serial, auth);
  //Blynk.begin(auth);
  //Serial.println("Connected to Blynk");
  printToLCD("Connected to","Blynk. :-)");
  Blynk_Delay(1000);
  printToLCD("Ready to drive","from your phone.");
  Blynk.virtualWrite(V3, 1);
  Blynk.virtualWrite(V5, remainingInCarousel);
  while (1 > 0){
    Blynk.run();
    
    if (blynkDirectionOfTravel == 0 && firebreakMode == true) {
        machineMove("Backward",1); //drive backwards
      } else if (blynkDirectionOfTravel == 1) {
        lcd.clear();
        //do nothing
      } else if (blynkDirectionOfTravel == 2) {
        machineMove("Forward",1); //drive forwards
        bumpCarousel(30);
      }      
   }
}

void printToLCD(String Line1, String Line2){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(Line1);
  lcd.setCursor(0,1);
  lcd.print(Line2);
}


void Blynk_Delay(int milli){
   int end_time = millis() + milli;
   while(millis() < end_time){
       Blynk.run();
   }
}

  BLYNK_WRITE(V2) {   //used to rotate the carousel
   int pinValue = param.asInt(); // Assigning incoming value from pin V2 to a variable
   //Serial.print("Virtual pin number V2: ");
   //Serial.println(pinValue);
   if (pinValue == 1) {
      //Serial.println("Moving carousel");
      moveCarousel(1); //move the carousel to the next stack
      Blynk.virtualWrite(V0, 0);
    } else {
      //do nothing
   }
  }

    
BLYNK_WRITE(V1) {    //used to control the steering
  int pinValue = param.asInt(); // Assigning incoming value from pin V1 to a variable
  //Serial.print("Virtual pin number V1: ");
  //Serial.println(pinValue);
  steeringServo.write(pinValue);
  bumpCarousel(30);  //carousel servo twitches sometime so make sure it is still against the switch
  }

BLYNK_WRITE(V3) {   //used to control direction of travel.i.e forward, backward or stop.
    int pinValue = param.asInt(); // Assigning incoming value from pin V3 to a variable
    //Serial.print("Virtual pin number V3: ");
    //Serial.println(pinValue);
    blynkDirectionOfTravel = pinValue;
  }

BLYNK_WRITE(V0) {   //used to control if firebreak mode is engaged or not
 int pinValue = param.asInt(); // Assigning incoming value from pin V0 to a variable
 //Serial.print("Virtual pin number V0: ");
 //Serial.println(pinValue);
    if (pinValue == 1) {
      //Serial.println("Moving carousel to firebreak position");
      firebreak(true); //move the carousel to the firebreak position
    } else {
      firebreak(false);
   }

}
