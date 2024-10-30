import 'package:editor/components/transform.dart';
import 'package:editor/common/vector_box.dart';
import 'package:editor/editors/world_editor/components/base_component.dart';
import 'package:flutter/material.dart';

class Transform extends StatelessWidget {
  final MSTransform component;
  final FocusNode globalFocus;

  const Transform(
      {required this.globalFocus, required this.component, super.key});

  @override
  Widget build(BuildContext context) {
    return BaseComponent(
      title: "Transform",
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const SizedBox(
              width: 60,
              child: Text("Position:"),
            ),
            VectorBox(globalFocus, component.position),
          ],
        ),
        const SizedBox(height: 8),
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const SizedBox(
              width: 60,
              child: Text("Rotation:"),
            ),
            VectorBox(globalFocus, component.rotation),
          ],
        ),
        const SizedBox(height: 8),
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const SizedBox(
              width: 60,
              child: Text("Scale:"),
            ),
            VectorBox(globalFocus, component.scale),
          ],
        ),
      ],
    );
  }
}
