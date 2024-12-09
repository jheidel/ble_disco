#include <bluefruit.h>
#include <nrf.h> // Access to Nordic-specific registers

BLEService service = BLEService("94cc0001-e5cb-483a-bb0c-58e34568674c");
BLECharacteristic spin = BLECharacteristic("94cc0002-e5cb-483a-bb0c-58e34568674c");

TimerHandle_t statusIntervalTimer;
TimerHandle_t statusFlashEndTimer;

#define STATUS_INTERVAL_MS 5000
#define STATUS_FLASH_MS 10

#define PIN_MOTOR D0

TimerHandle_t motorOffTimer;
int motor_pwm = 255;

#define MOTOR_OFF_DELAY_MS 5000

void startMotor(int duration_sec) {
  //if (motor_pwm > 0 && motor_pwm < 255) {
  //  analogWrite(PIN_MOTOR, motor_pwm);
  //} else {
    digitalWrite(PIN_MOTOR, HIGH);
  //
  digitalWrite(LED_BLUE, LOW);

  xTimerChangePeriod(motorOffTimer, pdMS_TO_TICKS(duration_sec * 1000), 100);
  xTimerReset(motorOffTimer, 0);
}

// Callback function to turn off the motor
void motorOffCallback(TimerHandle_t xTimer) {
  //analogWrite(PIN_MOTOR, 0);
  digitalWrite(PIN_MOTOR, LOW);  // Turn off the motor
  digitalWrite(LED_BLUE, HIGH);
}

bool is_connected = false;

void statusIntervalCallback(TimerHandle_t xTimer) {
  if (is_connected) {
    digitalWrite(LED_GREEN, LOW);
  } else {
    digitalWrite(LED_RED, LOW);
  }
  xTimerReset(statusFlashEndTimer, 0);
}

void statusFlashEndCallback(TimerHandle_t xTimer) {
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
}

TimerHandle_t postStartupTimer;

#define POST_STARTUP_DELAY_MS 30000

void postStartupCallback(TimerHandle_t xTimer) {
  #ifndef DEBUG_LEVEL
     // Disable the USB peripheral
    NRF_USBD->ENABLE = 0;
  #endif
}

// TODO battery measurements would be cool

void setup() {
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(PIN_MOTOR, OUTPUT);

  digitalWrite(PIN_MOTOR, LOW);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_BLUE, HIGH);

  // Create the motor off timer
  motorOffTimer = xTimerCreate(
    "MotorOffTimer",                    // Name of the timer
    pdMS_TO_TICKS(MOTOR_OFF_DELAY_MS),  // Timer period in ticks
    pdFALSE,                            // One-shot timer
    (void*)0,                           // Timer ID (not used here)
    motorOffCallback                    // Callback function
  );

  // Create the motor off timer
  statusIntervalTimer = xTimerCreate(
    "statusIntervalTimer",              // Name of the timer
    pdMS_TO_TICKS(STATUS_INTERVAL_MS),  // Timer period in ticks
    pdTRUE,                             // repeating timer
    (void*)0,                           // Timer ID (not used here)
    statusIntervalCallback              // Callback function
  );

  // Create the motor off timer
  statusFlashEndTimer = xTimerCreate(
    "statusFlashEndTimer",           // Name of the timer
    pdMS_TO_TICKS(STATUS_FLASH_MS),  // Timer period in ticks
    pdFALSE,                         // One-shot timer
    (void*)0,                        // Timer ID (not used here)
    statusFlashEndCallback           // Callback function
  );

  xTimerStart(statusIntervalTimer, 0);

    // Create the motor off timer
  postStartupTimer = xTimerCreate(
    "postStartupTimer",           // Name of the timer
    pdMS_TO_TICKS(POST_STARTUP_DELAY_MS),  // Timer period in ticks
    pdFALSE,                         // One-shot timer
    (void*)0,                        // Timer ID (not used here)
    postStartupCallback           // Callback function
  );

  xTimerStart(postStartupTimer, 0);

#ifdef DEBUG_LEVEL
  Serial.begin(115200);
  while (!Serial) delay(10);  // for nrf52840 with native usb
  Serial.println("== Disco Ball Controller ==");
#endif

  Bluefruit.autoConnLed(false);

  Bluefruit.configPrphBandwidth(BANDWIDTH_LOW);

  Bluefruit.begin();
  Bluefruit.setTxPower(-4);  // TODO: probably good enough

  Bluefruit.setName("jheidel Disco Ball");

  // NOTE: this seems to be the max that the device will accept...
  Bluefruit.Periph.setConnInterval(799, 799);

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  setupService();
  startAdvertisement();

  // Since loop() is empty, suspend its task so that the system never runs it
  // and can go to sleep properly.
  suspendLoop();
}

void startAdvertisement(void) {

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.addService(service);

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(400, 1600);  // in unit of 0.625 ms, start advertising every 250ms sec, drop down to every 1s
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

err_t setupService(void) {
  VERIFY_STATUS(service.begin());
  spin.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
  spin.setWriteCallback(spin_callback);
  spin.setPermission(SECMODE_OPEN, SECMODE_OPEN);  // TODO
  VERIFY_STATUS(spin.begin());
  return ERROR_NONE;
}


void spin_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  // first byte: duration of spinning
  int duration_sec = data[0];

  // second byte (if present): motor pwm to use
  if (len > 1) {
    motor_pwm = data[1];
  } else {
    motor_pwm = 255;
  }

#ifdef DEBUG_LEVEL
  Serial.println("write callback!");
  Serial.printf("Received payload of %d bytes\n", len);
  Serial.printf("spinning motor for %d sec\n", duration_sec);
#endif

  startMotor(duration_sec);
}

void connect_callback(uint16_t conn_handle) {

  #ifdef DEBUG_LEVEL
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
  #endif

  is_connected = true;

  // Insist on 1s connection interval to help reduce power
  ble_gap_conn_params_t conn_params;
  conn_params.min_conn_interval = 799;  // ~1s
  conn_params.max_conn_interval = 799;  // ~1s
  conn_params.slave_latency = 0;
  conn_params.conn_sup_timeout = 400;  // 4 seconds
  sd_ble_gap_conn_param_update(conn_handle, &conn_params);
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;

  #ifdef DEBUG_LEVEL
  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
  Serial.println("Advertising!");
  #endif

  is_connected = false;
}

void loop() {
  // never called, suspended in setup
}