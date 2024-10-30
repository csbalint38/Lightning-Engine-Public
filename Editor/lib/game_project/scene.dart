import 'package:editor/components/game_entity.dart';
import 'package:editor/common/relay_command.dart';
import 'package:editor/common/list_notifier.dart';
import 'package:editor/utilities/undo_redo.dart';
import 'package:xml/xml.dart' as xml;

class Scene {
  static final UndoRedo _undoRedo = UndoRedo();

  String name;
  bool isActive;
  final ListNotifier<GameEntity> entities = ListNotifier<GameEntity>();

  late RelayCommand addGameEntity;
  late RelayCommand removeGameEntity;

  Scene({
    required this.name,
    this.isActive = false,
    List<GameEntity> entities = const <GameEntity>[],
  }) {
    this.entities.addAll(entities);

    for (final entity in entities) {
      entity.isActive = isActive;
    }

    addGameEntity = RelayCommand<GameEntity>(
      (x) {
        _addGameEntity(x);
        int index = this.entities.value.length - 1;
        _undoRedo.add(
          UndoRedoAction(
            name: "Add ${x.name.value} to $name",
            undoAction: () => _removeGameEntity(x),
            redoAction: () => _addGameEntity(x, index: index),
          ),
        );
      },
    );

    removeGameEntity = RelayCommand<GameEntity>(
      (x) {
        int index = this.entities.value.indexOf(x);
        _removeGameEntity(x);
        _undoRedo.add(
          UndoRedoAction(
            name: "Remove ${x.name.value}",
            undoAction: () => _addGameEntity(x, index: index),
            redoAction: () => _removeGameEntity(x),
          ),
        );
      },
    );
  }

  factory Scene.fromXML(String xmlStr) {
    xml.XmlDocument document = xml.XmlDocument.parse(xmlStr);

    xml.XmlElement root = document.rootElement;
    final String name = (root.getElement("Name")?.innerText)!;
    final bool isActive =
        (((root.getElement("IsActive")?.innerText)!).toLowerCase() == 'true');
    final xml.XmlElement? entitiesNode = (root.getElement("Entities"));
    final List<GameEntity> entities = <GameEntity>[];

    if (entitiesNode != null) {
      for (final entity in entitiesNode.findElements("GameEntity")) {
        entities.add(GameEntity.fromXML(entity.toString()));
      }
    }

    return Scene(name: name, isActive: isActive, entities: entities);
  }

  String toXML() {
    final builder = xml.XmlBuilder();
    builder.element("Scene", nest: () {
      builder.element("Name", nest: name);
      builder.element("IsActive", nest: isActive.toString());
      builder.element("Entities", nest: () {
        for (final entity in entities.value) {
          builder.xml(entity.toXML());
        }
      });
    });

    return builder.buildDocument().toXmlString(pretty: true);
  }

  _addGameEntity(GameEntity entity, {int index = -1}) {
    entity.isActive = isActive;

    if (index == -1) {
      entities.add(entity);
    } else {
      entities.insert(index, entity);
    }
  }

  _removeGameEntity(GameEntity entity) {
    entity.isActive = isActive;
    entities.remove(entity);
  }
}
