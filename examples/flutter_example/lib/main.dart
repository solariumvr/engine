import 'package:flutter/material.dart';
import 'dart:developer';
void main() {
  runApp(new MaterialApp(
      title: 'Flutter Tutorial',
      home: new TutorialHome(),
      ));
}

class TutorialHome extends StatefulWidget {
  @override
  _TutorialHome createState() => new _TutorialHome();
}

class _TutorialHome extends State<TutorialHome> {
  InputValue inputValue = new InputValue(text: 'my text');

  @override
  Widget build(BuildContext context) {
    return new Scaffold(
        appBar: new AppBar(
            leading: new IconButton(
                icon: new Icon(Icons.menu),
                tooltip: 'Navigation menu',
                onPressed: null,
                ),
            title: new Text('Example title'),
            actions: <Widget>[
              new IconButton(
                  icon: new Icon(Icons.search),
                  tooltip: 'Search',
                  onPressed: null,
                  ),
            ],
            ),
        body: new Center(child: new Input(
            value: inputValue,
            labelText: 'INDIRIZZO',
            onChanged: (InputValue newInputValue) {
              setState(() {
                inputValue = newInputValue;
              });
            })),
        floatingActionButton: new FloatingActionButton(
            tooltip: 'Add',
            child: new Icon(Icons.add),
            onPressed: (){
              log("Pressed");
            }
            ),
        );
  }
}