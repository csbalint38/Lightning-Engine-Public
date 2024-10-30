import 'package:editor/common/scalar_box.dart';
import 'package:editor/common/vector_notifier.dart';
import 'package:flutter/material.dart';

enum VectorNotation { xyz, rgb, abc }

class VectorBox extends StatelessWidget {
  final FocusNode globalFocus;
  final Vector3Notifier notifier;

  const VectorBox(this.globalFocus, this.notifier, {super.key});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        ValueListenableBuilder(
          valueListenable: notifier,
          builder: (context, _, __) {
            return Row(
              children: [
                const Text('X'),
                const SizedBox(width: 5),
                ScalarBox(globalFocus, (x) => notifier.x = x, notifier.x),
                const SizedBox(width: 15),
                const Text('Y'),
                const SizedBox(width: 5),
                ScalarBox(globalFocus, (y) => notifier.y = y, notifier.y),
                const SizedBox(width: 15),
                const Text('Z'),
                const SizedBox(width: 5),
                ScalarBox(globalFocus, (z) => notifier.z = z, notifier.z),
              ],
            );
          },
        ),
      ],
    );
  }
}
