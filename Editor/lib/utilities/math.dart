class Math {
  static const double epsilon = 0.00001;

  static bool isNearEqual(double? value, double? other) {
    if (value == null || other == null) return false;

    return (value - other).abs() < Math.epsilon;
  }
}
