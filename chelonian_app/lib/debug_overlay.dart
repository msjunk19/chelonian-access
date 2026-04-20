import 'package:flutter/material.dart';
import 'dart:convert';

/// Global debug logger
class DebugLogger {
  static final DebugLogger _instance = DebugLogger._internal();
  final List<String> _logs = [];
  final ValueNotifier<List<String>> logsNotifier = ValueNotifier([]);

  factory DebugLogger() {
    return _instance;
  }

  DebugLogger._internal();

  void log(String message) {
    final timestamp = DateTime.now().toIso8601String().split('T')[1].substring(0, 8);
    final formatted = '[$timestamp] $message';
    _logs.add(formatted);
    // Keep only last 50 logs
    if (_logs.length > 50) _logs.removeAt(0);
    logsNotifier.value = List.from(_logs);
    print(formatted); // Still prints to console if available
  }

  void clear() {
    _logs.clear();
    logsNotifier.value = [];
  }

  List<String> getLogs() => List.from(_logs);
}

/// Overlay button + drawer
class DebugOverlay extends StatefulWidget {
  final Widget child;

  const DebugOverlay({required this.child});

  @override
  State<DebugOverlay> createState() => _DebugOverlayState();
}

class _DebugOverlayState extends State<DebugOverlay> {
  bool _showDebug = false;

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        widget.child,
        Positioned(
          bottom: 20,
          right: 20,
          child: FloatingActionButton(
            mini: true,
            backgroundColor: Colors.black87,
            onPressed: () => setState(() => _showDebug = !_showDebug),
            child: const Icon(Icons.bug_report, color: Colors.yellow),
          ),
        ),
        if (_showDebug)
          Positioned(
            bottom: 80,
            right: 20,
            width: 300,
            height: 400,
            child: _DebugDrawer(),
          ),
      ],
    );
  }
}

class _DebugDrawer extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Material(
      color: Colors.black87,
      borderRadius: BorderRadius.circular(8),
      child: Column(
        children: [
          Container(
            color: Colors.yellow.shade700,
            padding: const EdgeInsets.all(8),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                const Text('Debug Logs', style: TextStyle(color: Colors.black, fontWeight: FontWeight.bold)),
                IconButton(
                  icon: const Icon(Icons.close, color: Colors.black, size: 18),
                  onPressed: () => Navigator.pop(context),
                  padding: EdgeInsets.zero,
                  constraints: const BoxConstraints(),
                ),
              ],
            ),
          ),
          Expanded(
            child: ValueListenableBuilder<List<String>>(
              valueListenable: DebugLogger().logsNotifier,
              builder: (context, logs, _) {
                return SingleChildScrollView(
                  reverse: true,
                  child: Padding(
                    padding: const EdgeInsets.all(8),
                    child: Text(
                      logs.join('\n'),
                      style: const TextStyle(
                        color: Colors.green,
                        fontFamily: 'monospace',
                        fontSize: 10,
                      ),
                    ),
                  ),
                );
              },
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(8),
            child: ElevatedButton(
              onPressed: () => DebugLogger().clear(),
              style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
              child: const Text('Clear'),
            ),
          ),
        ],
      ),
    );
  }
}