import 'dart:io';
import 'package:editor/common/constants.dart';
import 'package:editor/common/relay_command.dart';
import 'package:editor/dll_wrappers/visual_studio.dart';
import 'package:editor/game_project/scene.dart';
import 'package:editor/common/list_notifier.dart';
import 'package:editor/utilities/capitalize.dart';
import 'package:editor/utilities/logger.dart';
import 'package:editor/utilities/undo_redo.dart';
import 'package:flutter/material.dart';
import 'package:xml/xml.dart' as xml;
import 'package:path/path.dart' as p;

class Project {
  static String extension = "lightning";
  static final UndoRedo _undoRedo = UndoRedo();

  final String name;
  final String path;
  final ListNotifier<Scene> scenes = ListNotifier<Scene>();
  late Scene activeScene;
  final ValueNotifier<BuildConfig> buildConfig =
      ValueNotifier<BuildConfig>(BuildConfig.debug);

  late final String fullPath;
  late final String solution;

  late RelayCommand addScene;
  late RelayCommand removeScene;

  static String getConfigurationName(BuildConfig config) {
    final String value = config.toString().split('.').last;
    return value[0].toUpperCase() + value.substring(1);
  }

  Project(List<Scene>? scenes, {required this.name, required this.path}) {
    fullPath = p.join(path, name, '$name.$extension');
    solution = p.join(path, name, '$name.sln');
    if (scenes == null) {
      this.scenes.add(Scene(name: "Default Scene", isActive: true));
      activeScene = this.scenes.value.first;
    } else {
      for (final Scene scene in scenes) {
        this.scenes.add(scene);
        if (scene.isActive) {
          activeScene = scene;
        }
      }
    }

    addScene = RelayCommand(
      (x) {
        _addScene("New Scene ${this.scenes.value.length}");
        Scene newScene = this.scenes.value.last;
        int index = this.scenes.value.length - 1;
        _undoRedo.add(
          UndoRedoAction(
            name: "Add ${newScene.name}",
            undoAction: () => _removeScene(newScene),
            redoAction: () => this.scenes.insert(index, newScene),
          ),
        );
      },
    );

    removeScene = RelayCommand<Scene>(
      (x) {
        int index = this.scenes.value.indexOf(x);
        _removeScene(x);
        _undoRedo.add(
          UndoRedoAction(
            name: "Remove ${x.name}",
            undoAction: () => this.scenes.insert(index, x),
            redoAction: () => _removeScene(x),
          ),
        );
      },
      (x) => !x.isActive,
    );
  }

  factory Project.fromXMLFile(File file) {
    try {
      String content = file.readAsStringSync();
      Project instance = Project.fromXML(content);

      return instance;
    } catch (err) {
      debugLogger.e(err);
      EditorLogger().log(
        LogLevel.error,
        "Failed to open $file",
        trace: StackTrace.current,
      );
      rethrow;
    }
  }

  factory Project.fromXML(String xmlStr) {
    xml.XmlDocument document = xml.XmlDocument.parse(xmlStr);

    xml.XmlElement root = document.rootElement;
    final String name = (root.getElement("Name")?.innerText)!;
    final String path = (root.getElement("Path")?.innerText)!;
    final xml.XmlElement scenesNode = (root.getElement("Scenes"))!;
    final List<Scene> scenes = <Scene>[];

    for (final scene in scenesNode.findElements("Scene")) {
      scenes.add(Scene.fromXML(scene.toString()));
    }

    return Project(scenes, name: name, path: path);
  }

  factory Project.load(File file) {
    return Project.fromXMLFile(file);
  }

  String toXML() {
    final builder = xml.XmlBuilder();

    builder.element("Project", nest: () {
      builder.element("Name", nest: name);
      builder.element("Path", nest: path);
      builder.element("Scenes", nest: () {
        for (final scene in scenes.value) {
          builder.xml(scene.toXML());
        }
      });
    });

    return builder.buildDocument().toXmlString(pretty: true);
  }

  void toXMLFile(String path) {
    try {
      final String xmlString = toXML();
      final File file = File(path);
      file.writeAsStringSync(xmlString);
    } catch (err) {
      debugLogger.e(err);
      EditorLogger().log(
        LogLevel.error,
        "Failed to save project to location $path",
        trace: StackTrace.current,
      );
    }
  }

  static void save(Project project) {
    EditorLogger().log(
      LogLevel.info,
      "Saved project to ${project.path}\\${project.name}",
      trace: StackTrace.current,
    );
    return project.toXMLFile(project.fullPath);
  }

  void unload() {
    VisualStudio.closeVisualStudio();
    UndoRedo().resset();
  }

  Future<void> buildGameCodeDll({bool showWindow = true}) async {
    final BuildConfig buildConfigValue = buildConfig.value == BuildConfig.debug
        ? BuildConfig.debugEditor
        : BuildConfig.releaseEditor;
    final String configName =
        capitalize(buildConfigValue.toString().split('.').last);

    EditorLogger().log(LogLevel.info, "Building $solution, $configName",
        trace: StackTrace.current);

    _unloadGameCodeDll();
    await VisualStudio.buildSolution(
      this,
      buildConfigValue,
      showWindow,
    );
  }

  void _onBuildEnd(String configName) async {
    if (await VisualStudio.getLastBuildInfo()) {
      EditorLogger().log(
          LogLevel.info, "Building $configName configuration succeeded",
          trace: StackTrace.current);
      _loadGameCodeDll();
    } else {
      EditorLogger().log(
          LogLevel.error, "Building $configName configuration failed",
          trace: StackTrace.current);
    }
  }

  void _loadGameCodeDll() {}

  void _unloadGameCodeDll() {}

  void _addScene(String sceneName) {
    scenes.add(Scene(name: sceneName));
  }

  void _removeScene(Scene scene) {
    scenes.remove(scene);
  }
}
