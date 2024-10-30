import 'dart:math';
import 'package:editor/editor.dart';
import 'package:editor/game_project/controllers/open_project_controller.dart';
import 'package:editor/game_project/new_project.dart';
import 'package:editor/themes/themes.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:intl/intl.dart';

class OpenProject extends StatefulWidget {
  const OpenProject({super.key});

  @override
  State<OpenProject> createState() => _OpenProjectState();
}

class _OpenProjectState extends State<OpenProject> {
  final OpenProjectController _controller = OpenProjectController();
  final FocusNode _localFocus = FocusNode();

  int selectedProjectIndex = 0;

  Route _newProject(Widget nextPage) {
    return PageRouteBuilder(
      pageBuilder: (context, animation, secondaryAnimation) => nextPage,
      transitionsBuilder: (context, animation, secondaryAnimation, child) {
        const begin = Offset(1, 0);
        const end = Offset.zero;
        const curve = Curves.easeInOut;

        var tween = Tween(begin: begin, end: end).chain(
          CurveTween(curve: curve),
        );
        var offsetAnimation = animation.drive(tween);

        return SlideTransition(position: offsetAnimation, child: child);
      },
    );
  }

  void _selectedProjectChanged(int index) {
    setState(() {
      selectedProjectIndex = index;
    });
  }

  void _openProject() {
    if (mounted) {
      Navigator.pushReplacement(
        context,
        MaterialPageRoute(
          builder: (context) => Editor(
            project: _controller.open(
              _controller.projects[selectedProjectIndex],
            ),
          ),
        ),
      );
    }
  }

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
    return Scaffold(
      body: Focus(
        autofocus: true,
        focusNode: _localFocus,
        onKeyEvent: (node, event) {
          if (event is KeyDownEvent &&
              event.logicalKey == LogicalKeyboardKey.enter) {
            _openProject();
            return KeyEventResult.handled;
          }
          if (event is KeyDownEvent &&
              event.logicalKey == LogicalKeyboardKey.arrowUp) {
            int index = min(selectedProjectIndex, 0);
            if (index != selectedProjectIndex) {
              _selectedProjectChanged(index);
            }
            return KeyEventResult.handled;
          }
          if (event is KeyDownEvent &&
              event.logicalKey == LogicalKeyboardKey.arrowDown) {
            int index = max(selectedProjectIndex, _controller.projects.length);
            if (index != selectedProjectIndex) {
              _selectedProjectChanged(index);
            }
            return KeyEventResult.handled;
          }
          return KeyEventResult.ignored;
        },
        child: Column(
          children: [
            Row(
              children: [
                Padding(
                  padding: const EdgeInsets.fromLTRB(48, 32, 0, 16),
                  child: ElevatedButton(
                    onPressed: () {
                      Navigator.push(context, _newProject(const NewProject()));
                    },
                    child: const Text(
                      "New Project",
                      style: TextStyle(fontSize: 18),
                    ),
                  ),
                )
              ],
            ),
            Expanded(
              flex: 4,
              child: Row(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Expanded(
                    flex: 1,
                    child: Column(
                      children: [
                        Container(
                          padding: const EdgeInsets.fromLTRB(48, 16, 0, 16),
                          child: Container(
                            height: 300,
                            decoration: BoxDecoration(
                              border: Border.all(
                                  color: Theme.of(context).borderColor,
                                  width: 1),
                            ),
                            child: Material(
                              child: ListView.builder(
                                itemCount: _controller.projects.length,
                                itemBuilder: (BuildContext context, int index) {
                                  return GestureDetector(
                                    onDoubleTap: _openProject,
                                    child: Listener(
                                      onPointerUp: (_) =>
                                          _selectedProjectChanged(index),
                                      child: ListTile(
                                        title: MouseRegion(
                                          cursor: SystemMouseCursors.click,
                                          child: Row(
                                            children: [
                                              Image.file(
                                                _controller
                                                    .projects[index].icon,
                                                width: 35,
                                                height: 35,
                                              ),
                                              const SizedBox(width: 17),
                                              Text(
                                                _controller.projects[index]
                                                    .projectName,
                                                maxLines: 1,
                                                overflow: TextOverflow.ellipsis,
                                                style: const TextStyle(
                                                    fontSize: 20),
                                              ),
                                            ],
                                          ),
                                        ),
                                        subtitle: MouseRegion(
                                          cursor: SystemMouseCursors.click,
                                          child: Column(
                                            children: [
                                              Row(
                                                children: [
                                                  Flexible(
                                                    child: Tooltip(
                                                      message: _controller
                                                          .projects[index]
                                                          .fullPath,
                                                      child: Text(
                                                        _controller
                                                            .projects[index]
                                                            .fullPath,
                                                        maxLines: 1,
                                                        overflow: TextOverflow
                                                            .ellipsis,
                                                        style: const TextStyle(
                                                            fontSize: 12),
                                                      ),
                                                    ),
                                                  )
                                                ],
                                              ),
                                              Row(
                                                mainAxisAlignment:
                                                    MainAxisAlignment
                                                        .spaceBetween,
                                                children: [
                                                  Tooltip(
                                                    message:
                                                        "Last modified: ${DateFormat('y.M.d H:mm').format(_controller.projects[index].lastModified)}",
                                                    child: Text(
                                                      DateFormat('y.M.d H:mm')
                                                          .format(_controller
                                                              .projects[index]
                                                              .lastModified),
                                                      maxLines: 1,
                                                      overflow:
                                                          TextOverflow.ellipsis,
                                                      style: const TextStyle(
                                                          fontSize: 12),
                                                    ),
                                                  ),
                                                  Tooltip(
                                                    message:
                                                        "Engine version: ${_controller.projects[index].engineVersion}",
                                                    child: Text(
                                                      _controller
                                                          .projects[index]
                                                          .engineVersion,
                                                      maxLines: 1,
                                                      overflow:
                                                          TextOverflow.ellipsis,
                                                      style: const TextStyle(
                                                          fontSize: 12),
                                                    ),
                                                  )
                                                ],
                                              )
                                            ],
                                          ),
                                        ),
                                        selected: index == selectedProjectIndex,
                                      ),
                                    ),
                                  );
                                },
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                  Expanded(
                    flex: 1,
                    child: Column(
                      children: [
                        Padding(
                          padding: const EdgeInsets.fromLTRB(0, 16, 40, 16),
                          child: _controller.projects.isNotEmpty
                              ? Image.file(
                                  _controller.projects[selectedProjectIndex]
                                      .screenshot,
                                  alignment: Alignment.centerRight,
                                  fit: BoxFit.fill,
                                  height: 300,
                                )
                              : const Center(
                                  child: Text("No projects was found")),
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
            Expanded(
              flex: 1,
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Column(
                    children: [
                      ElevatedButton(
                        onPressed: _openProject,
                        child: const Text("Open"),
                      ),
                    ],
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
