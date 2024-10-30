import 'package:flutter/material.dart';
import 'package:vector_math/vector_math.dart';

class Vector3Notifier extends ValueNotifier<Vector3> {
  Vector3Notifier() : super(Vector3.zero());

  double? _x;
  double? _y;
  double? _z;

  double? get x => _x;
  double? get y => _y;
  double? get z => _z;

  set x(double? x) {
    if (x != _x) {
      _x = x;
      if (x != null) value.x = x;
      notifyListeners();
    }
  }

  set y(double? y) {
    if (y != _y) {
      _y = y;
      if (y != null) value.y = y;
      notifyListeners();
    }
  }

  set z(double? z) {
    _z = z;
    if (z != null) value.z = z;
    notifyListeners();
  }

  void setValues(double x, double y, double z) {
    value.setValues(x, y, z);
    notifyListeners();
  }

  void setFrom(Vector3 other) {
    value.setFrom(other);
    notifyListeners();
  }
}
