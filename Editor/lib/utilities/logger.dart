import 'package:editor/common/list_notifier.dart';
import 'package:logger/logger.dart';

enum LogLevel { info, warning, error }

class LogMessage {
  final DateTime time;
  final LogLevel level;
  final String trace;
  final String message;
  final int line;

  LogMessage({
    required this.time,
    required this.level,
    required this.trace,
    required this.message,
    required this.line,
  });

  @override
  String toString() {
    final String time =
        "${this.time.hour}:${this.time.minute}:${this.time.second}";
    return "$time [${level.name.toUpperCase()}] $trace($line): $message";
  }
}

class EditorLogger {
  static final EditorLogger _instance = EditorLogger._internal();

  final List<LogMessage> _logs = [];
  final ListNotifier<LogMessage> _filteredLogs = ListNotifier();
  final List<LogLevel> _levels = [
    LogLevel.error,
    LogLevel.warning,
    LogLevel.info,
  ];

  factory EditorLogger() {
    return _instance;
  }

  EditorLogger._internal();

  List<LogMessage> get logs => _logs;
  ListNotifier<LogMessage> get logsNotifier => _filteredLogs;
  List<LogMessage> get filteredLogs => _filteredLogs.value;
  List<LogLevel> get levels => _levels;

  void log(LogLevel level, String message, {required StackTrace trace}) {
    final String frame = trace.toString().split('\n')[0];
    final RegExpMatch? match =
        RegExp(r'#\d+\s+(.+)\s+\((.*):(\d+):\d+\)').firstMatch(frame);

    if (match != null) {
      final String traceString = match.group(1) ?? "Unknown";
      final String fileName = match.group(2) ?? "Unknown";
      final int line = int.tryParse(match.group(3) ?? "") ?? 0;

      final LogMessage logMessage = LogMessage(
        time: DateTime.now(),
        level: level,
        trace: "$fileName [func]$traceString",
        message: message,
        line: line,
      );

      _logs.add(logMessage);

      if (_levels.contains(level)) {
        _filteredLogs.add(logMessage);
      }
    }
  }

  void filterLogs(List<LogLevel>? levels) {
    levels ??= [
      LogLevel.error,
      LogLevel.warning,
      LogLevel.info,
    ];

    _levels.clear();
    _levels.addAll(levels);

    _filteredLogs.clear(notify: false);
    _filteredLogs
        .addAll(_logs.where((log) => _levels.contains(log.level)).toList());
  }

  void clear() {
    _logs.clear();
    _filteredLogs.clear();
  }
}

final Logger debugLogger = Logger(
  printer: LogfmtPrinter(),
);
