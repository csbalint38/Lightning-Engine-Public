import 'dart:ffi';
import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as p;
import 'package:editor/congifg/config.dart';
import 'package:editor/components/transform.dart';
import 'package:editor/components/game_entity.dart';
import 'package:editor/utilities/id.dart';

typedef _CreateGameEntityNativeType = IdType Function(
    Pointer<GameEntityDescriptor>);
typedef _CreateGameEntityType = int Function(Pointer<GameEntityDescriptor>);
typedef _RemoveGameEntityNativeType = Void Function(IdType);
typedef _RemoveGameEntityType = void Function(int);

final class TransformComponentDescriptor extends Struct {
  @Array(3)
  external Array<Float> position;

  @Array(3)
  external Array<Float> rotation;

  @Array(3)
  external Array<Float> scale;
}

final class GameEntityDescriptor extends Struct {
  external TransformComponentDescriptor transform;
}

class EngineAPI {
  static const String _dllName =
      "x64/DebugEditor/EngineDll.dll"; // TODO: replace with proper path
  static final String _dllPath = Config().read<String>(ConfigProps.enginePath)!;
  static final DynamicLibrary _engineDll =
      DynamicLibrary.open(p.join(_dllPath, _dllName));

  static final _CreateGameEntityType _createGameEntity = _engineDll
      .lookupFunction<_CreateGameEntityNativeType, _CreateGameEntityType>(
          'create_game_entity');
  static final _RemoveGameEntityType _removeGameEntity = _engineDll
      .lookupFunction<_RemoveGameEntityNativeType, _RemoveGameEntityType>(
          'remove_game_entity');

  static int createGameEntity(GameEntity entity) {
    Pointer<GameEntityDescriptor> desc = malloc<GameEntityDescriptor>();

    {
      Transform c = entity.getComponent<Transform>()!;
      desc.ref.transform.position[0] = c.position.x;
      desc.ref.transform.position[1] = c.position.y;
      desc.ref.transform.position[2] = c.position.z;

      desc.ref.transform.rotation[0] = c.rotation.x;
      desc.ref.transform.rotation[1] = c.rotation.y;
      desc.ref.transform.rotation[2] = c.rotation.z;

      desc.ref.transform.scale[0] = c.scale.x;
      desc.ref.transform.scale[1] = c.scale.y;
      desc.ref.transform.scale[2] = c.scale.z;
    } // Transform component

    return _createGameEntity(desc);
  }

  static void removeGameEntity(GameEntity entity) {
    _removeGameEntity(entity.entityId);
  }

  EngineAPI._();
}
