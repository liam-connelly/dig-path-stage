#include <AccelStepper.h>
#include <MultiStepper.h>

#define STEPS_PER_REVOLUTION 200

#define M_X_PIN_1 2
#define M_X_PIN_2 3
#define M_X_PIN_3 4
#define M_X_PIN_4 5

#define M_Y_PIN_1 8
#define M_Y_PIN_2 9
#define M_Y_PIN_3 10
#define M_Y_PIN_4 11

#define LIM_X_POW 13
#define LIM_X_SENSE 12

#define LIM_Y_POW 7
#define LIM_Y_SENSE 6

#define ZEROING_SPEED 100
#define MAX_STEPPER_SPEED 200
#define BUF_LEN 128

AccelStepper stepperx(AccelStepper::FULL4WIRE, M_X_PIN_1, M_X_PIN_2, M_X_PIN_3, M_X_PIN_4);
AccelStepper steppery(AccelStepper::FULL4WIRE, M_Y_PIN_1, M_Y_PIN_2, M_Y_PIN_3, M_Y_PIN_4);
MultiStepper steppers;

void find_zero() {

  Serial.write("Zeroing stage...\n");

  digitalWrite(LIM_X_POW, HIGH); digitalWrite(LIM_Y_POW, HIGH);

  int is_x_zero = digitalRead(LIM_X_SENSE)==1;
  int is_y_zero = digitalRead(LIM_Y_SENSE)==1;

  stepperx.setSpeed(ZEROING_SPEED);
  steppery.setSpeed(ZEROING_SPEED);
  
  while(!is_x_zero || !is_y_zero) {

    if (!is_x_zero)
      stepperx.runSpeed();

    if (!is_y_zero)
      steppery.runSpeed();
    
    is_x_zero = digitalRead(LIM_X_SENSE)==1;
    is_y_zero = digitalRead(LIM_Y_SENSE)==1;
    
  }

  digitalWrite(LIM_X_POW, LOW); digitalWrite(LIM_Y_POW, LOW);

  stepperx.setCurrentPosition(0);
  steppery.setCurrentPosition(0);

  Serial.write("Stage returned to zero and home coordinates set\n");
  
  return;
  
}

void set_home() {

  stepperx.setCurrentPosition(0);
  steppery.setCurrentPosition(0);

  Serial.write("Home coordinates set\n");
  
}

void go_home() {

  char positions_cmd_str[BUF_LEN] = {'\0'};

  sprintf(positions_cmd_str, "gohome 0 0");

  move_to_positions(positions_cmd_str);

  Serial.write("Stage returned to home coordinates\n");

  return;
  
}

void move_to_positions(char* positions_cmd_str) {
  
  long positions[2];

  char output_line[BUF_LEN];
  char flag = '\0';
  
  sscanf(positions_cmd_str, "%*s %ld %ld %c", &positions[0], &positions[1], &flag);

  positions[0] = -1*positions[0];
  positions[1] = -1*positions[1];

  steppers.moveTo(positions);
  steppers.runSpeedToPosition();

  sprintf(output_line, "Stage moved to coordinates: %ld %ld\n", -1*positions[0], -1*positions[1]);

  if (flag != 'q')
    Serial.write(output_line);
  
}

void move_distances(char* distances_cmd_str) {

  long curr_positions[2]; long new_positions[2]; long distances[2];

  char positions_cmd_str[BUF_LEN] = {'\0'};
  char output_line[BUF_LEN] = {'\0'};
  char flag = '\0';

  sscanf(distances_cmd_str, "%*s %ld %ld %s", &distances[0], &distances[1], &flag);

  curr_positions[0] = stepperx.currentPosition();
  curr_positions[1] = steppery.currentPosition();
  
  new_positions[0] = curr_positions[0] - distances[0];
  new_positions[1] = curr_positions[1] - distances[1];

  if (max(new_positions[0], new_positions[1])>0) {
    Serial.write("Distances are invalid\n");
    sprintf(output_line, "%ld %ld\n", new_positions[0], new_positions[1]);
    Serial.write(output_line);
    return;
  }

  steppers.moveTo(new_positions);
  steppers.runSpeedToPosition();
  
  sprintf(output_line, "Stage moved distances: %ld %ld\n", distances[0], distances[1]);

  if (flag != 'q')
    Serial.write(output_line);
  
}

void line_sweep(char *line_sweep_pattern_str) {
  
  long distances[2]; long curr_positions[2]; long new_positions[2];
  long step_size; long step_time;
  
  char flag = '\0';
  
  sscanf(line_sweep_pattern_str, "%*s %ld %ld %ld %ld %c", &distances[0], &distances[1], &step_size, &step_time, &flag);
  
  int num_steps = max(distances[0]/step_size, distances[1]/step_size);
  
  if (!((distances[0]==0) ^ (distances[1]==0)) || !num_steps || !step_time) {
    Serial.write("Invalid format for line sweep\n");
    return;
  }
  
  curr_positions[0] = stepperx.currentPosition();
  curr_positions[1] = steppery.currentPosition();
  
  new_positions[0] = -1*(distances[0]>0)*step_size + stepperx.currentPosition();
  new_positions[1] = -1*(distances[1]>0)*step_size + steppery.currentPosition();

  for (int i=0; i<num_steps; i++) {

    new_positions[0] = -1*(distances[0]>0)*step_size + curr_positions[0];
    new_positions[1] = -1*(distances[1]>0)*step_size + curr_positions[1];

    steppers.moveTo(new_positions);
    steppers.runSpeedToPosition();

    curr_positions[0] = new_positions[0];
    curr_positions[1] = new_positions[1];

    delay(step_time);
    
  }

  if (flag != 'q')
    Serial.write("Line sweep completed successfully\n");

  return;
  
}

void raster_sweep(char *raster_sweep_cmd_str) {

  long distances[2]; long starting_pos[2];
  long step_size; long step_time;

  char line_sweep_cmd_str[BUF_LEN] = {'\0'};
  char distances_cmd_str[BUF_LEN] = {'\0'};
  char positions_cmd_str[BUF_LEN] = {'\0'};

  sscanf(raster_sweep_cmd_str, "%*s %ld %ld %ld %ld %d", &distances[0], &distances[1], &step_size, &step_time);

  int num_x_steps = distances[0]/step_size;
  int num_y_steps = distances[1]/step_size;

  starting_pos[0] = -1*stepperx.currentPosition();
  starting_pos[1] = -1*steppery.currentPosition();
  
  if (!num_x_steps || !num_y_steps) {
    Serial.write("Invalid format for raster sweep\n");
    return;
  }

  if (num_x_steps>=num_y_steps) {

    sprintf(line_sweep_cmd_str, "line %ld 0 %ld %ld q\n", distances[0], step_size, step_time);
    
    for (int i=0; i<=num_y_steps; i++) {
       
      line_sweep(line_sweep_cmd_str);
      
      if (i<num_y_steps) {
        sprintf(positions_cmd_str, "positions %ld %ld q\n", starting_pos[0], starting_pos[1] + (i+1)*step_size);
      } else {
        sprintf(positions_cmd_str, "positions %ld %ld q\n", starting_pos[0], starting_pos[1]);
      }
      
      move_to_positions(positions_cmd_str);

      delay(step_time);
      
    }

  } else {

    sprintf(line_sweep_cmd_str, "line 0 %ld %ld %ld q\n", distances[0], step_size, step_time);
    
    for (int i=0; i<=num_x_steps; i++) {
      
      line_sweep(line_sweep_cmd_str);

      if (i<num_x_steps) {
        sprintf(positions_cmd_str, "positions %ld %ld q\n", starting_pos[0] + (i+1)*step_size, starting_pos[0]);
      } else {
        sprintf(positions_cmd_str, "positions %ld %ld q\n", starting_pos[0], starting_pos[1]);
      }
      
      move_to_positions(positions_cmd_str);

      delay(step_time);
      
    }
    
  }
  
  Serial.write("Raster sweep completed successfully\n");

  return;
  
}

void led_control(char* led_control_cmd_str) {

  return;
  
}

void setup() {

  Serial.begin(9600);

  while (!Serial) {
    delay(5);
  }

  stepperx.setMaxSpeed(200);
  steppery.setMaxSpeed(200);

  stepperx.setMinPulseWidth(50);
  steppery.setMinPulseWidth(50);

  steppers.addStepper(stepperx);
  steppers.addStepper(steppery);

  pinMode(LIM_X_POW, OUTPUT); pinMode(LIM_Y_POW, OUTPUT);
  pinMode(LIM_X_SENSE, INPUT); pinMode(LIM_Y_SENSE, INPUT);

}

void loop() {
  
  char rec_line[BUF_LEN] = {'\0'};
  char cmd_type_str[BUF_LEN] = {'\0'};
  
  if (Serial.available() > 0) {

    while(Serial.readBytesUntil('\n', rec_line, BUF_LEN-1)) {

      for (int i=0; isalpha(rec_line[i]) && i<BUF_LEN-1; i++) {
        cmd_type_str[i] = rec_line[i];
      }

      if (!strcmp(cmd_type_str, "zero")) {
        find_zero();
      } else if (!strcmp(cmd_type_str, "sethome")) {
         set_home();
      } else if (!strcmp(cmd_type_str, "gohome")) {
        go_home();
      } else if (!strcmp(cmd_type_str, "position")) {
        move_to_positions(rec_line);
      } else if (!strcmp(cmd_type_str, "distances")) {
        move_distances(rec_line);
      } else if (!strcmp(cmd_type_str, "line")) {
        line_sweep(rec_line);
      } else if (!strcmp(cmd_type_str, "raster")) {
        raster_sweep(rec_line);
      } else if (!strcmp(cmd_type_str, "led")) {
        led_control(rec_line);
      } else {
        Serial.write("Invalid command\n");
      }

      memset(rec_line, 0, BUF_LEN);
      memset(cmd_type_str, 0, BUF_LEN);
      
    }
    
  }
  
  delay(100);

}
