void invoke_set_nichrome() {
  Serial.println("--------------------");
  Serial.println("Set Nichrome invoked");
  Serial.println("--------------------");
}

void invoke_set_motor() {
  Serial.println("-----------------");
  Serial.println("Set Motor invoked");
  Serial.println("-----------------");
}

void invoke_adjust_transmit_power() {
  Serial.println("-----------------------------");
  Serial.println("Adjust Transmit Power invoked");
  Serial.println("-----------------------------");
}

void invoke_cutdown_parachute() {
  Serial.println("-------------------------");
  Serial.println("CUTDOWN PARACHUTE invoked");
  Serial.println("-------------------------");
}

void invoke_cutdown_balloon() {
  Serial.println("--------------------");
  Serial.println("Set Nichrome invoked");
  Serial.println("--------------------");
}

typedef void (*FunctionPointer)();

struct tcInfo {
  char trigger;
  FunctionPointer callback;
  const char *callback_name;
  bool confirm_required;
};

int BAUD_RATE = 9600;
int NUM_COMMANDS = 5;
String INTRO_MESSAGE = "";

// tcDict stands for "trigger-callback array"
struct tcInfo tc_arr[NUM_COMMANDS] = {
  {'0', invoke_set_nichrome, "Set Nichrome", false},
  {'1', invoke_set_motor, "Set Motor", false},
  {'2', invoke_adjust_transmit_power, "Adjust Transmit Power", false},
  {'3', invoke_cutdown_parachute, "CUTDOWN PARACHUTE", true},
  {'4', invoke_cutdown_balloon, "CUTDOWN BALLOON", true}
};

for (int i = 0; i < NUM_COMMANDS; ++i) {
  struct tcInfo cur = tc_arr[i];
  INTRO_MESSAGE += ("To '" + String(cur.callback_name) + "', enter '" + cur.trigger + "' (without quotations)\n");
}

struct tcInfo *get_tcInfo(char trigger) {
  for (int i = 0; i < NUM_COMMANDS; ++i) {
    struct tcInfo *cur = &tc_arr[i];
    if (cur->trigger == trigger) {
      return cur;
    }
  }

  return nullptr;
}

void setup() {
  Serial.begin(BAUD_RATE);
  Serial.print(INTRO_MESSAGE);
  Serial.println();
}

void loop() {

  Serial.println("Waiting for character to be entered");
  
  while (Serial.available() == 0) {
    continue;
  }

  char entered_trigger = Serial.read();

  if (Serial.available() > 0) {
    Serial.println("-----------------------------------");
    Serial.println("INVALID: MORE THAN ONE CHAR ENTERED");
    Serial.println("-----------------------------------");
    continue;
  }

  struct tcInfo *tcPtr = get_tcInfo(entered_trigger);

  if (tcPtr == nullptr) {
    Serial.println("---------------");
    Serial.println("INVALID TRIGGER");
    Serial.println("---------------");
    continue;
  }

  struct tcInfo tc = *tcPtr;

  if (tc.confirm_required) {
    Serial.println("CONFIRM: Do you REALLY want to " + String(tc.callback_name) + "?");
    Serial.println("'Y' or 'y' for yes, any other character for no.");

    while (Serial.available() == 0) {
      continue;
    }

    char entered_confirm = Serial.read();

    if (Serial.available() > 0) {
      Serial.println("-----------------------------------");
      Serial.println("INVALID: MORE THAN ONE CHAR ENTERED");
      Serial.println("-----------------------------------");
      continue;
    }

    if (entered_confirm != 'Y' && entered_confirm != 'y') {
      Serial.println("------------------------------");
      Serial.println("ABORTED: " + String(tc.callback_name));
      Serial.println("------------------------------");
      continue;
    }
  }

  tc.callback();
}


