// pin definitions
#define CV1 A0
#define CV2 A1
#define CV3 A6
#define CV4 A7

// settings and constants
#define N_CV_CHANNELS 4 // number of CV inputs
const byte CV_PINS[N_CV_CHANNELS] = {CV1, CV2, CV3, CV4};
unsigned int analog_values[N_CV_CHANNELS] = {0, 0, 0, 0}; // cv input values  

// cv parameter mapping
const byte N_HITS_CV_CHAN[N_CHANNELS] = {1, 3, 0, 0}; // CV channels assigned to number of hits  (use 0 for no assignment)
const byte OFFSET_CV_CHAN[N_CHANNELS] = {2, 4, 0, 0}; // CV channels assigned to offset          (use 0 for no assignment)

// cv parameter values
int n_hits_cv_values[N_CHANNELS] = {0, 0, 0, 0};      // CV value for number of hits
int offset_cv_values[N_CHANNELS] = {0, 0, 0, 0};      // CV value for offset

////////////////////////////////////////////////////////////////////////////////////////////////////

// read CV inputs
bool read_cvs() {

  // read analog pins
  for (int i = 0; i < N_CV_CHANNELS; i++)
    analog_values[i] = analogRead(CV_PINS[i]);

  // map analog values to sequence parameters and check if values have changed
  bool has_changed = false;
  for (int i = 0; i < N_CHANNELS; i++) {
    
    if (N_HITS_CV_CHAN[i]) {
      int new_value = map(analog_values[N_HITS_CV_CHAN[i]-1], 0, 1023, seq_length[i], -seq_length[i]);
      if (n_hits_cv_values[i] != new_value) {
        has_changed = true;
        n_hits_cv_values[i] = new_value;
      }
    }
    if (OFFSET_CV_CHAN[i]) {
      int new_value = map(analog_values[OFFSET_CV_CHAN[i]-1], 0, 1023, seq_length[i], -seq_length[i]);
      if (offset_cv_values[i] != new_value) {
        has_changed = true;
        offset_cv_values[i] = new_value;
      }
    }
  }
  
  return has_changed;    
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void cv_expander_setup() {

  //pin modes
  for (int i = 0; i < N_CV_CHANNELS; i++)
    pinMode(CV_PINS[i], INPUT);

  // read initial value
  read_cvs();    
}




