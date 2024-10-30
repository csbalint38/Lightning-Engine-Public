import 'package:editor/components/game_entity.dart';
import 'package:editor/editors/world_editor/controllers/world_editor_controller.dart';
import 'package:editor/themes/themes.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class ScenesList extends StatefulWidget {
  const ScenesList({super.key});

  @override
  State<ScenesList> createState() => _ScenesListState();
}

class _ScenesListState extends State<ScenesList> {
  final _pageBucket = PageStorageBucket();
  final _controller = WorldEditorController();

  void _changeSelection(int index) {
    if (HardwareKeyboard.instance.isControlPressed) {
      _controller.changeSelection(index, false);
    } else if (HardwareKeyboard.instance.isShiftPressed) {
      _controller.changeSelection(index, true);
    } else {
      _controller.changeSelection(index, null);
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
    return Column(
      children: [
        Align(
          alignment: Alignment.centerLeft,
          child: Padding(
            padding: const EdgeInsets.all(8),
            child: ElevatedButton(
              onPressed: _controller.addScene,
              style: Theme.of(context).smallButton,
              child: const Text("Add scene"),
            ),
          ),
        ),
        Expanded(
          child: Padding(
            padding: const EdgeInsets.only(top: 8),
            child: PageStorage(
              bucket: _pageBucket,
              child: Material(
                child: ValueListenableBuilder(
                  valueListenable: _controller.getScenes(),
                  builder: (context, _, __) {
                    return ListView.builder(
                      itemCount: _controller.getScenes().value.length,
                      itemBuilder: (context, index) {
                        return ListTileTheme(
                          minVerticalPadding: 0,
                          child: ExpansionTile(
                            controlAffinity: ListTileControlAffinity.leading,
                            minTileHeight: 0,
                            initiallyExpanded:
                                _controller.getScenes().value[index].isActive,
                            iconColor: Theme.of(context).primaryColor,
                            collapsedIconColor: Theme.of(context).primaryColor,
                            title: Row(
                              mainAxisAlignment: MainAxisAlignment.spaceBetween,
                              children: [
                                Text(
                                  _controller.getScenes().value[index].name,
                                  style: Theme.of(context).accentSmall,
                                ),
                                Row(
                                  children: [
                                    Tooltip(
                                      message: "Add new GameEntity",
                                      child: OutlinedButton(
                                        style: Theme.of(context).smallIcon,
                                        onPressed: _controller
                                                .getScenes()
                                                .value[index]
                                                .isActive
                                            ? () =>
                                                _controller.addGameEntity(index)
                                            : null,
                                        child: const Icon(
                                            Icons.add_circle_outline_sharp),
                                      ),
                                    ),
                                    Padding(
                                      padding: const EdgeInsets.only(right: 8),
                                      child: OutlinedButton(
                                        style: Theme.of(context).smallButton,
                                        onPressed: () =>
                                            _controller.removeScene(index),
                                        child: const Text("Remove"),
                                      ),
                                    ),
                                  ],
                                ),
                              ],
                            ),
                            children: [
                              ValueListenableBuilder<List<GameEntity>>(
                                valueListenable:
                                    _controller.getActiveScene.entities,
                                builder: (context, value, _) {
                                  return Column(
                                    children: value
                                        .asMap()
                                        .entries
                                        .map(
                                          (entity) => GestureDetector(
                                            onTapUp: (_) {
                                              _changeSelection(entity.key);
                                            },
                                            child: ValueListenableBuilder(
                                              valueListenable: _controller
                                                  .msEntity,
                                              builder: (context, value, __) {
                                                return ListTile(
                                                  selectedTileColor:
                                                      Theme.of(context)
                                                          .outlineColor,
                                                  selectedColor:
                                                      Theme.of(context)
                                                          .primaryColor,
                                                  minTileHeight: 0,
                                                  title: MouseRegion(
                                                    cursor: SystemMouseCursors
                                                        .click,
                                                    child: Row(
                                                      mainAxisAlignment:
                                                          MainAxisAlignment
                                                              .spaceBetween,
                                                      children: [
                                                        ValueListenableBuilder(
                                                          valueListenable:
                                                              entity.value.name,
                                                          builder: (context, _,
                                                                  __) =>
                                                              Text(
                                                            entity.value.name
                                                                .value,
                                                            style: Theme.of(
                                                                    context)
                                                                .smallText,
                                                          ),
                                                        ),
                                                        OutlinedButton(
                                                          onPressed: _controller
                                                                  .getScenes()
                                                                  .value[index]
                                                                  .isActive
                                                              ? () => _controller
                                                                  .removeGameEntity(
                                                                      index,
                                                                      entity
                                                                          .value)
                                                              : null,
                                                          style:
                                                              Theme.of(context)
                                                                  .smallIcon,
                                                          child: const Icon(Icons
                                                              .cancel_outlined),
                                                        )
                                                      ],
                                                    ),
                                                  ),
                                                  titleTextStyle:
                                                      Theme.of(context)
                                                          .accentSmall,
                                                  selected: value?.selectedEntities.contains(entity.value) ?? false,
                                                );
                                              },
                                            ),
                                          ),
                                        )
                                        .toList(),
                                  );
                                },
                              ),
                            ],
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
      ],
    );
  }
}
