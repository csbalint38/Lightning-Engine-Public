import 'package:collection/collection.dart';
import 'package:editor/components/components.dart';
import 'package:editor/components/transform.dart' as lng;
import 'package:editor/common/list_notifier.dart';
import 'package:editor/dll_wrappers/engine_api.dart';
import 'package:editor/utilities/id.dart';
import 'package:flutter/material.dart';
import 'package:xml/xml.dart' as xml;
import 'package:editor/utilities/math.dart';

class GameEntity {
  final ValueNotifier<String> name = ValueNotifier<String>("");
  final ValueNotifier<bool> isEnabled = ValueNotifier<bool>(false);
  final ValueNotifier<List<Component>> components =
      ValueNotifier<List<Component>>([]);
  int entityId = Id.invalidId;
  bool _isActive = false;

  GameEntity(List<Component>? components, {required name, isEnabled = true}) {
    this.name.value = name;
    this.isEnabled.value = isEnabled;
    if (components == null ||
        !components.any((element) => element is lng.Transform)) {
      components?.add(lng.Transform());
    }
    this.components.value.addAll(components!);
  }

  factory GameEntity.fromXML(String xmlStr) {
    xml.XmlDocument document = xml.XmlDocument.parse(xmlStr);

    xml.XmlElement root = document.rootElement;
    final String name = (root.getElement("Name")?.innerText)!;
    final bool isEnabled = (root.getElement("IsEnabled")?.innerText) == 'true';
    final xml.XmlElement? componentsNode = root.getElement("Components");
    final List<Component> components = <Component>[];

    if (componentsNode != null) {
      for (final child in componentsNode.childElements) {
        switch (child.name.toString()) {
          case 'Transform':
            components.add(lng.Transform.fromXML(child.toString()));
        }
      }
    }

    return GameEntity(components, name: name, isEnabled: isEnabled);
  }

  bool get isActive => _isActive;

  set isActive(bool value) {
    if (_isActive != value) {
      _isActive = value;

      if (_isActive) {
        entityId = EngineAPI.createGameEntity(this);
      } else if (Id.isValid(entityId)) {
        EngineAPI.removeGameEntity(this);
        entityId = Id.invalidId;
      }
    }
  }

  String toXML() {
    final builder = xml.XmlBuilder();

    builder.element("GameEntity", nest: () {
      builder.element("Name", nest: name.value);
      builder.element("IsEnabled", nest: isEnabled.value);
      builder.element("Components", nest: () {
        for (final component in components.value) {
          builder.xml(component.toXML());
        }
      });
    });

    return builder.buildDocument().toXmlString(pretty: true);
  }

  Component? getComponentByRuntimeType(Type type) {
    return components.value
        .firstWhereOrNull((components) => components.runtimeType == type);
  }

  T? getComponent<T extends Component>() {
    return components.value.whereType<T>().firstOrNull;
  }
}

enum GameEntityProperty { name, isEnabled, component }

class MSGameEntity {
  static MSGameEntity? _currentInstance;

  final ValueNotifier<String> name = ValueNotifier<String>("");
  final ValueNotifier<bool?> isEnabled = ValueNotifier<bool?>(null);
  final ListNotifier<MSComponent> components = ListNotifier<MSComponent>();
  final List<GameEntity> selectedEntities;

  bool _enableUpdates = true;

  MSGameEntity(this.selectedEntities) {
    _currentInstance = this;

    name.addListener(() =>
        _enableUpdates ? updateGameEntities(GameEntityProperty.name) : null);
    isEnabled.addListener(() => _enableUpdates
        ? updateGameEntities(GameEntityProperty.isEnabled)
        : null);
    components.addListener(() => _enableUpdates
        ? updateGameEntities(GameEntityProperty.component)
        : null);

    refresh();
  }

  static MSGameEntity? getMSGameEntity() {
    return _currentInstance;
  }

  static U? getMixedValue<T, U>(List<T> objects, U Function(T) getProperty) {
    U value = getProperty(objects.first);

    if (value is double) {
      for (final T item in objects) {
        if (!Math.isNearEqual(value, getProperty(item) as double?)) {
          return null;
        }
      }
    } else {
      for (final T item in objects) {
        if (value != getProperty(item)) {
          return null;
        }
      }
    }

    return value;
  }

  T? getComponent<T extends MSComponent>() {
    return components.value.whereType<T>().firstOrNull;
  }

  bool updateGameEntities(GameEntityProperty prop) {
    switch (prop) {
      case GameEntityProperty.name:
        for (final GameEntity entity in selectedEntities) {
          entity.name.value = name.value;
        }
        return true;
      case GameEntityProperty.isEnabled:
        for (final GameEntity entity in selectedEntities) {
          entity.isEnabled.value = isEnabled.value!;
        }
        return true;
      case GameEntityProperty.component:
        return false;
    }
  }

  void makeComponentsList() {
    components.clear(notify: false);
    GameEntity? firstEntity = selectedEntities.firstOrNull;

    if (firstEntity == null) return;

    for (final component in firstEntity.components.value) {
      final Type type = component.runtimeType;
      if (!selectedEntities
          .any((entity) => entity.getComponentByRuntimeType(type) == null)) {
        components.add(component.getMultiselectComponent(this));
      }
    }
  }

  bool updateMSGameEntity() {
    name.value = getMixedValue<GameEntity, String>(
            selectedEntities, ((x) => x.name.value)) ??
        "";
    isEnabled.value = getMixedValue<GameEntity, bool>(
        selectedEntities, ((x) => x.isEnabled.value));

    return true;
  }

  void refresh() {
    _enableUpdates = false;
    updateMSGameEntity();
    makeComponentsList();
    _enableUpdates = true;
  }

  void dispose() {
    name.dispose();
    isEnabled.dispose();
    components.dispose();

    if (_currentInstance == this) {
      _currentInstance = null;
    }
  }
}
