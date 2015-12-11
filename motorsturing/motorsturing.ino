#include <Wire.h>
//#include "P&O3.h" 					//comment this line for development in arduino IDE

/*
  MMMMMMMM               MMMMMMMM     OOOOOOOOO     TTTTTTTTTTTTTTTTTTTTTTT     OOOOOOOOO     RRRRRRRRRRRRRRRRR
  M:::::::M             M:::::::M   OO:::::::::OO   T:::::::::::::::::::::T   OO:::::::::OO   R::::::::::::::::R
  M::::::::M           M::::::::M OO:::::::::::::OO T:::::::::::::::::::::T OO:::::::::::::OO R::::::RRRRRR:::::R
  M:::::::::M         M:::::::::MO:::::::OOO:::::::OT:::::TT:::::::TT:::::TO:::::::OOO:::::::ORR:::::R     R:::::R
  M::::::::::M       M::::::::::MO::::::O   O::::::OTTTTTT  T:::::T  TTTTTTO::::::O   O::::::O  R::::R     R:::::R
  M:::::::::::M     M:::::::::::MO:::::O     O:::::O        T:::::T        O:::::O     O:::::O  R::::R     R:::::R
  M:::::::M::::M   M::::M:::::::MO:::::O     O:::::O        T:::::T        O:::::O     O:::::O  R::::RRRRRR:::::R
  M::::::M M::::M M::::M M::::::MO:::::O     O:::::O        T:::::T        O:::::O     O:::::O  R:::::::::::::RR
  M::::::M  M::::M::::M  M::::::MO:::::O     O:::::O        T:::::T        O:::::O     O:::::O  R::::RRRRRR:::::R
  M::::::M   M:::::::M   M::::::MO:::::O     O:::::O        T:::::T        O:::::O     O:::::O  R::::R     R:::::R
  M::::::M    M:::::M    M::::::MO:::::O     O:::::O        T:::::T        O:::::O     O:::::O  R::::R     R:::::R
  M::::::M     MMMMM     M::::::MO::::::O   O::::::O        T:::::T        O::::::O   O::::::O  R::::R     R:::::R
  M::::::M               M::::::MO:::::::OOO:::::::O      TT:::::::TT      O:::::::OOO:::::::ORR:::::R     R:::::R
  M::::::M               M::::::M OO:::::::::::::OO       T:::::::::T       OO:::::::::::::OO R::::::R     R:::::R
  M::::::M               M::::::M   OO:::::::::OO         T:::::::::T         OO:::::::::OO   R::::::R     R:::::R
  MMMMMMMM               MMMMMMMM     OOOOOOOOO           TTTTTTTTTTT           OOOOOOOOO     RRRRRRRR     RRRRRRR
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////// GLOBAL VARIABLES - CONSTANTS - OVERAL SETTINGS//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//******Pin definitions********

//Sensors
const byte PIN_SPEED = A3;
const byte PIN_COLLISION = A4;

//Motor
const byte PIN_MOTOR_A = 9; 				//Forward engine
const byte PIN_MOTOR_V = 5; 				//Backward engine

//I2C Communication					//COMM master sender - MOTOR slave receiver
const byte I2C_ADDRESS = 2;

//Emergency interrupts
const byte PIN_EM_OUT = 0;
const byte PIN_EM_IN = 7;

//LCD
const byte PIN_LCD = 1;
//Unused Pins, will be flagged as INPUT
const byte UNUSED_PINS[] = {A0, A1, A2, A5, 4, 6, 8, 10, 11, 12, 13};
const byte AMOUNT_UNUSED_PINS = 11;			//needed to loop through the above array

//******Global variables********

//COMMUNICATION - Received by COMM
byte terminal = 0;					//0 = No Terminal, 1 = International Terminal, 2 = National Terminal


//ENGINE AND SPEED CONTROL
volatile byte direction = 2;   				//0 = backward, 1 = forward 2 = stand still - received by COMM
volatile int speed_COMM_raw = 0 ;  			//Speed wanted by COMM, can be changed by interrupt 0 - 255
int speed_COMM_sens = 0; 				//speed sensor value to be approached, calculated from speed_COMM_raw
byte speed_pwm = 0; 					//speed directly written to engine - PWM - 0-255

//SENSORS

//Speed
int speed_raw = 0; 					//measured value of speed sensor 0-1023

//Collision
int collision_raw = 0; 					//measured value of collision sensor 0-1023
bool collision_front = false;
bool collision_back = false;


const int BOTS_REF[] = {950, 390, 200};		/*
							measured value between 1023 - BOTS_REF[0]		=> none of the sensors detecting object
							measured value between BOTS_REF[0] - BOTS_REF[1] 	=> only front sensor detects object
							measured value between BOTS_REF[1] - BOTS_REF[2] 	=> only back sensor detects object
							measured value between BOTS_REF[2] - 0		   	=> front and back sensor detecting object
*/



//OTHERS
byte emergency_local = 0; 				//Local emergency level
volatile bool emergency_COMM = false; 			//COMM 	emergency level
bool debug = false;
bool booting = true;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// HELPER FUNCTIONS////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////



///SENSORS

void update_sensors() {


  //Speed sensor
  speed_raw = analogRead(PIN_SPEED);


  collision_raw = analogRead(PIN_COLLISION);	//Collision sensors

  bool found = false;				//Loop through array BOTS_REF_RAW until corresponding value range is found
  byte counter = 0;

  while ((not found) && (counter <= 2)) {
    if (collision_raw >= BOTS_REF[counter]) {
      found = true;
    }

    else {
      counter++;
    }
  }

  switch (counter) {
    case 0:
      collision_front = false;
      collision_back = false;
      break;

    case 1:
      collision_front = true;
      collision_back = false;
      break;

    case 2:
      collision_front = false;
      collision_back = true;
      break;

    case 3:
      collision_front = true;
      collision_back = true;
      break;

  }
}


//ENGINE CONTROL AND SPEED CALCULATION

void speed_send(bool dont_brake = true) {		//Send desired speed to engines

  if (dont_brake) { 				//purpose: speed_send(false) == brake

    if (direction == 0) {			//backward
      digitalWrite(PIN_MOTOR_A, LOW);
      analogWrite(PIN_MOTOR_V, speed_pwm);
    }
    else if (direction == 1) { 		//forward
      digitalWrite(PIN_MOTOR_V, LOW);
      analogWrite(PIN_MOTOR_A, speed_pwm);
    }
    else if (direction == 2) { 		//stand still => brake
      digitalWrite(PIN_MOTOR_V, LOW);
      digitalWrite(PIN_MOTOR_A, LOW);
    }

  }

  else {						//hard brake
    digitalWrite(PIN_MOTOR_V, HIGH);
    digitalWrite(PIN_MOTOR_A, HIGH);
    //speed_pwm = 0;
  }
}


void speed_COMM_to_speed_sens() {			//Calculates and updates value of speed_COMM_sens corresponding to last asked speed of COMM (speed_com_raw)

  speed_COMM_sens = speed_COMM_raw * 3; 		//Value of speed sensor is linear to real speed
  //Speed sensor must be adjusted so that max_speed = 255 * 3 (3.73volt)
}


void speed_calc() {					//Calculate desired speed written to engines

  speed_COMM_to_speed_sens();			//Updates speed_COMM_sens

  if (speed_COMM_sens == 0) { 			//stand still
    speed_pwm = 0;
  }

  //increase speed
  else if ((speed_COMM_sens > (speed_raw - 10)) && (speed_pwm < 255)) {
    speed_pwm += 1;
  }
  //decrease speed
  else if ((speed_COMM_sens < (speed_raw + 10)) && (speed_pwm > 0)) {
    speed_pwm -= 1;
  }
}


//EMERGENCY
void emergency_local_check() {
  //Local emergency status is divided in 6 levels

  //0: none of the sensors detecting object
  //1: train driving BACKWARD and only FRONT sensor detects object
  //2: train driving FORWARD and only BACK sensor detects object

  //3: train driving FORWARD and only FRONT sensor detects object
  //4: train driving BACKWARD and only BACK sensor detects object
  //5: both sensors detecting object

  //1,2,3,4,5 = emergency line to COMM will be set to HIGH
  //ONLY 3,4,5 = train will brake immediately

  //Level 0
  if ((collision_front == false) && (collision_back == false)) {
    emergency_local = 0;
    digitalWrite(PIN_EM_OUT, LOW);
  }
  //Level 1
  else if ((collision_front == true) && (collision_back == false) && (direction == 0)) {
    emergency_local = 1;
    digitalWrite(PIN_EM_OUT, LOW);
  }
  //Level 2
  else if ((collision_front == false) && (collision_back == true) && (direction == 1)) {
    emergency_local = 2;
    digitalWrite(PIN_EM_OUT, LOW);
  }
  //Level 3
  else if ((collision_front == true) && (collision_back == false)) {
    emergency_local = 3;
    speed_send(false); 			//brake
    digitalWrite(PIN_EM_OUT, HIGH);
  }
  //Level 4
  else if ((collision_front == false) && (collision_back == true)) {
    emergency_local = 4;
    speed_send(false); 			//brake
    digitalWrite(PIN_EM_OUT, HIGH);
  }
  //Level 5
  else if ((collision_front == true) && (collision_back == true)) {
    emergency_local = 5;
    speed_send(false);			//brake
    digitalWrite(PIN_EM_OUT, HIGH);
  }

}



//LCD
void setBacklight(byte brightness) {
  Serial1.write(0x80);  				//send the backlight command
  Serial1.write(brightness);  			//send the brightness value
}

void clearDisplay() {
  Serial1.write(0xFE);  				//send the special command
  Serial1.write(0x01);  				//send the clear screen command
}

void setlcdCursor(byte cursor_position) {
  Serial1.write(0xFE);  				//send the special command
  Serial1.write(0x80);  				//send the set cursor command
  Serial1.write(cursor_position);  		//send the cursor position
}

void update_lcd() {
    //clearDisplay();

    Serial1.print("SPEED ");
    Serial1.print(speed_COMM_raw);
    setlcdCursor(10);
    Serial1.print(speed_pwm);
    Serial1.print("m/s");
    setlcdCursor(16);
    Serial1.print(speed_raw);
    Serial1.print(" ");
    Serial1.print(emergency_local);
    Serial1.print(" ");
    Serial1.print(speed_COMM_sens);
  
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// I2C & INTERRUPT FUNCTIONS///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

//RECEIVE I2C MESSAGE
void i2c_receive(int bytes_received) {

  if (Wire.available() == 3) {
    direction = (byte) Wire.read();
    speed_COMM_raw = (int) Wire.read();
    terminal = (byte) Wire.read();
  }

  else if (Wire.available() == 2) {
    direction = (byte) Wire.read();
    speed_COMM_raw = (int) Wire.read();
    terminal = 0;
  }


  if (speed_COMM_raw == 0) {
    direction = 2;
  }
}


//EMERGENCY INTERRUPT FROM COMM
void emergency_COMM_isr() {

  digitalWrite(PIN_MOTOR_V, HIGH); 			//brake
  digitalWrite(PIN_MOTOR_A, HIGH);
  emergency_COMM = true;
  speed_COMM_raw = 0;
  direction = 2;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// MAIN SETUP /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


void setup() {

  //Input / output behavior of pins

  pinMode(PIN_SPEED, INPUT);
  pinMode(PIN_COLLISION, INPUT);

  pinMode(PIN_EM_OUT, OUTPUT);
  pinMode(PIN_EM_IN, INPUT);
  digitalWrite(PIN_EM_OUT, LOW);

  pinMode(PIN_MOTOR_V, OUTPUT);
  pinMode(PIN_MOTOR_A, OUTPUT);

  digitalWrite(PIN_MOTOR_V, LOW); 		//make sure engines are off
  digitalWrite(PIN_MOTOR_A, LOW);

  for (byte i = 0; i < AMOUNT_UNUSED_PINS; i++) {	//define unused pins as input
    pinMode(UNUSED_PINS[i], INPUT);
  }

  //I2C Communication
  Wire.begin(I2C_ADDRESS);      			//join i2c bus with address #2
  Wire.onReceive(i2c_receive); 			//read message from COMM

  //Attach emergency interrupt (emergency from COMM)
  //attachInterrupt(digitalPinToInterrupt(PIN_EM_IN), emergency_COMM_isr, RISING);

  Serial1.begin(19200); 				//Communication with LCD
  delay(2000);					//Boot time

  setBacklight(255);				//LCD on
  clearDisplay();
  booting = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// MAIN LOOP //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

  update_sensors();
  emergency_local_check();

  if ((emergency_local <= 2) && (emergency_COMM == false)) {
    //speed_pwm = speed_COMM_raw; 		//testing purposes
    speed_calc();
    speed_send();
  }

  update_lcd();
  delay(20);
  	/*						//stay in loop while COMM is in emergency mode
  	while((emergency_COMM == true) || (digitalRead(PIN_EM_IN) == HIGH)){

  		    speed_send(false);			    //brake
          speed_pwm = 0;
  	    	emergency_COMM = false;			//interrupt has been noticed

  	    	update_sensors();			//keep local emergency levels up to date
  	    	emergency_local_check();

  	   	update_lcd();

  	    	delay(100);
  	 }
  
*/
}








