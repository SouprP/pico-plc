# Modbus Master/Slave Test Setup

## Hardware Setup

### Option 1: Two Pico Boards (Recommended)
Connect two Raspberry Pi Pico boards together:

```
Master Pico          Slave Pico
-----------          ----------
GP16 (TX) ------>--- GP17 (RX)
GP17 (RX) ---<------ GP16 (TX)
GND       ----------- GND
```

**Important:** Both boards must share a common ground (GND).

### Option 2: Single Pico with Loopback (Testing Only)
Connect TX directly to RX on the same board:
```
GP16 (TX) ----+
              |
GP17 (RX) ----+
```

## Building the Test Programs

### Build Master:
```bash
cd cmake-windows
cmake --build . --target master_plc
```

Output: `master_plc.uf2`

### Build Slave:
```bash
cd cmake-windows
cmake --build . --target slave_plc
```

Output: `slave_plc.uf2`

## Flashing

### Manual Flash:
1. Hold BOOTSEL button while connecting USB
2. Drag and drop `master_plc.uf2` to master Pico
3. Drag and drop `slave_plc.uf2` to slave Pico

### Automatic Flash (if configured):
```bash
# Flash master
picotool load -f --ser ${MASTER_ID} -x master_plc.uf2

# Flash slave
picotool load -f --ser ${SLAVE_ID} -x slave_plc.uf2
```

## Running the Test

### 1. Start Slave First
Connect slave Pico via USB and open serial monitor:
```bash
# Windows
mode COM5  # Use your COM port

# Or use PuTTY / Tera Term
```

You should see:
```
=== Modbus Slave Test ===
Slave Address: 1
UART: UART0, Baudrate: 9600
Pins: TX=GP16, RX=GP17

Slave initialized and listening...
```

### 2. Start Master
Connect master Pico via USB and open serial monitor (different port):

You should see:
```
=== Modbus Master Test ===
UART: UART0, Baudrate: 9600
Pins: TX=GP16, RX=GP17

Master initialized. Starting to poll slave...

[Master] Request #1: Read 10 registers from address 0
[Master] Received response from slave 1
  Function: 0x03
  Byte count: 20
  Register values: 100 101 102 103 104 105 106 107 108 109
```

### 3. Slave Serial Output
The slave should show:
```
[Slave] Received request from master
  Address: 1
  Function: 0x03
  Start Address: 0
  Quantity: 10
  Sending response with 10 registers
```

## What the Test Does

### Master:
- Polls slave every 1 second
- Sends "Read Holding Registers" (0x03) request
- Reads 10 registers starting from address 0
- Displays received register values

### Slave:
- Listens for requests on address 1
- Has 256 holding registers (initialized with values 100-355)
- Responds to Read Holding Registers (0x03)
- Also supports Write Single Register (0x06)
- Register 0 increments automatically for testing

## Expected Output

### Master Console:
```
[Master] Request #1: Read 10 registers from address 0
[Master] Received response from slave 1
  Function: 0x03
  Byte count: 20
  Register values: 100 101 102 103 104 105 106 107 108 109

[Master] Request #2: Read 10 registers from address 0
[Master] Received response from slave 1
  Function: 0x03
  Byte count: 20
  Register values: 101 101 102 103 104 105 106 107 108 109
```
(Note register 0 incrementing: 100 → 101)

### Slave Console:
```
[Slave] Received request from master
  Address: 1
  Function: 0x03
  Start Address: 0
  Quantity: 10
  Sending response with 10 registers

[Slave] Received request from master
  Address: 1
  Function: 0x03
  Start Address: 0
  Quantity: 10
  Sending response with 10 registers
```

## Troubleshooting

### No Communication:
1. Check wiring (TX→RX, RX→TX, GND connected)
2. Verify both using same baudrate (9600)
3. Check UART pins (GP16=TX, GP17=RX)
4. Ensure both boards powered and running

### Garbled Data:
1. Check baudrate match
2. Verify ground connection
3. Check for electrical noise on wires
4. Try shorter wires

### Slave Not Responding:
1. Verify slave address matches (default: 1)
2. Check slave console for errors
3. Ensure slave started before master
4. Check CRC calculation

### CRC Errors:
The Modbus implementation includes automatic CRC checking. If you see CRC errors:
1. Check for electrical interference
2. Verify baudrate settings
3. Check wire quality/length
4. Add delays between transmissions

## Testing Write Operations

To test Write Single Register (0x06), modify `master.cpp`:

```cpp
// In main loop, add:
modbus_frame_t write_request;
write_request.address = SLAVE_ADDRESS;
write_request.function_code = 0x06; // Write single register

uint8_t write_data[4];
write_data[0] = 0x00; // Register address high
write_data[1] = 0x05; // Register address low (register 5)
write_data[2] = 0x12; // Value high
write_data[3] = 0x34; // Value low (value = 0x1234 = 4660)

write_request.data = write_data;
write_request.data_length = 4;

modbus_master.queue_write(write_request);
```

## UART Configuration

Current settings (Modbus RTU standard):
- **Baudrate:** 9600 bps
- **Data bits:** 8
- **Parity:** Even
- **Stop bits:** 1
- **Flow control:** None

To change baudrate, modify in both master.cpp and slave.cpp:
```cpp
#define MASTER_BAUDRATE 19200  // Or 115200
#define SLAVE_BAUDRATE 19200   // Must match master!
```

## Performance Notes

- At 9600 baud: ~10-15 requests/second possible
- At 115200 baud: ~100+ requests/second possible
- Frame timing (T3.5) automatically calculated per Modbus spec
- CPU usage: <1% at 9600 baud with moderate traffic
