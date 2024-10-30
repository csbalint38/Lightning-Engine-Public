import 'package:editor/editors/world_editor/controllers/world_editor_controller.dart';
import 'package:editor/utilities/undo_redo.dart';
import 'package:flutter/material.dart';

class History extends StatefulWidget {
  const History({super.key});

  @override
  State<History> createState() => _HistoryState();
}

class _HistoryState extends State<History> {
  final _controller = WorldEditorController();

  @override
  void initState() {
    super.initState();
  }

  @override
  void dispose() {
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: ValueListenableBuilder(
        valueListenable: UndoRedo().undoList,
        builder: (context, _, __) {
          return ListView.builder(
            itemCount: _controller.undoRedo.undoList.value.length +
                _controller.undoRedo.redoList.value.length,
            itemBuilder: (context, index) {
              if (index < _controller.undoRedo.undoList.value.length) {
                return Text(_controller.undoRedo.undoList.value[index].name);
              }
              final int idx =
                  index - _controller.undoRedo.undoList.value.length;
              return Text(
                _controller.undoRedo.redoList.value[idx].name,
                style: const TextStyle(color: Colors.black38),
              );
            },
          );
        },
      ),
    );
  }
}
