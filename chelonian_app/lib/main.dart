// import 'package:flutter/material.dart';
// import 'package:flutter/services.dart';
// import 'package:shared_preferences/shared_preferences.dart';
// import 'package:uuid/uuid.dart';
// import 'package:flutter_blue_plus/flutter_blue_plus.dart';
// import 'package:flutter_beacon/flutter_beacon.dart';
// import 'package:permission_handler/permission_handler.dart';
// import 'dart:convert';
// import 'dart:async';
// import 'dart:math' as math;

// const String SERVICE_UUID     = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
// const String CMD_UUID         = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
// const String STATUS_UUID      = "beb5483f-36e1-4688-b7f5-ea07361b26a8";
// const String PAIR_UUID        = "beb54840-36e1-4688-b7f5-ea07361b26a8";
// const String BEACON_UUID_CHAR = "beb54841-36e1-4688-b7f5-ea07361b26a8";
// const String MAC_UUID_CHAR    = "beb54842-36e1-4688-b7f5-ea07361b26a8";

// const int RSSI_UNLOCK_THRESHOLD = -70;  // auto-unlock when close
// const int RSSI_LOCK_THRESHOLD   = -85;    // auto-lock when nearby
// const int RSSI_LEAVE_THRESHOLD  = -95;    // must leave this range to reset auto-unlock

// const List<String> _unpairingErrors = [
//   'error:unknown_device',
//   'error:unauthorized',
// ];

// class AppStateNotifier extends ChangeNotifier {
//   bool   connected        = false;
//   bool   scanning         = false;
//   bool   paired           = false;
//   String status           = "Disconnected";

//   void update({
//     bool?   connected,
//     bool?   scanning,
//     bool?   paired,
//     String? status,
//   }) {
//     if (connected != null) this.connected = connected;
//     if (scanning  != null) this.scanning  = scanning;
//     if (paired    != null) this.paired    = paired;
//     if (status    != null) this.status    = status;
//     notifyListeners();
//   }
// }

// // Maps RSSI to 0–4 bars
// int rssiToBars(int rssi) {
//   if (rssi >= -65) return 4;
//   if (rssi >= -75) return 3;
//   if (rssi >= -85) return 2;
//   if (rssi >= -95) return 1;
//   return 0;
// }

// // ─────────────────────────────────────────────
// // Signal strength painter (WiFi-style arcs)
// // ─────────────────────────────────────────────
// class SignalStrengthPainter extends CustomPainter {
//   final int   bars;
//   final Color activeColor;
//   final Color inactiveColor;

//   SignalStrengthPainter({
//     required this.bars,
//     required this.activeColor,
//     required this.inactiveColor,
//   });

//   @override
//   void paint(Canvas canvas, Size size) {
//     final cx = size.width / 2;
//     final cy = size.height * 0.92;

//     // Dot at base
//     final dotPaint = Paint()
//       ..color = bars > 0 ? activeColor : inactiveColor
//       ..style = PaintingStyle.fill;
//     canvas.drawCircle(Offset(cx, cy), size.width * 0.07, dotPaint);

//     const totalArcs  = 4;
//     const startAngle = math.pi + math.pi * 0.22;
//     const sweepAngle = math.pi - math.pi * 0.44;

//     for (int i = 0; i < totalArcs; i++) {
//       final fraction = (i + 1) / totalArcs;
//       final radius   = size.width * 0.18 + size.width * 0.55 * fraction;
//       final isActive = (i + 1) <= bars;

//       final paint = Paint()
//         ..color       = isActive ? activeColor : inactiveColor
//         ..style       = PaintingStyle.stroke
//         ..strokeWidth = size.width * 0.09
//         ..strokeCap   = StrokeCap.round;

//       canvas.drawArc(
//         Rect.fromCircle(center: Offset(cx, cy), radius: radius),
//         startAngle, sweepAngle, false, paint,
//       );
//     }
//   }

//   @override
//   bool shouldRepaint(SignalStrengthPainter old) =>
//       old.bars != bars || old.activeColor != activeColor;
// }

// // ─────────────────────────────────────────────
// // Signal strength widget
// // ─────────────────────────────────────────────
// class SignalStrengthIcon extends StatelessWidget {
//   final int    rssi;
//   final bool   active;
//   final double size;

//   const SignalStrengthIcon({
//     super.key,
//     required this.rssi,
//     required this.active,
//     this.size = 28,
//   });

//   @override
//   Widget build(BuildContext context) {
//     final bars        = active ? rssiToBars(rssi) : 0;
//     final activeColor = bars >= 3
//         ? Colors.green
//         : bars >= 2
//             ? Colors.orange
//             : Colors.red;

//     return SizedBox(
//       width: size, height: size,
//       child: CustomPaint(
//         painter: SignalStrengthPainter(
//           bars: bars,
//           activeColor: active ? activeColor : Colors.grey.shade400,
//           inactiveColor: Colors.grey.shade300,
//         ),
//       ),
//     );
//   }
// }

// // ─────────────────────────────────────────────
// // App entry
// // ─────────────────────────────────────────────
// void main() {
//   runApp(const ChelonianApp());
// }

// class ChelonianApp extends StatelessWidget {
//   const ChelonianApp({super.key});

//   @override
//   Widget build(BuildContext context) {
//     return MaterialApp(
//       title: 'Chelonian',
//       theme: ThemeData(
//         colorScheme: ColorScheme.fromSeed(seedColor: Colors.green),
//         useMaterial3: true,
//       ),
//       home: const HomePage(),
//     );
//   }
// }

// // ─────────────────────────────────────────────
// // Home page
// // ─────────────────────────────────────────────
// class HomePage extends StatefulWidget {
//   const HomePage({super.key});

//   @override
//   State<HomePage> createState() => _HomePageState();
// }

// class _HomePageState extends State<HomePage> with TickerProviderStateMixin {
  
//   // BLE
//   BluetoothDevice?         _device;
//   BluetoothCharacteristic? _cmdChar;
//   BluetoothCharacteristic? _statusChar;
//   BluetoothCharacteristic? _pairChar;
//   BluetoothCharacteristic? _beaconUUIDCharacteristic;
//   BluetoothCharacteristic? _macCharacteristic;

//   bool _connected = false;
//   bool _scanning  = false; //not used rn
//   bool _paired    = false;

//   // Auth
//   String? _deviceId;
//   String? _token;
//   String? _savedMac;

//   // Beacon / proximity
//   String?             _beaconUUID;
//   StreamSubscription? _beaconSub;
//   bool   _proximityEnabled = false;
//   bool   _isUnlocked       = false;
//   int    _lastRSSI         = -999;
//   String _proximityStatus  = "Proximity: Off";
//   bool   _wasOutOfProximity = true;  // true initially so first approach auto-unlocks, must leave and return after lock

//   // UI
//   String _status              = "Disconnected";
//   bool   _repairDialogShowing = false;

//   // Button press animation
//   late AnimationController _pressController;
//   late Animation<double>   _pressAnim;
//   final _appState = AppStateNotifier();

//   @override
//   void initState() {
//     super.initState();
//     _pressController = AnimationController(
//       vsync: this,
//       duration: const Duration(milliseconds: 120),
//     );
//     _pressAnim = Tween<double>(begin: 1.0, end: 0.90).animate(
//       CurvedAnimation(parent: _pressController, curve: Curves.easeInOut),
//     );
//     _loadPairing();
//   }

//   @override
//   void dispose() {
//     _beaconSub?.cancel();
//     _pressController.dispose();
//     super.dispose();
//   }

//   // ── Storage ──────────────────────────────────────────────────────────

//   Future<void> _loadPairing() async {
//     final prefs = await SharedPreferences.getInstance();
//     setState(() {
//       _deviceId   = prefs.getString('device_id');
//       _token      = prefs.getString('device_token');
//       _beaconUUID = prefs.getString('beacon_uuid');
//       _savedMac   = prefs.getString('device_mac');
//       _paired     = _deviceId != null && _token != null;
//     });
//     _appState.update(paired: _paired); 
//     if (_beaconUUID != null && _paired) _startProximityMonitoring();
//     // Auto-open settings on first run
//     if (!_paired) {
//       WidgetsBinding.instance.addPostFrameCallback((_) => _openSettings());
//     }
//   }

//   Future<void> _savePairing(String deviceId, String token) async {
//     final prefs = await SharedPreferences.getInstance();
//     await prefs.setString('device_id', deviceId);
//     await prefs.setString('device_token', token);
//     setState(() { _deviceId = deviceId; _token = token; _paired = true; });
//     _appState.update(paired: true);
//   }

//   Future<void> _saveBeaconUUID(String uuid) async {
//     final prefs = await SharedPreferences.getInstance();
//     await prefs.setString('beacon_uuid', uuid);
//     setState(() { _beaconUUID = uuid; });
//   }

//   Future<void> _clearPairing() async {
//     final prefs = await SharedPreferences.getInstance();
//     await prefs.remove('device_id');
//     await prefs.remove('device_token');
//     await prefs.remove('beacon_uuid');
//     await prefs.remove('device_mac');
//     _stopProximityMonitoring();
//     setState(() {
//       _deviceId = null; _token = null;
//       _beaconUUID = null; _savedMac = null;
//       _paired = false;
//     });
//     _appState.update(paired: false, connected: false);
//   }

//   // ── BLE ──────────────────────────────────────────────────────────────

//   Future<void> _scanAndConnect() async {
//     // setState(() { _scanning = true; _status = "Scanning..."; });

//     setState(() { _scanning = true; _status = "Scanning..."; });
//     _appState.update(scanning: true, status: "Scanning...");

//     final state = await FlutterBluePlus.adapterState.first;
//     if (state != BluetoothAdapterState.on) {
//       setState(() { _status = "Bluetooth is off"; _scanning = false; });
//       return;
//     }

//     StreamSubscription? sub;
//     sub = FlutterBluePlus.scanResults.listen((results) async {
//       for (final r in results) {
//         if (r.advertisementData.advName == "Chelonian") {
//           await FlutterBluePlus.stopScan();
//           sub?.cancel();
//           await _connectToDevice(r.device);
//           break;
//         }
//       }
//     });

//     await FlutterBluePlus.startScan(
//       withServices: [Guid(SERVICE_UUID)],
//       timeout: const Duration(seconds: 10),
//     );
//     await Future.delayed(const Duration(seconds: 11));
//     sub.cancel();

//     if (!_connected) {
//       // setState(() { _status = "Device not found"; _scanning = false; });
//       setState(() { _status = "Device not found"; _scanning = false; });
//       _appState.update(status: "Device not found", scanning: false);
//     }
//   }

//   Future<void> _connectToDevice(BluetoothDevice device) async {
//     // setState(() { _status = "Connecting..."; });
//     setState(() { _status = "Connecting..."; });
//     _appState.update(status: "Connecting...");
//     try {
//       await device.connect(license: License.free);
//       _device = device;

//       final services = await device.discoverServices();
//       for (final svc in services) {
//         if (svc.uuid.toString() == SERVICE_UUID) {
//           for (final c in svc.characteristics) {
//             final u = c.uuid.toString();
//             if (u == CMD_UUID)         _cmdChar                  = c;
//             if (u == STATUS_UUID)      _statusChar               = c;
//             if (u == PAIR_UUID)        _pairChar                 = c;
//             if (u == BEACON_UUID_CHAR) _beaconUUIDCharacteristic = c;
//             if (u == MAC_UUID_CHAR)    _macCharacteristic        = c;
//           }
//         }
//       }

//       if (_statusChar != null) {
//         await _statusChar!.setNotifyValue(true);
//         _statusChar!.onValueReceived.listen((value) {
//           final response = utf8.decode(value).trim().toLowerCase();
//           if (_isPairingError(response)) _handlePairingLost();
//         });
//       }

//       device.connectionState.listen((s) {
//         if (s == BluetoothConnectionState.disconnected) {
//           // setState(() { _connected = false; _status = "Disconnected"; });
//           setState(() { _connected = false; _status = "Disconnected"; });
//           _appState.update(connected: false, status: "Disconnected");
//         }
//       });

//       // setState(() {
//       //   _connected = true;
//       //   _scanning  = false;
//       //   _status    = "Connected";
//       // });

//       setState(() { _connected = true; _scanning = false; _status = "Connected"; });
//       _appState.update(connected: true, scanning: false, status: "Connected");

//       if (_paired && _beaconUUID == null) await _fetchDeviceInfo();

//     } catch (e) {
//       // setState(() { _status = "Connection failed: $e"; _scanning = false; });
//       setState(() { _status = "Connection failed: $e"; _scanning = false; });
//       _appState.update(status: "Connection failed: $e", scanning: false);
//     }
//   }

//   Future<void> _disconnect() async {
//     await _device?.disconnect();
//     setState(() {
//       _connected = false; _device = null;
//       _cmdChar = null; _statusChar = null; _pairChar = null;
//       _beaconUUIDCharacteristic = null; _macCharacteristic = null;
//       _status = "Disconnected";
//     });
//   }

//   // ── Pairing ───────────────────────────────────────────────────────────

//   Future<void> _pair() async {
//     if (_pairChar == null) { 
//       // setState(() { _status = "Not connected"; }); 
//       setState(() { _status = "Pairing..."; });
//       _appState.update(status: "Pairing...");
//       return; }
//     final deviceId = const Uuid().v4();
//     setState(() { _status = "Pairing..."; });
//     try {
//       await _pairChar!.write(utf8.encode(deviceId));
//       final response = await _pairChar!.read();
//       final token    = utf8.decode(response).trim();
//       if (token.startsWith("error:")) {
//         // setState(() { _status = "Pairing failed: $token"; });
//         setState(() { _status = "Pairing failed: $token"; });
//         _appState.update(status: "Pairing failed: $token");
//         return;
//       }
//       await _savePairing(deviceId, token);
//       await _fetchDeviceInfo();
//       // setState(() { _status = "Paired successfully!"; });
//       setState(() { _status = "Paired successfully!"; });
//       _appState.update(paired: true, status: "Paired successfully!");
//     } catch (e) {
//       // setState(() { _status = "Pairing error: $e"; });
//       setState(() { _status = "Pairing error: $e"; });
//       _appState.update(status: "Pairing error: $e");
//     }
//   }

//   Future<void> _fetchDeviceInfo() async {
//     try {
//       if (_beaconUUIDCharacteristic != null) {
//         final v = await _beaconUUIDCharacteristic!.read();
//         await _saveBeaconUUID(utf8.decode(v).trim());
//       }
//       if (_macCharacteristic != null) {
//         final v   = await _macCharacteristic!.read();
//         final mac = utf8.decode(v).trim();
//         final prefs = await SharedPreferences.getInstance();
//         await prefs.setString('device_mac', mac);
//         setState(() { _savedMac = mac; });
//       }
//       if (_beaconUUID != null) _startProximityMonitoring();
//     } catch (e) {
//       setState(() { _proximityStatus = "Could not fetch device info: $e"; });
//     }
//   }

//   Future<void> _unpair() async {
//     await _clearPairing();
//     // setState(() { _status = "Unpaired"; });
//     setState(() { _status = "Unpaired"; });
//     _appState.update(paired: false, status: "Unpaired");
//   }

//   // ── Pairing-lost ──────────────────────────────────────────────────────

//   bool _isPairingError(String r) =>
//       _unpairingErrors.any((e) => r.contains(e));

//   Future<void> _handlePairingLost() async {
//     if (_repairDialogShowing) return;
//     _repairDialogShowing = true;
//     await _clearPairing();
//     // setState(() { _status = "Pairing lost"; });
//     setState(() { _status = "Pairing lost"; });
//     _appState.update(paired: false, status: "Pairing lost");
//     if (!mounted) return;

//     await showDialog(
//       context: context,
//       barrierDismissible: false,
//       builder: (ctx) => AlertDialog(
//         icon: const Icon(Icons.link_off, size: 48, color: Colors.red),
//         title: const Text("Device No Longer Paired"),
//         content: const Text(
//           "The device no longer recognises this phone. "
//           "This usually happens when the device has been reset or re-flashed.",
//         ),
//         actions: [
//           TextButton(
//             onPressed: () { Navigator.pop(ctx); _repairDialogShowing = false; },
//             child: const Text("Later"),
//           ),
//           ElevatedButton.icon(
//             icon: const Icon(Icons.bluetooth_searching),
//             label: const Text("Pair Now"),
//             style: ElevatedButton.styleFrom(
//               backgroundColor: Colors.green,
//               foregroundColor: Colors.white,
//             ),
//             onPressed: () async {
//               Navigator.pop(ctx);
//               _repairDialogShowing = false;
//               if (_connected && _pairChar != null) {
//                 await _pair();
//               } else {
//                 await _scanAndConnect();
//                 if (_connected) await _pair();
//               }
//             },
//           ),
//         ],
//       ),
//     );
//     _repairDialogShowing = false;
//   }

//   // ── Commands ──────────────────────────────────────────────────────────

//   Future<void> _sendCommand(int command) async {
//     if (!_connected || _cmdChar == null) {
//       await _scanAndConnect();
//       if (!_connected) { setState(() { _status = "Could not connect"; }); return; }
//     }
//     if (!_paired || _deviceId == null || _token == null) {
//       setState(() { _status = "Not paired"; });
//       return;
//     }

//     // Press animation + haptic
//     await _pressController.forward();
//     await _pressController.reverse();
//     HapticFeedback.mediumImpact();

//     final timestamp = (DateTime.now().millisecondsSinceEpoch / 1000).floor();
//     final payload = "${_deviceId!.trim()}|${_token!.trim()}|$command|$timestamp".trim();
//     try {
//       await _cmdChar!.write(utf8.encode(payload), withoutResponse: false);
//       setState(() {
//         _status     = _commandName(command);
//         _isUnlocked = command == 1;
//       });
//     } on Exception catch (e) {
//       final msg = e.toString();
//       // setState(() { _status = "Command failed: $msg"; });
//       setState(() { _status = "Command failed: $msg"; });
//       _appState.update(status: "Command failed: $msg");
//       if (!mounted) return;
//       ScaffoldMessenger.of(context).showSnackBar(
//         SnackBar(
//           content: Text("Failed to send command: $msg"),
//           backgroundColor: Colors.red,
//           action: SnackBarAction(
//             label: "Retry",
//             textColor: Colors.white,
//             onPressed: () => _sendCommand(command),
//           ),
//         ),
//       );
//     }
//   }

//   String _commandName(int command) {
//     switch (command) {
//       case 1: return "Unlock sent";
//       case 2: return "Lock sent";
//       case 5: return "Trunk sent";
//       case 6: return "Panic sent";
//       default: return "Command sent";
//     }
//   }

//   // ── Proximity ─────────────────────────────────────────────────────────

//   Future<void> _requestPermissions() async {
//     await Permission.location.request();
//     await Permission.locationAlways.request();
//     await Permission.bluetooth.request();
//   }

//   Future<void> _startProximityMonitoring() async {
//     if (_beaconUUID == null) {
//       setState(() { _proximityStatus = "No beacon UUID — pair first"; });
//       return;
//     }
//     await _requestPermissions();
//     try {
//       await flutterBeacon.initializeScanning;
//       final regions = [
//         Region(identifier: 'chelonian', proximityUUID: _beaconUUID!, major: 1, minor: 1),
//       ];
//       _beaconSub = flutterBeacon.ranging(regions).listen((result) {
//         if (result.beacons.isNotEmpty) {
//           final rssi = result.beacons.first.rssi;
//           final wasInRange = _lastRSSI > RSSI_LEAVE_THRESHOLD;
//           final isInRange = rssi > RSSI_LEAVE_THRESHOLD;
//           final isInProximity = rssi > RSSI_LOCK_THRESHOLD;
//           final isNearProximity = rssi > RSSI_UNLOCK_THRESHOLD;
          
//           // Track when we left the leave threshold zone
//           if (wasInRange && !isInRange) {
//             _wasOutOfProximity = true;
//           }
          
//           setState(() { _lastRSSI = rssi; _proximityStatus = "Signal: $rssi dBm"; });
          
//           // Auto-unlock: only if we left and returned to close proximity
//           if (isNearProximity && !_isUnlocked && _wasOutOfProximity) {
//             _sendCommand(1);
//             _wasOutOfProximity = false;  // reset after triggering
//           }
          
//           // Auto-lock: if out of proximity (but still in BLE range)
//           if (!isInProximity && _isUnlocked) {
//             _sendCommand(2);
//             // Don't reset _wasOutOfProximity here - user still needs to leave fully
//           }
//         } else {
//           // Beacon lost entirely - user definitely left
//           _wasOutOfProximity = true;
//           setState(() { _proximityStatus = "Beacon not detected"; });
//           if (_isUnlocked) _sendCommand(2);
//         }
//       });
//       setState(() { _proximityEnabled = true; _proximityStatus = "Monitoring..."; });
//     } catch (e) {
//       setState(() { _proximityStatus = "Beacon error: $e"; });
//     }
//   }

//   void _stopProximityMonitoring() {
//     _beaconSub?.cancel();
//     _beaconSub = null;
//     setState(() { _proximityEnabled = false; _proximityStatus = "Proximity: Off"; });
//   }

//   // ── Navigation ────────────────────────────────────────────────────────

// void _openSettings() {
//   Navigator.push(
//     context,
//     MaterialPageRoute(
//       builder: (_) => SettingsPage(
//         appState: _appState,
//         deviceId:         _deviceId,
//         savedMac:         _savedMac,
//         beaconUUID:       _beaconUUID,
//         proximityEnabled: _proximityEnabled,
//         lastRSSI:         _lastRSSI,
//         proximityStatus:  _proximityStatus,
//         onScanAndConnect: _scanAndConnect,
//         onDisconnect:     _disconnect,
//         onPair:           _pair,
//         onUnpair:         _unpair,
//         onStartProximity: _startProximityMonitoring,
//         onStopProximity:  _stopProximityMonitoring,
//       ),
//     ),
//   );
// }


//   // ── Build ─────────────────────────────────────────────────────────────

//   @override
//   Widget build(BuildContext context) {
//     final theme = Theme.of(context);

//     return Scaffold(
//       backgroundColor: theme.colorScheme.surface,
//       appBar: AppBar(
//         backgroundColor: theme.colorScheme.surface,
//         elevation: 0,
//         title: const Text(
//           'Chelonian',
//           style: TextStyle(fontWeight: FontWeight.w600, letterSpacing: 0.5),
//         ),
//         actions: [
//           // BT status dot
//           Padding(
//             padding: const EdgeInsets.only(right: 4),
//             child: Icon(
//               _connected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
//               color: _connected ? Colors.green : Colors.grey,
//               size: 22,
//             ),
//           ),
//           // Proximity signal
//           if (_proximityEnabled)
//             Padding(
//               padding: const EdgeInsets.only(right: 4),
//               child: SignalStrengthIcon(rssi: _lastRSSI, active: true, size: 24),
//             ),
//           IconButton(
//             icon: const Icon(Icons.settings_outlined),
//             onPressed: _openSettings,
//             tooltip: 'Settings',
//           ),
//         ],
//       ),
//       body: SafeArea(
//         child: Column(
//           children: [
//             // ── Status pill ─────────────────────────────────────────────
//             Padding(
//               padding: const EdgeInsets.only(top: 8),
//               child: AnimatedContainer(
//                 duration: const Duration(milliseconds: 300),
//                 padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 6),
//                 decoration: BoxDecoration(
//                   color: _connected
//                       ? Colors.green.withValues(alpha: 0.1)
//                       : Colors.grey.withValues(alpha: 0.1),
//                   borderRadius: BorderRadius.circular(20),
//                 ),
//                 child: Row(
//                   mainAxisSize: MainAxisSize.min,
//                   children: [
//                     AnimatedContainer(
//                       duration: const Duration(milliseconds: 300),
//                       width: 8, height: 8,
//                       decoration: BoxDecoration(
//                         shape: BoxShape.circle,
//                         color: _connected ? Colors.green : Colors.grey,
//                       ),
//                     ),
//                     const SizedBox(width: 8),
//                     Text(
//                       _status,
//                       style: TextStyle(
//                         fontSize: 13,
//                         fontWeight: FontWeight.w500,
//                         color: _connected
//                             ? Colors.green.shade700
//                             : Colors.grey.shade600,
//                       ),
//                     ),
//                   ],
//                 ),
//               ),
//             ),

//             // ── Main content — vertically centered ───────────────────────
//             Expanded(
//               child: Center(
//                 child: Column(
//                   mainAxisSize: MainAxisSize.min,
//                   children: [

//                     // ── Unlock button ────────────────────────────────────
//                     _LockButton(
//                       onPressed: _paired ? () => _sendCommand(1) : null,
//                       icon:  Icons.lock_open_rounded,
//                       label: "Unlock",
//                       color: Colors.green,
//                       pressAnim: _pressAnim,
//                       active: _isUnlocked,
//                     ),

//                     const SizedBox(height: 36),

//                     // ── Lock state icon ──────────────────────────────────
//                     AnimatedSwitcher(
//                       duration: const Duration(milliseconds: 350),
//                       transitionBuilder: (child, anim) =>
//                           ScaleTransition(scale: anim, child: child),
//                       child: Icon(
//                         _isUnlocked
//                             ? Icons.lock_open_rounded
//                             : Icons.lock_rounded,
//                         key: ValueKey(_isUnlocked),
//                         size: 68,
//                         color: _isUnlocked
//                             ? Colors.green.withValues(alpha: 0.1)
//                             : Colors.red.withValues(alpha: 0.1),
//                       ),
//                     ),

//                     // Proximity status under state icon
//                     const SizedBox(height: 10),
//                     if (_proximityEnabled)
//                       Row(
//                         mainAxisSize: MainAxisSize.min,
//                         children: [
//                           SignalStrengthIcon(rssi: _lastRSSI, active: true, size: 16),
//                           const SizedBox(width: 6),
//                           Text(
//                             _proximityStatus,
//                             style: TextStyle(
//                                 fontSize: 12, color: Colors.grey.shade500),
//                           ),
//                         ],
//                       )
//                     else
//                       // Reserve space so layout doesn't jump
//                       const SizedBox(height: 16),

//                     const SizedBox(height: 36),

//                     // ── Lock button ──────────────────────────────────────
//                     _LockButton(
//                       onPressed: _paired ? () => _sendCommand(2) : null,
//                       icon:  Icons.lock_rounded,
//                       label: "Lock",
//                       color: Colors.red,
//                       pressAnim: _pressAnim,
//                       active: !_isUnlocked,
//                     ),

//                     const SizedBox(height: 24),

//                     // ── Trunk and Panic buttons ──────────────────────────
//                     Row(
//                       mainAxisAlignment: MainAxisAlignment.center,
//                       children: [
//                         _SmallButton(
//                           onPressed: _paired ? () => _sendCommand(5) : null,
//                           icon: Icons.car_rental_rounded,
//                           label: "Trunk",
//                           color: Colors.purple,
//                         ),
//                         const SizedBox(width: 24),
//                         _SmallButton(
//                           onPressed: _paired ? () => _sendCommand(6) : null,
//                           icon: Icons.warning_amber_rounded,
//                           label: "Panic",
//                           color: Colors.orange,
//                         ),
//                       ],
//                     ),
//                   ],
//                 ),
//               ),
//             ),
//           ],
//         ),
//       ),
//     );
//   }
// }

// // ─────────────────────────────────────────────
// // Large lock / unlock button
// // ─────────────────────────────────────────────
// class _LockButton extends StatelessWidget {
//   final VoidCallback?      onPressed;
//   final IconData           icon;
//   final String             label;
//   final Color              color;
//   final Animation<double>  pressAnim;
//   final bool               active; // is this the current state?

//   const _LockButton({
//     required this.onPressed,
//     required this.icon,
//     required this.label,
//     required this.color,
//     required this.pressAnim,
//     required this.active,
//   });

//   @override
//   Widget build(BuildContext context) {
//     final enabled        = onPressed != null;
//     final effectiveColor = enabled ? color : Colors.grey.shade300;

//     return ScaleTransition(
//       scale: active ? pressAnim : const AlwaysStoppedAnimation(1.0),
//       child: GestureDetector(
//         onTap: enabled ? onPressed : null,
//         child: Column(
//           mainAxisSize: MainAxisSize.min,
//           children: [
//             AnimatedContainer(
//               duration: const Duration(milliseconds: 200),
//               width: 108, height: 108,
//               decoration: BoxDecoration(
//                 shape: BoxShape.circle,
//                 color: effectiveColor.withValues(
//                   alpha: enabled ? (active ? 0.18 : 0.10) : 0.05,
//                 ),
//               border: Border.all(
//                 color: effectiveColor.withValues(
//                   alpha: enabled ? (active ? 0.5 : 0.25) : 0.12,
//                 ),
//                 width: 2,
//               ),
//                 boxShadow: enabled && active
//                     ? [
//                         BoxShadow(
//                           color: effectiveColor.withValues(alpha: 0.25),
//                           blurRadius: 18,
//                           spreadRadius: 2,
//                         )
//                       ]
//                     : [],
//               ),
//               child: Icon(icon, size: 50, color: effectiveColor),
//             ),
//             const SizedBox(height: 10),
//             Text(
//               label,
//               style: TextStyle(
//                 fontSize: 13,
//                 fontWeight: FontWeight.w600,
//                 letterSpacing: 1.0,
//                 color: effectiveColor,
//               ),
//             ),
//           ],
//         ),
//       ),
//     );
//   }
// }

// // ─────────────────────────────────────────────
// // Small button for Trunk / Panic
// // ─────────────────────────────────────────────
// class _SmallButton extends StatelessWidget {
//   final VoidCallback? onPressed;
//   final IconData      icon;
//   final String        label;
//   final Color         color;

//   const _SmallButton({
//     required this.onPressed,
//     required this.icon,
//     required this.label,
//     required this.color,
//   });

//   @override
//   Widget build(BuildContext context) {
//     final enabled        = onPressed != null;
//     final effectiveColor = enabled ? color : Colors.grey.shade300;

//     return GestureDetector(
//       onTap: enabled ? onPressed : null,
//       child: Column(
//         mainAxisSize: MainAxisSize.min,
//         children: [
//           Container(
//             width: 64,
//             height: 64,
//             decoration: BoxDecoration(
//               color: effectiveColor.withValues(alpha: enabled ? 0.15 : 0.05),
//               borderRadius: BorderRadius.circular(16),
//               border: Border.all(
//                 color: effectiveColor.withValues(alpha: enabled ? 0.3 : 0.12),
//                 width: 2,
//               ),
//             ),
//             child: Icon(icon, size: 28, color: effectiveColor),
//           ),
//           const SizedBox(height: 6),
//           Text(
//             label,
//             style: TextStyle(
//               fontSize: 12,
//               fontWeight: FontWeight.w600,
//               letterSpacing: 0.5,
//               color: effectiveColor,
//             ),
//           ),
//         ],
//       ),
//     );
//   }
// }

// class SettingsPage extends StatelessWidget {
//   final AppStateNotifier appState;
//   final String?          savedMac;
//   final String?          deviceId;
//   final String?          beaconUUID;
//   final bool             proximityEnabled;
//   final int              lastRSSI;
//   final String           proximityStatus;

//   final VoidCallback onScanAndConnect;
//   final VoidCallback onDisconnect;
//   final VoidCallback onPair;
//   final VoidCallback onUnpair;
//   final VoidCallback onStartProximity;
//   final VoidCallback onStopProximity;

//   const SettingsPage({
//     super.key,
//     required this.appState,
//     required this.savedMac,
//     required this.deviceId,
//     required this.beaconUUID,
//     required this.proximityEnabled,
//     required this.lastRSSI,
//     required this.proximityStatus,
//     required this.onScanAndConnect,
//     required this.onDisconnect,
//     required this.onPair,
//     required this.onUnpair,
//     required this.onStartProximity,
//     required this.onStopProximity,
//   });

//   @override
//   Widget build(BuildContext context) {
//     return Scaffold(
//       appBar: AppBar(
//         title: const Text('Settings'),
//         backgroundColor: Theme.of(context).colorScheme.surface,
//         elevation: 0,
//       ),
//       body: ListenableBuilder(
//         listenable: appState,
//         builder: (context, _) {
//           final connected = appState.connected;
//           final scanning  = appState.scanning;
//           final paired    = appState.paired;
//           final status    = appState.status;

//           return ListView(
//             children: [

//               // ── Connection ────────────────────────────────────────────
//               _SettingsSection(
//                 title: "Connection",
//                 children: [
//                   _InfoTile(
//                     icon: connected
//                         ? Icons.bluetooth_connected
//                         : Icons.bluetooth_disabled,
//                     iconColor: connected ? Colors.green : Colors.grey,
//                     title: connected ? "Connected to Chelonian" : "Not connected",
//                     subtitle: savedMac,
//                   ),
//                   // Status text
//                   Padding(
//                     padding: const EdgeInsets.fromLTRB(16, 0, 16, 4),
//                     child: Text(
//                       status,
//                       style: TextStyle(fontSize: 12, color: Colors.grey.shade500),
//                     ),
//                   ),
//                   Padding(
//                     padding: const EdgeInsets.fromLTRB(16, 4, 16, 12),
//                     child: connected
//                         ? OutlinedButton.icon(
//                             onPressed: onDisconnect,
//                             icon: const Icon(Icons.bluetooth_disabled),
//                             label: const Text("Disconnect"),
//                           )
//                         : ElevatedButton.icon(
//                             onPressed: scanning ? null : onScanAndConnect,
//                             icon: scanning
//                                 ? const SizedBox(
//                                     width: 18, height: 18,
//                                     child: CircularProgressIndicator(strokeWidth: 2),
//                                   )
//                                 : const Icon(Icons.bluetooth_searching),
//                             label: Text(scanning ? "Scanning..." : "Connect"),
//                             style: ElevatedButton.styleFrom(
//                               backgroundColor: Colors.green,
//                               foregroundColor: Colors.white,
//                             ),
//                           ),
//                   ),
//                 ],
//               ),

//               // ── Pairing ───────────────────────────────────────────────
//               _SettingsSection(
//                 title: "Pairing",
//                 children: [
//                   _InfoTile(
//                     icon: paired ? Icons.link : Icons.link_off,
//                     iconColor: paired ? Colors.green : Colors.orange,
//                     title: paired ? "Device paired" : "Not paired",
//                     subtitle: paired && deviceId != null
//                         ? "ID: ${deviceId!.substring(0, 8)}..."
//                         : connected
//                             ? "Tap below to pair"
//                             : "Connect first, then pair",
//                   ),
//                   Padding(
//                     padding: const EdgeInsets.fromLTRB(16, 4, 16, 12),
//                     child: paired
//                         ? OutlinedButton.icon(
//                             onPressed: onUnpair,
//                             icon: const Icon(Icons.link_off, color: Colors.red),
//                             label: const Text("Unpair",
//                                 style: TextStyle(color: Colors.red)),
//                             style: OutlinedButton.styleFrom(
//                               side: const BorderSide(color: Colors.red),
//                             ),
//                           )
//                         : ElevatedButton.icon(
//                             onPressed: connected ? onPair : null,
//                             icon: const Icon(Icons.link),
//                             label: const Text("Pair Device"),
//                             style: ElevatedButton.styleFrom(
//                               backgroundColor: Colors.orange,
//                               foregroundColor: Colors.white,
//                             ),
//                           ),
//                   ),
//                 ],
//               ),

//               // ── Proximity ─────────────────────────────────────────────
//               _SettingsSection(
//                 title: "Proximity Unlock",
//                 children: [
//                   SwitchListTile(
//                     secondary: const Icon(Icons.sensors),
//                     title: const Text("Auto-unlock when nearby"),
//                     subtitle: Text(proximityStatus),
//                     value: proximityEnabled,
//                     onChanged: paired
//                         ? (v) => v ? onStartProximity() : onStopProximity()
//                         : null,
//                   ),
//                   if (proximityEnabled)
//                     Padding(
//                       padding: const EdgeInsets.fromLTRB(16, 0, 16, 14),
//                       child: Row(
//                         children: [
//                           SignalStrengthIcon(rssi: lastRSSI, active: true, size: 40),
//                           const SizedBox(width: 16),
//                           Column(
//                             crossAxisAlignment: CrossAxisAlignment.start,
//                             children: [
//                               Text(
//                                 "${rssiToBars(lastRSSI)} / 4 bars",
//                                 style: const TextStyle(
//                                     fontWeight: FontWeight.w600, fontSize: 15),
//                               ),
//                               Text(
//                                 "RSSI: $lastRSSI dBm",
//                                 style: TextStyle(
//                                     fontSize: 12, color: Colors.grey.shade600),
//                               ),
//                             ],
//                           ),
//                         ],
//                       ),
//                     ),
//                 ],
//               ),

//               // ── Debug ─────────────────────────────────────────────────
//               _SettingsSection(
//                 title: "Debug Info",
//                 children: [
//                   _DebugTile(label: "Status",      value: status),
//                   _DebugTile(label: "Device ID",   value: deviceId ?? "—"),
//                   _DebugTile(label: "MAC",         value: savedMac ?? "—"),
//                   _DebugTile(label: "Beacon UUID", value: beaconUUID ?? "—"),
//                   _DebugTile(label: "RSSI",        value: "$lastRSSI dBm"),
//                   const SizedBox(height: 4),
//                 ],
//               ),

//               const SizedBox(height: 40),
//             ],
//           );
//         },
//       ),
//     );
//   }
// }

// // ─────────────────────────────────────────────
// // Settings helpers
// // ─────────────────────────────────────────────

// class _SettingsSection extends StatelessWidget {
//   final String       title;
//   final List<Widget> children;

//   const _SettingsSection({required this.title, required this.children});

//   @override
//   Widget build(BuildContext context) {
//     return Column(
//       crossAxisAlignment: CrossAxisAlignment.start,
//       children: [
//         Padding(
//           padding: const EdgeInsets.fromLTRB(16, 20, 16, 6),
//           child: Text(
//             title.toUpperCase(),
//             style: TextStyle(
//               fontSize: 11,
//               fontWeight: FontWeight.w700,
//               letterSpacing: 1.2,
//               color: Colors.grey.shade500,
//             ),
//           ),
//         ),
//         Container(
//           margin: const EdgeInsets.symmetric(horizontal: 16),
//           decoration: BoxDecoration(
//             color: Theme.of(context)
//                 .colorScheme
//                 .surfaceContainerHighest
//                 .withValues(alpha: 0.45),
//             borderRadius: BorderRadius.circular(12),
//           ),
//           child: Column(children: children),
//         ),
//       ],
//     );
//   }
// }

// class _InfoTile extends StatelessWidget {
//   final IconData icon;
//   final Color    iconColor;
//   final String   title;
//   final String?  subtitle;

//   const _InfoTile({
//     required this.icon,
//     required this.iconColor,
//     required this.title,
//     this.subtitle,
//   });

//   @override
//   Widget build(BuildContext context) {
//     return ListTile(
//       leading: Icon(icon, color: iconColor),
//       title: Text(title, style: const TextStyle(fontWeight: FontWeight.w500)),
//       subtitle: subtitle != null ? Text(subtitle!) : null,
//     );
//   }
// }

// class _DebugTile extends StatelessWidget {
//   final String label;
//   final String value;

//   const _DebugTile({required this.label, required this.value});

//   @override
//   Widget build(BuildContext context) {
//     return Padding(
//       padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 7),
//       child: Row(
//         crossAxisAlignment: CrossAxisAlignment.start,
//         children: [
//           SizedBox(
//             width: 96,
//             child: Text(label,
//                 style: TextStyle(fontSize: 12, color: Colors.grey.shade500)),
//           ),
//           Expanded(
//             child: Text(
//               value,
//               style: const TextStyle(
//                 fontSize: 12,
//                 fontFamily: 'monospace',
//                 fontWeight: FontWeight.w500,
//               ),
//               overflow: TextOverflow.ellipsis,
//             ),
//           ),
//         ],
//       ),
//     );
//   }
// }


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
const String MACRO_GET_UUID  = "beb54844-36e1-4688-b7f5-ea07361b26a8";
const String MACRO_SET_UUID  = "beb54845-36e1-4688-b7f5-ea07361b26a8";

const int RSSI_UNLOCK_THRESHOLD = -70;  // auto-unlock when close
const int RSSI_LOCK_THRESHOLD   = -85;    // auto-lock when nearby
const int RSSI_LEAVE_THRESHOLD  = -95;    // must leave this range to reset auto-unlock

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
  BluetoothCharacteristic? _macroGetChar;
  BluetoothCharacteristic? _macroSetChar;

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
  bool   _wasOutOfProximity = true;  // true initially so first approach auto-unlocks, must leave and return after lock

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
            if (u == MACRO_GET_UUID)   _macroGetChar             = c;
            if (u == MACRO_SET_UUID)   _macroSetChar             = c;
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
      _macroGetChar = null; _macroSetChar = null;
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

  // ── Macro Config ────────────────────────────────────────────────────────

  Future<Map<String, dynamic>?> readMacros() async {
    if (_macroGetChar == null) return null;
    try {
      await _macroGetChar!.read();
      final value = _macroGetChar!.lastValue;
      if (value != null && value.isNotEmpty) {
        final jsonStr = utf8.decode(value).trim();
        // Parse simple JSON format
        return _parseMacroJson(jsonStr);
      }
    } catch (e) {
      debugPrint("Failed to read macros: $e");
    }
    return null;
  }

  Map<String, dynamic> _parseMacroJson(String json) {
    // Simple JSON parser for macro config
    final result = <String, dynamic>{};
    result['macro_count'] = 0;
    result['tag_macro'] = 0;
    result['macros'] = [];
    
    // Extract macro_count
    final countMatch = RegExp(r'"macro_count":(\d+)').firstMatch(json);
    if (countMatch != null) {
      result['macro_count'] = int.parse(countMatch.group(1)!);
    }
    
    // Extract tag_macro
    final tagMatch = RegExp(r'"tag_macro":(\d+)').firstMatch(json);
    if (tagMatch != null) {
      result['tag_macro'] = int.parse(tagMatch.group(1)!);
    }
    
    // Extract macros array
    final macrosMatch = RegExp(r'"macros":\s*\[(.*)\]', dotAll: true).firstMatch(json);
    if (macrosMatch != null) {
      final macrosStr = macrosMatch.group(1)!;
      // Find each macro object
      final macroRegex = RegExp(r'\{[^}]+\}', dotAll: true);
      for (final match in macroRegex.allMatches(macrosStr)) {
        final macroStr = match.group(0)!;
        final macro = <String, dynamic>{};
        
        final nameMatch = RegExp(r'"name":\s*"([^"]*)"').firstMatch(macroStr);
        if (nameMatch != null) macro['name'] = nameMatch.group(1);
        
        final stepsMatch = RegExp(r'"steps":\s*\[(.*)\]', dotAll: true).firstMatch(macroStr);
        if (stepsMatch != null) {
          macro['steps'] = _parseSteps(stepsMatch.group(1)!);
        }
        
        if (macro.isNotEmpty) {
          (result['macros'] as List).add(macro);
        }
      }
    }
    
    return result;
  }

  List<Map<String, dynamic>> _parseSteps(String stepsStr) {
    final steps = <Map<String, dynamic>>[];
    final stepRegex = RegExp(r'\{[^}]+\}', dotAll: true);
    for (final match in stepRegex.allMatches(stepsStr)) {
      final stepStr = match.group(0)!;
      final step = <String, dynamic>{};
      
      final relayMatch = RegExp(r'"relay_mask":(\d+)').firstMatch(stepStr);
      if (relayMatch != null) step['relay_mask'] = int.parse(relayMatch.group(1)!);
      
      final durMatch = RegExp(r'"duration":(\d+)').firstMatch(stepStr);
      if (durMatch != null) step['duration'] = int.parse(durMatch.group(1)!);
      
      final gapMatch = RegExp(r'"gap":(\d+)').firstMatch(stepStr);
      if (gapMatch != null) step['gap'] = int.parse(gapMatch.group(1)!);
      
      if (step.isNotEmpty) steps.add(step);
    }
    return steps;
  }

  Future<bool> writeMacros(int macroCount, int tagMacro, List<Map<String, dynamic>> macros) async {
    if (_macroSetChar == null) return false;
    try {
      // Format: macro_count|tag_macro|macro1_name|steps|relay|duration|gap|...
      final buffer = StringBuffer();
      buffer.write('$macroCount|$tagMacro|');
      for (final macro in macros) {
        buffer.write('${macro['name']}|');
        final steps = macro['steps'] as List;
        buffer.write('${steps.length}|');
        for (final step in steps) {
          buffer.write('${step['relay_mask']}|');
          buffer.write('${step['duration']}|');
          buffer.write('${step['gap']}|');
        }
      }
      await _macroSetChar!.write(utf8.encode(buffer.toString()), withoutResponse: false);
      return true;
    } catch (e) {
      debugPrint("Failed to write macros: $e");
      return false;
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
          final wasInRange = _lastRSSI > RSSI_LEAVE_THRESHOLD;
          final isInRange = rssi > RSSI_LEAVE_THRESHOLD;
          final isInProximity = rssi > RSSI_LOCK_THRESHOLD;
          final isNearProximity = rssi > RSSI_UNLOCK_THRESHOLD;
          
          // Track when we left the leave threshold zone
          if (wasInRange && !isInRange) {
            _wasOutOfProximity = true;
          }
          
          setState(() { _lastRSSI = rssi; _proximityStatus = "Signal: $rssi dBm"; });
          
          // Auto-unlock: only if we left and returned to close proximity
          if (isNearProximity && !_isUnlocked && _wasOutOfProximity) {
            _sendCommand(1);
            _wasOutOfProximity = false;  // reset after triggering
          }
          
          // Auto-lock: if out of proximity (but still in BLE range)
          if (!isInProximity && _isUnlocked) {
            _sendCommand(2);
            // Don't reset _wasOutOfProximity here - user still needs to leave fully
          }
        } else {
          // Beacon lost entirely - user definitely left
          _wasOutOfProximity = true;
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
        onReadMacros:     readMacros,
        onWriteMacros:    writeMacros,
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
  final Future<Map<String, dynamic>?> Function() onReadMacros;
  final Future<bool> Function(int, int, List<Map<String, dynamic>>) onWriteMacros;

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
    required this.onReadMacros,
    required this.onWriteMacros,
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

              // ── Configuration ──────────────────────────────────────────
              _SettingsSection(
                title: "Configuration",
                children: [
                  ListTile(
                    leading: const Icon(Icons.tune),
                    title: const Text("Relay Macros"),
                    subtitle: const Text("Configure unlock/lock/trunk/panic sequences"),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () {
                      Navigator.push(
                        context,
                        MaterialPageRoute(
                          builder: (context) => MacroConfigPage(
                            onReadMacros: onReadMacros,
                            onWriteMacros: onWriteMacros,
                          ),
                        ),
                      );
                    },
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
          ],
        ],
      ),
    );
  }
}

// ─────────────────────────────────────────────
// Macro Configuration Page
// ─────────────────────────────────────────────
class MacroConfigPage extends StatefulWidget {
  final Function() onReadMacros;
  final Function(int, int, List<Map<String, dynamic>>) onWriteMacros;

  const MacroConfigPage({
    super.key,
    required this.onReadMacros,
    required this.onWriteMacros,
  });

  @override
  State<MacroConfigPage> createState() => _MacroConfigPageState();
}

class _MacroConfigPageState extends State<MacroConfigPage> {
  List<Map<String, dynamic>> _macros = [];
  int _tagMacro = 0;
  bool _useMs = false;
  bool _loading = true;
  bool _saving = false;

  @override
  void initState() {
    super.initState();
    _loadMacros();
  }

  Future<void> _loadMacros() async {
    setState(() => _loading = true);
    final data = await widget.onReadMacros();
    if (data != null && mounted) {
      setState(() {
        _macros = List<Map<String, dynamic>>.from(data['macros'] ?? []);
        _tagMacro = data['tag_macro'] ?? 0;
        _loading = false;
      });
    } else if (mounted) {
      setState(() => _loading = false);
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Failed to load macros'), backgroundColor: Colors.red),
      );
    }
  }

  Future<void> _saveMacros() async {
    setState(() => _saving = true);
    final success = await widget.onWriteMacros(_macros.length, _tagMacro, _macros);
    if (mounted) {
      setState(() => _saving = false);
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(success ? 'Macros saved!' : 'Failed to save macros'),
          backgroundColor: success ? Colors.green : Colors.red,
        ),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Macro Config'),
        backgroundColor: Theme.of(context).colorScheme.surface,
        actions: [
          if (!_loading)
            IconButton(
              icon: const Icon(Icons.refresh),
              onPressed: _loadMacros,
            ),
        ],
      ),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : Column(
              children: [
                // Settings row
                Padding(
                  padding: const EdgeInsets.all(16),
                  child: Row(
                    children: [
                      Expanded(
                        child: Row(
                          children: [
                            Checkbox(
                              value: _useMs,
                              onChanged: (v) => setState(() => _useMs = v ?? false),
                            ),
                            const Text('Show ms'),
                          ],
                        ),
                      ),
                      Expanded(
                        child: DropdownButtonFormField<int>(
                          value: _tagMacro < _macros.length ? _tagMacro : 0,
                          decoration: const InputDecoration(
                            labelText: 'Tag fires:',
                            border: OutlineInputBorder(),
                            contentPadding: EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                          ),
                          items: _macros.asMap().entries.map((e) {
                            return DropdownMenuItem(
                              value: e.key,
                              child: Text(e.value['name'] ?? 'Macro ${e.key + 1}'),
                            );
                          }).toList(),
                          onChanged: (v) => setState(() => _tagMacro = v ?? 0),
                        ),
                      ),
                    ],
                  ),
                ),
                // Macro list
                Expanded(
                  child: ListView.builder(
                    padding: const EdgeInsets.symmetric(horizontal: 16),
                    itemCount: _macros.length,
                    itemBuilder: (context, i) => _MacroCard(
                      macro: _macros[i],
                      index: i,
                      useMs: _useMs,
                      onChanged: (updated) {
                        setState(() => _macros[i] = updated);
                      },
                      onRemove: _macros.length > 1
                          ? () => setState(() {
                                _macros.removeAt(i);
                                if (_tagMacro >= _macros.length) _tagMacro = 0;
                              })
                          : null,
                    ),
                  ),
                ),
                // Add macro button
                Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    children: [
                      if (_macros.length < 6)
                        OutlinedButton.icon(
                          onPressed: () => setState(() {
                            _macros.add({'name': '', 'steps': [{'relay_mask': 0, 'duration': 1000, 'gap': 0}]});
                          }),
                          icon: const Icon(Icons.add),
                          label: const Text('Add Macro'),
                        ),
                      const SizedBox(height: 8),
                      SizedBox(
                        width: double.infinity,
                        child: ElevatedButton(
                          onPressed: _saving ? null : _saveMacros,
                          style: ElevatedButton.styleFrom(
                            backgroundColor: Colors.green,
                            foregroundColor: Colors.white,
                            padding: const EdgeInsets.symmetric(vertical: 16),
                          ),
                          child: _saving
                              ? const SizedBox(
                                  width: 20,
                                  height: 20,
                                  child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white),
                                )
                              : const Text('Save Changes'),
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
    );
  }
}

class _MacroCard extends StatelessWidget {
  final Map<String, dynamic> macro;
  final int index;
  final bool useMs;
  final Function(Map<String, dynamic>) onChanged;
  final VoidCallback? onRemove;

  const _MacroCard({
    required this.macro,
    required this.index,
    required this.useMs,
    required this.onChanged,
    this.onRemove,
  });

  @override
  Widget build(BuildContext context) {
    final steps = macro['steps'] as List;
    
    return Card(
      margin: const EdgeInsets.only(bottom: 16),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                  child: TextFormField(
                    initialValue: macro['name'] ?? '',
                    decoration: const InputDecoration(
                      labelText: 'Macro Name',
                      border: OutlineInputBorder(),
                    ),
                    onChanged: (v) {
                      macro['name'] = v;
                      onChanged(macro);
                    },
                  ),
                ),
                if (onRemove != null) ...[
                  const SizedBox(width: 8),
                  IconButton(
                    icon: const Icon(Icons.delete, color: Colors.red),
                    onPressed: onRemove,
                  ),
                ],
              ],
            ),
            const SizedBox(height: 16),
            Text('Steps', style: Theme.of(context).textTheme.titleSmall),
            const SizedBox(height: 8),
            ...steps.asMap().entries.map((e) => _StepEditor(
              step: e.value,
              stepIndex: e.key,
              isLast: e.key == steps.length - 1,
              useMs: useMs,
              onChanged: (updated) {
                steps[e.key] = updated;
                onChanged(macro);
              },
              onRemove: steps.length > 1
                  ? () {
                      steps.removeAt(e.key);
                      onChanged(macro);
                    }
                  : null,
            )),
            if (steps.length < 4)
              TextButton.icon(
                onPressed: () {
                  steps.add({'relay_mask': 0, 'duration': 1000, 'gap': 0});
                  onChanged(macro);
                },
                icon: const Icon(Icons.add, size: 18),
                label: const Text('Add Step'),
              ),
          ],
        ),
      ),
    );
  }
}

class _StepEditor extends StatelessWidget {
  final Map<String, dynamic> step;
  final int stepIndex;
  final bool isLast;
  final bool useMs;
  final Function(Map<String, dynamic>) onChanged;
  final VoidCallback? onRemove;

  const _StepEditor({
    required this.step,
    required this.stepIndex,
    required this.isLast,
    required this.useMs,
    required this.onChanged,
    this.onRemove,
  });

  @override
  Widget build(BuildContext context) {
    final relayMask = step['relay_mask'] as int? ?? 0;
    final duration = step['duration'] as int? ?? 1000;
    final gap = step['gap'] as int? ?? 0;

    return Container(
      margin: const EdgeInsets.only(bottom: 12),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: Colors.grey.withValues(alpha: 0.1),
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Text('Step ${stepIndex + 1}', style: const TextStyle(fontWeight: FontWeight.bold)),
              const Spacer(),
              if (onRemove != null)
                IconButton(
                  icon: const Icon(Icons.close, size: 18),
                  onPressed: onRemove,
                  padding: EdgeInsets.zero,
                  constraints: const BoxConstraints(),
                ),
            ],
          ),
          const SizedBox(height: 8),
          const Text('Relays:', style: TextStyle(fontSize: 12)),
          Row(
            children: List.generate(4, (i) {
              final bit = 1 << i;
              return Padding(
                padding: const EdgeInsets.only(right: 16),
                child: Row(
                  children: [
                    Checkbox(
                      value: (relayMask & bit) != 0,
                      onChanged: (v) {
                        step['relay_mask'] = v == true ? (relayMask | bit) : (relayMask & ~bit);
                        onChanged(step);
                      },
                    ),
                    Text('R${i + 1}'),
                  ],
                ),
              );
            }),
          ),
          const SizedBox(height: 8),
          Row(
            children: [
              Expanded(
                child: TextFormField(
                  initialValue: useMs ? '$duration' : '${duration / 1000}',
                  decoration: InputDecoration(
                    labelText: 'Duration (${useMs ? 'ms' : 's'})',
                    border: const OutlineInputBorder(),
                    contentPadding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                  ),
                  keyboardType: TextInputType.number,
                  onChanged: (v) {
                    step['duration'] = useMs ? int.tryParse(v) ?? 1000 : ((double.tryParse(v) ?? 1) * 1000).round();
                    onChanged(step);
                  },
                ),
              ),
              if (!isLast) ...[
                const SizedBox(width: 12),
                Expanded(
                  child: TextFormField(
                    initialValue: useMs ? '$gap' : '${gap / 1000}',
                    decoration: InputDecoration(
                      labelText: 'Gap (${useMs ? 'ms' : 's'})',
                      border: const OutlineInputBorder(),
                      contentPadding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
                    ),
                    keyboardType: TextInputType.number,
                    onChanged: (v) {
                      step['gap'] = useMs ? int.tryParse(v) ?? 0 : ((double.tryParse(v) ?? 0) * 1000).round();
                      onChanged(step);
                    },
                  ),
                ),
              ],
            ],
          ),
        ],
      ),
    );
  }
}
}
