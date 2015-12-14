#include <LiquidCrystal.h>

//Unused Pins, will be flagged as INPUT
const byte UNUSED_PINS[] = {A0, A1, A2, A3, A4, A5, 1, 10, 11, 12, 13};
const byte AMOUNT_UNUSED_PINS = 11;      //needed to loop through the above array

char in;  // Character received from Serial1 input

byte status = 2; // 0= booting, 1 = emergency 2 = normal
byte boot_status = 3;
byte emergency_level = 6;
byte current_location = 4;
int speed_raw = 900;
int speed_cm = 420;
long time_next_depart = millis() + 15000;
int time_till_depart = 0;

byte old_status = 100;
byte old_location = 100;
int indicator_line = 0;
long indicator_time = 0;
byte indicator_i2c_counter = 0;
const char indicator_i2c[] = {char(124), char(47), char(45), char(96)};
char international_station[] = "International Station"; //21 characters
char initiating_self_checks[] = "Initiating self checks"; //22 characters
char initiated_by_central_command[] = "Initiated by central command"; //28
char object_detected_at_front[] = "Object detected at front"; //24
char object_detected_at_back[] = "Object detected at back"; //23

// initialize the LCD at pins defined above
LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8);

void banner(char string[], byte length_string, bool show_i2c = false){
    if(show_i2c){
        if (indicator_line > length_string - 13){
            indicator_line = 0;
            indicator_time = millis();
        }
        if (indicator_line < (length_string - 14)){
            for(byte i = indicator_line; i < (15 + indicator_line); i++){
            lcd.print(string[i]);
            }
            lcd.print(indicator_i2c[indicator_i2c_counter/64]);          
        }
        else{
            lcd.print("               ");
            lcd.print(indicator_i2c[indicator_i2c_counter/64]);
            } 
    }
    
    else{
        if (indicator_line > length_string - 14){
            indicator_line = 0;
            indicator_time = millis();
        }
        if (indicator_line < (length_string - 15)){
            for(byte i = indicator_line; i < (16 + indicator_line); i++){
            lcd.print(string[i]);
            }          
        }
        else{
              lcd.print("                ");
            }
      
    }
    
}

void speed_print(){
  lcd.print("Speed: ");
  if (speed_cm < 10){
      lcd.print(" ");
  }
  lcd.print(speed_cm);
  lcd.print("cm/s");
  lcd.print("  ");
  lcd.print(indicator_i2c[indicator_i2c_counter/64]);
}

void setup(){
  Serial1.begin(19200);
 
  for (byte i = 0; i < AMOUNT_UNUSED_PINS; i++) {
  pinMode(UNUSED_PINS[i], INPUT);
  }

  lcd.begin(16, 2);
  
  // Set up the backlight
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
  
  lcd.print(" Airport People ");
  lcd.setCursor(0, 1);
  lcd.print("Mover: Shuttle A");
  delay(2000);
  lcd.setCursor(0, 0);
  lcd.print("  Waiting for   ");
  lcd.setCursor(0, 1);
  lcd.print("    arduino     ");
  while(!Serial1);
  while(Serial1.available() == 0);
  lcd.clear(); 
}


void loop()
{
  update_status();
  if((old_location != current_location) || (old_status != status)){
    indicator_line = 0;   
    indicator_time = millis();
    old_location = current_location;
    old_status = status;
    time_next_depart = millis() + 20000;
  }
  indicator_line = ((millis() - indicator_time) / 500);
  switch(status){
    case 0:
      boot();
      break;
    case 1:
      emergency();
      break;
    case 2:
      time_till_depart = (time_next_depart - millis()) / 1000;
      normal();
      break;
  }

}

void update_status()
{
  while (Serial1.available() > 0) {
      in = Serial1.read();
      indicator_i2c_counter += 2;
      if ((in&0xff) == 0xfe){
         while(Serial1.available() == 0);
         boot_status = Serial1.read();
         status = 0;
          
        }
        else if((in&0xff) == 0xfd){
          while(Serial1.available() == 0);
          //EMERGENCY
          emergency_level = Serial1.read();
          status = 1;
        }
        else if((in&0xff) == 0xfc){
          //NORMAL
          while(Serial1.available() == 0);
          current_location = Serial1.read();
          while(Serial1.available() == 0);
          speed_raw = Serial1.read() * 4;
          speed_cm = speed_raw / 10;
          status = 2;
        }
   }
}

void boot(){
  lcd.setCursor(0,0);
  lcd.print("Booting system ");
  lcd.print(indicator_i2c[indicator_i2c_counter/64]);
  lcd.setCursor(0,1);
  switch(boot_status){
    case 0:
      banner(initiating_self_checks, 22);
      break;
    case 1:
      lcd.print("Self check 1 / 2");
      break;
    case 2:
      lcd.print("Self check 2 / 2");
    case 3:
      lcd.print("Waiting for COMM");
      break;
    
  }
}

void emergency(){
  lcd.setCursor(0,0);
  lcd.print("   EMERGENCY   ");
  lcd.print(indicator_i2c[indicator_i2c_counter/64]);
  lcd.setCursor(0,1);
  switch(emergency_level){
    case 6:
      banner(initiated_by_central_command,28);
      break;
    case 3:
      banner(object_detected_at_front,24);
      break;
    case 4:
      banner(object_detected_at_back,23);
      break;
    case 5:
      lcd.print("We're surrounded");
      break;
  }
}
void normal(){
  lcd.setCursor(0,0);
  switch(current_location){
    case 0: //Straight track
      speed_print();
      lcd.setCursor(0,1);
      lcd.print("straight track  ");
      break;
    case 3: //Bocht
      speed_print();
      lcd.setCursor(0,1);
      lcd.print("bend track ");
      break;
    case 7: //Arriving at international station
      lcd.print("  Arriving at  ");
      lcd.print(indicator_i2c[indicator_i2c_counter/64]);
      lcd.setCursor(0,1);
      lcd.print("    Terminal    ");
      break;
    case 4: //Track switch
 	speed_print();
      lcd.setCursor(0,1);
      lcd.print("switch track    ");
      break;
    case 2: //In international station
      banner(international_station, 21, true);      
      lcd.setCursor(0,1);
      if (time_till_depart > 9){
        lcd.print("Departure in ");
        lcd.print(time_till_depart);
        lcd.print("s");
      }

      else if (time_till_depart < 0){
        lcd.print("Starting engines");
      }
      else{
        lcd.print("Departure in  ");
        lcd.print(time_till_depart);
        lcd.print("s");
      }
      break;
    case 1: //in national station
    
      lcd.print("National Station");
      lcd.setCursor(0,1);
      if (time_till_depart > 9){
        lcd.print("Departure in ");
        lcd.print(time_till_depart);
        lcd.print("s");
      }
      else if (time_till_depart < 0){
        lcd.print("starting engines");
      }
      else{
        lcd.print("Departure in  ");
        lcd.print(time_till_depart);
        lcd.print("s");
      }
      break;
  
    }
}



