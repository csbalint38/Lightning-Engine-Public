import 'package:editor/components/components.dart';
import 'package:editor/components/transform.dart';
import 'package:editor/editors/world_editor/controllers/world_editor_controller.dart';
import 'package:editor/editors/world_editor/components/transform.dart' as view;
import 'package:editor/themes/themes.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class Components extends StatefulWidget {
  final FocusNode focusNode;
  const Components({super.key, required this.focusNode});

  @override
  State<Components> createState() => _ComponentsState();
}

class _ComponentsState extends State<Components> {
  final _controller = WorldEditorController();
  final TextEditingController _nameController = TextEditingController();
  final FocusNode _localFocus = FocusNode();

  @override
  void initState() {
    super.initState();
  }

  @override
  void dispose() {
    _nameController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return ValueListenableBuilder(
      valueListenable: _controller.msEntity,
      builder: (context, msEntity, _) {
        if (msEntity == null || msEntity.components.value.isEmpty) {
          return const Column();
        }
        _nameController.text = msEntity.name.value;
        return SingleChildScrollView(
          child: Column(
            children: [
              Padding(
                padding: const EdgeInsets.all(8.0),
                child: Row(
                  children: [
                    ElevatedButton(
                      onPressed: () {},
                      style: Theme.of(context).smallButton,
                      child: const Text("Add Component"),
                    ),
                  ],
                ),
              ),
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 8.0),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Flexible(
                      child: Container(
                        constraints: const BoxConstraints(maxWidth: 250),
                        child: Focus(
                          focusNode: _localFocus,
                          onKeyEvent: (node, event) {
                            if (event is KeyDownEvent &&
                                event.logicalKey == LogicalKeyboardKey.escape) {
                              _localFocus.unfocus();
                              widget.focusNode.requestFocus();
                              return KeyEventResult.handled;
                            }
                            return KeyEventResult.ignored;
                          },
                          child: TextField(
                            textAlignVertical: TextAlignVertical.center,
                            cursorHeight: 14,
                            maxLines: 1,
                            controller: _nameController,
                            style: Theme.of(context).smallText,
                            decoration: Theme.of(context).smallInput,
                            onSubmitted: (name) {
                              _controller.setMsEntityName(name);
                              widget.focusNode.requestFocus();
                            },
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(width: 10),
                    Column(
                      children: [
                        Row(
                          children: [
                            Text(
                              "Enabled: ",
                              style: TextStyle(
                                  fontSize: 14,
                                  color: Theme.of(context).lightColor),
                            ),
                            Checkbox(
                              tristate: true,
                              value: msEntity.isEnabled.value,
                              onChanged: (_) {
                                if (msEntity.isEnabled.value == null) {
                                  _controller.enableMsEntity(true);
                                } else {
                                  _controller.enableMsEntity(
                                      !msEntity.isEnabled.value!);
                                }
                                setState(() {});
                              },
                            ),
                          ],
                        ),
                      ],
                    ),
                  ],
                ),
              ),
              Row(
                children: [
                  Expanded(
                    child: Padding(
                      padding: const EdgeInsets.only(top: 4),
                      child: ValueListenableBuilder(
                        valueListenable: msEntity.components,
                        builder: (context, value, _) {
                          final List<Widget> componentsWidget = value.map(
                            (MSComponent component) {
                              if (component is MSTransform) {
                                return view.Transform(
                                  component: component,
                                  globalFocus: widget.focusNode,
                                );
                              }
                              return Container();
                            },
                          ).toList();
                          return Column(
                            children: componentsWidget,
                          );
                        },
                      ),
                    ),
                  ),
                ],
              )
            ],
          ),
        );
      },
    );
  }
}
