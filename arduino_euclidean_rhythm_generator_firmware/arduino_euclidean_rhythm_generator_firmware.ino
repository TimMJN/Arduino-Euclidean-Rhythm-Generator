////////////////////////////////////////////////////////////////////////////////////////////////////

/*
   Arduino Euclidean Rhythm Generator
   synthesizer module firmware

   by TimMJN

   v1.0
   24-07-2021

   For schematics and other information, see
   https://github.com/TimMJN/Arduino-Euclidean-Rhythm-Generator
*/

#include "src/Adafruit-MCP23017-Arduino-Library/Adafruit_MCP23017.h"
#include "src/Tlc5940/Tlc5940.h"

// Arduino pin definitions
#define CLK_IN      2
#define RESET_IN    4
#define OUT1        5
#define OUT2        6
#define OUT3        7
#define OUT4        8

#define LED_GS_CLK  3
#define LED_LAT     9
#define LED_BLANK   10
#define LED_DAT     11
#define LED_SER_CLK 13

#define GPIO_INT_A  16
#define GPIO_INT_B  17
#define GPIO_DAT    18
#define GPIO_CLK    19

// MCP23017 pin definitions
#define ENC_A1      0
#define ENC_B1      1
#define ENC_S1      8
#define ENC_A2      2
#define ENC_B2      3
#define ENC_S2      9
#define ENC_A3      4
#define ENC_B3      5
#define ENC_S3     10
#define ENC_A4      6
#define ENC_B4      7
#define ENC_S4     11

// constants
#define N_CHANNELS  4     // number of channels -  If you adjust this, also set it in src/Tlc5940/tlc_config.h
#define MAX_LENGTH  16    // max sequence length
#define MIN_LENGTH  1     // min sequence length
#define TIMEOUT     5000  // time-out of sequence length adjustment (ms)

// led brightness settings (0 - 4095)
#define CUR_STEP_BRIGHTNESS     4095
#define HIT_BRIGHTNESS          800
#define NO_HIT_BRIGHTNESS       20
#define INACTIVE_BRIGHTNESS     0
#define LENGTH_MODE_BRIGHTNESS  200

const byte ENC_A_PINS[N_CHANNELS] = {ENC_A1, ENC_A2, ENC_A3, ENC_A4};
const byte ENC_B_PINS[N_CHANNELS] = {ENC_B1, ENC_B2, ENC_B3, ENC_B4};
const byte ENC_S_PINS[N_CHANNELS] = {ENC_S1, ENC_S2, ENC_S3, ENC_S4};

const byte OUT_PINS[N_CHANNELS]   = {OUT1,  OUT2,  OUT3,  OUT4};

uint8_t enc_int_cap_reg;   // MCP23017 interrupt capture register address for encoder
uint8_t sw_int_cap_reg;    // MCP23017 interrupt capture register address for encoder switches

// initial settings
byte seq_length[N_CHANNELS] = {MAX_LENGTH, MAX_LENGTH, MAX_LENGTH, MAX_LENGTH}; // length of the sequence
byte n_hits[N_CHANNELS]     = {1, 1, 1, 1};          // number of hits in the sequence
byte offset[N_CHANNELS]     = {0, 0, 0, 0};          // off-set of the sequence
byte curr_step[N_CHANNELS]  = {0, 0, 0, 0};          // current step

// sequence
bool sequence[N_CHANNELS][MAX_LENGTH];

// encoder pin A states
bool curr_enc_states[N_CHANNELS];
bool prev_enc_states[N_CHANNELS];

// encoder switch states
bool curr_switch_states[N_CHANNELS];
bool prev_switch_states[N_CHANNELS];

// reset pin states
bool cur_reset_state  = false;
bool prev_reset_state = false;
bool do_reset         = false;

// mode selection
bool length_mode[N_CHANNELS];                 // sequence length editing mode
bool has_turned_since_press[N_CHANNELS];      // has the encoder been turned since the last button press
unsigned long time_at_last_turn[N_CHANNELS];  // timestamp of last encoder turn (ms)
byte seq_length_temp[N_CHANNELS];             // temporary sequence length displayed during editing

Adafruit_MCP23017 mcp;

////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  // set Arduino pinModes
  pinMode(GPIO_INT_A, INPUT_PULLUP);
  pinMode(GPIO_INT_B, INPUT_PULLUP);

  pinMode(CLK_IN,   INPUT);
  pinMode(RESET_IN, INPUT);
  for (int i = 0; i < N_CHANNELS; i++) {
    pinMode(OUT_PINS[i], OUTPUT);
    digitalWrite(OUT_PINS[i], LOW);
  }

  // set up Arduino interrupt
  attachInterrupt(digitalPinToInterrupt(CLK_IN), clock_isr, CHANGE);

  // set up MCP23017 pins
  mcp.begin();
  for (int i = 0; i < N_CHANNELS; i++) {
    mcp.pinMode(ENC_A_PINS[i], INPUT);
    mcp.pinMode(ENC_B_PINS[i], INPUT);
    mcp.pinMode(ENC_S_PINS[i], INPUT);
  }

  // set up interrupts on MCP23017
  // all encoder pins must be on the same bank!!
  enc_int_cap_reg = mcp.regForPin(ENC_A_PINS[0], MCP23017_INTCAPA, MCP23017_INTCAPB);
  // all encoder switch pins must be on the same bank!!
  sw_int_cap_reg = mcp.regForPin(ENC_S_PINS[0], MCP23017_INTCAPA, MCP23017_INTCAPB);

  mcp.setupInterrupts(false, false, LOW);
  for (int i = 0; i < N_CHANNELS; i++) {
    mcp.setupInterruptPin(ENC_A_PINS[i], CHANGE);
    mcp.disableInterruptPin(ENC_B_PINS[i]);
    mcp.setupInterruptPin(ENC_S_PINS[i], CHANGE);
  }

  // initialise encoder states
  for (int i = 0; i < N_CHANNELS; i++) {
    curr_enc_states[i]    = mcp.digitalRead(ENC_A_PINS[i]);
    prev_enc_states[i]    = curr_enc_states[i];
    curr_switch_states[i] = mcp.digitalRead(ENC_S_PINS[i]);
    prev_switch_states[i] = curr_switch_states[i];
  }

  // initialise modes
  for (int i = 0; i < N_CHANNELS; i++) {
    length_mode[i] = false;
  }

  // initialise sequences
  update_sequence();

  // set-up leds
  Tlc.init();
  delay(10);
  update_leds();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  // handle encoders and switches
  if (!digitalRead(GPIO_INT_B))
    readSwitches();
  if (!digitalRead(GPIO_INT_A))
    readEncoders();

  // handle length_mode timeout
  for (int i = 0; i < N_CHANNELS; i++) {
    if (length_mode[i]) {
      if ( (millis() - time_at_last_turn[i]) > TIMEOUT) {
        length_mode[i] = false;
        update_leds();
      }
    }
  }

  // handle reset pin
  cur_reset_state = !digitalRead(RESET_IN);
  if (cur_reset_state && !prev_reset_state) { // input rising edge
    do_reset = true;
  }
  prev_reset_state = cur_reset_state;

}

////////////////////////////////////////////////////////////////////////////////////////////////////

// read the encoders and process the results
void readEncoders() {
  uint8_t reg = mcp.readRegister(enc_int_cap_reg); // pull data from MCP23017

  for (int i = 0; i < N_CHANNELS; i++) {

    bool enc_a = (reg >> mcp.bitForPin(ENC_A_PINS[i])) & (0x01);
    bool enc_b = (reg >> mcp.bitForPin(ENC_B_PINS[i])) & (0x01);

    prev_enc_states[i] = curr_enc_states[i];
    curr_enc_states[i] = enc_a;

    if (!curr_enc_states[i] && prev_enc_states[i]) {
      has_turned_since_press[i] = true;
      time_at_last_turn[i] = millis();

      // determine direction
      int increment = 0;
      if (!enc_b)
        increment = 1;
      else
        increment = -1;

      // handle turn depending on current mode
      if (length_mode[i]) {
        seq_length_temp[i] = constrain((int) seq_length_temp[i] + increment, MIN_LENGTH, MAX_LENGTH);
        update_leds();
      }
      else if (!curr_switch_states[i]) {
        offset[i] += (increment + seq_length[i]);
        offset[i] %= seq_length[i];
      }
      else {
        n_hits[i] = constrain((int) n_hits[i] + increment, 0, seq_length[i]);
      }

      update_sequence();
      update_leds();
    }

  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// read the encoder switches and process the results
void readSwitches() {
  uint8_t reg = mcp.readRegister(sw_int_cap_reg); // pull data from MCP23017

  for (int i = 0; i < N_CHANNELS; i++) {
    prev_switch_states[i] = curr_switch_states[i];
    curr_switch_states[i] = (reg >> mcp.bitForPin(ENC_S_PINS[i])) & 0x1;

    // switch pressed down
    if (!curr_switch_states[i] && prev_switch_states[i]) {
      // exit length mode with second press
      if (length_mode[i]) {
        length_mode[i] = false;

        // update the sequence length
        n_hits[i]      = (int) n_hits[i] * seq_length_temp[i] / seq_length[i];  // keep fill ratio (approximately) constant
        n_hits[i]      = min(n_hits[i], seq_length[i]);   // constrain n_hits
        seq_length[i]  = seq_length_temp[i];              // update sequence length
        curr_step[i]  %= seq_length[i];                   // constrain current step
        offset[i]     %= seq_length[i];                   // constrain offset

        has_turned_since_press[i] = true; // prevent getting stuck in length mode

        update_sequence();
        update_leds();
      }
      else {
        has_turned_since_press[i] = false;
      }
    }

    // switch released
    if (curr_switch_states[i] && !prev_switch_states[i]) {

      // enter length mode by pressing and releasing switch without turning
      if (!has_turned_since_press[i]) {
        length_mode[i]        = true;

        seq_length_temp[i]    = seq_length[i];
        time_at_last_turn[i]  = millis(); // reset timout

        update_leds();
      }
    }

  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// send data out to the LEDs
void update_leds() {

  for (int i = 0; i < N_CHANNELS; i++) {

    // length selection mode
    if (length_mode[i]) {
      // active steps
      for (int j = 0; j < seq_length_temp[i]; j++)
        Tlc.set(16 * i + j, LENGTH_MODE_BRIGHTNESS);

      // inactive steps
      for (int j = seq_length_temp[i]; j < MAX_LENGTH; j++) {
        Tlc.set(16 * i + j, INACTIVE_BRIGHTNESS);
      }
    }

    // regular mode
    else {
      // active steps
      for (int j = 0; j < seq_length[i]; j++) {
        if (sequence[i][j])
          Tlc.set(16 * i + j, HIT_BRIGHTNESS);
        else
          Tlc.set(16 * i + j, NO_HIT_BRIGHTNESS);
      }

      // inactive steps
      for (int j = seq_length[i]; j < MAX_LENGTH; j++) {
        Tlc.set(16 * i + j, INACTIVE_BRIGHTNESS);
      }
    }

    // current step
    Tlc.set(16 * i + curr_step[i], CUR_STEP_BRIGHTNESS);
  }

  // push to TLCs
  Tlc.update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// determine the new sequences
void update_sequence() {
  for (int i = 0; i < N_CHANNELS; i++) {

    // see: https://www.computermusicdesign.com/simplest-euclidean-rhythm-algorithm-explained/
    int counter;
    if (n_hits[i] == 0)
      counter = 0;
    else
      counter = seq_length[i] - n_hits[i];

    for (int j = 0; j < seq_length[i]; j++) {
      int k = (j + offset[i]) % seq_length[i];
      counter += n_hits[i];
      sequence[i][k] = (counter >= seq_length[i]);
      counter %= seq_length[i];
    }
    for (int j = seq_length[i]; j < MAX_LENGTH; j++) {
      sequence[i][j] = false;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// handle clock falling/rising edges
void clock_isr() {

  // rising edge on input
  if (!digitalRead(CLK_IN)) {
    for (int i = 0; i < N_CHANNELS; i++) {
      // update current step
      if (do_reset) {
        curr_step[i] = 0;
      }
      else {
        curr_step[i]++;
        curr_step[i] %= seq_length[i];
      }

      // set outputs
      if (sequence[i][curr_step[i]])
        digitalWrite(OUT_PINS[i], HIGH);
      else
        digitalWrite(OUT_PINS[i], LOW);
    }
    do_reset = false;
    update_leds();
  }

  // falling edge on input
  else {
    for (int i = 0; i < N_CHANNELS; i++) {
      digitalWrite(OUT_PINS[i], LOW);
    }
  }

}
