import 'dart:ffi';

typedef IdType = Uint32;

final class Id {
  static int invalidId = -1;
  static bool isValid(int id) => id != invalidId;

  Id._();
}
