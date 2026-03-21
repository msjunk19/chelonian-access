# Feature: Master Card Programming

**Complexity**: 🟢 Low
**Hardware Required**: ✅ None
**User Value**: ⭐⭐⭐ Essential

## Overview

Implement a master card system that allows authorized users to add or remove access cards without connecting to a computer or modifying code. This greatly simplifies card management for non-technical users.

## Benefits

- Simplified initial setup
- No computer needed for card management
- Quick on-site card enrollment
- Easy card removal for lost/stolen cards
- Delegate card management to trusted users
- Maintain security with master card protection

## Programming Modes

1. **Add Master Card Mode** - Next card scanned will be set as Master
2. **Add User Card Mode** - Next card scanned will be added
3. **Remove User Card Mode** - Next card scanned will be removed
4. **Wipe All Mode** - Remove all cards except masters

5. **Clone Mode** - Copy permissions from one card to another

## Implementation Checklist

### Phase 1: Master Card Storage
- [X] Master UID Manager handles all Master UIDs and EEPROM read/write:
- [X] Master UIDs written to EEPROM
- [X] Device prompts user to scan master card if one is not already learned

### Phase 2: Programming State Machine
- [ ] Create programming modes:
  ```cpp
  enum ProgrammingMode {
      MODE_NORMAL = 0,
      MODE_ADD_CARD,
      MODE_REMOVE_CARD,
      MODE_ADD_MASTER,
      MODE_WIPE_ALL,
      MODE_CLONE_PERMISSIONS
  };
  ```
- [ ] State machine implementation:
  - [ ] Enter programming with master card hold (3 seconds)
  - [ ] LED/audio feedback for mode selection
  - [ ] Timeout to exit programming mode
  - [ ] Confirmation sequences

### Phase 3: Card Management Functions
- [ ] **Add Card Flow**:
  - [ ] Scan master card for 2 seconds
  - [ ] System enters add mode (LED pattern)
  - [ ] Scan new card to add
  - [ ] Success/failure feedback
  - [ ] Auto-exit after operation

- [ ] **Remove Card Flow**:
  - [ ] Scan master card twice quickly
  - [ ] System enters remove mode
  - [ ] Scan card to remove
  - [ ] Confirmation feedback

- [ ] **Emergency Wipe**:
  - [ ] Scan master card 5 times
  - [ ] Warning sequence
  - [ ] Hold master card to confirm
  - [ ] Wipe all except masters

### Phase 4: User Interface
- [ ] LED patterns for modes:
  ```cpp
  // Programming mode indicators
  MODE_ADD_CARD:    Slow green pulse
  MODE_REMOVE_CARD: Slow red pulse
  MODE_ADD_MASTER:  Alternating green/blue
  MODE_WIPE_ALL:    Fast red strobe
  SUCCESS:          3 green flashes
  FAILURE:          5 red flashes
  ```
- [ ] Audio feedback:
  - [ ] Mode entry sounds
  - [ ] Success/failure tones
  - [ ] Warning sounds for destructive operations

### Phase 5: Security Features
- [ ] **Master Card Protection**:
  - [ ] Cannot remove last master card
  - [ ] Master cards cannot be removed via programming
  - [ ] Only master can add new masters
  - [ ] Master card timeout (30 seconds)

- [ ] **Anti-Tamper**:
  - [ ] Log all programming attempts
  - [ ] Failed master scan lockout
  - [ ] Duress mode (special master card)

### Phase 6: Advanced Features
- [ ] **Batch Operations**:
  ```cpp
  // Rapid enrollment mode
  - Scan master card + special sequence
  - System stays in add mode
  - Scan multiple cards quickly
  - Exit with master card
  ```

- [ ] **Permission Cloning**:
  ```cpp
  // Copy access level from one card to another
  - Enter clone mode with master
  - Scan source card
  - Scan destination card(s)
  - Permissions copied
  ```

- [ ] **Temporary Programming**:
  - [ ] Time-limited programming access
  - [ ] One-time use programming cards
  - [ ] Remote programming activation

### Phase 7: Persistence
- [ ] Save programming state to EEPROM:
  - [ ] Current mode
  - [ ] Master card list
  - [ ] Programming attempt counter
- [ ] Recovery from power loss:
  - [ ] Exit programming mode on startup
  - [ ] Restore master card list
  - [ ] Log interrupted operations

## Usage Examples

### Adding a New Employee Card
1. Supervisor scans their master card for 2 seconds
2. System LED starts pulsing purple
3. New employee scans their card
4. System flashes green 3 times and beeps
5. Card is now authorized

### Removing a Lost Card
1. Supervisor double-taps their master card
2. System LED pulses red
3. Supervisor scans a backup of the lost card
4. System confirms removal with red flashes
5. Lost card no longer works

### Emergency Lockdown
1. Security scans master card 5 times rapidly
2. System enters emergency wipe mode (red strobe)
3. Hold master card for 5 seconds to confirm
4. All non-master cards are removed
5. Only master cards can access

## Testing Checklist

- [ ] Test all programming modes
- [ ] Verify master card protection
- [ ] Test timeout functionality
- [ ] Verify persistence across reboot
- [ ] Test anti-tamper features
- [ ] Verify logging of operations
- [ ] Test batch enrollment
- [ ] Verify error handling

## Configuration Options

```cpp
// Configuration constants
#define MASTER_HOLD_TIME 3000        // ms to enter programming
#define PROGRAMMING_TIMEOUT 30000    // ms before auto-exit
#define MAX_CARDS_STORAGE 50         // Maximum enrolled cards
#define ENABLE_BATCH_MODE true       // Allow rapid enrollment
#define REQUIRE_DUAL_MASTER false    // Require 2 masters for wipe
```

## Future Enhancements

- Hierarchical master cards (super admin, admin, supervisor)
- Biometric confirmation for critical operations
- Cloud backup of card database
- Audit trail with master card ID
- Mobile app master card via NFC
