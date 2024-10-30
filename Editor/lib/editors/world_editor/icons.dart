import 'package:editor/common/constants.dart';
import 'package:editor/common/dropdown_button.dart';
import 'package:editor/editors/world_editor/controllers/world_editor_controller.dart';
import 'package:editor/game_code/new_script_dialog.dart';
import 'package:editor/game_project/project.dart';
import 'package:flutter/material.dart';

class IconsRow extends StatefulWidget {
  const IconsRow({super.key});

  @override
  State<IconsRow> createState() => _IconsRowState();
}

class _IconsRowState extends State<IconsRow> {
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
    return Row(
      children: [
        const SizedBox(width: 8),
        Tooltip(
          message: "Undo\nCtrl + Z",
          child: ValueListenableBuilder(
            valueListenable: _controller.undoRedo.redoList,
            builder: (context, _, __) => IconButton(
              onPressed: _controller.undoRedo.undoList.value.isNotEmpty
                  ? () => _controller.undoCommand.execute(null)
                  : null,
              icon: const Icon(Icons.undo),
            ),
          ),
        ),
        Tooltip(
          message: "Redo\nCtrl + Y",
          child: ValueListenableBuilder(
            valueListenable: _controller.undoRedo.undoList,
            builder: (context, _, __) => IconButton(
              onPressed: _controller.undoRedo.redoList.value.isNotEmpty
                  ? () => _controller.redoCommand.execute(null)
                  : null,
              icon: const Icon(Icons.redo),
            ),
          ),
        ),
        Tooltip(
          message: "Save\nCtrl + S",
          child: ValueListenableBuilder(
            valueListenable: _controller.canSave,
            builder: (context, __, _) => IconButton(
              onPressed: _controller.saveCommand.canExecute(null)
                  ? () => _controller.saveCommand.execute(null)
                  : null,
              icon: const Icon(Icons.save),
            ),
          ),
        ),
        Tooltip(
          message: 'Create new script',
          child: IconButton(
            icon: const Icon(Icons.note_add_rounded),
            onPressed: () {
              showDialog(
                barrierDismissible: false,
                context: context,
                builder: (BuildContext context) {
                  return NewScriptDialog(_controller.project);
                },
              );
            },
          ),
        ),
        const VerticalDivider(
          width: 15,
          endIndent: 2,
          indent: 2,
        ),
        Tooltip(
          message: "Build configuration",
          child: ValueListenableBuilder(
            valueListenable: _controller.project.buildConfig,
            builder: (context, value, child) => DropdownButtonHideUnderline(
              child: LightningDropdownButton<BuildConfig>(
                  value: _controller.project.buildConfig.value,
                  isDense: true,
                  height: 25,
                  focusColor: Colors.transparent,
                  items: [
                    DropdownMenuItem<BuildConfig>(
                      value: BuildConfig.debug,
                      child: Padding(
                        padding: const EdgeInsets.symmetric(
                            vertical: 2, horizontal: 4),
                        child: Text(
                          Project.getConfigurationName(BuildConfig.debug),
                          style: Theme.of(context).textTheme.bodySmall,
                          textAlign: TextAlign.center,
                        ),
                      ),
                    ),
                    DropdownMenuItem<BuildConfig>(
                      value: BuildConfig.release,
                      child: Padding(
                        padding: const EdgeInsets.symmetric(
                            vertical: 2, horizontal: 4),
                        child: Text(
                          Project.getConfigurationName(BuildConfig.release),
                          style: Theme.of(context).textTheme.bodySmall,
                          textAlign: TextAlign.center,
                        ),
                      ),
                    ),
                  ],
                  onChanged: (BuildConfig? value) {
                    _controller.project.buildConfig.value = value!;
                  }),
            ),
          ),
        ),
        Tooltip(
          message: "Build\nCtrl + Shift + B\nF7",
          child: IconButton(
              icon: const Icon(Icons.construction_rounded),
              onPressed: () => _controller.buildCommand.execute(null)),
        ),
        Tooltip(
          message: "Stop Debugging\nCtrl + F5",
          child: IconButton(
            icon: const Icon(Icons.stop, color: Colors.red),
            onPressed: () {},
          ),
        ),
        Tooltip(
          message: "Start Debugging",
          child: IconButton(
            icon: const Icon(
              Icons.play_arrow_sharp,
              color: Colors.green,
            ),
            onPressed: () {},
          ),
        )
      ],
    );
  }
}
