#include <AccelStepper.h>
#include <MultiStepper.h>

#define stepsPerRevolution = 200;

#define m1pin1 4
#define m1pin2 5
#define m1pin3 6
#define m1pin4 7

#define m2pin1 8
#define m2pin2 9
#define m2pin3 10
#define m2pin4 11

#define lim1pow 2
#define lim1sense 3

#define lim2pow 12
#define lim2sense 13

#define ZEROING_SPEED 100
#define maxStepperSpeed 200
#define BUF_LEN 128

AccelStepper stepper1(AccelStepper::FULL4WIRE, m1pin1, m1pin2, m1pin3, m1pin4);
AccelStepper stepper2(AccelStepper::FULL4WIRE, m2pin1, m2pin2, m2pin3, m2pin4);
MultiStepper steppers;



void find_zero() {

  digitalWrite(lim1pow, HIGH); digitalWrite(lim2pow, HIGH);

  int is_1_zero = digitalRead(lim1sense)==1;
  int is_2_zero = digitalRead(lim2sense)==1;

  stepper1.setSpeed(ZEROING_SPEED);
  stepper2.setSpeed(ZEROING_SPEED);
  
  while(!is_1_zero || !is_2_zero) {

    if (!is_1_zero)
      stepper1.runSpeed();

    if (!is_2_zero)
      stepper2.runSpeed();
    
    is_1_zero = digitalRead(lim1sense)==1;
    is_2_zero = digitalRead(lim2sense)==1;
    
  }

  digitalWrite(lim1pow, LOW); digitalWrite(lim2pow, LOW);

  set_rel_home();

  Serial.write("done\n");
  
  return;
  
}

void set_rel_home() {

  stepper1.setCurrentPosition(0);
  stepper2.setCurrentPosition(0);

  Serial.write("done\n");
  
}

void move_to_positions(char* position_str) {
  
  long positions[2];
  
  Serial.println(position_str);
  
  sscanf(position_str, "%ld %ld", &positions[0], &positions[1]);

  positions[0] = -1*positions[0];
  positions[1] = -1*positions[1];
  
  Serial.println(positions[0]);
  Serial.println(positions[1]);

  steppers.moveTo(positions);
  steppers.runSpeedToPosition();

  Serial.println("done\n");
  
}

void line_sweep(char *pattern_str) {
  
  long distances[2]; long step_size; long step_time;
  
  sscanf(pattern_str, "%ld %ld %ld %ld %d", &distances[0], &distances[1], &step_size, &step_time);

  //set_rel_home();
  
  int num_steps = max(distances[0]/step_size, distances[1]/step_size);
  
  if (!((distances[0]==0) ^ (distances[1]==0)) || !num_steps) {
    Serial.write("invalid format\n");
    return;
  }

  long positions[num_steps][2];
  positions[0][0] = -1*(distances[0]>0)*step_size + stepper1.currentPosition();
  positions[0][1] = -1*(distances[1]>0)*step_size + stepper2.currentPosition();

  Serial.write("trying\n");

  for (int i=1; i<num_steps; i++) {

    positions[i][0] = -1*(distances[0]>0)*step_size + positions[i-1][0];
    positions[i][1] = -1*(distances[1]>0)*step_size + positions[i-1][1];

    Serial.print(positions[i][0]);
    Serial.write("\t");
    Serial.print(positions[i][1]);
    Serial.write("\n");
    
  }

  //long zeroes[2]; zeroes[0] = 0; zeroes[1] = 0;
  //steppers.moveTo(zeroes);
  //steppers.runSpeedToPosition();

  for (int i=0; i<num_steps; i++) {
    
    steppers.moveTo(positions[i]);
    steppers.runSpeedToPosition();

    delay(step_time);
    
  }

  Serial.write("done\n");

  return;
  
}

void raster_sweep(char *pattern_str) {

  long distances[2]; long step_size; long step_time;

  sscanf(pattern_str, "%ld %ld %ld %ld %d", &distances[0], &distances[1], &step_size, &step_time);

  int num_x_steps = distances[0]/step_size;
  int num_y_steps = distances[1]/step_size;

  if (!num_x_steps || !num_y_steps) {
    Serial.write("invalid format\n");
    return;
  }

  int anti_sweep_dir = num_x_steps>num_y_steps ? 1 : 0;

  if (anti_sweep_dir) {

    

    
    
  }

  

  

  set_rel_home();
  
}

void setup() {

  Serial.begin(9600);

  while (!Serial) {
    delay(5);
  }

  stepper1.setMaxSpeed(200);
  stepper2.setMaxSpeed(200);

  stepper1.setMinPulseWidth(25);
  stepper2.setMinPulseWidth(25);

  steppers.addStepper(stepper1);
  steppers.addStepper(stepper2);

  pinMode(lim1pow, OUTPUT); pinMode(lim2pow, OUTPUT);
  pinMode(lim1sense, INPUT); pinMode(lim2sense, INPUT);

}

void loop() {
  
  char rec_line[BUF_LEN] = {'\0'};
  
  if (Serial.available() > 0) {

    while(Serial.readBytesUntil('\n', rec_line, BUF_LEN-1)) {

      if(rec_line[0] == 'z') {
        Serial.write("going to absolute zero\n"); Serial.flush();
        find_zero();
      }

      if (rec_line[0] == 's') {
        Serial.write("set current position as relative home\n");
        set_rel_home();
      }

      if (rec_line[0] == 'm') {
        Serial.write("moving to target locations\n");
        move_to_positions(&rec_line[1]);
      }

      if(rec_line[0] == 'l') {
        Serial.write("starting linear sweep\n");
        line_sweep(&rec_line[1]);
      }

      if(rec_line[0] == 'r') {
        Serial.write("starting raster sweep\n");
        raster_sweep(&rec_line[0]);
      }
      
    }
  }

  
  delay(100);

}
