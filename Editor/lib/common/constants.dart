final RegExp fileInvalidCharacters = RegExp(
    r'^(?!^(?:CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\..+)?$)[^\\/:*?"<>|\r\n]{0,254}[^\\/:*?"<>|\r\n. ]$');
final RegExp pathAllowedCharacters = RegExp(
    r'^(?:[a-zA-Z]:\\|\\)(?:[^\\/:*?"<>|\r\n]+\\)*(?:[^\\/:*?"<>|\r\n]*[^\\/:*?"<>|\r\n. ])?$');
final RegExp strictFileNameCharacters = RegExp(r'^[\w\-. ]+$');

enum BuildConfig {
  debug,
  debugEditor,
  release,
  releaseEditor
}
