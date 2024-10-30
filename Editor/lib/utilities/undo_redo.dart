import 'package:editor/common/list_notifier.dart';

abstract class IUndoRedo {
  String get name;
  void undo();
  void redo();
}

class UndoRedoAction implements IUndoRedo {
  @override
  late final String name;
  late final Function undoAction;
  late final Function redoAction;

  UndoRedoAction(
      {required this.name, required this.undoAction, required this.redoAction});

  @override
  void undo() => undoAction();

  @override
  void redo() => redoAction();
}

class UndoRedo {
  static final UndoRedo _undoRedo = UndoRedo._internal();

  factory UndoRedo() {
    return _undoRedo;
  }

  UndoRedo._internal();

  final ListNotifier<IUndoRedo> redoList = ListNotifier<IUndoRedo>();
  final ListNotifier<IUndoRedo> undoList = ListNotifier<IUndoRedo>();

  void add(IUndoRedo cmd) {
    undoList.add(cmd);
    redoList.clear();
  }

  void undo() {
    if (undoList.value.isNotEmpty) {
      var cmd = undoList.value.last;
      undoList.remove(undoList.value.last);
      cmd.undo();
      redoList.insert(0, cmd);
    }
  }

  void redo() {
    if (redoList.value.isNotEmpty) {
      var cmd = redoList.value.first;
      redoList.removeAt(0);
      cmd.redo();
      undoList.add(cmd);
    }
  }

  void resset() {
    redoList.clear();
    undoList.clear();
  }
}
