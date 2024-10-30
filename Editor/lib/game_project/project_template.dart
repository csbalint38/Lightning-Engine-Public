import 'dart:io';
import 'package:xml/xml.dart' as xml;
import 'package:path/path.dart' as p;

class ProjectTemplate {
  final Directory _templatesDir = Directory("ProjectTemplates");

  String projectType;
  String projectFile;
  List<String> folders;

  late final Directory templateFolder;
  late String projectName;
  late File icon;
  late File screenshot;
  late String projectFilePath;
  late String iconPath;
  late String screenshotPath;

  ProjectTemplate(
      {required this.projectType,
      required this.projectFile,
      required this.folders}) {
    templateFolder =
        Directory(p.join(_templatesDir.path, projectType));
    projectName = _getProjectName();
    iconPath = p.join(templateFolder.path, 'icon.png');
    screenshotPath = p.join(templateFolder.path, 'screenshot.png');
    projectFilePath = p.join(templateFolder.path, 'project.lightning');
    icon = File(iconPath);
    screenshot = File(screenshotPath);
  }

  factory ProjectTemplate.fromXMLFile(File file) {
    String content = file.readAsStringSync();
    return ProjectTemplate.fromXML(content);
  }

  factory ProjectTemplate.fromXML(String xmlStr) {
    xml.XmlDocument document = xml.XmlDocument.parse(xmlStr);

    final xml.XmlElement root = document.rootElement;
    final String projectType = (root.getElement('ProjectType')?.innerText)!;
    final String projectFile = (root.getElement('ProjectFile')?.innerText)!;
    final xml.XmlElement foldersNode = (root.getElement("Folders"))!;
    final List<String> folders = <String>[];

    for (final folder in foldersNode.findElements("Folder")) {
      folders.add(folder.innerText);
    }

    return ProjectTemplate(
        projectType: projectType, projectFile: projectFile, folders: folders);
  }

  String _getProjectName() {
    return projectType
        .replaceAllMapped(
            RegExp(r'([A-Z])'), (Match match) => ' ${match.group(0)}')
        .trim();
  }
}
