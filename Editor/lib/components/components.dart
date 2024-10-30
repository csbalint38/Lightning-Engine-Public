import 'package:editor/components/game_entity.dart';

abstract class Component {
  factory Component.fromXML() {
    throw UnimplementedError();
  }

  String toXML() {
    throw UnimplementedError();
  }

  MSComponent getMultiselectComponent(MSGameEntity msEntity);
}

abstract class MSComponent<T extends Component> {
  late final List<T> selectedComponents;
  bool enableUpdates = true;
  late Enum properties;

  bool updateComponents(dynamic propertyName);
  bool updateMSComponent();

  void refresh() {
    enableUpdates = false;
    updateMSComponent();
    enableUpdates = true;
  }

  MSComponent(MSGameEntity msEntity) {
    selectedComponents = msEntity.selectedEntities
        .where((entity) => entity.getComponent<T>() != null)
        .map((entity) => entity.getComponent<T>()!)
        .toList();
  }
}
