import 'package:flutter/material.dart';

class ListNotifier<T> extends ValueNotifier<List<T>> {
  ListNotifier() : super([]);

  void add(T item, {bool notify = true}) {
    value.add(item);
    notify ? notifyListeners() : null;
  }

  void addAll(Iterable<T> items, {bool notify = true}) {
    value.addAll(items);
    notify ? notifyListeners() : null;
  }

  void insert(int index, T item, {bool notify = true}) {
    value.insert(index, item);
    notify ? notifyListeners() : null;
  }

  bool remove(T item, {bool notify = true}) {
    final bool wasRemoved = value.remove(item);
    if (wasRemoved && notify) {
      notifyListeners();
    }

    return wasRemoved;
  }

  T removeAt(int index, {bool notify = true}) {
    final removedItem = value.removeAt(index);
    notify ? notifyListeners() : null;

    return removedItem;
  }

  void clear({bool notify = true}) {
    value.clear();
    notify ? notifyListeners() : null;
  }
}
