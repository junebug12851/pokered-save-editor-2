// Progressive enhancement for the site header navigation.
// The menu works without JS (native <details>); this just makes it nicer:
//  - mobile hamburger toggles the nav
//  - only one dropdown open at a time
//  - click outside or press Escape to close dropdowns
(function () {
  "use strict";
  var header = document.querySelector(".site-header");
  if (!header) return;

  var toggle = header.querySelector(".nav-toggle");
  var nav = header.querySelector(".nav");
  var dropdowns = Array.prototype.slice.call(header.querySelectorAll("details.dd"));

  // Mobile hamburger
  if (toggle && nav) {
    toggle.addEventListener("click", function () {
      var open = header.classList.toggle("nav-open");
      toggle.setAttribute("aria-expanded", open ? "true" : "false");
    });
  }

  // Only one dropdown open at a time
  dropdowns.forEach(function (d) {
    d.addEventListener("toggle", function () {
      if (d.open) {
        dropdowns.forEach(function (other) {
          if (other !== d) other.open = false;
        });
      }
    });
  });

  // Close dropdowns on outside click
  document.addEventListener("click", function (e) {
    if (!header.contains(e.target)) {
      dropdowns.forEach(function (d) { d.open = false; });
    }
  });

  // Close on Escape
  document.addEventListener("keydown", function (e) {
    if (e.key === "Escape") {
      dropdowns.forEach(function (d) { d.open = false; });
      if (header.classList.contains("nav-open")) {
        header.classList.remove("nav-open");
        if (toggle) toggle.setAttribute("aria-expanded", "false");
      }
    }
  });
})();
