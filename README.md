## ContentTools
DLL, amit az engine használ FBX modellek és textúrák importálására és feldolgozására.
Használatához a *packages/FBX SDK* mappába kell másolni egy érvényes FBX SDK verziót.

## Editor
Flutter Editor. Lib mappában van a lényeg, a többi generált.
### Editor/lib
**components** - ebben vannak a komponensek, amiket az engine-ben is létre lehet hozni.
**dll_wrappers** - EngineDLL és VisualStudioDLL-eket hívó Dart függvények
**editors/world_editor** - Ebben vannak a UI Widgetek és a fő ablak controllere
**game_code** - kód template-ek és a kód generáló widget és controllere
**game_project** - főmenü UI Widgetek és főmenü kontrollerek illetve a modellek
**themes** - kiszervezett UI téma hogy később lehessen dark mode

## Engine
StaticLibrary. DirectX12 renderer van benne és egyébként minden, amit az engine csinál.

## EngineDLL
DLL, amin keresztül az engine függvényei meg vannak hívva.

## EngineTest
Egy teszt program, amin meg lehet nézni mit tud az engine.
Le kell bildeni az Engine projektet Debug módban és az EngineDLL projektet DebugEditor és utána futtatható.

## VisualStudioDLL
VisualStudio COM Objectet kezelő dll.

## VisualStudioDLLTest
Tesztprogram a VisualStudioDLL-hez

**Kell hozzá: Windows 10, olyan DirectX12 verzió, ami támogatja a shader_model 5_6-ot vagy újabbat, VisualStudio2022**

## Források
https://www.youtube.com/@GameEngineSeries ->  A renderer rész és a ContentTools innen van
https://www.youtube.com/@TheCherno        ->  A komponensek és az Id kezelés meg még pár apróság
https://github.com/turanszkij/WickedEngine->  Shaderek, legfőképp a compute shaderek vannak innen

*This software contains Autodesk® FBX® code developed by Autodesk, Inc. Copyright 2008 Autodesk, Inc. All rights, reserved.*