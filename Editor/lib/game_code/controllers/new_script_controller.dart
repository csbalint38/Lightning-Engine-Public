import 'dart:io';
import 'package:editor/common/constants.dart';
import 'package:editor/dll_wrappers/visual_studio.dart';
import 'package:editor/game_project/project.dart';
import 'package:editor/utilities/logger.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:path/path.dart' as p;

class NewScriptController {
  final Project project;
  ValueNotifier<String> errorMessage = ValueNotifier<String>("");
  ValueNotifier<String> name = ValueNotifier<String>("NewScript");
  ValueNotifier<String> path = ValueNotifier<String>("");

  NewScriptController(this.project) {
    path.value = p.join(project.path, project.name, 'Code\\');
  }

  void setName(String value) {
    name.value = value;
    validate();
  }

  void setPath(String value) {
    path.value = value;
    validate();
  }

  bool validate() {
    errorMessage.value = "";

    if (name.value.isEmpty) {
      errorMessage.value = "Type in a script name";
      return false;
    }

    if (!strictFileNameCharacters.hasMatch(name.value) ||
        name.value.contains(' ')) {
      errorMessage.value = "Invalid character(s) used in script name";
      return false;
    }

    if (path.value.isEmpty) {
      errorMessage.value = "Select script folder";
      return false;
    }

    if (!pathAllowedCharacters.hasMatch(path.value)) {
      errorMessage.value = "Invalid character(s) used in script path";
      return false;
    }

    if (!path.value.startsWith(p.join(project.path, project.name, "Code\\")) &&
        path.value != p.join(project.path, project.name, "Code")) {
      errorMessage.value =
          "Script must be added to Code folder or to its subfolder";
      return false;
    }

    if (File(p.join(path.value, "${name.value}.cpp")).existsSync() ||
        File(p.join(path.value, "${name.value}.h")).existsSync()) {
      errorMessage.value = "Script ${name.value} alredy exists in this folder";
      return false;
    }

    return true;
  }

  Future<void> create() async {
    if (!validate()) return;

    try {
      if (!Directory(path.value).existsSync()) {
        await Directory(path.value).create(recursive: true);
      }

      String h = p.join(path.value, "${name.value}.h");
      String cpp = p.join(path.value, "${name.value}.cpp");

      String r0 = _toCamelCase(name.value);
      String r1 = _toSnakeCase(name.value);

      String hTemplate =
          await rootBundle.loadString('lib/game_code/templates/h.txt');
      hTemplate = hTemplate.replaceAll('{{0}}', r0).replaceAll('{{1}}', r1);

      String cppTemplate =
          await rootBundle.loadString('lib/game_code/templates/cpp.txt');
      cppTemplate = cppTemplate.replaceAll('{{0}}', r0).replaceAll('{{1}}', r1);

      await File(h).writeAsString(hTemplate);
      await File(cpp).writeAsString(cppTemplate);

      for (int i = 0; i < 3; i++) {
        if (!await (VisualStudio.addFiles(project.solution, [h, cpp]))) {
          await Future.delayed(const Duration(seconds: 1));
        } else {
          break;
        }
      }
    } catch (e) {
      debugLogger.e(e);
      EditorLogger().log(
          LogLevel.error, "Failed to create script ${name.value}",
          trace: StackTrace.current);
    }
  }

  String _toSnakeCase(String value) {
    return value
        .replaceAll(RegExp(r'[^a-zA-Z0-9]'), '')
        .replaceAllMapped(RegExp(r'([a-z]([A-Z]))'),
            (Match m) => '${m.group(1)}_${m.group(2)}')
        .toLowerCase()
        .replaceAllMapped(RegExp(r'([a-zA-Z])(\d)'),
            (Match m) => '${m.group(1)}_${m.group(2)}');
  }

  String _toCamelCase(String value) {
    return value
      ..replaceAll(RegExp(r'[^a-zA-Z0-9]'), '')
          .split(r'\s+')
          .map((word) => word.isEmpty
              ? word
              : word[0].toUpperCase() + word.substring(1).toLowerCase())
          .join('');
  }
}
