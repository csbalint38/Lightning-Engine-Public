import 'package:editor/game_project/controllers/open_project_controller.dart';
import 'package:editor/game_project/new_project.dart';
import 'package:editor/game_project/open_project.dart';
import 'package:flutter/material.dart';

class ProjectBrowserDialog extends StatefulWidget {
  const ProjectBrowserDialog({super.key});

  @override
  State<ProjectBrowserDialog> createState() => _ProjectBrowserDialogState();
}

class _ProjectBrowserDialogState extends State<ProjectBrowserDialog> {
  @override
  Widget build(BuildContext context) {
    if (OpenProjectController().projects.isEmpty) {
      return const NewProject();
    }
    return const OpenProject();
  }
}
