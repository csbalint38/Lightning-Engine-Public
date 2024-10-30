import 'dart:async';
import 'dart:ffi';
import 'dart:isolate';
import 'package:editor/common/constants.dart';
import 'package:editor/game_project/project.dart';
import 'package:editor/utilities/capitalize.dart';
import 'package:ffi/ffi.dart';
import 'package:flutter/material.dart';
import 'package:path/path.dart' as p;
import 'package:editor/congifg/config.dart';

typedef _OpenVisualStudioNativeType = Void Function(Pointer<Utf16>);
typedef _OpenVisualStudioType = void Function(Pointer<Utf16>);
typedef _CloseVisualStudioNativeType = Void Function();
typedef _CloseVisualStudioType = void Function();
typedef _AddFilesNativeType = Bool Function(
    Pointer<Utf16>, Pointer<Utf16>, Pointer<Pointer<Utf16>>, Int32);
typedef _AddFilesType = bool Function(
    Pointer<Utf16>, Pointer<Utf16>, Pointer<Pointer<Utf16>>, int);
typedef _BuildSolutionNativeType = Void Function(
    Pointer<Utf16>, Pointer<Utf16>, Bool);
typedef _BuildSolutionType = void Function(
    Pointer<Utf16>, Pointer<Utf16>, bool);
typedef _GetLastBuildInfoNativeType = Bool Function();
typedef _GetLastBuildInfoType = bool Function();

final class VisualStudio {
  static const String _dllName =
      "x64/DebugEditor/vsidll.dll"; // TODO: replace with proper path
  static final String _dllPath = Config().read<String>(ConfigProps.enginePath)!;
  static final DynamicLibrary _vsiDll =
      DynamicLibrary.open(p.join(_dllPath, _dllName));

  static final _OpenVisualStudioType _openVisualStudio = _vsiDll.lookupFunction<
      _OpenVisualStudioNativeType, _OpenVisualStudioType>('open_visual_studio');
  static final _CloseVisualStudioType _closeVisualStudio = _vsiDll
      .lookupFunction<_CloseVisualStudioNativeType, _CloseVisualStudioType>(
          'close_visual_studio');
  static final _AddFilesType _addFiles =
      _vsiDll.lookupFunction<_AddFilesNativeType, _AddFilesType>('add_files');
  static final _BuildSolutionType _buildSolution =
      _vsiDll.lookupFunction<_BuildSolutionNativeType, _BuildSolutionType>(
          'build_solution');
  static final _GetLastBuildInfoType _getLastBuildInfo = _vsiDll.lookupFunction<
      _GetLastBuildInfoNativeType,
      _GetLastBuildInfoType>('get_last_build_info');

  static bool isDebugging = false;
  static bool buildDone = true;
  static bool buildSucceeded = false;

  static Future<void> openVisualStudio(String solutionPath) async {
    await _runInIsolate((String solutionPath) {
      final Pointer<Utf16> solutionPathPtr = solutionPath.toNativeUtf16();
      _openVisualStudio(solutionPathPtr);
      calloc.free(solutionPathPtr);
    }, [solutionPath]);
  }

  static Future<void> closeVisualStudio() async {
    await _runInIsolate(_closeVisualStudio, List.empty());
  }

  static Future<bool> addFiles(String solutionPath, List<String> files) async {
    final String normalizedSolutionPath =
        solutionPath.replaceAll('/', '\\').replaceAll(r'\', r'\\');

    return await _runInIsolate<bool>((String solutionPath, List<String> files) {
      final Pointer<Utf16> normalizedSolutionPathPtr =
          solutionPath.toNativeUtf16();
      final Pointer<Utf16> projectPtr =
          solutionPath.split('\\').last.split('.').first.toNativeUtf16();

      final Pointer<Pointer<Utf16>> filesPointers =
          calloc<Pointer<Utf16>>(files.length);

      for (int i = 0; i < files.length; i++) {
        filesPointers[i] = files[i].toNativeUtf16();
      }

      final result = _addFiles(
          normalizedSolutionPathPtr, projectPtr, filesPointers, files.length);

      calloc.free(normalizedSolutionPathPtr);
      calloc.free(projectPtr);

      for (int i = 0; i < files.length; i++) {
        calloc.free(filesPointers[i]);
      }

      calloc.free(filesPointers);

      return result;
    }, [normalizedSolutionPath, files]);
  }

  static Future<void> buildSolution(
      Project project, BuildConfig config, bool showWindow) async {
    buildSucceeded = false;
    buildDone = false;
    isDebugging = true;

    final String solutionName =
        project.solution.replaceAll('/', '\\').replaceAll(r'\', r'\\');
    final String configName = capitalize(config.toString().split('.').last);

    await _runInIsolate(
        (String solutionPath, String configName, bool showWindow) {
      final Pointer<Utf16> solutionNamePtr = solutionPath.toNativeUtf16();
      final Pointer<Utf16> configNamePointer = configName.toNativeUtf16();

      _buildSolution(solutionNamePtr, configNamePointer, showWindow);

      calloc.free(solutionNamePtr);
      calloc.free(configNamePointer);
    }, [solutionName, configName, showWindow]);

    buildDone = true;
    isDebugging = false;
  }

  static Future<bool> getLastBuildInfo() async {
    return await _runInIsolate<bool>(() {
      return _getLastBuildInfo();
    }, List.empty());
  }

  static Future<T> _runInIsolate<T>(Function task, List<dynamic> args) async {
    final ReceivePort port = ReceivePort();
    final ReceivePort exitPort = ReceivePort();

    await Isolate.spawn(
        _isolateEntry, [task, args, port.sendPort, exitPort.sendPort]);

    exitPort.listen((_) {
      exitPort.close();
    });

    return await port.first as T;
  }

  static void _isolateEntry(List<dynamic> args) {
    final Function task = args[0];
    final List<dynamic> funcArgs = args[1];
    final SendPort port = args[2];

    final result = Function.apply(task, funcArgs);
    port.send(result);
  }
}
