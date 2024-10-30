import 'package:editor/editor.dart';
import 'package:editor/game_project/controllers/new_project_controller.dart';
import 'package:editor/game_project/controllers/open_project_controller.dart';
import 'package:editor/game_project/project.dart';
import 'package:editor/game_project/project_data.dart';
import 'package:editor/game_project/project_template.dart';
import 'package:editor/themes/themes.dart';
import 'package:flutter/material.dart';

class NewProject extends StatefulWidget {
  const NewProject({super.key});

  @override
  State<NewProject> createState() => _NewProjectState();
}

class _NewProjectState extends State<NewProject> {
  final _controller = NewProjectController();

  late final TextEditingController _nameController;
  late final TextEditingController _pathController;

  final FocusNode _listFocus = FocusNode();

  int selectedTemplateIndex = 0;

  void _selectedThemeChanged(int index) {
    setState(() {
      selectedTemplateIndex = index;
    });
  }

  void _locateEngine() {
    String enginePath = "";
    ValueNotifier<bool> isPathValid = ValueNotifier<bool>(true);

    showDialog(
        context: context,
        builder: (BuildContext context) {
          return Dialog(
            child: Padding(
              padding: const EdgeInsets.all(24),
              child: Column(
                mainAxisSize: MainAxisSize.min,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Padding(
                    padding: const EdgeInsets.only(bottom: 20),
                    child: Text(
                      "Where is Lighning Engine installed?",
                      style: Theme.of(context).textTheme.headlineSmall,
                    ),
                  ),
                  const Padding(
                    padding: EdgeInsets.only(bottom: 8.0),
                    child: SelectableText(
                        "Typycal installation path is 'C:/USER/Documents/LightningEngine/"),
                  ),
                  Row(
                    children: [
                      const Padding(
                        padding: EdgeInsets.only(right: 8),
                        child: Text("Engine path:"),
                      ),
                      SizedBox(
                        width: 480,
                        child: TextField(
                          textAlignVertical: TextAlignVertical.center,
                          cursorHeight: 16,
                          maxLines: 1,
                          style: Theme.of(context).smallText,
                          decoration: Theme.of(context).smallInput,
                          onChanged: (value) {
                            isPathValid.value =
                                _controller.validateEnginePath(value);
                            if (_controller.validateEnginePath(value)) {
                              enginePath = value;
                            }
                          },
                        ),
                      ),
                    ],
                  ),
                  ValueListenableBuilder(
                      valueListenable: isPathValid,
                      builder: (context, value, _) {
                        return Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Padding(
                              padding: const EdgeInsets.only(top: 8),
                              child: Text(
                                value ? "" : "This isn't it",
                                style: const TextStyle(color: Colors.red),
                              ),
                            ),
                          ],
                        );
                      }),
                  const SizedBox(height: 20),
                  Row(
                    mainAxisAlignment: MainAxisAlignment.end,
                    children: [
                      TextButton(
                          onPressed: () => Navigator.of(context).pop(),
                          child: const Text("Close")),
                      TextButton(
                        onPressed: () {
                          _controller.setEnginePath(enginePath);
                          Navigator.of(context).pop();
                          Navigator.of(context).pop();
                        },
                        child: const Text("Ok"),
                      )
                    ],
                  ),
                ],
              ),
            ),
          );
        });
  }

  void _createProject() async {
    if (!_controller.canFindEngine) {
      showDialog(
        context: context,
        builder: (BuildContext context) {
          return AlertDialog(
              title: const Text("Error!", style: TextStyle(color: Colors.red)),
              content: const Text(
                  "Can't locate Lightning Engine. You need to locate it before creating new project."),
              actions: [
                TextButton(
                    onPressed: () => Navigator.of(context).pop(),
                    child: const Text('Close')),
                TextButton(
                    onPressed: _locateEngine, child: const Text('Locate')),
              ]);
        },
      );
      return;
    }

    await _controller
        .createProject(_controller.getTemplates()[selectedTemplateIndex]);
    final Project project = OpenProjectController().open(
      ProjectData(
        projectName: _controller.name.value,
        projectPath: _controller.path.value,
        lastModified: DateTime.now(),
      ),
    );
    if (mounted) {
      Navigator.pushReplacement(
        context,
        MaterialPageRoute(builder: (context) => Editor(project: project)),
      );
    }
  }

  @override
  void initState() {
    super.initState();

    _nameController = TextEditingController(text: _controller.name.value);
    _pathController = TextEditingController(text: _controller.path.value);
    _controller.validate();
  }

  @override
  void dispose() {
    _listFocus.dispose();
    _nameController.dispose();
    _pathController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Column(
        children: [
          Expanded(
            flex: 4,
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Expanded(
                  flex: 3,
                  child: Container(
                    padding: const EdgeInsets.fromLTRB(48, 16, 0, 16),
                    child: Container(
                      height: 300,
                      decoration: BoxDecoration(
                        border: Border.all(
                            color: Theme.of(context).borderColor, width: 1),
                      ),
                      child: Material(
                        child: ValueListenableBuilder<List<ProjectTemplate>>(
                          valueListenable: _controller.templates,
                          builder: (context, value, _) {
                            return ListView.builder(
                              itemCount: _controller.getTemplates().length,
                              itemBuilder: (BuildContext context, int index) {
                                return Listener(
                                  onPointerUp: (_) =>
                                      _selectedThemeChanged(index),
                                  child: ListTile(
                                    title: MouseRegion(
                                      cursor: SystemMouseCursors.click,
                                      child: Row(
                                        children: [
                                          Image.file(
                                            _controller
                                                .getTemplates()[index]
                                                .icon,
                                            width: 25,
                                            height: 25,
                                          ),
                                          const SizedBox(width: 15),
                                          Text(
                                            _controller
                                                .getTemplates()[index]
                                                .projectName,
                                          ),
                                        ],
                                      ),
                                    ),
                                    selected: index == selectedTemplateIndex,
                                  ),
                                );
                              },
                            );
                          },
                        ),
                      ),
                    ),
                  ),
                ),
                Expanded(
                  flex: 4,
                  child: Padding(
                    padding: const EdgeInsets.fromLTRB(0, 16, 40, 16),
                    child: ValueListenableBuilder<List<ProjectTemplate>>(
                      valueListenable: _controller.templates,
                      builder: (context, value, child) {
                        return Image.file(
                          _controller
                              .getTemplates()[selectedTemplateIndex]
                              .screenshot,
                          alignment: Alignment.centerRight,
                        );
                      },
                    ),
                  ),
                ),
              ],
            ),
          ),
          Expanded(
            flex: 3,
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Padding(
                  padding: const EdgeInsets.fromLTRB(16, 16, 16, 4),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.start,
                    children: [
                      const SizedBox(
                        width: 60,
                        child: Text(
                          "Name",
                          style: TextStyle(
                            fontSize: 18,
                          ),
                        ),
                      ),
                      Expanded(
                        child: TextField(
                          controller: _nameController,
                          onChanged: (text) {
                            _controller.setName(text);
                          },
                        ),
                      ),
                    ],
                  ),
                ),
                Padding(
                  padding: const EdgeInsets.fromLTRB(16, 4, 16, 0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.start,
                    children: [
                      const SizedBox(
                        width: 60,
                        child: Text(
                          "Path",
                          style: TextStyle(
                            fontSize: 18,
                          ),
                        ),
                      ),
                      Expanded(
                        child: TextField(
                          controller: _pathController,
                          onChanged: (text) {
                            _controller.setPath(text);
                          },
                        ),
                      ),
                      Padding(
                        padding: const EdgeInsets.only(left: 8),
                        child: OutlinedButton(
                          onPressed: () {},
                          child: const Text("Browse"),
                        ),
                      )
                    ],
                  ),
                ),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Padding(
                      padding: const EdgeInsets.only(top: 16),
                      child: ValueListenableBuilder<String>(
                        valueListenable: _controller.errorMessage,
                        builder: (context, value, _) {
                          return Text(
                            value,
                            style: const TextStyle(color: Colors.red),
                          );
                        },
                      ),
                    ),
                  ],
                ),
                Expanded(
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    crossAxisAlignment: CrossAxisAlignment.center,
                    children: [
                      OutlinedButton(
                        onPressed: Navigator.canPop(context)
                            ? () => Navigator.pop(context)
                            : null,
                        child: const Text("Back"),
                      ),
                      const SizedBox(width: 50),
                      ValueListenableBuilder<bool>(
                        valueListenable: _controller.isValid,
                        builder: (context, value, _) {
                          return ElevatedButton(
                            onPressed: value ? _createProject : null,
                            child: const Text("Create"),
                          );
                        },
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
