#pragma once
// =====================================================================
//  ui.h — встроенный веб-интерфейс (PROGMEM, gzip).
//  web_assets.h генерируется автоматически из index.html и dev.html
// =====================================================================
#include <Arduino.h>
#include "web_assets.h"

// ---------------------------------------------------------------------
//  PWA MANIFEST (/manifest.json)
// ---------------------------------------------------------------------
const char MANIFEST_JSON[] PROGMEM = R"JSON({
  "name": "Такси Шашка",
  "short_name": "TaxiLight",
  "start_url": "/",
  "scope": "/",
  "id": "/",
  "display": "standalone",
  "display_override": ["standalone", "minimal-ui", "window-controls-overlay"],
  "background_color": "#0c0c14",
  "theme_color": "#0c0c14",
  "orientation": "portrait-primary",
  "icons": [
    {
      "src": "/icon.png",
      "sizes": "192x192",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon.png",
      "sizes": "192x192",
      "type": "image/png",
      "purpose": "maskable"
    },
    {
      "src": "/icon-512.png",
      "sizes": "512x512",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-512.png",
      "sizes": "512x512",
      "type": "image/png",
      "purpose": "maskable"
    },
    {
      "src": "/icon.svg",
      "sizes": "512x512",
      "type": "image/svg+xml",
      "purpose": "any"
    }
  ]
})JSON";

// ---------------------------------------------------------------------
//  SERVICE WORKER (/sw.js)
// ---------------------------------------------------------------------
const char SW_JS[] PROGMEM = R"JS(
var CACHE_NAME = 'taxilight-v1';
self.addEventListener('install', function(e) { self.skipWaiting(); });
self.addEventListener('activate', function(e) {
  e.waitUntil(
    caches.keys().then(function(keys) {
      return Promise.all(keys.map(function(k) { return caches.delete(k); }));
    }).then(function() { return clients.claim(); })
  );
});
self.addEventListener('fetch', function(e) {
  e.respondWith(
    fetch(e.request).catch(function() {
      return caches.match(e.request);
    })
  );
});
)JS";

// ---------------------------------------------------------------------
//  РАСТРОВЫЕ ИКОНКИ PNG (/icon.png и /icon-512.png)
// ---------------------------------------------------------------------
#include "icon_png.inl"
#include "icon512_png.inl"

// ---------------------------------------------------------------------
//  МИНИМАЛИСТИЧНАЯ ВЕКТОРНАЯ ИКОНКА (/icon.svg)
// ---------------------------------------------------------------------
const char ICON_SVG[] PROGMEM = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
  <style>
    .bg { fill: #0c0c14; }
    .sq-y { fill: #ffb800; }
    .sq-d { fill: #222234; }
    @media (prefers-color-scheme: light) {
      .bg { fill: #f4f4f8; }
      .sq-y { fill: #e69500; }
      .sq-d { fill: #2c2c3e; }
    }
  </style>
  <rect width="512" height="512" class="bg"/>
  <g transform="translate(106, 175)">
    <rect x="0" y="0" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="61" y="0" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="122" y="0" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="183" y="0" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="244" y="0" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="0" y="56" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="61" y="56" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="122" y="56" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="183" y="56" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="244" y="56" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="0" y="112" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="61" y="112" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="122" y="112" width="56" height="50" rx="8" class="sq-y"/>
    <rect x="183" y="112" width="56" height="50" rx="8" class="sq-d"/>
    <rect x="244" y="112" width="56" height="50" rx="8" class="sq-y"/>
  </g>
</svg>
)SVG";
