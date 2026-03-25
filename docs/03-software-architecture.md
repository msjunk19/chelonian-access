# Software Architecture

This document describes how the Chelonian Access firmware is structured and how the main components interact.

---

## Overview

The system is built around a **central access loop**, which continuously:

1. Reads RFID input  
2. Determines system state  
3. Executes actions (relay, audio, LED)  
4. Updates timers and security logic  

The architecture is designed to be:

- Modular (separate controllers)
- Testable (mockable interfaces)
- Non-blocking (timers instead of delays where possible)

---

## Core Components

### Access Service Loop

The main logic runs inside the **access service loop**, which is responsible for:

- Detecting RFID cards  
- Handling master programming mode  
- Validating user access  
- Managing timeouts and delays  

This loop acts as the **central state machine** for the system.

---

### Controllers

The system is divided into controllers, each handling a specific subsystem.

#### RFID Controller
- Reads card UIDs from the PN532
- Detects card presence
- Passes UID data to access logic

#### Relay Controller
- Activates relay(s) for door unlock
- Handles relay timing (auto shutoff)
- Currently uses **Relay 1** for unlock

#### Audio Controller
- Sends UART commands to JQ6500
- Plays sounds for:
  - Startup
  - Access granted
  - Access denied
  - Inactivity prompt

#### LED Controller (if implemented)
- Provides visual feedback
- Mirrors system state (granted/denied/idle)

---

## State Management

The system maintains a central state structure that tracks:

- Programming mode (master card active)
- Invalid attempt count
- Timeout durations
- Last activity timestamp
- Relay state

This allows the system to operate without blocking delays.

---

## Access Flow

### 1. Card Detection
- RFID controller reads a UID
- System checks if a card is present

---

### 2. Programming Mode Check
- If master card is detected:
  - Enter or continue programming mode
  - Add/remove user cards

---

### 3. Validation
- If not in programming mode:
  - Check if UID matches stored users

---

### 4. Result Handling

#### Valid Card
- Reset invalid attempt counter
- Activate relay (unlock door)
- Trigger audio/LED feedback
- Log event

#### Invalid Card
- Increment invalid attempt counter
- Apply progressive delay
- Trigger error feedback
- Log attempt

---

## Security Features

### Progressive Delay (Brute-Force Protection)

- Each invalid attempt increases timeout duration
- Prevents rapid retry attacks
- Timeout is enforced before next valid read

---

### Anti-Passback

- Prevents repeated rapid use of the same card
- Requires time gap between valid uses

---

## Timing System

Instead of using blocking delays, the system uses:

- `millis()`-based timing
- Timestamp comparisons
- State-driven transitions

This allows:
- Responsive input handling
- Concurrent features (audio, relay, RFID)

---

## Master Card Programming

- Master card enables programming mode
- While active:
  - Scanning a new card → adds user
  - Scanning an existing card → removes user

Stored data is saved in flash/EEPROM (limited slots).

---

## Logging

The system logs via serial:

- Card reads (UIDs)
- Access granted/denied
- Invalid attempt counts
- Timeout durations

Useful for debugging and system verification.

---

## Testing

The project includes:

- Unit tests using Unity
- Mocked hardware interfaces
- Separation of logic from hardware dependencies

This allows testing without physical hardware.

---

## Future Improvements

- Multi-relay coordination (lock + unlock)
- Remote logging (WiFi/BLE)
- Expanded user storage
- Configurable access rules

> These features are not currently implemented.