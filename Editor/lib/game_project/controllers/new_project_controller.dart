import 'dart:io';
import 'package:editor/common/constants.dart';
import 'package:editor/congifg/config.dart';
import 'package:flutter/material.dart';
import 'package:flutter/widgets.dart';
import 'package:path/path.dart' as p;
import 'package:editor/game_project/project.dart';
import 'package:editor/game_project/project_template.dart';
import 'package:editor/utilities/logger.dart';
import 'package:uuid/uuid.dart';

class NewProjectController {
  static final NewProjectController _newProjectController =
      NewProjectController._internal();

  final Directory _templatesDir = Directory("ProjectTemplates");
  final ValueNotifier<String> name = ValueNotifier<String>("NewProject");
  final ValueNotifier<String> path = ValueNotifier<String>(p.join(
      Platform.environment['USERPROFILE']!, 'Documents', 'LightningProjects'));
  final ValueNotifier<String> errorMessage = ValueNotifier<String>("");
  final ValueNotifier<List<ProjectTemplate>> templates =
      ValueNotifier<List<ProjectTemplate>>([]);
  final ValueNotifier<bool> isValid = ValueNotifier<bool>(true);
  final Config config = Config();

  factory NewProjectController() {
    return _newProjectController;
  }

  NewProjectController._internal() {
    _getProjectTemplates();
  }

  void setName(name) {
    this.name.value = name;
    setValidationState(validate());
  }

  void setPath(String path) {
    this.path.value = path;
    setValidationState(validate());
  }

  void setValidationState(bool isValid) {
    this.isValid.value = isValid;
  }

  void setErrorMessage(String errorMessage) {
    this.errorMessage.value = errorMessage;
  }

  List<ProjectTemplate> getTemplates() {
    return templates.value;
  }

  bool get canFindEngine {
    final dataFile = File(p.join(Platform.environment['LOCALAPPDATA']!,
        "LightningEditor", "environment.json"));
    if (!dataFile.existsSync()) return false;

    final String? path = config.read(ConfigProps.enginePath);

    if (path == null) return false;

    return Directory(p.join(path, 'Engine', 'EngineAPI')).existsSync();
  }

  bool validate() {
    String path = this.path.value;

    if (!path.endsWith('\\')) {
      path += '\\';
    }
    path += '${name.value}\\';

    if (name.value.trim().isEmpty) {
      setErrorMessage("Project name can't be empty or just white spaces");
      return false;
    }

    if (fileInvalidCharacters.hasMatch(path)) {
      setErrorMessage("Project name contains invalid characters");
      return false;
    }

    if (!pathAllowedCharacters.hasMatch(path)) {
      setErrorMessage("Path contains invalid characters");
      return false;
    }

    Directory dir = Directory(path);
    if (dir.existsSync() && dir.listSync().isNotEmpty) {
      setErrorMessage("Folder alredy exists and its not empty");
      return false;
    }

    setErrorMessage("");
    return true;
  }

  bool validateEnginePath(String path) {
    if (!pathAllowedCharacters.hasMatch(path)) return false;

    return Directory(p.join(path, "Engine", "EngineAPI")).existsSync();
  }

  Future<void> createProject(ProjectTemplate template) async {
    String fullPath = p.join(path.value, name.value);

    if (!fullPath.endsWith('\\')) {
      fullPath += '\\';
    }

    try {
      Directory projectDir = Directory(fullPath);
      if (!projectDir.existsSync()) {
        await projectDir.create(recursive: true);
      }
      for (final String folder in template.folders) {
        await Directory(p.join(fullPath, folder)).create();
      }
      await Process.run('attrib', ['+h', '$fullPath\\.Lightning']);
    } catch (err) {
      debugLogger.e(
          "Failed to create project from template. The following error occured:$err");
      EditorLogger().log(
        LogLevel.error,
        "Failed to create project from template.",
        trace: StackTrace.current,
      );
      rethrow;
    }

    try {
      await template.icon.copy(p.join(fullPath, '.Lightning', 'icon.jpg'));
      await template.screenshot
          .copy(p.join(fullPath, '.Lightning', 'screenshot.jpg'));
    } catch (e) {
      debugLogger.e("Failed to copy icon and/or screenshot from the template");
    }

    String templateString = await File(template.projectFilePath).readAsString();
    templateString = templateString
        .replaceAll('{{0}}', name.value)
        .replaceAll('{{1}}', path.value);
    final Project project = Project.fromXML(templateString);
    project.toXMLFile(p.join(fullPath, '${name.value}.${Project.extension}'));

    await _createMSVCSolution(template, path.value);
  }

  void setEnginePath(String path) async {
    if (!validateEnginePath(path)) return;

    config.write(ConfigProps.enginePath, path);
  }

  Future<void> _createMSVCSolution(
      ProjectTemplate template, String projectPath) async {
    final String? enginePath = config.read(ConfigProps.enginePath);
    File solutionTemplate =
        File(p.join(template.templateFolder.path, "SolutionTemplate.txt"));
    File projectTemplate =
        File(p.join(template.templateFolder.path, "ProjectTemplate.txt"));
    Directory engineAPIDirectory =
        Directory(p.join(enginePath!, 'Engine', 'EngineAPI'));

    if (solutionTemplate.existsSync() &&
        projectTemplate.existsSync() &&
        engineAPIDirectory.existsSync()) {
      final String projectUuid = const Uuid().v4().toUpperCase();

      String solutionString = solutionTemplate.readAsStringSync();
      solutionString = solutionString
          .replaceAll('{{0}}', name.value)
          .replaceAll('{{1}}', projectUuid)
          .replaceAll(
            '{{2}}',
            const Uuid().v4().toUpperCase(),
          );
      await File(p.join(projectPath, name.value, '${name.value}.sln'))
          .writeAsString(solutionString);

      String projectString = projectTemplate.readAsStringSync();
      projectString = projectString
          .replaceAll('{{0}}', name.value)
          .replaceAll('{{1}}', projectUuid)
          .replaceAll('{{2}}', engineAPIDirectory.path)
          .replaceAll('{{3}}', enginePath);
      await File(
              p.join(projectPath, name.value, 'Code', '${name.value}.vcxproj'))
          .writeAsString(projectString);
    }
  }

  void _getProjectTemplates() {
    _templatesDir.listSync().forEach((item) {
      if (item is Directory) {
        templates.value.add(
          ProjectTemplate.fromXMLFile(
            File(p.join(item.path, 'template.xml')),
          ),
        );
      }
    });
  }
}
