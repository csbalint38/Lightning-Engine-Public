import 'package:editor/game_code/controllers/new_script_controller.dart';
import 'package:editor/game_project/project.dart';
import 'package:editor/themes/themes.dart';
import 'package:flutter/material.dart';

class NewScriptDialog extends StatefulWidget {
  final Project project;

  const NewScriptDialog(this.project, {super.key});

  @override
  State<NewScriptDialog> createState() => _NewScriptDialogState();
}

class _NewScriptDialogState extends State<NewScriptDialog> {
  late final NewScriptController _controller;
  late final TextEditingController _nameController;
  late final TextEditingController _pathController;
  Future<void>? _createScriptFuture;

  void _closeDialog() async {
    await _createScriptFuture;
    await Future.delayed(const Duration(milliseconds: 30));

    if (mounted) {
      Navigator.of(context).pop();
    }
  }

  @override
  void initState() {
    super.initState();
    _controller = NewScriptController(widget.project);
    _nameController = TextEditingController(text: _controller.name.value);
    _pathController = TextEditingController(text: _controller.path.value);

    _controller.validate();
  }

  @override
  void dispose() {
    _nameController.dispose();
    _pathController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Dialog(
      child: SizedBox(
        width: 720,
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Padding(
                padding: const EdgeInsets.only(bottom: 20),
                child: Text(
                  "New Script",
                  style: Theme.of(context).textTheme.headlineSmall,
                ),
              ),
              Row(
                children: [
                  const Padding(
                    padding: EdgeInsets.only(right: 8),
                    child: SizedBox(
                      width: 100,
                      child: Text("Script name:"),
                    ),
                  ),
                  SizedBox(
                    width: 480,
                    child: TextField(
                      textAlignVertical: TextAlignVertical.center,
                      cursorHeight: 16,
                      maxLines: 1,
                      style: Theme.of(context).smallText,
                      decoration: Theme.of(context).smallInput,
                      controller: _nameController,
                      onChanged: (value) => _controller.setName(value),
                    ),
                  ),
                ],
              ),
              const SizedBox(
                height: 16,
              ),
              Row(
                children: [
                  const Padding(
                    padding: EdgeInsets.only(right: 8),
                    child: SizedBox(
                      width: 100,
                      child: Text("Path:"),
                    ),
                  ),
                  SizedBox(
                    width: 480,
                    child: TextField(
                      textAlignVertical: TextAlignVertical.center,
                      cursorHeight: 16,
                      maxLines: 1,
                      style: Theme.of(context).smallText,
                      decoration: Theme.of(context).smallInput,
                      controller: _pathController,
                      onChanged: (value) => _controller.setPath(value),
                    ),
                  ),
                ],
              ),
              ValueListenableBuilder(
                valueListenable: _controller.errorMessage,
                builder: (context, value, _) {
                  return Padding(
                    padding: const EdgeInsets.only(top: 16),
                    child: _controller.errorMessage.value.isNotEmpty
                        ? Column(
                            children: [
                              Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  Text(
                                    _controller.errorMessage.value,
                                    style: const TextStyle(color: Colors.red),
                                  ),
                                ],
                              ),
                              const Row(
                                children: [
                                  SizedBox(height: 20),
                                ],
                              )
                            ],
                          )
                        : Column(
                            children: [
                              Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  ValueListenableBuilder(
                                    valueListenable: _controller.name,
                                    builder: (context, _, __) {
                                      return Expanded(
                                        child: Text(
                                          "'${_controller.name.value}.h' and '${_controller.name.value}.cpp' will be added to:",
                                          textAlign: TextAlign.center,
                                          overflow: TextOverflow.ellipsis,
                                        ),
                                      );
                                    },
                                  ),
                                ],
                              ),
                              Row(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  ValueListenableBuilder(
                                    valueListenable: _controller.path,
                                    builder: (context, _, __) {
                                      return Expanded(
                                        child: Text(
                                          _controller.path.value,
                                          textAlign: TextAlign.center,
                                          overflow: TextOverflow.ellipsis,
                                        ),
                                      );
                                    },
                                  ),
                                ],
                              )
                            ],
                          ),
                  );
                },
              ),
              const SizedBox(height: 20),
              Row(
                mainAxisAlignment: MainAxisAlignment.end,
                children: [
                  TextButton(
                    onPressed: _createScriptFuture == null
                        ? () => Navigator.of(context).pop()
                        : null,
                    child: const Text("Cancel"),
                  ),
                  FutureBuilder<void>(
                    future: _createScriptFuture,
                    builder: (context, snapshot) {
                      if (snapshot.connectionState == ConnectionState.waiting) {
                        return const Padding(
                          padding: EdgeInsets.symmetric(horizontal: 26.5),
                          child: SizedBox(
                            width: 20,
                            height: 20,
                            child: CircularProgressIndicator(
                              strokeWidth: 2,
                            ),
                          ),
                        );
                      } else {
                        return ValueListenableBuilder(
                          valueListenable: _controller.errorMessage,
                          builder: (context, value, _) => TextButton(
                            onPressed: value == ''
                                ? () {
                                    setState(() {
                                      _createScriptFuture =
                                          _controller.create();
                                    });
                                    _closeDialog();
                                  }
                                : null,
                            child: const Text("Create"),
                          ),
                        );
                      }
                    },
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
