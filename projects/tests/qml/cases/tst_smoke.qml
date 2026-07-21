/*
  * Copyright 2026 Fairy Fox
  * Licensed under the Apache License, Version 2.0 (the "License").
*/

// Phase-10 QML smoke test: proves the Qt Quick Test harness builds and runs the
// QML engine headless (offscreen). Self-contained -- no `brg` context. Screen-level
// tests are future work (need the Bridge/DB booted into the engine).

import QtQuick
import QtTest

TestCase {
    name: "QmlSmoke"

    function test_arithmetic() {
        compare(1 + 1, 2)
    }

    function test_string() {
        var s = "PIKA"
        compare(s.length, 4)
        compare(s.toLowerCase(), "pika")
    }

    property int counter: 0
    function test_property_assignment() {
        counter = 7
        compare(counter, 7)
    }

    Item {
        id: box
        width: 10
        height: 4
    }
    function test_item_properties() {
        verify(box !== null)
        compare(box.width * box.height, 40)
    }
}
