import 'package:flutter/material.dart';

class LightningDropdownButton<T> extends DropdownButton<T> {
  final double? height;

  @override
  double? get itemHeight => this.height;

  LightningDropdownButton({
    super.key,
    required super.items,
    super.selectedItemBuilder,
    super.value,
    super.hint,
    super.disabledHint,
    required super.onChanged,
    super.onTap,
    super.elevation = 8,
    super.style,
    super.underline,
    super.icon,
    super.iconDisabledColor,
    super.iconEnabledColor,
    super.iconSize = 24.0,
    super.isDense = false,
    super.isExpanded = false,
    super.itemHeight = kMinInteractiveDimension,
    super.focusColor,
    super.focusNode,
    super.autofocus = false,
    super.dropdownColor,
    super.menuMaxHeight,
    super.enableFeedback,
    super.alignment = AlignmentDirectional.centerStart,
    super.borderRadius,
    super.padding,
    this.height,
  });
}
