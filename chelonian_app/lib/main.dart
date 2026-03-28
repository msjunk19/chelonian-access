import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:uuid/uuid.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:convert';
import 'dart:async';

// -------------------------
// BLE UUIDs — must match ESP32
const String SERVICE_UUID  = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const String CMD_UUID      = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const String STATUS_UUID   = "beb5483f-36e1-4688-b7f5-ea07361b26a8";
const String PAIR_UUID     = "beb54840-36e1-4688-b7f5-ea07361b26a8";

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

// -------------------------
// Home page

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  // BLE state
  BluetoothDevice?      _device;
  BluetoothCharacteristic? _cmdChar;
  BluetoothCharacteristic? _statusChar;
  BluetoothCharacteristic? _pairChar;
  bool _connected    = false;
  bool _scanning     = false;
  bool _paired       = false;

  // Auth state
  String? _deviceId;
  String? _token;

  // UI state
  String _status     = "Disconnected";
  String _lastAction = "";

  @override
  void initState() {
    super.initState();
    _loadPairing();
  }

  // -------------------------
  // Persistent storage

  Future<void> _loadPairing() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      _deviceId = prefs.getString('device_id');
      _token    = prefs.getString('device_token');
      _paired   = _deviceId != null && _token != null;
    });
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

  Future<void> _clearPairing() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove('device_id');
    await prefs.remove('device_token');
    setState(() {
      _deviceId = null;
      _token    = null;
      _paired   = false;
    });
  }

  // -------------------------
  // BLE scan and connect

  Future<void> _scanAndConnect() async {
    setState(() {
      _scanning = true;
      _status   = "Scanning...";
    });

    // Check BLE is on
    BluetoothAdapterState state = await FlutterBluePlus.adapterState.first;
    if (state != BluetoothAdapterState.on) {
      setState(() {
        _status   = "Bluetooth is off";
        _scanning = false;
      });
      return;
    }

    // Listen for scan results
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

    // Timeout handling
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
      await device.connectionState
          .where((s) => s == BluetoothConnectionState.connected)
          .first
          .timeout(const Duration(seconds: 10));
      _device = device;

      // Discover services
      List<BluetoothService> services = await device.discoverServices();
      for (BluetoothService service in services) {
        if (service.uuid.toString() == SERVICE_UUID) {
          for (BluetoothCharacteristic c in service.characteristics) {
            String uuid = c.uuid.toString();
            if (uuid == CMD_UUID)    _cmdChar    = c;
            if (uuid == STATUS_UUID) _statusChar = c;
            if (uuid == PAIR_UUID)   _pairChar   = c;
          }
        }
      }

      // Subscribe to status notifications
      if (_statusChar != null) {
        await _statusChar!.setNotifyValue(true);
        _statusChar!.onValueReceived.listen((value) {
          setState(() {
            _lastAction = utf8.decode(value);
          });
        });
      }

      // Listen for disconnection
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

    } catch (e) {
      setState(() {
        _status  = "Connection failed: $e";
        _scanning = false;
      });
    }
  }

  Future<void> _disconnect() async {
    await _device?.disconnect();
    setState(() {
      _connected = false;
      _device    = null;
      _cmdChar   = null;
      _statusChar = null;
      _pairChar  = null;
      _status    = "Disconnected";
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
      // Write device ID to pair characteristic
      await _pairChar!.write(utf8.encode(deviceId));

      // Read token back
      List<int> response = await _pairChar!.read();
      String token = utf8.decode(response);

      if (token.startsWith("error:")) {
        setState(() { _status = "Pairing failed: $token"; });
        return;
      }

      await _savePairing(deviceId, token);
      setState(() { _status = "Paired successfully!"; });

    } catch (e) {
      setState(() { _status = "Pairing error: $e"; });
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
      setState(() { _status = "Not connected"; });
      return;
    }

    if (!_paired || _deviceId == null || _token == null) {
      setState(() { _status = "Not paired"; });
      return;
    }

    // final payload = "$_deviceId|$_token|$command";
    final payload = "${_deviceId!.trim()}|${_token!.trim()}|$command";
    try {
      await _cmdChar!.write(utf8.encode(payload));
      setState(() {
        _status = command == 1 ? "Unlock sent" : "Lock sent";
      });
    } catch (e) {
      setState(() { _status = "Command failed: $e"; });
    }
  }

  // -------------------------
  // UI

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Chelonian'),
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
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
                      _connected ? Icons.bluetooth_connected
                                 : Icons.bluetooth_disabled,
                      size: 48,
                      color: _connected ? Colors.green : Colors.grey,
                    ),
                    const SizedBox(height: 8),
                    Text(_status,
                      textAlign: TextAlign.center,
                      style: const TextStyle(fontSize: 16)),
                    if (_lastAction.isNotEmpty) ...[
                      const SizedBox(height: 4),
                      Text("Last: $_lastAction",
                        style: const TextStyle(
                          fontSize: 12, color: Colors.grey)),
                    ]
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
                      width: 20, height: 20,
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
                  backgroundColor: Colors.orange),
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

            // Control buttons
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: (_connected && _paired)
                      ? () => _sendCommand(1) : null,
                    icon: const Icon(Icons.lock_open),
                    label: const Text("Unlock"),
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.all(20),
                      backgroundColor: Colors.green,
                      foregroundColor: Colors.white),
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: (_connected && _paired)
                      ? () => _sendCommand(2) : null,
                    icon: const Icon(Icons.lock),
                    label: const Text("Lock"),
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.all(20),
                      backgroundColor: Colors.red,
                      foregroundColor: Colors.white),
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