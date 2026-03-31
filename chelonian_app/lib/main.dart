import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:uuid/uuid.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_beacon/flutter_beacon.dart';
import 'package:permission_handler/permission_handler.dart';
import 'dart:convert';
import 'dart:async';

const String SERVICE_UUID     = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const String CMD_UUID         = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const String STATUS_UUID      = "beb5483f-36e1-4688-b7f5-ea07361b26a8";
const String PAIR_UUID        = "beb54840-36e1-4688-b7f5-ea07361b26a8";
const String BEACON_UUID_CHAR = "beb54841-36e1-4688-b7f5-ea07361b26a8";
const String MAC_UUID_CHAR    = "beb54842-36e1-4688-b7f5-ea07361b26a8";

const int RSSI_UNLOCK_THRESHOLD = -70;
const int RSSI_LOCK_THRESHOLD   = -85;

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

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  // BLE state
  BluetoothDevice?         _device;
  BluetoothCharacteristic? _cmdChar;
  BluetoothCharacteristic? _statusChar;
  BluetoothCharacteristic? _pairChar;
  BluetoothCharacteristic? _beaconUUIDCharacteristic;
  BluetoothCharacteristic? _macCharacteristic;
  bool _connected = false;
  bool _scanning  = false;
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

  // UI
  String _status     = "Disconnected";
  String _lastAction = "";

  @override
  void initState() {
    super.initState();
    _loadPairing();
  }

  @override
  void dispose() {
    _beaconSub?.cancel();
    super.dispose();
  }

  // -------------------------
  // Storage

  Future<void> _loadPairing() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      _deviceId   = prefs.getString('device_id');
      _token      = prefs.getString('device_token');
      _beaconUUID = prefs.getString('beacon_uuid');
      _savedMac   = prefs.getString('device_mac');
      _paired     = _deviceId != null && _token != null;
    });

    if (_beaconUUID != null && _paired) {
      _startProximityMonitoring();
    }
  }

  Future<void> _savePairing(String deviceId, String token) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('device_id', deviceId);
    await prefs.setString('device_token', token);
    setState(() {
      _deviceId = deviceId;
      _token    = token;
      _paired   = true;
    });
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
      _deviceId   = null;
      _token      = null;
      _beaconUUID = null;
      _savedMac   = null;
      _paired     = false;
    });
  }

  // -------------------------
  // BLE scan and connect

  Future<void> _scanAndConnect() async {
    setState(() {
      _scanning = true;
      _status   = "Scanning...";
    });

    BluetoothAdapterState state = await FlutterBluePlus.adapterState.first;
    if (state != BluetoothAdapterState.on) {
      setState(() {
        _status   = "Bluetooth is off";
        _scanning = false;
      });
      return;
    }

    StreamSubscription? subscription;
    subscription = FlutterBluePlus.scanResults.listen((results) async {
      for (ScanResult r in results) {
        if (r.advertisementData.advName == "Chelonian") {
          await FlutterBluePlus.stopScan();
          subscription?.cancel();
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
    subscription.cancel();
    if (!_connected) {
      setState(() {
        _status   = "Device not found";
        _scanning = false;
      });
    }
  }

  Future<void> _connectToDevice(BluetoothDevice device) async {
    setState(() { _status = "Connecting..."; });

    try {
      await device.connect(license: License.free);
      // await device.requestMtu(128);
      _device = device;

      List<BluetoothService> services = await device.discoverServices();
      for (BluetoothService service in services) {
        if (service.uuid.toString() == SERVICE_UUID) {
          for (BluetoothCharacteristic c in service.characteristics) {
            String uuid = c.uuid.toString();
            if (uuid == CMD_UUID)         _cmdChar                 = c;
            if (uuid == STATUS_UUID)      _statusChar              = c;
            if (uuid == PAIR_UUID)        _pairChar                = c;
            if (uuid == BEACON_UUID_CHAR) _beaconUUIDCharacteristic = c;
            if (uuid == MAC_UUID_CHAR)    _macCharacteristic        = c;
          }
        }
      }

      if (_statusChar != null) {
        await _statusChar!.setNotifyValue(true);
        _statusChar!.onValueReceived.listen((value) {
          setState(() { _lastAction = utf8.decode(value); });
        });
      }

      device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.disconnected) {
          setState(() {
            _connected = false;
            _status    = "Disconnected";
          });
        }
      });

setState(() {
        _connected = true;
        _scanning  = false;
        _status    = "Connected to Chelonian";
      });

      // Auto-fetch device info if paired but missing beacon UUID
      if (_paired && _beaconUUID == null) {
        await _fetchDeviceInfo();
      }

    } catch (e) {
      setState(() {
        _status   = "Connection failed: $e";
        _scanning = false;
      });
    }
  }

  Future<void> _disconnect() async {
    await _device?.disconnect();
    setState(() {
      _connected                = false;
      _device                   = null;
      _cmdChar                  = null;
      _statusChar               = null;
      _pairChar                 = null;
      _beaconUUIDCharacteristic = null;
      _macCharacteristic        = null;
      _status                   = "Disconnected";
    });
  }

  // -------------------------
  // Pairing

  Future<void> _pair() async {
    if (_pairChar == null) {
      setState(() { _status = "Not connected"; });
      return;
    }

    final deviceId = const Uuid().v4();
    setState(() { _status = "Pairing..."; });

    try {
      await _pairChar!.write(utf8.encode(deviceId));
      List<int> response = await _pairChar!.read();
      String token = utf8.decode(response).trim();

      if (token.startsWith("error:")) {
        setState(() { _status = "Pairing failed: $token"; });
        return;
      }

      await _savePairing(deviceId, token);
      await _fetchDeviceInfo(); // auto-fetch beacon UUID and MAC
      setState(() { _status = "Paired successfully!"; });

    } catch (e) {
      setState(() { _status = "Pairing error: $e"; });
    }
  }

  Future<void> _fetchDeviceInfo() async {
    try {
      // Read and save beacon UUID
      if (_beaconUUIDCharacteristic != null) {
        List<int> value = await _beaconUUIDCharacteristic!.read();
        String uuid = utf8.decode(value).trim();
        await _saveBeaconUUID(uuid);
        debugPrint("Got beacon UUID: $uuid");
      }

      // Read and save device MAC
      if (_macCharacteristic != null) {
        List<int> value = await _macCharacteristic!.read();
        String mac = utf8.decode(value).trim();
        final prefs = await SharedPreferences.getInstance();
        await prefs.setString('device_mac', mac);
        setState(() { _savedMac = mac; });
        debugPrint("Got device MAC: $mac");
      }

      // Start proximity monitoring now that we have beacon UUID
      if (_beaconUUID != null) {
        _startProximityMonitoring();
      }

    } catch (e) {
      setState(() { _proximityStatus = "Could not fetch device info: $e"; });
    }
  }

  Future<void> _unpair() async {
    await _clearPairing();
    setState(() { _status = "Unpaired"; });
  }

  // -------------------------
  // Commands

  Future<void> _sendCommand(int command) async {
    if (!_connected || _cmdChar == null) {
      await _scanAndConnect();
      if (!_connected) {
        setState(() { _status = "Could not connect"; });
        return;
      }
    }

    if (!_paired || _deviceId == null || _token == null) {
      setState(() { _status = "Not paired"; });
      return;
    }

    // final payload = "${_deviceId!.trim()}|${_token!.trim()}|$command";
    final payload = "${_deviceId!.trim()}|${_token!.trim()}|$command".trim();
    try {
      // await _cmdChar!.write(utf8.encode(payload));
      await _cmdChar!.write(utf8.encode(payload), withoutResponse: false);

      setState(() {
        _status     = command == 1 ? "Unlock sent" : "Lock sent";
        _isUnlocked = command == 1;
      });
    } catch (e) {
      setState(() { _status = "Command failed: $e"; });
    }
  }

  // -------------------------
  // Proximity monitoring

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

      final regions = <Region>[
        Region(
          identifier: 'chelonian',
          proximityUUID: _beaconUUID!,
          major: 1,
          minor: 1,
        ),
      ];

      _beaconSub = flutterBeacon.ranging(regions).listen((result) {
        if (result.beacons.isNotEmpty) {
          final beacon = result.beacons.first;
          final rssi   = beacon.rssi;

          setState(() {
            _lastRSSI        = rssi;
            _proximityStatus = "Signal: $rssi dBm";
          });

          if (rssi > RSSI_UNLOCK_THRESHOLD && !_isUnlocked) {
            setState(() { _proximityStatus = "In range — unlocking"; });
            _sendCommand(1);
          }

          if (rssi < RSSI_LOCK_THRESHOLD && _isUnlocked) {
            setState(() { _proximityStatus = "Out of range — locking"; });
            _sendCommand(2);
          }
        } else {
          setState(() { _proximityStatus = "Beacon not detected"; });
          if (_isUnlocked) {
            _sendCommand(2);
          }
        }
      });

      setState(() {
        _proximityEnabled = true;
        _proximityStatus  = "Monitoring...";
      });

    } catch (e) {
      setState(() { _proximityStatus = "Beacon error: $e"; });
    }
  }

  void _stopProximityMonitoring() {
    _beaconSub?.cancel();
    _beaconSub = null;
    setState(() {
      _proximityEnabled = false;
      _proximityStatus  = "Proximity: Off";
    });
  }

  void _showBeaconUUIDDialog() {
    final controller = TextEditingController(text: _beaconUUID ?? '');
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Enter Beacon UUID'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            const Text(
              'Find this in your ESP32 serial log:\n[BLE] iBeacon UUID: ...',
              style: TextStyle(fontSize: 12, color: Colors.grey),
            ),
            const SizedBox(height: 8),
            TextField(
              controller: controller,
              decoration: const InputDecoration(
                hintText: '43484c4e-xxxx-xxxx-xxxx-xxxxxxxxxxxx',
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () async {
              await _saveBeaconUUID(controller.text.trim());
              Navigator.pop(ctx);
              _startProximityMonitoring();
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  // -------------------------
  // UI

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Chelonian'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        actions: [
IconButton(
    icon: Icon(
      _proximityEnabled ? Icons.sensors : Icons.sensors_off,
      color: _proximityEnabled ? Colors.green : Colors.grey,
    ),
    onPressed: _proximityEnabled
        ? _stopProximityMonitoring
        : (_beaconUUID != null
            ? _startProximityMonitoring
            : _showBeaconUUIDDialog),
    tooltip: 'Proximity unlock',
),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [

            // Status card
            Card(
              child: Padding(
                padding: const EdgeInsets.all(16.0),
                child: Column(
                  children: [
                    Icon(
                      _connected
                          ? Icons.bluetooth_connected
                          : Icons.bluetooth_disabled,
                      size: 48,
                      color: _connected ? Colors.green : Colors.grey,
                    ),
                    const SizedBox(height: 8),
                    Text(
  "paired=$_paired id=${_deviceId?.substring(0,4) ?? 'null'} token=${_token?.length ?? 0}",
  style: const TextStyle(fontSize: 11, color: Colors.grey),
),
                    Text(_status,
                        textAlign: TextAlign.center,
                        style: const TextStyle(fontSize: 16)),
                    if (_lastAction.isNotEmpty) ...[
                      const SizedBox(height: 4),
                      Text("Last: $_lastAction",
                          style: const TextStyle(
                              fontSize: 12, color: Colors.grey)),
                    ],
                    if (_proximityEnabled) ...[
                      const SizedBox(height: 4),
                      Text(_proximityStatus,
                          style: TextStyle(
                              fontSize: 12,
                              color: _lastRSSI > RSSI_UNLOCK_THRESHOLD
                                  ? Colors.green
                                  : Colors.orange)),
                    ],
                  ],
                ),
              ),
            ),

            const SizedBox(height: 24),

            // Connect / disconnect
            if (!_connected)
              ElevatedButton.icon(
                onPressed: _scanning ? null : _scanAndConnect,
                icon: _scanning
                    ? const SizedBox(
                        width: 20,
                        height: 20,
                        child: CircularProgressIndicator(strokeWidth: 2))
                    : const Icon(Icons.bluetooth_searching),
                label: Text(_scanning ? "Scanning..." : "Connect"),
                style: ElevatedButton.styleFrom(
                    padding: const EdgeInsets.all(16)),
              )
            else
              OutlinedButton.icon(
                onPressed: _disconnect,
                icon: const Icon(Icons.bluetooth_disabled),
                label: const Text("Disconnect"),
                style: OutlinedButton.styleFrom(
                    padding: const EdgeInsets.all(16)),
              ),

            const SizedBox(height: 16),

            // Pairing
            if (_connected && !_paired)
              ElevatedButton(
                onPressed: _pair,
                style: ElevatedButton.styleFrom(
                  padding: const EdgeInsets.all(16),
                  backgroundColor: Colors.orange,
                ),
                child: const Text("Pair Device",
                    style: TextStyle(color: Colors.white)),
              ),

            if (_paired) ...[
              const SizedBox(height: 8),
              Text(
                "Paired — ID: ${_deviceId!.substring(0, 8)}...",
                textAlign: TextAlign.center,
                style: const TextStyle(fontSize: 12, color: Colors.grey),
              ),
              
              TextButton(
                onPressed: _unpair,
                child: const Text("Unpair",
                    style: TextStyle(color: Colors.red)),
              ),
            ],

            const Spacer(),

            // Lock status indicator
            Center(
              child: Icon(
                _isUnlocked ? Icons.lock_open : Icons.lock,
                size: 64,
                color: _isUnlocked ? Colors.green : Colors.red,
              ),
            ),

            const SizedBox(height: 16),

            // Control buttons
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _paired ? () => _sendCommand(1) : null,
                    icon: const Icon(Icons.lock_open),
                    label: const Text("Unlock"),
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.all(20),
                      backgroundColor: Colors.green,
                      foregroundColor: Colors.white,
                    ),
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _paired ? () => _sendCommand(2) : null,
                    icon: const Icon(Icons.lock),
                    label: const Text("Lock"),
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.all(20),
                      backgroundColor: Colors.red,
                      foregroundColor: Colors.white,
                    ),
                  ),
                ),
              ],
            ),

            const SizedBox(height: 16),
          ],
        ),
      ),
    );
  }
}