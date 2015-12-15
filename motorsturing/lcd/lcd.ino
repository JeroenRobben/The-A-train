#include <LiquidCrystal.h>

//Unused Pins, will be flagged as INPUT
const byte UNUSED_PINS[] = {A0, A1, A2, A3, A4, A5, 1, 10, 11, 12, 13};
const byte AMOUNT_UNUSED_PINS = 11;      //needed to loop through the above array

char in;  // Character received from Serial input

byte status = 0; // 0= booting, 1 = emergency 2 = normal
byte boot_status = 0;
byte emergency_level = 0;
byte current_location = 10;
int speed_raw = 0;
int speed_cm = 0;
long time_next_depart_international = millis() + 43000;
long time_next_depart_national = millis() + 23000;
int time_till_depart_international = 0;
int time_till_depart_national = 0;

byte old_status = 100;
byte old_location = 100;
int indicator_line = 0;
long indicator_time = 0;
byte indicator_i2c_counter = 0;
const char indicator_i2c[] = {char(124), char(47), char(45), char(96)};
char international_station[] = " International Station"; //21 characters
char initiating_self_checks[] = " Initiating self checks"; //22 characters
char initiated_by_central_command[] = " Initiated by central command"; //28
char object_detected_at_front[] = " Object detected at front"; //24
char object_detected_at_back[] = " Object detected at back"; //23

// initialize the LCD at pins defined above
LiquidCrystal lcd(2, 3, 4, 5, 6, 7, 8);

void banner(char string[], byte length_string, bool show_i2c = false){
    if(show_i2c){
        if (indicator_line > length_string - 14){
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
  lcd.print(" ");
  lcd.print("Speed: ");
  if (speed_cm < 10){
      lcd.print(" ");
  }
  lcd.print(speed_cm);
  lcd.print("cm/s");
  lcd.print(" ");
  lcd.print(indicator_i2c[indicator_i2c_counter/64]);
}

void setup(){
  Serial.begin(19200);
 
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
  while(!Serial);
  while(Serial.available() == 0);
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
    time_next_depart_international = millis() + 43000;
    time_next_depart_national = millis() + 23000;
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
      time_till_depart_international = (time_next_depart_international - millis()) / 1000;
      time_till_depart_national = (time_next_depart_national - millis()) / 1000;
      normal();
      break;
  }

}

void update_status()
{
  while (Serial.available() > 0) {
      in = Serial.read();
      indicator_i2c_counter += 2;
      if ((in&0xff) == 0xfe){
         while(Serial.available() == 0);
         boot_status = Serial.read();
         status = 0;
          
        }
        else if((in&0xff) == 0xfd){
          while(Serial.available() == 0);
          //EMERGENCY
          emergency_level = Serial.read();
          status = 1;
        }
        else if((in&0xff) == 0xfc){
          //NORMAL
          while(Serial.available() == 0);
          current_location = Serial.read();
          while(Serial.available() == 0);
          speed_raw = Serial.read() * 4;
          speed_cm = ((speed_raw / 10) / 2) * 2;
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
      banner(initiating_self_checks, 23);
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
      banner(initiated_by_central_command,29);
      break;
    case 3:
      banner(object_detected_at_front,25);
      break;
    case 4:
      banner(object_detected_at_back,24);
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
      lcd.print(" straight track ");
      break;
      
    case 1: //in national station    
      lcd.print("National Station");
      lcd.setCursor(0,1);
      
      if (time_till_depart_national >= 21){   //Doors Opening                         
        lcd.print(" Doors opening  ");
      }
      
      else if ((time_till_depart_national > 9) && (time_till_depart_national < 21)){  //Waiting for depart 2 digits
        lcd.print("Departure in ");
        lcd.print(time_till_depart_national);
        lcd.print("s");
      }
           
      else if ((time_till_depart_national > 2) && (time_till_depart_national <= 9)){ //Waiting for depart 1 digit
        lcd.print("Departure in  ");
        lcd.print(time_till_depart_national);
        lcd.print("s");
      }

      else if ((time_till_depart_national >= 0) && (time_till_depart_national <= 2)){ //Doors closing
        lcd.print(" Doors closing  ");
      }
      else{                                                                                     //Start engines
        lcd.print("Starting engines");
      }
      break;
      
    case 2: //In international station
      banner(international_station, 22, true);      
      lcd.setCursor(0,1);
      
      if (time_till_depart_international >= 41){   //Doors Opening                         
        lcd.print(" Doors opening  ");
      }
      
      else if ((time_till_depart_international > 9) && (time_till_depart_international < 41)){  //Waiting for depart 2 digits
        lcd.print("Departure in ");
        lcd.print(time_till_depart_international);
        lcd.print("s");
      }
           
      else if ((time_till_depart_international > 2) && (time_till_depart_international <= 9)){ //Waiting for depart 1 digit
        lcd.print("Departure in  ");
        lcd.print(time_till_depart_international);
        lcd.print("s");
      }

      else if ((time_till_depart_international >= 0) && (time_till_depart_international <= 2)){ //Doors closing
        lcd.print(" Doors closing  ");
      }
      else{                                                                                     //Start engines
        lcd.print("Starting engines");
      }
      break;
      
    case 3: //Bocht
      speed_print();
      lcd.setCursor(0,1);
      lcd.print("   bend track   ");
      break;
      
    case 4: //Track switch
      speed_print();
      lcd.setCursor(0,1);
      lcd.print("  switch track  ");
      break;

    case 7: //Arriving at terminal
      lcd.print("  Arriving at  ");
      lcd.print(indicator_i2c[indicator_i2c_counter/64]);
      lcd.setCursor(0,1);
      lcd.print("    Terminal    ");
      break;
    

    case 9: //Track switch
      speed_print();
      lcd.setCursor(0,1);
      lcd.print("   Departing    ");
      break;

    case 10://Never received I2C message
      speed_print();
      lcd.setCursor(0,1);
      lcd.print("Waiting for COMM");
  }
}
