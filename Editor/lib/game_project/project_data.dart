import 'dart:io';
import 'package:xml/xml.dart' as xml;

class ProjectData {
  final String projectName;
  final String projectPath;
  DateTime lastModified;
  final engineVersion = "0.2.0";
  late final String fullPath;
  late File icon;
  late File screenshot;

  ProjectData(
      {required this.projectName,
      required this.projectPath,
      required this.lastModified}) {
    fullPath = '$projectPath\\$projectName';
  }

  factory ProjectData.fromXML(String xmlString) {
    xml.XmlDocument document = xml.XmlDocument.parse(xmlString);

    xml.XmlElement root = document.rootElement;
    final String name = (root.getElement("Name")?.innerText)!;
    final String path = (root.getElement("Path")?.innerText)!;
    final DateTime lastModified =
        DateTime.parse((root.getElement("LastModified")?.innerText)!);

    return ProjectData(
        projectName: name, projectPath: path, lastModified: lastModified);
  }

  String toXML() {
    final builder = xml.XmlBuilder();

    builder.element("Project", nest: () {
      builder.element("Name", nest: projectName);
      builder.element("Path", nest: projectPath);
      builder.element("LastModified", nest: lastModified.toString());
    });

    return builder.buildDocument().toXmlString(pretty: true);
  }
}

class ProjectsData {
  final List<ProjectData> projects;

  ProjectsData({required this.projects});

  factory ProjectsData.fromXMLFile(File file) {
    String content = file.readAsStringSync();
    return ProjectsData.fromXML(content);
  }

  factory ProjectsData.fromXML(String xmlString) {
    xml.XmlDocument document = xml.XmlDocument.parse(xmlString);

    xml.XmlElement root = document.rootElement;
    final List<ProjectData> projects = <ProjectData>[];

    for (final project in root.findElements("Project")) {
      projects.add(ProjectData.fromXML(project.toString()));
    }

    return ProjectsData(projects: projects);
  }

  String toXml() {
    final builder = xml.XmlBuilder();

    builder.element("Projects", nest: () {
      for (final project in projects) {
        builder.xml(project.toXML());
      }
    });

    return builder.buildDocument().toXmlString(pretty: true);
  }

  void toXMLFile(String path) {
    final String xmlString = toXml();
    final File file = File(path);
    file.writeAsStringSync(xmlString);
  }
}
