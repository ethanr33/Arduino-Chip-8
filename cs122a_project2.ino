#include <Adafruit_SSD1306.h>
#include <splash.h>
#include <Keypad.h>
#include <Key.h>
#include <SPI.h>
#include <SD.h>
#include <avr/wdt.h>

using namespace std;

typedef struct task {
  long period; // Rate at which the task should tick
  unsigned long elapsedTime; // Time since task's previous tick
  int (*SM)(); // Function to call for task's tick
} task;

void(* resetFunc) (void) = 0; // Declare reset function at address 0

const int NUM_TASKS = 5;

task tasks[NUM_TASKS];
const int TASK_PERIOD_GCD = 1;

// Reset Button
const int RESET_BUTTON_PIN = 41;
const int RESET_PIN = 36;

// Keypad variables

const int ROW_NUM = 4;
const int COLUMN_NUM = 4;

// Add each key by 1 to make detecting easier
const char KEY_MAP[ROW_NUM][COLUMN_NUM] = {
  {2, 3, 4, 0xD},
  {5, 6, 7, 0xE},
  {8, 9, 0xA, 0xF},
  {0xB, 1, 0xC, 0x10}
};

const byte PIN_ROWS[ROW_NUM] = {22, 23, 24, 25}; //connect to the row pinouts of the keypad
const byte PIN_COLUMNS[COLUMN_NUM] = {26, 27, 28, 29}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(KEY_MAP), PIN_ROWS, PIN_COLUMNS, ROW_NUM, COLUMN_NUM);

char last_pressed_key = 0;
bool is_pressed = false;

// Chip 8 Global Variables

int program_counter = 0x200; // Program counter
int stack_pointer = 0; // Stack pointer

const int MEM_SIZE = 4096;
const int STACK_SIZE = 32;

byte memory[MEM_SIZE];
int stack[STACK_SIZE];


byte registers[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int I = 0; // Address register

unsigned char delay_timer = 0;
unsigned char sound_timer = 0;

// Flags
bool load_store = true;
bool shift = true;

enum Processor_States {SM_Processor_Start, SM_Processor_Waiting, SM_Processor_Run} Processor_State = SM_Processor_Start;
enum Timer_States {SM_Timer_Start, SM_Timer_Off, SM_Timer_On} Timer_State = SM_Timer_Start;

// Sound variables

const int BUZZER_PIN = 45;
enum Buzzer_States {SM_Buzzer_Start, SM_Buzzer_Off, SM_Buzzer_On} Buzzer_State = SM_Buzzer_Start;

// SD Card Reader variables

File game_file;
const int SD_PIN = 53;
const String GAMEFILE_NAME = "game"; // pong, chip, corax, flags, ibmlogo, keypad, quirks, maze, invaders, blinky, breakout, merlin

enum Keypad_States {SM_Keypad_Start, SM_Keypad_Idle, SM_Keypad_Pressed, SM_Keypad_Holding} Keypad_State = SM_Keypad_Start;

// Display variables

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 screen(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum Screen_States {SM_Screen_Start, SM_Screen_On} Screen_State = SM_Screen_Start;

const int DISPLAY_WIDTH = 64;
const int DISPLAY_HEIGHT = 32;

bool display_buffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

// Puts digits into memory
void setup_memory() {

  memory[0] = 0xF0;
  memory[1] = 0x90;
  memory[2] = 0x90;
  memory[3] = 0x90;
  memory[4] = 0xF0;
  memory[5] = 0x20;
  memory[6] = 0x60;
  memory[7] = 0x20;
  memory[8] = 0x20;
  memory[9] = 0x70;
  memory[10] = 0xF0;
  memory[11] = 0x10;
  memory[12] = 0xF0;
  memory[13] = 0x80;
  memory[14] = 0xF0;
  memory[15] = 0xF0;
  memory[16] = 0x10;
  memory[17] = 0xF0;
  memory[18] = 0x10;
  memory[19] = 0xF0;
  memory[20] = 0x90;
  memory[21] = 0x90;
  memory[22] = 0xF0;
  memory[23] = 0x10;
  memory[24] = 0x10;
  memory[25] = 0xF0;
  memory[26] = 0xF0;
  memory[27] = 0x80;
  memory[28] = 0xF0;
  memory[29] = 0x10;
  memory[30] = 0xF0;
  memory[31] = 0xF0;
  memory[32] = 0x80;
  memory[33] = 0xF0;
  memory[34] = 0x90;
  memory[35] = 0xF0;
  memory[36] = 0xF0;
  memory[37] = 0x10;
  memory[38] = 0x20;
  memory[39] = 0x40;
  memory[40] = 0x40;
  memory[41] = 0xF0;
  memory[42] = 0x90;
  memory[43] = 0xF0;
  memory[44] = 0x90;
  memory[45] = 0xF0;
  memory[46] = 0xF0;
  memory[47] = 0x90;
  memory[48] = 0xF0;
  memory[49] = 0x10;
  memory[50] = 0xF0;
  memory[51] = 0xF0;
  memory[52] = 0x90;
  memory[53] = 0xF0;
  memory[54] = 0x90;
  memory[55] = 0x90;
  memory[56] = 0xE0;
  memory[57] = 0x90;
  memory[58] = 0xE0;
  memory[59] = 0x90;
  memory[60] = 0xE0;
  memory[61] = 0xF0;
  memory[62] = 0x80;
  memory[63] = 0x80;
  memory[64] = 0x80;
  memory[65] = 0xF0;
  memory[66] = 0xE0;
  memory[67] = 0x90;
  memory[68] = 0x90;
  memory[69] = 0x90;
  memory[70] = 0xE0;
  memory[71] = 0xF0;
  memory[72] = 0x80;
  memory[73] = 0xF0;
  memory[74] = 0x80;
  memory[75] = 0xF0;
  memory[76] = 0xF0;
  memory[77] = 0x80;
  memory[78] = 0xF0;
  memory[79] = 0x80;
  memory[80] = 0x80;
}

void clear_display() {
  for (int i = 0; i < DISPLAY_HEIGHT; i++) {
    for (int j = 0; j < DISPLAY_WIDTH; j++) {
      display_buffer[i][j] = 0;
    }
  }
}

void execute_instruction(int instruction) {
  int first_byte = (instruction & 0xF000) >> 12;
  int first_nibble = instruction >> 12;
  int n = instruction & 0x000F;
  int nn = instruction & 0x00FF;
  int nnn = instruction & 0x0FFF;
  int x = (instruction & 0x0F00) >> 8;
  int y = (instruction & 0x00F0) >> 4;

  if (first_byte == 0) {
    if (nn == 0xE0) {
      screen.clearDisplay();
      screen.display();
    } else if (nn == 0xEE) {
      // Return from subroutine
      stack_pointer--;
      program_counter = stack[stack_pointer];
      // No need to change stack because it must be updated later
    } else {
      Serial.println("Unknown instruction " + String(instruction, HEX));
    }
  } else if (first_byte == 0x1) {
    program_counter = nnn - 2; // Subtract 2 to counteract program counter increment
  } else if (first_byte == 0x2) {
    stack[stack_pointer] = program_counter;
    stack_pointer++;
    program_counter = nnn - 2; // Subtract 2 to counteract program counter increment
  } else if (first_byte == 0x3) {
    if (registers[x] == nn) {
      program_counter += 2;
    }
  } else if (first_byte == 0x4) {
    if (registers[x] != nn) {
      program_counter += 2;
    }
  } else if (first_byte == 0x5) {
    if (registers[x] == registers[y]) {
      program_counter += 2;
    }
  } else if (first_byte == 0x6) {
    registers[x] = nn;
  } else if (first_byte == 0x7) {
    registers[x] += nn;
  } else if (first_byte == 0x8) {
    if (n == 0) {
      registers[x] = registers[y];
    } else if (n == 1) {
      registers[x] = registers[x] | registers[y];
    } else if (n == 2) {
      registers[x] = registers[x] & registers[y];
    } else if (n == 3) {
      registers[x] = registers[x] ^ registers[y];
    } else if (n == 4) {
      int sum = registers[x] + registers[y];
      registers[x] += registers[y];
      registers[0xF] = (sum > 0xFF);
    } else if (n == 5) {
      int diff = registers[x] - registers[y];
      registers[x] = registers[x] - registers[y];
      registers[0xF] = (diff >= 0);
    } else if (n == 6) {
      if (shift) {
        y = x;
      }

      bool lsb = registers[y] % 2;
      registers[x] = registers[y] >> 1;
      registers[0xF] = lsb;
    } else if (n == 7) {
      int diff = registers[y] - registers[x];
      registers[x] = registers[y] - registers[x];
      registers[0xF] = (diff >= 0);
    } else if (n == 0xE) {
      if (shift) {
        y = x;
      }

      bool msb =  registers[y] >> 7;
      registers[x] = registers[y] << 1;
      registers[0xF] = msb;
    }
  } else if (first_byte == 0x9) {
    if (registers[x] != registers[y]) {
      program_counter += 2;
    }
  } else if (first_byte == 0xA) {
    I = nnn;
  } else if (first_byte == 0xB) {
    program_counter = nnn + registers[0] - 2;
  } else if (first_byte == 0xC) {
    registers[x] = random(256) & nn;
  } else if (first_byte == 0xD) {
    // Draw instruction
    int x_start = registers[x] % DISPLAY_WIDTH;
    int y_start = registers[y] % DISPLAY_HEIGHT;
    bool pixels_turned_off = false;

    for (int i = 0; i < n; i++) {
      byte cur_byte = memory[I + i];
      for (int j = 0; j < 8 && x_start + j < DISPLAY_WIDTH && y_start + i < DISPLAY_HEIGHT; j++) {
        bool cur_bit = ((1 << (7 - j)) & cur_byte) >> (7 - j);
        bool new_bit = screen.getPixel(2 * (x_start + j), 2 * (y_start + i)) ^ cur_bit;

        if (cur_bit == 1 && new_bit == 0) {
          pixels_turned_off = true;
        }

        if (new_bit) {
          screen.drawPixel(2 * (x_start + j), 2 * (y_start + i), WHITE);
          screen.drawPixel(2 * (x_start + j) + 1, 2 * (y_start + i), WHITE);
          screen.drawPixel(2 * (x_start + j), 2 * (y_start + i) + 1, WHITE);
          screen.drawPixel(2 * (x_start + j) + 1, 2 * (y_start + i) + 1, WHITE);
        } else {
          screen.drawPixel(2 * (x_start + j), 2 * (y_start + i), BLACK);
          screen.drawPixel(2 * (x_start + j) + 1, 2 * (y_start + i), BLACK);
          screen.drawPixel(2 * (x_start + j), 2 * (y_start + i) + 1, BLACK);
          screen.drawPixel(2 * (x_start + j) + 1, 2 * (y_start + i) + 1, BLACK);
        }

      }
    }
    registers[0xF] = pixels_turned_off;
    screen.display();
  } else if (first_byte == 0xE) {
    if (nn == 0x9E) {
      if (last_pressed_key == registers[x] + 1) {
        program_counter += 2;
      }
    } else if (nn == 0xA1) {
      if (last_pressed_key != registers[x] + 1) {
        program_counter += 2;
      }
    } else {
      Serial.println("Unknown instruction " + String(instruction, HEX));
    }
  } else if (first_byte == 0xF) {
    if (nn == 0x07) {
      registers[x] = delay_timer;
    } else if (nn == 0x0A) {
      char key = keypad.waitForKey();
      registers[x] = key - 1;
    } else if (nn == 0x15) {
      delay_timer = registers[x];
    } else if (nn == 0x18) {
      sound_timer = registers[x];
    } else if (nn == 0x1E) {
      I += registers[x];
    } else if (nn == 0x29) {
      I = 5 * registers[x];
    } else if (nn == 0x33) {
      int num = registers[x];
      memory[I] = num / 100;
      memory[I + 1] = (num / 10) % 10;
      memory[I + 2] = num % 10;
    } else if (nn == 0x55) {
      for (int i = 0; i <= x; i++) {
        memory[I + i] = registers[i];
      }

      if (load_store) {
        I = I + x + 1;
      }
    } else if (nn == 0x65) {
      for (int i = 0; i <= x; i++) {
        registers[i] = memory[I + i];
      }

      if (load_store) {
        I = I + x + 1;
      }
    } else {
      Serial.println("Unknown instruction " + String(instruction, HEX));
    }
  } else {
      Serial.println("Unknown instruction " + String(instruction, HEX));
  }
}

void Screen_SM() {
  // State transitions
  switch (Screen_State) {
    case (SM_Screen_Start):
      for (int i = 0; i < DISPLAY_HEIGHT; i++) {
        for (int j = 0; j < DISPLAY_WIDTH; j++) {
          display_buffer[i][j] = false;
        }
      }

      Screen_State = SM_Screen_On;
      break;
    case (SM_Screen_On):
      break;
    default:
      break;
  }

  // State actions
  switch (Screen_State) {
    case (SM_Screen_Start):
      Screen_State = SM_Screen_On;
      break;
    case (SM_Screen_On):
      break;
    default:
      break;
  }
}

bool debounce = false;
void Processor_SM() {
  // State Transitions
  switch (Processor_State) {
    case (SM_Processor_Start):
      program_counter = 0x200;
      I = 0;
      stack_pointer = 0;
      delay_timer = 0;
      sound_timer = 0;
      last_pressed_key = 0;

      debounce = false;

      for (int i = 0; i < STACK_SIZE; i++) {
        stack[i] = 0;
      }
      
      for (int i = 0; i < 16; i++) {
        registers[i] = 0;
      }

      screen.clearDisplay();
      screen.display();

      game_file = SD.open(GAMEFILE_NAME);

      if (!game_file) {
        Serial.println("Couldn't find game file");
        while (true) {}
      }

      setup_memory();

      for (int i = 0x200; game_file.available(); i++) {
        memory[i] = game_file.read();
      }

      game_file.close();

      Processor_State = SM_Processor_Run;
      break;
    case (SM_Processor_Run):
      if (!debounce && digitalRead(RESET_BUTTON_PIN)) {
        Serial.println("got here");
        debounce = true;
      } else if (debounce && !digitalRead(RESET_BUTTON_PIN)) {
        Serial.println("resetting");
        wdt_disable();
        wdt_enable(WDTO_15MS);
        while (1) {}
      }
      break;
    default:
      break;
  }

  // State Actions
  switch (Processor_State) {
    case (SM_Processor_Start):
      break;
    case (SM_Processor_Run):
      int cur_instruction = (memory[program_counter] << 8) | memory[program_counter + 1];
      //Serial.println(String(cur_instruction, HEX));

      execute_instruction(cur_instruction);
      program_counter += 2;
      break;
    default:
      break;
  }
}

void Timer_SM() {
  // State transitions
  switch (Timer_State) {
    case (SM_Timer_Start):
      Timer_State = SM_Timer_Off;
      break;
    case (SM_Timer_Off):
      if (delay_timer > 0) {
        Timer_State = SM_Timer_On;
      }
      break;
    case (SM_Timer_On):
      if (delay_timer == 0) {
        Timer_State = SM_Timer_On;
      }
      break;
    default:
      break;
  }

  // State actions
  switch (Timer_State) {
    case (SM_Timer_Start):
      Timer_State = SM_Timer_Off;
      break;
    case (SM_Timer_Off):
      delay_timer = 0;
      break;
    case (SM_Timer_On):
      delay_timer--;
      break;
    default:
      break;
  }
}

void Buzzer_SM() {
  // State transitions
  switch (Buzzer_State) {
    case (SM_Buzzer_Start):
      Buzzer_State = SM_Buzzer_Off;
      break;
    case (SM_Buzzer_Off):
      if (sound_timer > 0) {
        Buzzer_State = SM_Buzzer_On;
      }
      break;
    case (SM_Buzzer_On):
      if (sound_timer == 0) {
        Buzzer_State = SM_Buzzer_Off;
      }
      break;
    default:
      break;
  }

  // State actions
  switch (Buzzer_State) {
    case (SM_Buzzer_Start):
      break;
    case (SM_Buzzer_Off):
      noTone(BUZZER_PIN);
      break;
    case (SM_Buzzer_On):
      sound_timer--;
      tone(BUZZER_PIN, 750);
      break;
    default:
      break;
  }
}

void Keypad_SM() {
  int key;
  switch (Keypad_State) {
    case (SM_Keypad_Start):
      Keypad_State = SM_Keypad_Idle;
      break;
    case (SM_Keypad_Idle):
      key = keypad.getKey();
      if (key) {
        Keypad_State = SM_Keypad_Pressed;
      }
      break;
    case (SM_Keypad_Pressed):
      Keypad_State = SM_Keypad_Holding;
      break;
    case (SM_Keypad_Holding):
      key = keypad.getKey();
      if (!key) {
        Keypad_State = SM_Keypad_Idle;
      }
      break;
    default:
      break;
  }

  switch (Keypad_State) {
    case (SM_Keypad_Start):
      Keypad_State = SM_Keypad_Idle;
      break;
    case (SM_Keypad_Idle):
      //Serial.println("idling");
      last_pressed_key = 0;
      break;
    case (SM_Keypad_Pressed):
      //Serial.println("key press detected");
      last_pressed_key = key;
      break;
    case (SM_Keypad_Holding):
      //Serial.println("holding key");
      break;
    default:
      break;
  }
}

void setup_tasks() {
  tasks[0].period = 16;
  tasks[0].elapsedTime = tasks[0].period;
  tasks[0].SM = &Buzzer_SM;

  tasks[1].period = 1;
  tasks[1].elapsedTime = tasks[1].period;
  tasks[1].SM = &Processor_SM;

  tasks[2].period = 2;
  tasks[2].elapsedTime = tasks[2].period;
  tasks[2].SM = &Screen_SM;

  tasks[3].period = 100;
  tasks[3].elapsedTime = tasks[3].period;
  tasks[3].SM = &Keypad_SM;

  tasks[4].period = 16;
  tasks[4].elapsedTime = tasks[4].period;
  tasks[4].SM = &Timer_SM;
}

void setup(){
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  digitalWrite(10, HIGH);

  if (!SD.begin(SD_PIN)) {
    Serial.println("Failed to initialize SD card");
    while (true) {}
  } else {
    Serial.println("Initialized SD card");
  }

  if(!screen.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    while (true) {}
  } else {
    //Serial.println("Initialized SSD1306 display");
    screen.clearDisplay();
    screen.display();
  }

  keypad.setHoldTime(100);

  setup_tasks();
}

unsigned long last_executed_time = millis();
void loop(){
  if (millis() - last_executed_time >= TASK_PERIOD_GCD) {
    for (int i = 0; i < NUM_TASKS; i++) {
      if (tasks[i].elapsedTime >= tasks[i].period) {
        tasks[i].SM();
        tasks[i].elapsedTime = 0;
      }
      tasks[i].elapsedTime += TASK_PERIOD_GCD;
    }
    last_executed_time = millis();
  }




}
