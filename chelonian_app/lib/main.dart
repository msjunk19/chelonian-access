import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:uuid/uuid.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_beacon/flutter_beacon.dart';
import 'package:permission_handler/permission_handler.dart';
import 'dart:convert';
import 'dart:async';
import 'dart:math' as math;

const String SERVICE_UUID     = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const String CMD_UUID         = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const String STATUS_UUID      = "beb5483f-36e1-4688-b7f5-ea07361b26a8";
const String PAIR_UUID        = "beb54840-36e1-4688-b7f5-ea07361b26a8";
const String BEACON_UUID_CHAR = "beb54841-36e1-4688-b7f5-ea07361b26a8";
const String MAC_UUID_CHAR    = "beb54842-36e1-4688-b7f5-ea07361b26a8";

const int RSSI_UNLOCK_THRESHOLD = -70;
const int RSSI_LOCK_THRESHOLD   = -85;
const int PROXIMITY_LOCK_COOLDOWN_SECS = 10;

const List<String> _unpairingErrors = [
  'error:unknown_device',
  'error:unauthorized',
];

class AppStateNotifier extends ChangeNotifier {
  bool   connected        = false;
  bool   scanning         = false;
  bool   paired           = false;
  String status           = "Disconnected";

  void update({
    bool?   connected,
    bool?   scanning,
    bool?   paired,
    String? status,
  }) {
    if (connected != null) this.connected = connected;
    if (scanning  != null) this.scanning  = scanning;
    if (paired    != null) this.paired    = paired;
    if (status    != null) this.status    = status;
    notifyListeners();
  }
}

// Maps RSSI to 0–4 bars
int rssiToBars(int rssi) {
  if (rssi >= -65) return 4;
  if (rssi >= -75) return 3;
  if (rssi >= -85) return 2;
  if (rssi >= -95) return 1;
  return 0;
}

// ─────────────────────────────────────────────
// Signal strength painter (WiFi-style arcs)
// ─────────────────────────────────────────────
class SignalStrengthPainter extends CustomPainter {
  final int   bars;
  final Color activeColor;
  final Color inactiveColor;

  SignalStrengthPainter({
    required this.bars,
    required this.activeColor,
    required this.inactiveColor,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final cx = size.width / 2;
    final cy = size.height * 0.92;

    // Dot at base
    final dotPaint = Paint()
      ..color = bars > 0 ? activeColor : inactiveColor
      ..style = PaintingStyle.fill;
    canvas.drawCircle(Offset(cx, cy), size.width * 0.07, dotPaint);

    const totalArcs  = 4;
    const startAngle = math.pi + math.pi * 0.22;
    const sweepAngle = math.pi - math.pi * 0.44;

    for (int i = 0; i < totalArcs; i++) {
      final fraction = (i + 1) / totalArcs;
      final radius   = size.width * 0.18 + size.width * 0.55 * fraction;
      final isActive = (i + 1) <= bars;

      final paint = Paint()
        ..color       = isActive ? activeColor : inactiveColor
        ..style       = PaintingStyle.stroke
        ..strokeWidth = size.width * 0.09
        ..strokeCap   = StrokeCap.round;

      canvas.drawArc(
        Rect.fromCircle(center: Offset(cx, cy), radius: radius),
        startAngle, sweepAngle, false, paint,
      );
    }
  }

  @override
  bool shouldRepaint(SignalStrengthPainter old) =>
      old.bars != bars || old.activeColor != activeColor;
}

// ─────────────────────────────────────────────
// Signal strength widget
// ─────────────────────────────────────────────
class SignalStrengthIcon extends StatelessWidget {
  final int    rssi;
  final bool   active;
  final double size;

  const SignalStrengthIcon({
    super.key,
    required this.rssi,
    required this.active,
    this.size = 28,
  });

  @override
  Widget build(BuildContext context) {
    final bars        = active ? rssiToBars(rssi) : 0;
    final activeColor = bars >= 3
        ? Colors.green
        : bars >= 2
            ? Colors.orange
            : Colors.red;

    return SizedBox(
      width: size, height: size,
      child: CustomPaint(
        painter: SignalStrengthPainter(
          bars: bars,
          activeColor: active ? activeColor : Colors.grey.shade400,
          inactiveColor: Colors.grey.shade300,
        ),
      ),
    );
  }
}

// ─────────────────────────────────────────────
// App entry
// ─────────────────────────────────────────────
void main() {
  runApp(const ChelonianApp());
}

class ChelonianApp extends StatelessWidget {
  const ChelonianApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Chelonian',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.green),
        useMaterial3: true,
      ),
      home: const HomePage(),
    );
  }
}

// ─────────────────────────────────────────────
// Home page
// ─────────────────────────────────────────────
class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> with TickerProviderStateMixin {
  
  // BLE
  BluetoothDevice?         _device;
  BluetoothCharacteristic? _cmdChar;
  BluetoothCharacteristic? _statusChar;
  BluetoothCharacteristic? _pairChar;
  BluetoothCharacteristic? _beaconUUIDCharacteristic;
  BluetoothCharacteristic? _macCharacteristic;

  bool _connected = false;
  bool _scanning  = false; //not used rn
  bool _paired    = false;

  // Auth
  String? _deviceId;
  String? _token;
  String? _savedMac;

  // Beacon / proximity
  String?             _beaconUUID;
  StreamSubscription? _beaconSub;
  bool   _proximityEnabled = false;
  bool   _isUnlocked       = false;
  int    _lastRSSI         = -999;
  String _proximityStatus  = "Proximity: Off";
  DateTime? _lastLockTime;  // prevents rapid unlock after lock

  // UI
  String _status              = "Disconnected";
  bool   _repairDialogShowing = false;

  // Button press animation
  late AnimationController _pressController;
  late Animation<double>   _pressAnim;
  final _appState = AppStateNotifier();

  @override
  void initState() {
    super.initState();
    _pressController = AnimationController(
      vsync: this,
      duration: const Duration(milliseconds: 120),
    );
    _pressAnim = Tween<double>(begin: 1.0, end: 0.90).animate(
      CurvedAnimation(parent: _pressController, curve: Curves.easeInOut),
    );
    _loadPairing();
  }

  @override
  void dispose() {
    _beaconSub?.cancel();
    _pressController.dispose();
    super.dispose();
  }

  // ── Storage ──────────────────────────────────────────────────────────

  Future<void> _loadPairing() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      _deviceId   = prefs.getString('device_id');
      _token      = prefs.getString('device_token');
      _beaconUUID = prefs.getString('beacon_uuid');
      _savedMac   = prefs.getString('device_mac');
      _paired     = _deviceId != null && _token != null;
    });
    _appState.update(paired: _paired); 
    if (_beaconUUID != null && _paired) _startProximityMonitoring();
    // Auto-open settings on first run
    if (!_paired) {
      WidgetsBinding.instance.addPostFrameCallback((_) => _openSettings());
    }
  }

  Future<void> _savePairing(String deviceId, String token) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('device_id', deviceId);
    await prefs.setString('device_token', token);
    setState(() { _deviceId = deviceId; _token = token; _paired = true; });
    _appState.update(paired: true);
  }

  Future<void> _saveBeaconUUID(String uuid) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('beacon_uuid', uuid);
    setState(() { _beaconUUID = uuid; });
  }

  Future<void> _clearPairing() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove('device_id');
    await prefs.remove('device_token');
    await prefs.remove('beacon_uuid');
    await prefs.remove('device_mac');
    _stopProximityMonitoring();
    setState(() {
      _deviceId = null; _token = null;
      _beaconUUID = null; _savedMac = null;
      _paired = false;
    });
    _appState.update(paired: false, connected: false);
  }

  // ── BLE ──────────────────────────────────────────────────────────────

  Future<void> _scanAndConnect() async {
    // setState(() { _scanning = true; _status = "Scanning..."; });

    setState(() { _scanning = true; _status = "Scanning..."; });
    _appState.update(scanning: true, status: "Scanning...");

    final state = await FlutterBluePlus.adapterState.first;
    if (state != BluetoothAdapterState.on) {
      setState(() { _status = "Bluetooth is off"; _scanning = false; });
      return;
    }

    StreamSubscription? sub;
    sub = FlutterBluePlus.scanResults.listen((results) async {
      for (final r in results) {
        if (r.advertisementData.advName == "Chelonian") {
          await FlutterBluePlus.stopScan();
          sub?.cancel();
          await _connectToDevice(r.device);
          break;
        }
      }
    });

    await FlutterBluePlus.startScan(
      withServices: [Guid(SERVICE_UUID)],
      timeout: const Duration(seconds: 10),
    );
    await Future.delayed(const Duration(seconds: 11));
    sub.cancel();

    if (!_connected) {
      // setState(() { _status = "Device not found"; _scanning = false; });
      setState(() { _status = "Device not found"; _scanning = false; });
      _appState.update(status: "Device not found", scanning: false);
    }
  }

  Future<void> _connectToDevice(BluetoothDevice device) async {
    // setState(() { _status = "Connecting..."; });
    setState(() { _status = "Connecting..."; });
    _appState.update(status: "Connecting...");
    try {
      await device.connect(license: License.free);
      _device = device;

      final services = await device.discoverServices();
      for (final svc in services) {
        if (svc.uuid.toString() == SERVICE_UUID) {
          for (final c in svc.characteristics) {
            final u = c.uuid.toString();
            if (u == CMD_UUID)         _cmdChar                  = c;
            if (u == STATUS_UUID)      _statusChar               = c;
            if (u == PAIR_UUID)        _pairChar                 = c;
            if (u == BEACON_UUID_CHAR) _beaconUUIDCharacteristic = c;
            if (u == MAC_UUID_CHAR)    _macCharacteristic        = c;
          }
        }
      }

      if (_statusChar != null) {
        await _statusChar!.setNotifyValue(true);
        _statusChar!.onValueReceived.listen((value) {
          final response = utf8.decode(value).trim().toLowerCase();
          if (_isPairingError(response)) _handlePairingLost();
        });
      }

      device.connectionState.listen((s) {
        if (s == BluetoothConnectionState.disconnected) {
          // setState(() { _connected = false; _status = "Disconnected"; });
          setState(() { _connected = false; _status = "Disconnected"; });
          _appState.update(connected: false, status: "Disconnected");
        }
      });

      // setState(() {
      //   _connected = true;
      //   _scanning  = false;
      //   _status    = "Connected";
      // });

      setState(() { _connected = true; _scanning = false; _status = "Connected"; });
      _appState.update(connected: true, scanning: false, status: "Connected");

      if (_paired && _beaconUUID == null) await _fetchDeviceInfo();

    } catch (e) {
      // setState(() { _status = "Connection failed: $e"; _scanning = false; });
      setState(() { _status = "Connection failed: $e"; _scanning = false; });
      _appState.update(status: "Connection failed: $e", scanning: false);
    }
  }

  Future<void> _disconnect() async {
    await _device?.disconnect();
    setState(() {
      _connected = false; _device = null;
      _cmdChar = null; _statusChar = null; _pairChar = null;
      _beaconUUIDCharacteristic = null; _macCharacteristic = null;
      _status = "Disconnected";
    });
  }

  // ── Pairing ───────────────────────────────────────────────────────────

  Future<void> _pair() async {
    if (_pairChar == null) { 
      // setState(() { _status = "Not connected"; }); 
      setState(() { _status = "Pairing..."; });
      _appState.update(status: "Pairing...");
      return; }
    final deviceId = const Uuid().v4();
    setState(() { _status = "Pairing..."; });
    try {
      await _pairChar!.write(utf8.encode(deviceId));
      final response = await _pairChar!.read();
      final token    = utf8.decode(response).trim();
      if (token.startsWith("error:")) {
        // setState(() { _status = "Pairing failed: $token"; });
        setState(() { _status = "Pairing failed: $token"; });
        _appState.update(status: "Pairing failed: $token");
        return;
      }
      await _savePairing(deviceId, token);
      await _fetchDeviceInfo();
      // setState(() { _status = "Paired successfully!"; });
      setState(() { _status = "Paired successfully!"; });
      _appState.update(paired: true, status: "Paired successfully!");
    } catch (e) {
      // setState(() { _status = "Pairing error: $e"; });
      setState(() { _status = "Pairing error: $e"; });
      _appState.update(status: "Pairing error: $e");
    }
  }

  Future<void> _fetchDeviceInfo() async {
    try {
      if (_beaconUUIDCharacteristic != null) {
        final v = await _beaconUUIDCharacteristic!.read();
        await _saveBeaconUUID(utf8.decode(v).trim());
      }
      if (_macCharacteristic != null) {
        final v   = await _macCharacteristic!.read();
        final mac = utf8.decode(v).trim();
        final prefs = await SharedPreferences.getInstance();
        await prefs.setString('device_mac', mac);
        setState(() { _savedMac = mac; });
      }
      if (_beaconUUID != null) _startProximityMonitoring();
    } catch (e) {
      setState(() { _proximityStatus = "Could not fetch device info: $e"; });
    }
  }

  Future<void> _unpair() async {
    await _clearPairing();
    // setState(() { _status = "Unpaired"; });
    setState(() { _status = "Unpaired"; });
    _appState.update(paired: false, status: "Unpaired");
  }

  // ── Pairing-lost ──────────────────────────────────────────────────────

  bool _isPairingError(String r) =>
      _unpairingErrors.any((e) => r.contains(e));

  Future<void> _handlePairingLost() async {
    if (_repairDialogShowing) return;
    _repairDialogShowing = true;
    await _clearPairing();
    // setState(() { _status = "Pairing lost"; });
    setState(() { _status = "Pairing lost"; });
    _appState.update(paired: false, status: "Pairing lost");
    if (!mounted) return;

    await showDialog(
      context: context,
      barrierDismissible: false,
      builder: (ctx) => AlertDialog(
        icon: const Icon(Icons.link_off, size: 48, color: Colors.red),
        title: const Text("Device No Longer Paired"),
        content: const Text(
          "The device no longer recognises this phone. "
          "This usually happens when the device has been reset or re-flashed.",
        ),
        actions: [
          TextButton(
            onPressed: () { Navigator.pop(ctx); _repairDialogShowing = false; },
            child: const Text("Later"),
          ),
          ElevatedButton.icon(
            icon: const Icon(Icons.bluetooth_searching),
            label: const Text("Pair Now"),
            style: ElevatedButton.styleFrom(
              backgroundColor: Colors.green,
              foregroundColor: Colors.white,
            ),
            onPressed: () async {
              Navigator.pop(ctx);
              _repairDialogShowing = false;
              if (_connected && _pairChar != null) {
                await _pair();
              } else {
                await _scanAndConnect();
                if (_connected) await _pair();
              }
            },
          ),
        ],
      ),
    );
    _repairDialogShowing = false;
  }

  // ── Commands ──────────────────────────────────────────────────────────

  Future<void> _sendCommand(int command) async {
    if (!_connected || _cmdChar == null) {
      await _scanAndConnect();
      if (!_connected) { setState(() { _status = "Could not connect"; }); return; }
    }
    if (!_paired || _deviceId == null || _token == null) {
      setState(() { _status = "Not paired"; });
      return;
    }

    // Press animation + haptic
    await _pressController.forward();
    await _pressController.reverse();
    HapticFeedback.mediumImpact();

    final timestamp = (DateTime.now().millisecondsSinceEpoch / 1000).floor();
    final payload = "${_deviceId!.trim()}|${_token!.trim()}|$command|$timestamp".trim();
    try {
      await _cmdChar!.write(utf8.encode(payload), withoutResponse: false);
      setState(() {
        _status     = _commandName(command);
        _isUnlocked = command == 1;
        if (command == 2) _lastLockTime = DateTime.now();
      });
    } on Exception catch (e) {
      final msg = e.toString();
      // setState(() { _status = "Command failed: $msg"; });
      setState(() { _status = "Command failed: $msg"; });
      _appState.update(status: "Command failed: $msg");
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text("Failed to send command: $msg"),
          backgroundColor: Colors.red,
          action: SnackBarAction(
            label: "Retry",
            textColor: Colors.white,
            onPressed: () => _sendCommand(command),
          ),
        ),
      );
    }
  }

  String _commandName(int command) {
    switch (command) {
      case 1: return "Unlock sent";
      case 2: return "Lock sent";
      case 5: return "Trunk sent";
      case 6: return "Panic sent";
      default: return "Command sent";
    }
  }

  // ── Proximity ─────────────────────────────────────────────────────────

  Future<void> _requestPermissions() async {
    await Permission.location.request();
    await Permission.locationAlways.request();
    await Permission.bluetooth.request();
  }

  Future<void> _startProximityMonitoring() async {
    if (_beaconUUID == null) {
      setState(() { _proximityStatus = "No beacon UUID — pair first"; });
      return;
    }
    await _requestPermissions();
    try {
      await flutterBeacon.initializeScanning;
      final regions = [
        Region(identifier: 'chelonian', proximityUUID: _beaconUUID!, major: 1, minor: 1),
      ];
      _beaconSub = flutterBeacon.ranging(regions).listen((result) {
        if (result.beacons.isNotEmpty) {
          final rssi = result.beacons.first.rssi;
          setState(() { _lastRSSI = rssi; _proximityStatus = "Signal: $rssi dBm"; });
          
          // Proximity unlock with cooldown protection
          bool inCooldown = _lastLockTime != null &&
            DateTime.now().difference(_lastLockTime!).inSeconds < PROXIMITY_LOCK_COOLDOWN_SECS;
          if (inCooldown) {
            final secs = PROXIMITY_LOCK_COOLDOWN_SECS - 
              DateTime.now().difference(_lastLockTime!).inSeconds;
            setState(() { _proximityStatus = "Cooldown: ${secs}s"; });
          }
          if (rssi > RSSI_UNLOCK_THRESHOLD && !_isUnlocked && !inCooldown) {
            _sendCommand(1);
          }
          if (rssi < RSSI_LOCK_THRESHOLD   &&  _isUnlocked) _sendCommand(2);
        } else {
          setState(() { _proximityStatus = "Beacon not detected"; });
          if (_isUnlocked) _sendCommand(2);
        }
      });
      setState(() { _proximityEnabled = true; _proximityStatus = "Monitoring..."; });
    } catch (e) {
      setState(() { _proximityStatus = "Beacon error: $e"; });
    }
  }

  void _stopProximityMonitoring() {
    _beaconSub?.cancel();
    _beaconSub = null;
    setState(() { _proximityEnabled = false; _proximityStatus = "Proximity: Off"; });
  }

  // ── Navigation ────────────────────────────────────────────────────────

void _openSettings() {
  Navigator.push(
    context,
    MaterialPageRoute(
      builder: (_) => SettingsPage(
        appState: _appState,
        deviceId:         _deviceId,
        savedMac:         _savedMac,
        beaconUUID:       _beaconUUID,
        proximityEnabled: _proximityEnabled,
        lastRSSI:         _lastRSSI,
        proximityStatus:  _proximityStatus,
        onScanAndConnect: _scanAndConnect,
        onDisconnect:     _disconnect,
        onPair:           _pair,
        onUnpair:         _unpair,
        onStartProximity: _startProximityMonitoring,
        onStopProximity:  _stopProximityMonitoring,
      ),
    ),
  );
}


  // ── Build ─────────────────────────────────────────────────────────────

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);

    return Scaffold(
      backgroundColor: theme.colorScheme.surface,
      appBar: AppBar(
        backgroundColor: theme.colorScheme.surface,
        elevation: 0,
        title: const Text(
          'Chelonian',
          style: TextStyle(fontWeight: FontWeight.w600, letterSpacing: 0.5),
        ),
        actions: [
          // BT status dot
          Padding(
            padding: const EdgeInsets.only(right: 4),
            child: Icon(
              _connected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
              color: _connected ? Colors.green : Colors.grey,
              size: 22,
            ),
          ),
          // Proximity signal
          if (_proximityEnabled)
            Padding(
              padding: const EdgeInsets.only(right: 4),
              child: SignalStrengthIcon(rssi: _lastRSSI, active: true, size: 24),
            ),
          IconButton(
            icon: const Icon(Icons.settings_outlined),
            onPressed: _openSettings,
            tooltip: 'Settings',
          ),
        ],
      ),
      body: SafeArea(
        child: Column(
          children: [
            // ── Status pill ─────────────────────────────────────────────
            Padding(
              padding: const EdgeInsets.only(top: 8),
              child: AnimatedContainer(
                duration: const Duration(milliseconds: 300),
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
                decoration: BoxDecoration(
                  color: _connected
                      ? Colors.green.withValues(alpha: 0.1)
                      : Colors.grey.withValues(alpha: 0.1),
                  borderRadius: BorderRadius.circular(20),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    AnimatedContainer(
                      duration: const Duration(milliseconds: 300),
                      width: 8, height: 8,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: _connected ? Colors.green : Colors.grey,
                      ),
                    ),
                    const SizedBox(width: 8),
                    Text(
                      _status,
                      style: TextStyle(
                        fontSize: 13,
                        fontWeight: FontWeight.w500,
                        color: _connected
                            ? Colors.green.shade700
                            : Colors.grey.shade600,
                      ),
                    ),
                  ],
                ),
              ),
            ),

            // ── Main content — vertically centered ───────────────────────
            Expanded(
              child: Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [

                    // ── Unlock button ────────────────────────────────────
                    _LockButton(
                      onPressed: _paired ? () => _sendCommand(1) : null,
                      icon:  Icons.lock_open_rounded,
                      label: "Unlock",
                      color: Colors.green,
                      pressAnim: _pressAnim,
                      active: _isUnlocked,
                    ),

                    const SizedBox(height: 36),

                    // ── Lock state icon ──────────────────────────────────
                    AnimatedSwitcher(
                      duration: const Duration(milliseconds: 350),
                      transitionBuilder: (child, anim) =>
                          ScaleTransition(scale: anim, child: child),
                      child: Icon(
                        _isUnlocked
                            ? Icons.lock_open_rounded
                            : Icons.lock_rounded,
                        key: ValueKey(_isUnlocked),
                        size: 68,
                        color: _isUnlocked
                            ? Colors.green.withValues(alpha: 0.1)
                            : Colors.red.withValues(alpha: 0.1),
                      ),
                    ),

                    // Proximity status under state icon
                    const SizedBox(height: 10),
                    if (_proximityEnabled)
                      Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          SignalStrengthIcon(rssi: _lastRSSI, active: true, size: 16),
                          const SizedBox(width: 6),
                          Text(
                            _proximityStatus,
                            style: TextStyle(
                                fontSize: 12, color: Colors.grey.shade500),
                          ),
                        ],
                      )
                    else
                      // Reserve space so layout doesn't jump
                      const SizedBox(height: 16),

                    const SizedBox(height: 36),

                    // ── Lock button ──────────────────────────────────────
                    _LockButton(
                      onPressed: _paired ? () => _sendCommand(2) : null,
                      icon:  Icons.lock_rounded,
                      label: "Lock",
                      color: Colors.red,
                      pressAnim: _pressAnim,
                      active: !_isUnlocked,
                    ),

                    const SizedBox(height: 24),

                    // ── Trunk and Panic buttons ──────────────────────────
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        _SmallButton(
                          onPressed: _paired ? () => _sendCommand(5) : null,
                          icon: Icons.car_rental_rounded,
                          label: "Trunk",
                          color: Colors.purple,
                        ),
                        const SizedBox(width: 24),
                        _SmallButton(
                          onPressed: _paired ? () => _sendCommand(6) : null,
                          icon: Icons.warning_amber_rounded,
                          label: "Panic",
                          color: Colors.orange,
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ─────────────────────────────────────────────
// Large lock / unlock button
// ─────────────────────────────────────────────
class _LockButton extends StatelessWidget {
  final VoidCallback?      onPressed;
  final IconData           icon;
  final String             label;
  final Color              color;
  final Animation<double>  pressAnim;
  final bool               active; // is this the current state?

  const _LockButton({
    required this.onPressed,
    required this.icon,
    required this.label,
    required this.color,
    required this.pressAnim,
    required this.active,
  });

  @override
  Widget build(BuildContext context) {
    final enabled        = onPressed != null;
    final effectiveColor = enabled ? color : Colors.grey.shade300;

    return ScaleTransition(
      scale: active ? pressAnim : const AlwaysStoppedAnimation(1.0),
      child: GestureDetector(
        onTap: enabled ? onPressed : null,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            AnimatedContainer(
              duration: const Duration(milliseconds: 200),
              width: 108, height: 108,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                color: effectiveColor.withValues(
                  alpha: enabled ? (active ? 0.18 : 0.10) : 0.05,
                ),
              border: Border.all(
                color: effectiveColor.withValues(
                  alpha: enabled ? (active ? 0.5 : 0.25) : 0.12,
                ),
                width: 2,
              ),
                boxShadow: enabled && active
                    ? [
                        BoxShadow(
                          color: effectiveColor.withValues(alpha: 0.25),
                          blurRadius: 18,
                          spreadRadius: 2,
                        )
                      ]
                    : [],
              ),
              child: Icon(icon, size: 50, color: effectiveColor),
            ),
            const SizedBox(height: 10),
            Text(
              label,
              style: TextStyle(
                fontSize: 13,
                fontWeight: FontWeight.w600,
                letterSpacing: 1.0,
                color: effectiveColor,
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ─────────────────────────────────────────────
// Small button for Trunk / Panic
// ─────────────────────────────────────────────
class _SmallButton extends StatelessWidget {
  final VoidCallback? onPressed;
  final IconData      icon;
  final String        label;
  final Color         color;

  const _SmallButton({
    required this.onPressed,
    required this.icon,
    required this.label,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    final enabled        = onPressed != null;
    final effectiveColor = enabled ? color : Colors.grey.shade300;

    return GestureDetector(
      onTap: enabled ? onPressed : null,
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 64,
            height: 64,
            decoration: BoxDecoration(
              color: effectiveColor.withValues(alpha: enabled ? 0.15 : 0.05),
              borderRadius: BorderRadius.circular(16),
              border: Border.all(
                color: effectiveColor.withValues(alpha: enabled ? 0.3 : 0.12),
                width: 2,
              ),
            ),
            child: Icon(icon, size: 28, color: effectiveColor),
          ),
          const SizedBox(height: 6),
          Text(
            label,
            style: TextStyle(
              fontSize: 12,
              fontWeight: FontWeight.w600,
              letterSpacing: 0.5,
              color: effectiveColor,
            ),
          ),
        ],
      ),
    );
  }
}

class SettingsPage extends StatelessWidget {
  final AppStateNotifier appState;
  final String?          savedMac;
  final String?          deviceId;
  final String?          beaconUUID;
  final bool             proximityEnabled;
  final int              lastRSSI;
  final String           proximityStatus;

  final VoidCallback onScanAndConnect;
  final VoidCallback onDisconnect;
  final VoidCallback onPair;
  final VoidCallback onUnpair;
  final VoidCallback onStartProximity;
  final VoidCallback onStopProximity;

  const SettingsPage({
    super.key,
    required this.appState,
    required this.savedMac,
    required this.deviceId,
    required this.beaconUUID,
    required this.proximityEnabled,
    required this.lastRSSI,
    required this.proximityStatus,
    required this.onScanAndConnect,
    required this.onDisconnect,
    required this.onPair,
    required this.onUnpair,
    required this.onStartProximity,
    required this.onStopProximity,
  });

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings'),
        backgroundColor: Theme.of(context).colorScheme.surface,
        elevation: 0,
      ),
      body: ListenableBuilder(
        listenable: appState,
        builder: (context, _) {
          final connected = appState.connected;
          final scanning  = appState.scanning;
          final paired    = appState.paired;
          final status    = appState.status;

          return ListView(
            children: [

              // ── Connection ────────────────────────────────────────────
              _SettingsSection(
                title: "Connection",
                children: [
                  _InfoTile(
                    icon: connected
                        ? Icons.bluetooth_connected
                        : Icons.bluetooth_disabled,
                    iconColor: connected ? Colors.green : Colors.grey,
                    title: connected ? "Connected to Chelonian" : "Not connected",
                    subtitle: savedMac,
                  ),
                  // Status text
                  Padding(
                    padding: const EdgeInsets.fromLTRB(16, 0, 16, 4),
                    child: Text(
                      status,
                      style: TextStyle(fontSize: 12, color: Colors.grey.shade500),
                    ),
                  ),
                  Padding(
                    padding: const EdgeInsets.fromLTRB(16, 4, 16, 12),
                    child: connected
                        ? OutlinedButton.icon(
                            onPressed: onDisconnect,
                            icon: const Icon(Icons.bluetooth_disabled),
                            label: const Text("Disconnect"),
                          )
                        : ElevatedButton.icon(
                            onPressed: scanning ? null : onScanAndConnect,
                            icon: scanning
                                ? const SizedBox(
                                    width: 18, height: 18,
                                    child: CircularProgressIndicator(strokeWidth: 2),
                                  )
                                : const Icon(Icons.bluetooth_searching),
                            label: Text(scanning ? "Scanning..." : "Connect"),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.green,
                              foregroundColor: Colors.white,
                            ),
                          ),
                  ),
                ],
              ),

              // ── Pairing ───────────────────────────────────────────────
              _SettingsSection(
                title: "Pairing",
                children: [
                  _InfoTile(
                    icon: paired ? Icons.link : Icons.link_off,
                    iconColor: paired ? Colors.green : Colors.orange,
                    title: paired ? "Device paired" : "Not paired",
                    subtitle: paired && deviceId != null
                        ? "ID: ${deviceId!.substring(0, 8)}..."
                        : connected
                            ? "Tap below to pair"
                            : "Connect first, then pair",
                  ),
                  Padding(
                    padding: const EdgeInsets.fromLTRB(16, 4, 16, 12),
                    child: paired
                        ? OutlinedButton.icon(
                            onPressed: onUnpair,
                            icon: const Icon(Icons.link_off, color: Colors.red),
                            label: const Text("Unpair",
                                style: TextStyle(color: Colors.red)),
                            style: OutlinedButton.styleFrom(
                              side: const BorderSide(color: Colors.red),
                            ),
                          )
                        : ElevatedButton.icon(
                            onPressed: connected ? onPair : null,
                            icon: const Icon(Icons.link),
                            label: const Text("Pair Device"),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.orange,
                              foregroundColor: Colors.white,
                            ),
                          ),
                  ),
                ],
              ),

              // ── Proximity ─────────────────────────────────────────────
              _SettingsSection(
                title: "Proximity Unlock",
                children: [
                  SwitchListTile(
                    secondary: const Icon(Icons.sensors),
                    title: const Text("Auto-unlock when nearby"),
                    subtitle: Text(proximityStatus),
                    value: proximityEnabled,
                    onChanged: paired
                        ? (v) => v ? onStartProximity() : onStopProximity()
                        : null,
                  ),
                  if (proximityEnabled)
                    Padding(
                      padding: const EdgeInsets.fromLTRB(16, 0, 16, 14),
                      child: Row(
                        children: [
                          SignalStrengthIcon(rssi: lastRSSI, active: true, size: 40),
                          const SizedBox(width: 16),
                          Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                "${rssiToBars(lastRSSI)} / 4 bars",
                                style: const TextStyle(
                                    fontWeight: FontWeight.w600, fontSize: 15),
                              ),
                              Text(
                                "RSSI: $lastRSSI dBm",
                                style: TextStyle(
                                    fontSize: 12, color: Colors.grey.shade600),
                              ),
                            ],
                          ),
                        ],
                      ),
                    ),
                ],
              ),

              // ── Debug ─────────────────────────────────────────────────
              _SettingsSection(
                title: "Debug Info",
                children: [
                  _DebugTile(label: "Status",      value: status),
                  _DebugTile(label: "Device ID",   value: deviceId ?? "—"),
                  _DebugTile(label: "MAC",         value: savedMac ?? "—"),
                  _DebugTile(label: "Beacon UUID", value: beaconUUID ?? "—"),
                  _DebugTile(label: "RSSI",        value: "$lastRSSI dBm"),
                  const SizedBox(height: 4),
                ],
              ),

              const SizedBox(height: 40),
            ],
          );
        },
      ),
    );
  }
}

// ─────────────────────────────────────────────
// Settings helpers
// ─────────────────────────────────────────────

class _SettingsSection extends StatelessWidget {
  final String       title;
  final List<Widget> children;

  const _SettingsSection({required this.title, required this.children});

  @override
  Widget build(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.fromLTRB(16, 20, 16, 6),
          child: Text(
            title.toUpperCase(),
            style: TextStyle(
              fontSize: 11,
              fontWeight: FontWeight.w700,
              letterSpacing: 1.2,
              color: Colors.grey.shade500,
            ),
          ),
        ),
        Container(
          margin: const EdgeInsets.symmetric(horizontal: 16),
          decoration: BoxDecoration(
            color: Theme.of(context)
                .colorScheme
                .surfaceContainerHighest
                .withValues(alpha: 0.45),
            borderRadius: BorderRadius.circular(12),
          ),
          child: Column(children: children),
        ),
      ],
    );
  }
}

class _InfoTile extends StatelessWidget {
  final IconData icon;
  final Color    iconColor;
  final String   title;
  final String?  subtitle;

  const _InfoTile({
    required this.icon,
    required this.iconColor,
    required this.title,
    this.subtitle,
  });

  @override
  Widget build(BuildContext context) {
    return ListTile(
      leading: Icon(icon, color: iconColor),
      title: Text(title, style: const TextStyle(fontWeight: FontWeight.w500)),
      subtitle: subtitle != null ? Text(subtitle!) : null,
    );
  }
}

class _DebugTile extends StatelessWidget {
  final String label;
  final String value;

  const _DebugTile({required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 7),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 96,
            child: Text(label,
                style: TextStyle(fontSize: 12, color: Colors.grey.shade500)),
          ),
          Expanded(
            child: Text(
              value,
              style: const TextStyle(
                fontSize: 12,
                fontFamily: 'monospace',
                fontWeight: FontWeight.w500,
              ),
              overflow: TextOverflow.ellipsis,
            ),
          ),
        ],
      ),
    );
  }
}
