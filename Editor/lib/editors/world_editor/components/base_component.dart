import 'package:editor/themes/themes.dart';
import 'package:flutter/material.dart';

class BaseComponent extends StatelessWidget {
  final String title;
  final List<Widget> children;

  const BaseComponent({super.key, required this.title, required this.children});

  @override
  Widget build(BuildContext context) {
    return Theme(
      data: ThemeData(
        splashColor: Theme.of(context).primaryColor.withOpacity(0.2),
        hoverColor: Colors.transparent,
        dividerColor: Theme.of(context).primaryColor,
      ),
      child: ListTileTheme(
        tileColor: Theme.of(context).outlineColor,
        child: DecoratedBox(
          decoration: BoxDecoration(
            border: Border.symmetric(
              horizontal: BorderSide(color: Theme.of(context).primaryColor),
            ),
          ),
          child: ExpansionTile(
            controlAffinity: ListTileControlAffinity.leading,
            minTileHeight: 0,
            initiallyExpanded: true,
            iconColor: Theme.of(context).primaryColor,
            collapsedIconColor: Theme.of(context).primaryColor,
            title: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text(
                  title,
                  style: TextStyle(
                    color: Theme.of(context).primaryColor,
                    fontSize: 14,
                  ),
                ),
              ],
            ),
            children: [
              Padding(
                padding: const EdgeInsets.all(4),
                child: Column(
                  children: children,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
