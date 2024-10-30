import 'package:editor/editors/world_editor/controllers/world_editor_controller.dart';
import 'package:editor/themes/themes.dart';
import 'package:editor/utilities/logger.dart';
import 'package:flutter/material.dart';
import 'package:intl/intl.dart';

class Console extends StatefulWidget {
  const Console({super.key});

  @override
  State<Console> createState() => _ConsoleState();
}

class _ConsoleState extends State<Console> {
  final _controller = WorldEditorController();

  bool _isErrorActive = true;
  bool _isWarningActive = true;
  bool _isInfoActive = true;

  Color _getLogColor(LogLevel level) {
    switch (level) {
      case LogLevel.error:
        return Colors.red;
      case LogLevel.warning:
        return Colors.orange;
      case LogLevel.info:
        return Colors.blue;
    }
  }

  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: Column(
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.end,
            children: [
              MouseRegion(
                cursor: SystemMouseCursors.click,
                child: GestureDetector(
                  onTap: (_controller.clearLogs),
                  child: Tooltip(
                    message: "Clear all messages",
                    child: Text(
                      "clear",
                      style: TextStyle(color: Theme.of(context).primaryColor),
                    ),
                  ),
                ),
              ),
              const SizedBox(width: 15),
              Tooltip(
                message: "Filter errors",
                child: OutlinedButton(
                  style: Theme.of(context).smallIcon,
                  onPressed: () {
                    _controller.filterLogs(LogLevel.error);
                    setState(() {
                      _isErrorActive = !_isErrorActive;
                    });
                  },
                  child: Icon(
                    Icons.error_outline,
                    color: _isErrorActive ? Colors.red : Colors.grey,
                  ),
                ),
              ),
              Tooltip(
                message: "Filter warnings",
                child: OutlinedButton(
                  style: Theme.of(context).smallIcon,
                  onPressed: () {
                    _controller.filterLogs(LogLevel.warning);
                    setState(() {
                      _isWarningActive = !_isWarningActive;
                    });
                  },
                  child: Icon(
                    Icons.warning_amber_rounded,
                    color: _isWarningActive ? Colors.orange : Colors.grey,
                  ),
                ),
              ),
              Tooltip(
                message: "Filter infos",
                child: OutlinedButton(
                  style: Theme.of(context).smallIcon,
                  onPressed: () {
                    _controller.filterLogs(LogLevel.info);
                    setState(() {
                      _isInfoActive = !_isInfoActive;
                    });
                  },
                  child: Icon(
                    Icons.info_outline,
                    color: _isInfoActive ? Colors.blue : Colors.grey,
                  ),
                ),
              ),
            ],
          ),
          Expanded(
            child: ValueListenableBuilder(
              valueListenable: _controller.logger.logsNotifier,
              builder: (context, value, _) {
                return ListView.builder(
                  itemCount: _controller.logger.filteredLogs.length,
                  itemBuilder: (context, index) {
                    return Tooltip(
                      message: value[index].toString(),
                      child: Row(
                        children: [
                          Text(DateFormat('H:mm:ss').format(value[index].time)),
                          const SizedBox(width: 10),
                          Text.rich(
                            TextSpan(
                              children: <TextSpan>[
                                const TextSpan(text: "["),
                                TextSpan(
                                  text: value[index].level.name.toUpperCase(),
                                  style: TextStyle(
                                    color: _getLogColor(value[index].level),
                                  ),
                                ),
                                const TextSpan(text: "]"),
                              ],
                            ),
                          ),
                          const SizedBox(width: 10),
                          Text(value[index].message),
                        ],
                      ),
                    );
                  },
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
