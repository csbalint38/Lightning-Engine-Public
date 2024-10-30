class RelayCommand<T> {
  final void Function(T) _action;
  final bool Function(T)? _canExecute;

  RelayCommand(this._action, [this._canExecute]);

  bool canExecute(T param) {
    if (_canExecute != null) {
      return _canExecute(param);
    }
    return true;
  }

  void execute(T param) {
    if (canExecute(param)) {
      _action(param);
    }
  }
}
