import 'package:editor/components/game_entity.dart';
import 'package:editor/common/relay_command.dart';
import 'package:editor/dll_wrappers/visual_studio.dart';
import 'package:editor/game_project/project.dart';
import 'package:editor/game_project/scene.dart';
import 'package:editor/utilities/logger.dart';
import 'package:editor/utilities/undo_redo.dart';
import 'package:flutter/foundation.dart';

class WorldEditorController {
  static final WorldEditorController _worldEditorController =
      WorldEditorController._internal();

  late final Project _project;
  final undoRedo = UndoRedo();

  late RelayCommand undoCommand;
  late RelayCommand redoCommand;
  late RelayCommand saveCommand;
  late RelayCommand renameMultipleCommand;
  late RelayCommand enableMultipleCommand;
  late RelayCommand buildCommand;

  final ValueNotifier<bool> canSave = ValueNotifier(false);
  final List<int> selectedEntityIndices = <int>[];
  final ValueNotifier<MSGameEntity?> msEntity = ValueNotifier(null);

  final _logger = EditorLogger();

  factory WorldEditorController() {
    return _worldEditorController;
  }

  WorldEditorController._internal() {
    _createCommands();
    _addListeners();
  }

  Project get project => _project;
  EditorLogger get logger => _logger;

  clearLogs() {
    _logger.clear();
  }

  filterLogs(LogLevel filter) {
    List<LogLevel> newFilter = List<LogLevel>.from(_logger.levels);
    _logger.levels.contains(filter)
        ? newFilter.remove(filter)
        : newFilter.add(filter);
    _logger.filterLogs(newFilter);
  }

  _canSaveChanged() {
    canSave.value = true;
  }

  void setProject(Project project) {
    _project = project;
  }

  ValueNotifier<List<Scene>> getScenes() {
    return _project.scenes;
  }

  Scene get getActiveScene => _project.activeScene;

  void addScene() {
    final sceneName = "Scene ${_project.scenes.value.length}";
    _project.addScene.execute(sceneName);
  }

  void removeScene(int index) {
    final Scene scene = _project.scenes.value[index];
    _project.removeScene.execute(scene);
  }

  void addGameEntity(int sceneIndex) {
    final GameEntity entity = GameEntity([], name: "Empty GameEntity");
    _project.scenes.value[sceneIndex].addGameEntity.execute(entity);
  }

  void removeGameEntity(int sceneIndex, GameEntity entity) {
    int index =
        _project.scenes.value[sceneIndex].entities.value.indexOf(entity);
    selectedEntityIndices.remove(index);
    _project.scenes.value[sceneIndex].removeGameEntity.execute(entity);
  }

  void _setMsEntity() {
    final List<GameEntity> selectedEntities = selectedEntityIndices
        .map((index) => _project.activeScene.entities.value[index])
        .toList();

    if (selectedEntities.isNotEmpty) {
      msEntity.value = MSGameEntity(selectedEntities);
    }
  }

  void setMsEntityName(String name) {
    renameMultipleCommand.execute(name);
  }

  void enableMsEntity(bool? isEnabled) {
    enableMultipleCommand.execute(isEnabled);
  }

  void save() {
    Project.save(_project);
    canSave.value = false;
  }

  // modifier:
  // * null - nothing
  // * 0    - Ctrl
  // * 1    - Shift
  void changeSelection(int index, bool? modifier) {
    List<int> prevSelection = List.from(selectedEntityIndices);

    // Ctrl
    if (modifier != null && !modifier) {
      if (selectedEntityIndices.contains(index)) {
        selectedEntityIndices.remove(index);
      } else {
        selectedEntityIndices.add(index);
      }
    }
    // Shift
    else if (modifier != null && modifier) {
      if (selectedEntityIndices.isEmpty) {
        selectedEntityIndices.add(index);
      } else {
        int a = selectedEntityIndices[0];
        int b = index;
        if (a > b) {
          a = a ^ b;
          b = a ^ b;
          a = a ^ b;
        }

        final newList = [for (int i = a; i <= b; i++) i];

        selectedEntityIndices.clear();
        selectedEntityIndices.addAll(newList);
      }
    }
    // No modifier
    else {
      selectedEntityIndices.clear();
      selectedEntityIndices.add(index);
    }
    List<int> currentSelection = List.from(selectedEntityIndices);

    if (!listEquals(selectedEntityIndices, prevSelection)) {
      undoRedo.add(
        UndoRedoAction(
          name: "Selection changed",
          undoAction: () {
            selectedEntityIndices.clear();
            selectedEntityIndices.addAll(prevSelection);
            _setMsEntity();
          },
          redoAction: () {
            selectedEntityIndices.clear();
            selectedEntityIndices.addAll(currentSelection);
            _setMsEntity();
          },
        ),
      );
    }

    _setMsEntity();
  }

  void _createCommands() {
    undoCommand = RelayCommand(
        (x) => undoRedo.undo(), (x) => undoRedo.undoList.value.isNotEmpty);
    redoCommand = RelayCommand(
        (x) => undoRedo.redo(), (x) => undoRedo.redoList.value.isNotEmpty);
    saveCommand = RelayCommand((x) => save(), (x) => canSave.value);
    buildCommand = RelayCommand((x) async => await project.buildGameCodeDll());
    //(x) => !VisualStudio.isDebugging && VisualStudio.buildDone);

    renameMultipleCommand = RelayCommand<String>(
      (x) {
        Map<GameEntity, String> oldData = Map.fromIterables(
          List.from(msEntity.value!.selectedEntities),
          msEntity.value!.selectedEntities.map((e) => e.name.value).toList(),
        );

        msEntity.value!.name.value = x;

        UndoRedo().add(
          UndoRedoAction(
            name:
                "Rename ${msEntity.value!.selectedEntities.length} entities to '$x'",
            undoAction: () {
              for (final data in oldData.entries) {
                data.key.name.value = data.value;
              }
              _setMsEntity();
            },
            redoAction: () {
              msEntity.value?.name.value = x;
              _setMsEntity();
            },
          ),
        );
      },
      (x) => x != msEntity.value?.name.value,
    );

    enableMultipleCommand = RelayCommand<bool>(
      (x) {
        Map<GameEntity, bool> oldData = Map.fromIterables(
          List.from(msEntity.value!.selectedEntities),
          msEntity.value!.selectedEntities
              .map((e) => e.isEnabled.value)
              .toList(),
        );

        msEntity.value?.isEnabled.value = x;

        UndoRedo().add(
          UndoRedoAction(
            name:
                "${x ? "'En" : "'Dis"}able' ${msEntity.value?.selectedEntities.length} GameEntities",
            undoAction: () {
              for (final data in oldData.entries) {
                data.key.isEnabled.value = data.value;
              }
              _setMsEntity();
            },
            redoAction: () {
              msEntity.value?.isEnabled.value = x;
              _setMsEntity();
            },
          ),
        );
      },
      (x) => x != msEntity.value?.isEnabled.value,
    );
  }

  void _addListeners() {
    undoRedo.undoList.addListener(_canSaveChanged);
    undoRedo.redoList.addListener(_canSaveChanged);
  }
}
