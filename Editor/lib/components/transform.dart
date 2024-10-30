import 'package:editor/components/components.dart';
import 'package:editor/components/game_entity.dart';
import 'package:editor/common/relay_command.dart';
import 'package:editor/common/vector_notifier.dart';
import 'package:editor/utilities/capitalize.dart';
import 'package:editor/utilities/undo_redo.dart';
import 'package:vector_math/vector_math.dart';
import 'package:xml/xml.dart' as xml;

class Transform implements Component {
  late Vector3 position;
  late Vector3 rotation;
  late Vector3 scale;

  Transform({Vector3? position, Vector3? rotation, Vector3? scale}) {
    this.position = position ?? Vector3.zero();
    this.rotation = rotation ?? Vector3.zero();
    this.scale = scale ?? Vector3.all(1);
  }

  Transform.fromXML(String xmlStr) {
    xml.XmlDocument document = xml.XmlDocument.parse(xmlStr);

    xml.XmlElement root = document.rootElement;
    final xml.XmlElement? positionNode = (root.getElement("Position"));
    final xml.XmlElement? rotationNode = (root.getElement("Rotation"));
    final xml.XmlElement? scaleNode = (root.getElement("Scale"));

    final Vector3 position = Vector3(
      double.parse((positionNode?.getElement("X")?.innerText)!),
      double.parse((positionNode?.getElement("Y")?.innerText)!),
      double.parse((positionNode?.getElement("Z")?.innerText)!),
    );

    final Vector3 rotation = Vector3(
      double.parse((rotationNode?.getElement("X")?.innerText)!),
      double.parse((rotationNode?.getElement("Y")?.innerText)!),
      double.parse((rotationNode?.getElement("Z")?.innerText)!),
    );

    final Vector3 scale = Vector3(
      double.parse((scaleNode?.getElement("X")?.innerText)!),
      double.parse((scaleNode?.getElement("Y")?.innerText)!),
      double.parse((scaleNode?.getElement("Z")?.innerText)!),
    );

    Transform(position: position, rotation: rotation, scale: scale);
  }

  @override
  String toXML() {
    final builder = xml.XmlBuilder();
    builder.element("Position", nest: () {
      builder.element("X", nest: position.x.toString());
      builder.element("Y", nest: position.y.toString());
      builder.element("Z", nest: position.z.toString());
    });
    builder.element("Rotation", nest: () {
      builder.element("X", nest: rotation.x.toString());
      builder.element("Y", nest: rotation.y.toString());
      builder.element("Z", nest: rotation.z.toString());
    });
    builder.element("Scale", nest: () {
      builder.element("X", nest: scale.x.toString());
      builder.element("Y", nest: scale.y.toString());
      builder.element("Z", nest: scale.z.toString());
    });

    return builder.buildDocument().toXmlString(pretty: true);
  }

  @override
  MSComponent<Component> getMultiselectComponent(MSGameEntity msEntity) =>
      MSTransform(msEntity);
}

enum TransformProperty { position, rotation, scale }

class MSTransform extends MSComponent<Transform> {
  final Vector3Notifier position = Vector3Notifier();
  final Vector3Notifier rotation = Vector3Notifier();
  final Vector3Notifier scale = Vector3Notifier();

  late RelayCommand updateComponentsCommand;

  MSTransform(super.msEntity) {
    _createCommands();
    refresh();
    _initializeListeners();
  }

  @override
  bool updateComponents(dynamic propertyName) {
    if (propertyName is! TransformProperty) return false;

    final updateMap = {
      TransformProperty.position: (Transform c) =>
          c.position.setFrom(position.value),
      TransformProperty.rotation: (Transform c) =>
          c.rotation.setFrom(rotation.value),
      TransformProperty.scale: (Transform c) => c.scale.setFrom(scale.value),
    };

    for (var c in selectedComponents) {
      updateMap[propertyName]?.call(c);
    }

    return true;
  }

  @override
  bool updateMSComponent() {
    position.x = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.position.x),
    );
    position.y = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.position.y),
    );
    position.z = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.position.z),
    );
    rotation.x = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.rotation.x),
    );
    rotation.y = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.rotation.y),
    );
    rotation.z = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.rotation.z),
    );
    scale.x = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.scale.x),
    );
    scale.y = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.scale.y),
    );
    scale.z = MSGameEntity.getMixedValue(
      selectedComponents,
      ((x) => x.scale.z),
    );

    return true;
  }

  void _initializeListeners() {
    position.addListener(
        () => updateComponentsCommand.execute(TransformProperty.position));
    rotation.addListener(
        () => updateComponentsCommand.execute(TransformProperty.rotation));
    scale.addListener(
        () => updateComponentsCommand.execute(TransformProperty.scale));
  }

  void _createCommands() {
    updateComponentsCommand = RelayCommand<TransformProperty>(
      (x) {
        final List<Vector3> oldValues;
        final Vector3 newValue;
        switch (x) {
          case TransformProperty.position:
            newValue = position.value;
            oldValues = selectedComponents
                .map((transform) => transform.position.clone())
                .toList();
            break;
          case TransformProperty.rotation:
            newValue = rotation.value;
            oldValues = selectedComponents
                .map((transform) => transform.rotation.clone())
                .toList();
            break;
          case TransformProperty.scale:
            newValue = scale.value;
            oldValues = selectedComponents
                .map((transform) => transform.scale.clone())
                .toList();
            break;
        }

        updateComponents(x);

        final Map<TransformProperty, void Function(int, Vector3)> actions = {
          TransformProperty.position: (i, other) =>
              selectedComponents[i].position.setFrom(other),
          TransformProperty.rotation: (i, other) =>
              selectedComponents[i].rotation.setFrom(other),
          TransformProperty.scale: (i, other) =>
              selectedComponents[i].scale.setFrom(other),
        };

        UndoRedo().add(
          UndoRedoAction(
            name: "${capitalize(x.name)} changed",
            undoAction: () {
              for (int i = 0; i < oldValues.length; i++) {
                actions[x]?.call(i, oldValues[i]);
              }
              MSGameEntity.getMSGameEntity()?.refresh();
            },
            redoAction: () {
              for (int i = 0; i < oldValues.length; i++) {
                actions[x]?.call(i, newValue);
              }
              MSGameEntity.getMSGameEntity()?.refresh();
            },
          ),
        );
      },
    );
  }
}
