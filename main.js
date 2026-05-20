"use strict";

var vertexShaderSource = `#version 300 es

in vec4 a_position;
in vec3 a_normal;
in vec2 a_texcoord;

uniform vec3 u_lightWorldPosition;
uniform vec3 u_viewWorldPosition;

uniform mat4 u_world;
uniform mat4 u_worldViewProjection;
uniform mat4 u_worldInverseTranspose;

out vec2 v_texCoord;
out vec3 v_normal;
out vec3 v_surfaceToLight;
out vec3 v_surfaceToView;

void main() {
  v_texCoord = a_texcoord;

  gl_Position = u_worldViewProjection * a_position;

  v_normal = mat3(u_worldInverseTranspose) * a_normal;

  vec3 surfaceWorldPosition = (u_world * a_position).xyz;
  v_surfaceToLight = u_lightWorldPosition - surfaceWorldPosition;
  v_surfaceToView = u_viewWorldPosition - surfaceWorldPosition;
}
`;

var fragmentShaderSource = `#version 300 es

precision highp float;

in vec2 v_texCoord;
in vec3 v_normal;
in vec3 v_surfaceToLight;
in vec3 v_surfaceToView;

uniform float u_shininess;
uniform vec3 u_lightColor;
uniform vec3 u_specularColor;
uniform vec3 u_ambientLight;

uniform sampler2D u_texture;
uniform int u_useTexture;
uniform vec4 u_color;
uniform int u_shadowPass;
uniform int u_emissive;

out vec4 outColor;

void main() {
  if (u_shadowPass == 1) {
    outColor = vec4(0.0, 0.0, 0.0, 0.9);
    return;
  }

  if (u_emissive == 1) {
    vec4 c = u_useTexture == 1 ? texture(u_texture, v_texCoord) : u_color;
    outColor = vec4(c.rgb, c.a);
    return;
  }

  vec3 normal = normalize(v_normal);
  vec3 surfaceToLightDirection = normalize(v_surfaceToLight);
  vec3 surfaceToViewDirection = normalize(v_surfaceToView);
  vec3 halfVector = normalize(surfaceToLightDirection + surfaceToViewDirection);

  float light = max(dot(normal, surfaceToLightDirection), 0.0);
  float specular = 0.0;
  if (light > 0.0) {
    specular = pow(max(dot(normal, halfVector), 0.0), u_shininess);
  }

  vec3 diffuseLight = light * u_lightColor;
  vec3 specularLight = specular * u_specularColor;

  vec4 baseColor = u_color;
  if (u_useTexture == 1) {
    vec4 texColor = texture(u_texture, v_texCoord);
    baseColor = vec4(texColor.rgb, texColor.a * u_color.a);
  }

  outColor.rgb = baseColor.rgb * (u_ambientLight + diffuseLight) + specularLight;
  outColor.a = baseColor.a;
}
`;

// --- Variabile Globale (Lumina si Camera) ---
var shininess = 128;
var lightColor = [1.0, 0.95, 0.89];
var lightLocation = [0.0, 475.0, 0.0];

var cameraPos   = [0, 150, 0];
var cameraFront = [0, 0, -1];
var cameraUp    = [0, 1, 0];
var cameraSpeed = 2.0;

var yaw = -90.0; 
var pitch = 0.0;
var rotSpeed = 0.7;
var keys = {};

window.addEventListener('keydown', function(e) { keys[e.key.toLowerCase()] = true; });
window.addEventListener('keyup', function(e) { keys[e.key.toLowerCase()] = false; });

// Array-ul global de obiecte in scena. Fiecare obiect are forma:
// { bufferInfo, vao, worldMatrix, material, transparent? }
var sceneObjects = [];

async function main() {
  var canvas = document.querySelector("#canvas");
  var gl = canvas.getContext("webgl2");
  if (!gl) return;

  // twgl.primitives produce buffere cu nume default (position/normal/texcoord).
  // Cu acest prefix, twgl creeaza atribute cu nume a_position/a_normal/a_texcoord.
  twgl.setAttributePrefix("a_");

  var programInfo = twgl.createProgramInfo(gl, [vertexShaderSource, fragmentShaderSource]);

  // --- Texturi ---
  var floorTexture = twgl.createTexture(gl, {
    src: "/OpenGL_proj/Textures/wooden_floor.png",
    wrap: gl.REPEAT,
    min: gl.LINEAR_MIPMAP_LINEAR,
    mag: gl.LINEAR,
  });
  var couchTexture = twgl.createTexture(gl, {
    src: "/OpenGL_proj/Textures/sofa_fabric.jpg",
    min: gl.LINEAR_MIPMAP_LINEAR,
    mag: gl.LINEAR,
  });
  var armchairTexture = twgl.createTexture(gl, {
    src: "/OpenGL_proj/Textures/armchair_texture.jpg",
    min: gl.LINEAR_MIPMAP_LINEAR,
    mag: gl.LINEAR,
  });
  var chandelierTexture = twgl.createTexture(gl, {
    src: "/OpenGL_proj/Textures/lamp_cover.jpg",
    wrap: gl.REPEAT,
    min: gl.LINEAR_MIPMAP_LINEAR,
    mag: gl.LINEAR,
  });
  var radioTexture = twgl.createTexture(gl, {
    src: "/Models/radio/textures/radio_hires.png",
    flipY: 1,
    min: gl.LINEAR_MIPMAP_LINEAR,
    mag: gl.LINEAR,
  });

  // --- Textura video pentru ecranul TV ---
  // Placeholder 1x1 negru pana ce primul frame e gata. Updatat in drawScene().
  var tvVideoElement = document.getElementById("tvVideo");
  var tvVideoTexture = twgl.createTexture(gl, {
    src: [0, 0, 0, 255],
    min: gl.LINEAR,
    mag: gl.LINEAR,
    wrap: gl.CLAMP_TO_EDGE,
  });
  // Porneste video la prima interactiune (daca autoplay a fost blocked de browser).
  function tryPlayVideo() { tvVideoElement.play().catch(function () {}); }
  tryPlayVideo();
  document.addEventListener("click",   tryPlayVideo, { once: true });
  document.addEventListener("keydown", tryPlayVideo, { once: true });

  // --- Camera (podea texturata, tavan, pereti cu fereastra) ---
  createRoomMeshes(gl, programInfo, floorTexture).forEach(function (m) { sceneObjects.push(m); });

  // --- 1. MASA DE CAFEA (OBJ) ---
  var tableMesh = await loadObjAsMesh(gl, programInfo, "/OpenGL_proj/Table2.obj", {
    u_useTexture: 0,
    u_texture: floorTexture,
    u_color: [0.35, 0.2, 0.1, 1.0],
    u_shininess: 16,
    u_specularColor: [0.1, 0.08, 0.05],
  });
  // Transform: in OpenGL (Z-up) era translate(0,0,27) * rotateX(pi/2) * scale(60)
  // In WebGL (Y-up), OBJ-ul Blender e deja Y-up, deci doar translate(0,27,0) * scale(60)
  tableMesh.worldMatrix = m4.scale(m4.translation(0, 27, 0), 60, 60, 60);
  tableMesh.castShadow = true;
  sceneObjects.push(tableMesh);

  // --- 2. CANAPEA + 2 FOTOLII (OBJ + texturi) ---
  // Remap OpenGL (x_o, y_o, z_o) -> WebGL (y_o, z_o, x_o). Skipuim rotateX(PI/2) local
  // (OBJ-urile sunt deja Y-up in WebGL). Pastram doar rotatiile in jurul verticalei.

  // Canapea: OpenGL translate(0, 300, 0) * rotateX(PI/2) * scale(25)
  // -> WebGL translate(300, 0, 0) * scale(25). Langa peretele drept.
  var couchMesh = await loadObjAsMesh(gl, programInfo, "/OpenGL_proj/couch2.obj", {
    u_useTexture: 1,
    u_texture: couchTexture,
    u_color: [1.0, 1.0, 1.0, 1.0],
    u_shininess: 4,
    u_specularColor: [0.1, 0.1, 0.1],
  });
  var couchMat = m4.translation(300, 0, 0);
  couchMat = m4.yRotate(couchMat, degToRad(-90));
  couchMat = m4.scale(couchMat, 25, 25, 25);
  couchMesh.worldMatrix = couchMat;
  couchMesh.castShadow = true;
  sceneObjects.push(couchMesh);

  // Fotoliu 1: OpenGL translate(380, 250, 0) * rotateX(PI/2) * rotateY(-PI/4) * scale(18)
  // -> WebGL translate(250, 0, 380) * rotateY(-PI/4) * scale(18). Coltul din spate-dreapta.
  var armchair1Mesh = await loadObjAsMesh(gl, programInfo, "/OpenGL_proj/Armchair2.obj", {
    u_useTexture: 1,
    u_texture: armchairTexture,
    u_color: [1.0, 1.0, 1.0, 1.0],
    u_shininess: 4,
    u_specularColor: [0.1, 0.1, 0.1],
  });
  var m1 = m4.translation(250, 0, 380);
  m1 = m4.yRotate(m1, degToRad(225));
  m1 = m4.scale(m1, 18, 18, 18);
  armchair1Mesh.worldMatrix = m1;
  armchair1Mesh.castShadow = true;
  sceneObjects.push(armchair1Mesh);

  // Fotoliu 2: OpenGL translate(380, -250, 0) * rotateX(PI/2) * rotateY(-3*PI/4) * scale(18)
  // -> WebGL translate(-250, 0, 380) * rotateY(-3*PI/4) * scale(18). Coltul din spate-stanga.
  var armchair2Mesh = await loadObjAsMesh(gl, programInfo, "/OpenGL_proj/Armchair2.obj", {
    u_useTexture: 1,
    u_texture: armchairTexture,
    u_color: [1.0, 1.0, 1.0, 1.0],
    u_shininess: 4,
    u_specularColor: [0.1, 0.1, 0.1],
  });
  var m2 = m4.translation(-250, 0, 380);
  m2 = m4.yRotate(m2, degToRad(135));
  m2 = m4.scale(m2, 18, 18, 18);
  armchair2Mesh.worldMatrix = m2;
  armchair2Mesh.castShadow = true;
  sceneObjects.push(armchair2Mesh);

  // --- 3. TV (body + screen) ---
  // OpenGL TV body: translate(0, -380, 250) * rotateX(-PI/2) * rotateZ(PI) * scale(2.5)
  // Remap pozitie: WebGL (-380, 250, 0). Lipit de peretele stang (X=-400), mid-height.
  // Rotatia rotateY(90) orienteaza ecranul sa priveasca spre +X (in camera).

  // TV body - culoare solida neagra
  var tvBodyMesh = await loadObjAsMesh(gl, programInfo, "/OpenGL_proj/tv_body2.obj", {
    u_useTexture: 0,
    u_texture: floorTexture, //Placeholder
    u_color: [0.05, 0.05, 0.05, 1.0],
    u_shininess: 64,
    u_specularColor: [0.3, 0.3, 0.3],
  });
  var tvMat = m4.translation(-380, 250, 0);
  tvMat = m4.yRotate(tvMat, degToRad(90));
  tvMat = m4.scale(tvMat, 2.5, 2.5, 2.5);
  tvBodyMesh.worldMatrix = tvMat;
  sceneObjects.push(tvBodyMesh);

  // TV screen OBJ contine 2 sub-obiecte: "tv-screen_Cube.003" (fata cu UV 0..1 pentru video)
  // si "tv_Cube.001" (bezelul/rama). Le incarcam separat ca sa aplicam materiale diferite.
  var screenMat = m4.translation(-380, 250, 0);
  screenMat = m4.yRotate(screenMat, degToRad(90));
  screenMat = m4.translate(screenMat, 0, 0, 0.5);
  screenMat = m4.scale(screenMat, 2.5, 2.5, 2.5);

  // Ecranul propriu-zis: textura video, emissive (sare peste lighting ca sa para aprins).
  var tvScreenFaceMesh = await loadObjSubMesh(gl, programInfo, "/OpenGL_proj/tv_screen.obj",
    "tv-screen_Cube.003", {
      u_useTexture: 1,
      u_texture: tvVideoTexture,
      u_color: [1.0, 1.0, 1.0, 1.0],
      u_shininess: 1,
      u_specularColor: [0.0, 0.0, 0.0],
      u_emissive: 1,
    });
  tvScreenFaceMesh.worldMatrix = screenMat;
  sceneObjects.push(tvScreenFaceMesh);

  // Bezelul (rama TV-ului): culoare solida neagra, iluminare normala.
  var tvBezelMesh = await loadObjSubMesh(gl, programInfo, "/OpenGL_proj/tv_screen.obj",
    "tv_Cube.001", {
      u_useTexture: 0,
      u_texture: floorTexture, //Placeholder
      u_color: [0.05, 0.05, 0.05, 1.0],
      u_shininess: 64,
      u_specularColor: [0.3, 0.3, 0.3],
    });
  tvBezelMesh.worldMatrix = screenMat;
  sceneObjects.push(tvBezelMesh);

  // --- 4. LUSTRA (con) + BEC + VAZA ---
  // Con lustra procedural (twgl.primitives): raza jos 18, apex sus, inaltime 37.
  // OpenGL: translate(0, 0, 485) -> WebGL (0, 485, 0). Chiar sub tavan.
  // topCap=false, bottomCap=false -> cilindru/con deschis (fara capace), ca becul sa fie vizibil.
  var coneBufferInfo = twgl.primitives.createTruncatedConeBufferInfo(gl, 18, 0, 37, 60, 1, false, false);
  var coneMesh = {
    bufferInfo: coneBufferInfo,
    vao: twgl.createVAOFromBufferInfo(gl, programInfo, coneBufferInfo),
    worldMatrix: m4.translation(0, 500, 0),
    material: {
      u_useTexture: 1,
      u_texture: chandelierTexture,
      u_color: [1.0, 1.0, 1.0, 1.0],
      u_shininess: 8,
      u_specularColor: [0.1, 0.1, 0.1],
    },
  };
  sceneObjects.push(coneMesh);

  // Bec (transparent, galbui). OpenGL: translate(0,0,500) * rotateX(3*PI/2) * scale(0.4).
  // rotateX(PI)rotateX in WebGL si translate(0,500,0).
  var bulbMesh = await loadObjAsMesh(gl, programInfo, "/OpenGL_proj/lightbulb2.obj", {
    u_useTexture: 0,
    u_texture: floorTexture,
    u_color: [1.0, 1.0, 0.7, 0.7],
    u_shininess: 128,
    u_specularColor: [1.0, 1.0, 0.9],
  });
  var bulbMat = m4.translation(0, 500, 0);
  bulbMat = m4.xRotate(bulbMat, degToRad(180));
  bulbMat = m4.scale(bulbMat, 0.4, 0.4, 0.4);
  bulbMesh.worldMatrix = bulbMat;
  bulbMesh.transparent = true;
  sceneObjects.push(bulbMesh);

  // Vaza pe masa (transparent, sticla albastruie). OpenGL translate(0,0,55)*rotateX(PI/2)*scale(6.5).
  var vaseMesh = await loadObjAsMesh(gl, programInfo, "/OpenGL_proj/vase3_2.obj", {
    u_useTexture: 0,
    u_texture: floorTexture,
    u_color: [1.0, 0.45, 0.55, 0.75],
    u_shininess: 128,
    u_specularColor: [1.0, 0.9, 0.9],
  });
  vaseMesh.worldMatrix = m4.scale(m4.translation(0, 55, 0), 6.5, 6.5, 6.5);
  vaseMesh.transparent = true;
  sceneObjects.push(vaseMesh);

  //----- 5. RADIO pe masa de cafea
  var radioMesh = await loadObjAsMesh(gl, programInfo, "/Models/radio/source/radio/radio.obj", {
    u_useTexture: 1,
    u_texture: radioTexture,
    u_color: [1.0, 1.0, 1.0, 1.0],
    u_shininess: 32,
    u_specularColor: [0.2, 0.2, 0.2],
  });
  var radioMat = m4.translation(-20, 50, 20);
  radioMat = m4.yRotate(radioMat, degToRad(-30));
  radioMat = m4.scale(radioMat, 200, 200, 200);
  radioMesh.worldMatrix = radioMat;
  radioMesh.castShadow = true;
  sceneObjects.push(radioMesh);

  // --- Audio PD (WebPd) ---
  var radioWorldPos = [-20, 50, 20];

  var audioPanel = document.getElementById("audioPanel");
  var radioOnBtn = document.getElementById("radioOn");
  var radioOffBtn = document.getElementById("radioOff");
  var tvOnBtn = document.getElementById("tvOn");
  var tvOffBtn = document.getElementById("tvOff");
  var masterVolumeInput = document.getElementById("masterVolume");
  var masterVolumeLabel = document.getElementById("masterVolumeLabel");
  var musicBar = document.getElementById("musicBar");
  var musicValue = document.getElementById("musicValue");
  var staticBar = document.getElementById("staticBar");
  var staticValue = document.getElementById("staticValue");
  var tvBar = document.getElementById("tvBar");
  var tvValue = document.getElementById("tvValue");

  var userVolume = 1.0;
  var radioIsOn = false;
  var tvIsOn = false;

  var radioMusicPatch = null;
  var radioStaticPatch = null;
  var tvMusicPatch = null;
  var radioMusicLoaded = false;
  var radioStaticLoaded = false;
  var tvMusicLoaded = false;
  var pdStarted = false;
  var lastDistNorm = -1;
  var lastTvDistNorm = -1;
  var lastSentDistNorm = 0;
  var lastSentTvDistNorm = 0;
  var radioMusicPending = [];
  var radioStaticPending = [];
  var tvMusicPending = [];

  function flushRadioMusicPending() {
    while (radioMusicPending.length > 0) {
      var msg = radioMusicPending.shift();
      Pd.send(msg.name, msg.value);
    }
  }

  function flushRadioStaticPending() {
    while (radioStaticPending.length > 0) {
      var msg = radioStaticPending.shift();
      Pd.send(msg.name, msg.value);
    }
  }

  function flushTvMusicPending() {
    while (tvMusicPending.length > 0) {
      var msg = tvMusicPending.shift();
      Pd.send(msg.name, msg.value);
    }
  }

  function sendToPd(name, value) {
    if (typeof Pd === 'undefined') return;
    if (name.indexOf('radio') === 0 || name === 'dist_norm') {
      if (radioMusicLoaded) { Pd.send(name, value); }
      else { radioMusicPending.push({ name: name, value: value }); }
      if (radioStaticLoaded) { Pd.send(name, value); }
      else { radioStaticPending.push({ name: name, value: value }); }
    }
    if (name.indexOf('tv') === 0) {
      if (tvMusicLoaded) { Pd.send(name, value); }
      else { tvMusicPending.push({ name: name, value: value }); }
    }
    if (name === 'master_vol') {
      if (radioMusicLoaded) { Pd.send(name, value); }
      else { radioMusicPending.push({ name: name, value: value }); }
      if (radioStaticLoaded) { Pd.send(name, value); }
      else { radioStaticPending.push({ name: name, value: value }); }
      if (tvMusicLoaded) { Pd.send(name, value); }
      else { tvMusicPending.push({ name: name, value: value }); }
    }
  }

  function startPd() {
    if (pdStarted) return;
    if (typeof Pd === 'undefined') {
      console.error("WebPd not loaded. Check lib/webpd.min.js");
      return;
    }
    Pd.start();
    pdStarted = true;

    fetch('/Sound_Patches/radio_music.pd')
      .then(function(r) { if (!r.ok) throw new Error('radio_music.pd not found'); return r.text(); })
      .then(function(text) {
        try {
          radioMusicPatch = Pd.loadPatch(text);
          radioMusicLoaded = true;
          flushRadioMusicPending();
          if (radioIsOn) {
            Pd.send('radio_on', [1]);
            Pd.send('dist_norm', [lastSentDistNorm, 50]);
          }
          Pd.send('master_vol', [userVolume]);
        } catch (e) {
          console.error('Failed to parse radio_music.pd:', e);
        }
      })
      .catch(function(e) { console.error('Failed to load radio_music.pd:', e); });

    fetch('/Sound_Patches/radio_static.pd')
      .then(function(r) { if (!r.ok) throw new Error('radio_static.pd not found'); return r.text(); })
      .then(function(text) {
        try {
          radioStaticPatch = Pd.loadPatch(text);
          radioStaticLoaded = true;
          flushRadioStaticPending();
          if (radioIsOn) {
            Pd.send('radio_on', [1]);
            Pd.send('dist_norm', [lastSentDistNorm, 50]);
          }
          Pd.send('master_vol', [userVolume]);
        } catch (e) {
          console.error('Failed to parse radio_static.pd:', e);
        }
      })
      .catch(function(e) { console.error('Failed to load radio_static.pd:', e); });

    fetch('/Sound_Patches/tv_music.pd')
      .then(function(r) { if (!r.ok) throw new Error('tv_music.pd not found'); return r.text(); })
      .then(function(text) {
        try {
          tvMusicPatch = Pd.loadPatch(text);
          tvMusicLoaded = true;
          flushTvMusicPending();
          if (tvIsOn) {
            Pd.send('tv_on', [1]);
            Pd.send('tv_dist_norm', [lastSentTvDistNorm, 50]);
          }
          Pd.send('master_vol', [userVolume]);
        } catch (e) {
          console.error('Failed to parse tv_music.pd:', e);
        }
      })
      .catch(function(e) { console.error('Failed to load tv_music.pd:', e); });
  }

  // Radio controls
  radioOnBtn.addEventListener("click", function () {
    startPd();
    sendToPd('radio_on', [1]);
    radioIsOn = true;
    radioOnBtn.classList.add("active");
    radioOffBtn.classList.remove("active");
  });
  radioOffBtn.addEventListener("click", function () {
    sendToPd('radio_on', [0]);
    radioIsOn = false;
    lastDistNorm = -1;
    lastSentDistNorm = -1;
    radioOffBtn.classList.add("active");
    radioOnBtn.classList.remove("active");
  });

  // TV controls
  tvOnBtn.addEventListener("click", function () {
    startPd();
    sendToPd('tv_on', [1]);
    tvIsOn = true;
    tvOnBtn.classList.add("active");
    tvOffBtn.classList.remove("active");
  });
  tvOffBtn.addEventListener("click", function () {
    sendToPd('tv_on', [0]);
    tvIsOn = false;
    lastTvDistNorm = -1;
    lastSentTvDistNorm = -1;
    tvOffBtn.classList.add("active");
    tvOnBtn.classList.remove("active");
  });

  // Master volume slider
  masterVolumeInput.addEventListener("input", function () {
    var v = parseInt(masterVolumeInput.value, 10);
    userVolume = v / 100;
    masterVolumeLabel.textContent = v + "%";
    if (typeof Pd !== 'undefined') {
      sendToPd('master_vol', [userVolume]);
    }
  });

  var fieldOfViewRadians = degToRad(60);

  // Matrice de proiectie planara pentru umbra pe podea (y = D).
  // Lumina L = lightLocation = (Lx, Ly, Lz). Derivata din geometria proiectiei
  // punct-prin-lumina pe planul y=D, apoi NEGATA pentru ca w sa ramana pozitiv
  // (obiectele sunt sub lumina, asa ca Py-Ly<0 — fara negare ar fi clipped).
  //   col 0: [Ly-D,  0,     0,      0 ]
  //   col 1: [-Lx,   -D,    -Lz,    -1]
  //   col 2: [0,     0,     Ly-D,   0 ]
  //   col 3: [D*Lx,  D*Ly,  D*Lz,   Ly]
  var D = 0.5;
  var Lx = lightLocation[0], Ly = lightLocation[1], Lz = lightLocation[2];
  var shadowMatrix = [
    Ly - D,   0,       0,       0,
    -Lx,      -D,      -Lz,     -1,
    0,        0,       Ly - D,  0,
    D * Lx,   D * Ly,  D * Lz,  Ly,
  ];

  function drawScene() {
    updateCamera();
    twgl.resizeCanvasToDisplaySize(gl.canvas);
    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

    // Upload frame-ul curent al video-ului in textura, daca e ready
    // readyState >= 2 (HAVE_CURRENT_DATA) garanteaza ca exista frame decodat
    // flipY: 1 — video-urile au origin top-left, WebGL sampleaza bottom-left
    if (tvVideoElement.readyState >= 2) {
      twgl.setTextureFromElement(gl, tvVideoTexture, tvVideoElement, { flipY: 1 });
    }

    // --- Radio distance ---
    var dx = cameraPos[0] - radioWorldPos[0];
    var dy = cameraPos[1] - radioWorldPos[1];
    var dz = cameraPos[2] - radioWorldPos[2];
    var distToRadio = Math.sqrt(dx * dx + dy * dy + dz * dz);
    var maxAudibleDist = 600;
    var fullVolumeDist = 50;
    var attenuation;
    if (distToRadio <= fullVolumeDist) {
      attenuation = 1.0;
    } else if (distToRadio >= maxAudibleDist) {
      attenuation = 0.0;
    } else {
      attenuation = 1.0 - (distToRadio - fullVolumeDist) / (maxAudibleDist - fullVolumeDist);
    }
    var distNorm = 1.0 - attenuation;

    // --- TV distance ---
    var tvWorldPos = [-380, 250, 0];
    var tvDx = cameraPos[0] - tvWorldPos[0];
    var tvDy = cameraPos[1] - tvWorldPos[1];
    var tvDz = cameraPos[2] - tvWorldPos[2];
    var distToTv = Math.sqrt(tvDx * tvDx + tvDy * tvDy + tvDz * tvDz);
    var tvAtt;
    if (distToTv <= 60) tvAtt = 1.0;
    else if (distToTv >= 500) tvAtt = 0.0;
    else tvAtt = 1.0 - (distToTv - 60) / (500 - 60);
    var tvDistNorm = 1.0 - tvAtt;

    // --- Send distance to PD patches (throttled) ---
    if ((radioMusicLoaded || radioStaticLoaded) && radioIsOn && Math.abs(distNorm - lastDistNorm) > 0.01) {
      Pd.send('dist_norm', [distNorm, 50]);
      lastDistNorm = distNorm;
      lastSentDistNorm = distNorm;
    }
    if (tvMusicLoaded && tvIsOn && Math.abs(tvDistNorm - lastTvDistNorm) > 0.01) {
      Pd.send('tv_dist_norm', [tvDistNorm, 50]);
      lastTvDistNorm = tvDistNorm;
      lastSentTvDistNorm = tvDistNorm;
    }

    // --- Update UI bars ---
    var radioMusicVol = radioIsOn ? (1.0 - distNorm) * userVolume : 0;
    var radioStaticVol = radioIsOn ? distNorm * userVolume : 0;
    var tvSoundVol = tvIsOn ? tvAtt * userVolume : 0;

    musicBar.style.width = Math.round(radioMusicVol * 100) + "%";
    musicValue.textContent = Math.round(radioMusicVol * 100) + "%";
    staticBar.style.width = Math.round(radioStaticVol * 100) + "%";
    staticValue.textContent = Math.round(radioStaticVol * 100) + "%";
    tvBar.style.width = Math.round(tvSoundVol * 100) + "%";
    tvValue.textContent = Math.round(tvSoundVol * 100) + "%";

    gl.clearColor(0.529, 0.808, 0.922, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.enable(gl.DEPTH_TEST);

    gl.useProgram(programInfo.program);

    var aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
    var projectionMatrix = m4.perspective(fieldOfViewRadians, aspect, 1, 5000);

    var target = m4.add(cameraPos, cameraFront);
    var cameraMatrix = m4.lookAt(cameraPos, target, cameraUp);
    var viewMatrix = m4.inverse(cameraMatrix);
    var viewProjectionMatrix = m4.multiply(projectionMatrix, viewMatrix);

    var sharedUniforms = {
      u_lightWorldPosition: lightLocation,
      u_viewWorldPosition: cameraPos,
      u_lightColor: m4.normalize(lightColor),
      u_ambientLight: [0.2, 0.2, 0.2],
    };
    twgl.setUniforms(programInfo, sharedUniforms);

    // Pass A — opaque: depth write on, blend off, fara shadow.
    gl.disable(gl.BLEND);
    gl.depthMask(true);
    twgl.setUniforms(programInfo, { u_shadowPass: 0 });
    for (var i = 0; i < sceneObjects.length; i++) {
      var obj = sceneObjects[i];
      if (obj.transparent) continue;
      var objectUniforms = {
        u_world: obj.worldMatrix,
        u_worldViewProjection: m4.multiply(viewProjectionMatrix, obj.worldMatrix),
        u_worldInverseTranspose: m4.transpose(m4.inverse(obj.worldMatrix)),
        u_emissive: 0,
      };
      gl.bindVertexArray(obj.vao);
      twgl.setUniforms(programInfo, objectUniforms);
      twgl.setUniforms(programInfo, obj.material);
      twgl.drawBufferInfo(gl, obj.bufferInfo);
    }

    // Pass B — shadow: doar obiectele marcate cu castShadow, proiectate pe podea.
    // Blend on, depth write off, u_shadowPass=1 (frag outputs negru semi-transparent).
    gl.enable(gl.BLEND);
    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    gl.depthMask(false);
    twgl.setUniforms(programInfo, { u_shadowPass: 1 });
    for (var i = 0; i < sceneObjects.length; i++) {
      var obj = sceneObjects[i];
      if (!obj.castShadow || obj.transparent) continue;
      // worldViewProjection = viewProjection * shadowMatrix * world
      var shadowWorld = m4.multiply(shadowMatrix, obj.worldMatrix);
      var shadowWvp = m4.multiply(viewProjectionMatrix, shadowWorld);
      twgl.setUniforms(programInfo, {
        u_world: obj.worldMatrix,
        u_worldViewProjection: shadowWvp,
        u_worldInverseTranspose: m4.transpose(m4.inverse(obj.worldMatrix)),
      });
      gl.bindVertexArray(obj.vao);
      twgl.drawBufferInfo(gl, obj.bufferInfo);
    }

    // Pass C — transparent (bec, vaza): blend on, depth write off, fara shadow.
    twgl.setUniforms(programInfo, { u_shadowPass: 0 });
    for (var i = 0; i < sceneObjects.length; i++) {
      var obj = sceneObjects[i];
      if (!obj.transparent) continue;
      var objectUniforms = {
        u_world: obj.worldMatrix,
        u_worldViewProjection: m4.multiply(viewProjectionMatrix, obj.worldMatrix),
        u_worldInverseTranspose: m4.transpose(m4.inverse(obj.worldMatrix)),
        u_emissive: 0,
      };
      gl.bindVertexArray(obj.vao);
      twgl.setUniforms(programInfo, objectUniforms);
      twgl.setUniforms(programInfo, obj.material);
      twgl.drawBufferInfo(gl, obj.bufferInfo);
    }

    // Reset la defaults dupa frame.
    gl.depthMask(true);
    gl.disable(gl.BLEND);

    requestAnimationFrame(drawScene);
  }

  requestAnimationFrame(drawScene);

  window.addEventListener('beforeunload', function() {
    if (pdStarted && typeof Pd !== 'undefined') Pd.stop();
  });
}

// ----------------------------------------------------------------------------
// GEOMETRIA CAMEREI (pereti, podea, tavan)
// ----------------------------------------------------------------------------
// Coordonate remapate din OpenGL (Z-up) in WebGL (Y-up): (x_ogl, y_ogl, z_ogl) -> (y_ogl, z_ogl, x_ogl).
// Camera WebGL: X in [-400, 400], Y in [0, 500], Z in [-500, 500].
// Peretele cu fereastra: Z=-500 (4 segmente).
// Peretele spate: Z=+500.
// Pereti laterali: X=±400. Podea: Y=0. Tavan: Y=500.
//
// Winding-ul fiecarui quad e CCW vazut din interiorul camerei (normala spre interior),
// ca sa functioneze corect cu gl.CULL_FACE pornit.
function createRoomMeshes(gl, programInfo, floorTexture) {
  // Adauga un quad (v0,v1,v2,v3) ca 2 triunghiuri cu normala flat n.
  // UV-urile (uv0..uv3) sunt optionale — implicit toate devin (0, 0).
  function addQuad(positions, normals, uvs, v0, v1, v2, v3, n, uv0, uv1, uv2, uv3) {
    var posTri = [v0, v1, v2, v0, v2, v3];
    var uvTri  = [uv0, uv1, uv2, uv0, uv2, uv3];
    for (var i = 0; i < 6; i++) {
      positions.push(posTri[i][0], posTri[i][1], posTri[i][2]);
      normals.push(n[0], n[1], n[2]);
      var uv = uvTri[i] || [0, 0];
      uvs.push(uv[0], uv[1]);
    }
  }

  function makeMesh(positions, normals, uvs, material) {
    var bufferInfo = twgl.createBufferInfoFromArrays(gl, {
      position: { numComponents: 3, data: positions },
      normal:   { numComponents: 3, data: normals },
      texcoord: { numComponents: 2, data: uvs },
    });
    return {
      bufferInfo: bufferInfo,
      vao: twgl.createVAOFromBufferInfo(gl, programInfo, bufferInfo),
      worldMatrix: m4.translation(0, 0, 0),
      material: material,
    };
  }

  // --- PODEA (Y=0, normala spre sus) cu textura de lemn tile-uita 3x3 ---
  var fp = [], fn = [], fu = [];
  addQuad(fp, fn, fu,
    [-400, 0, -500], [-400, 0, 500], [400, 0, 500], [400, 0, -500],
    [0, 1, 0],
    [0, 0], [0, 3], [3, 3], [3, 0]
  );
  var floor = makeMesh(fp, fn, fu, {
    u_useTexture: 1,
    u_texture: floorTexture,
    u_color: [1.0, 1.0, 1.0, 1.0],
    u_shininess: 8,
    u_specularColor: [0.05, 0.05, 0.05],
  });

  // --- TAVAN (Y=500, normala in jos) ---
  var cp = [], cn = [], cu = [];
  addQuad(cp, cn, cu,
    [-400, 500, -500], [400, 500, -500], [400, 500, 500], [-400, 500, 500],
    [0, -1, 0]
  );
  var ceiling = makeMesh(cp, cn, cu, {
    u_useTexture: 0,
    u_texture: floorTexture,
    u_color: [0.9, 0.9, 0.9, 1.0],
    u_shininess: 4,
    u_specularColor: [0.02, 0.02, 0.02],
  });

  // --- PERETI (beige) ---
  var wp = [], wn = [], wu = [];
  // Perete stanga X=-400 (n=+X)
  addQuad(wp, wn, wu,
    [-400, 0, -500], [-400, 500, -500], [-400, 500, 500], [-400, 0, 500],
    [1, 0, 0]
  );
  // Perete dreapta X=+400 (n=-X)
  addQuad(wp, wn, wu,
    [400, 0, -500], [400, 0, 500], [400, 500, 500], [400, 500, -500],
    [-1, 0, 0]
  );
  // Perete spate Z=+500 (n=-Z)
  addQuad(wp, wn, wu,
    [-400, 0, 500], [-400, 500, 500], [400, 500, 500], [400, 0, 500],
    [0, 0, -1]
  );
  // Perete fereastra Z=-500 (n=+Z), 4 segmente cu o gaura in mijloc
  // Bucata de jos (Y: 0..100, toata latimea)
  addQuad(wp, wn, wu,
    [-400, 0, -500], [400, 0, -500], [400, 100, -500], [-400, 100, -500],
    [0, 0, 1]
  );
  // Bucata de sus (Y: 400..500, toata latimea)
  addQuad(wp, wn, wu,
    [-400, 400, -500], [400, 400, -500], [400, 500, -500], [-400, 500, -500],
    [0, 0, 1]
  );
  // Banda stanga fereastra (X: -400..-200, Y: 100..400)
  addQuad(wp, wn, wu,
    [-400, 100, -500], [-200, 100, -500], [-200, 400, -500], [-400, 400, -500],
    [0, 0, 1]
  );
  // Banda dreapta fereastra (X: 200..400, Y: 100..400)
  addQuad(wp, wn, wu,
    [200, 100, -500], [400, 100, -500], [400, 400, -500], [200, 400, -500],
    [0, 0, 1]
  );
  var walls = makeMesh(wp, wn, wu, {
    u_useTexture: 0,
    u_texture: floorTexture,
    u_color: [0.87, 0.81, 0.75, 1.0],
    u_shininess: 4,
    u_specularColor: [0.02, 0.02, 0.02],
  });

  // --- GEAM STICLA peste golul de fereastra (Z=-500, X: -200..200, Y: 100..400) ---
  var gp = [], gn = [], gu = [];
  addQuad(gp, gn, gu,
    [-200, 100, -500], [200, 100, -500], [200, 400, -500], [-200, 400, -500],
    [0, 0, 1]
  );
  var windowPane = makeMesh(gp, gn, gu, {
    u_useTexture: 0,
    u_texture: floorTexture,
    u_color: [0.6, 0.75, 0.85, 0.25],
    u_shininess: 128,
    u_specularColor: [0.5, 0.5, 0.5],
  });
  windowPane.transparent = true;

  // --- RAMA NEAGRA in jurul geamului (4 bare, usor in fata peretelui la Z=-499.5) ---
  var fp = [], fn = [], fu = [];
  var zF = -499.5;
  var w = 10; // latimea ramei
  // Bara de jos
  addQuad(fp, fn, fu,
    [-200, 100, zF], [200, 100, zF], [200, 100 + w, zF], [-200, 100 + w, zF],
    [0, 0, 1]
  );
  // Bara de sus
  addQuad(fp, fn, fu,
    [-200, 400 - w, zF], [200, 400 - w, zF], [200, 400, zF], [-200, 400, zF],
    [0, 0, 1]
  );
  // Bara stanga
  addQuad(fp, fn, fu,
    [-200, 100 + w, zF], [-200 + w, 100 + w, zF], [-200 + w, 400 - w, zF], [-200, 400 - w, zF],
    [0, 0, 1]
  );
  // Bara dreapta
  addQuad(fp, fn, fu,
    [200 - w, 100 + w, zF], [200, 100 + w, zF], [200, 400 - w, zF], [200 - w, 400 - w, zF],
    [0, 0, 1]
  );
  var windowFrame = makeMesh(fp, fn, fu, {
    u_useTexture: 0,
    u_texture: floorTexture,
    u_color: [0.05, 0.05, 0.05, 1.0],
    u_shininess: 8,
    u_specularColor: [0.1, 0.1, 0.1],
  });

  return [floor, ceiling, walls, windowFrame, windowPane];
}

// ----------------------------------------------------------------------------
// OBJ Loader (minimal: v, vn, vt, f cu triangulare fan)
// ----------------------------------------------------------------------------
// Parseaza un fisier .obj si returneaza arrays flat non-indexed:
// { position: [x,y,z, ...], normal: [x,y,z, ...], texcoord: [u,v, ...] }
// Suporta fete cu format: v, v/vt, v//vn, v/vt/vn (inclusiv quads, triangulate fan).
function parseObj(text) {
  var vs = [];    // pozitii (array de [x,y,z])
  var vts = [];   // uv (array de [u,v])
  var vns = [];   // normale (array de [x,y,z])

  var outPos = [], outNor = [], outUv = [];

  var lines = text.split("\n");
  for (var li = 0; li < lines.length; li++) {
    var line = lines[li].trim();
    if (line === "" || line[0] === "#") continue;
    var parts = line.split(/\s+/);
    var tag = parts[0];
    if (tag === "v") {
      vs.push([parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3])]);
    } else if (tag === "vt") {
      vts.push([parseFloat(parts[1]), parseFloat(parts[2])]);
    } else if (tag === "vn") {
      vns.push([parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3])]);
    } else if (tag === "f") {
      // parts[1..n] sunt "v/vt/vn" (sau variante)
      var faceVerts = [];
      for (var i = 1; i < parts.length; i++) {
        var toks = parts[i].split("/");
        var vi  = parseInt(toks[0], 10);
        var vti = toks.length > 1 && toks[1] !== "" ? parseInt(toks[1], 10) : 0;
        var vni = toks.length > 2 && toks[2] !== "" ? parseInt(toks[2], 10) : 0;
        // indici OBJ sunt 1-based; valori negative = offset de la coada
        if (vi  < 0) vi  = vs.length + 1 + vi;
        if (vti < 0) vti = vts.length + 1 + vti;
        if (vni < 0) vni = vns.length + 1 + vni;
        faceVerts.push([vi, vti, vni]);
      }
      // triangulare fan: pentru n verts -> (v0,v1,v2), (v0,v2,v3), ...
      for (var t = 1; t < faceVerts.length - 1; t++) {
        var tri = [faceVerts[0], faceVerts[t], faceVerts[t + 1]];
        for (var k = 0; k < 3; k++) {
          var idx = tri[k];
          var p = vs[idx[0] - 1];
          outPos.push(p[0], p[1], p[2]);
          if (idx[1] > 0 && vts[idx[1] - 1]) {
            var uv = vts[idx[1] - 1];
            outUv.push(uv[0], uv[1]);
          } else {
            outUv.push(0, 0);
          }
          if (idx[2] > 0 && vns[idx[2] - 1]) {
            var n = vns[idx[2] - 1];
            outNor.push(n[0], n[1], n[2]);
          } else {
            outNor.push(0, 1, 0); // fallback
          }
        }
      }
    }
  }
  return { position: outPos, normal: outNor, texcoord: outUv };
}

// Fetch OBJ + creare bufferInfo/VAO + structura de scena.
async function loadObjAsMesh(gl, programInfo, url, material) {
  var resp = await fetch(url);
  if (!resp.ok) throw new Error("Nu pot incarca " + url + ": " + resp.status);
  var text = await resp.text();
  var data = parseObj(text);
  var bufferInfo = twgl.createBufferInfoFromArrays(gl, {
    position: { numComponents: 3, data: data.position },
    normal:   { numComponents: 3, data: data.normal },
    texcoord: { numComponents: 2, data: data.texcoord },
  });
  return {
    bufferInfo: bufferInfo,
    vao: twgl.createVAOFromBufferInfo(gl, programInfo, bufferInfo),
    worldMatrix: m4.translation(0, 0, 0),
    material: material,
  };
}

// Varianta care separa output-ul pe sub-obiecte (directive `o <nume>` in OBJ).
// Returneaza { groupName: {position, normal, texcoord} } — un map de grupuri.
// Face-urile fara `o` anterior merg intr-un grup implicit "__default__".
function parseObjByGroup(text) {
  var vs = [], vts = [], vns = [];
  var groups = {};
  var currentName = "__default__";
  function ensure(name) {
    if (!groups[name]) groups[name] = { position: [], normal: [], texcoord: [] };
    return groups[name];
  }

  var lines = text.split("\n");
  for (var li = 0; li < lines.length; li++) {
    var line = lines[li].trim();
    if (line === "" || line[0] === "#") continue;
    var parts = line.split(/\s+/);
    var tag = parts[0];
    if (tag === "o" || tag === "g") {
      currentName = parts.slice(1).join(" ") || "__default__";
    } else if (tag === "v") {
      vs.push([parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3])]);
    } else if (tag === "vt") {
      vts.push([parseFloat(parts[1]), parseFloat(parts[2])]);
    } else if (tag === "vn") {
      vns.push([parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3])]);
    } else if (tag === "f") {
      var g = ensure(currentName);
      var faceVerts = [];
      for (var i = 1; i < parts.length; i++) {
        var toks = parts[i].split("/");
        var vi  = parseInt(toks[0], 10);
        var vti = toks.length > 1 && toks[1] !== "" ? parseInt(toks[1], 10) : 0;
        var vni = toks.length > 2 && toks[2] !== "" ? parseInt(toks[2], 10) : 0;
        if (vi  < 0) vi  = vs.length + 1 + vi;
        if (vti < 0) vti = vts.length + 1 + vti;
        if (vni < 0) vni = vns.length + 1 + vni;
        faceVerts.push([vi, vti, vni]);
      }
      for (var t = 1; t < faceVerts.length - 1; t++) {
        var tri = [faceVerts[0], faceVerts[t], faceVerts[t + 1]];
        for (var k = 0; k < 3; k++) {
          var idx = tri[k];
          var p = vs[idx[0] - 1];
          g.position.push(p[0], p[1], p[2]);
          if (idx[1] > 0 && vts[idx[1] - 1]) {
            var uv = vts[idx[1] - 1];
            g.texcoord.push(uv[0], uv[1]);
          } else {
            g.texcoord.push(0, 0);
          }
          if (idx[2] > 0 && vns[idx[2] - 1]) {
            var n = vns[idx[2] - 1];
            g.normal.push(n[0], n[1], n[2]);
          } else {
            g.normal.push(0, 1, 0);
          }
        }
      }
    }
  }
  return groups;
}

// Incarca un singur sub-grup dintr-un OBJ (dupa numele dat de directiva `o`).
async function loadObjSubMesh(gl, programInfo, url, groupName, material) {
  var resp = await fetch(url);
  if (!resp.ok) throw new Error("Nu pot incarca " + url + ": " + resp.status);
  var text = await resp.text();
  var groups = parseObjByGroup(text);
  var data = groups[groupName];
  if (!data) throw new Error("Grupul '" + groupName + "' nu exista in " + url + ". Disponibile: " + Object.keys(groups).join(", "));
  var bufferInfo = twgl.createBufferInfoFromArrays(gl, {
    position: { numComponents: 3, data: data.position },
    normal:   { numComponents: 3, data: data.normal },
    texcoord: { numComponents: 2, data: data.texcoord },
  });
  return {
    bufferInfo: bufferInfo,
    vao: twgl.createVAOFromBufferInfo(gl, programInfo, bufferInfo),
    worldMatrix: m4.translation(0, 0, 0),
    material: material,
  };
}

// ----------------------------------------------------------------------------
// Functii Utilitare matematica si miscare camera
// ----------------------------------------------------------------------------
function degToRad(d) { return d * Math.PI / 180; }
function clamp(value, min, max) { return Math.min(Math.max(value, min), max); }

function updateCamera() {
  if (keys['arrowleft'])  yaw -= rotSpeed;
  if (keys['arrowright']) yaw += rotSpeed;
  if (keys['arrowup'])    pitch += rotSpeed;
  if (keys['arrowdown'])  pitch -= rotSpeed;

  pitch = clamp(pitch, -89.0, 89.0);

  var yawRad = degToRad(yaw);
  var pitchRad = degToRad(pitch);

  var front = [0, 0, 0];
  front[0] = Math.cos(yawRad) * Math.cos(pitchRad);
  front[1] = Math.sin(pitchRad);
  front[2] = Math.sin(yawRad) * Math.cos(pitchRad);
  
  cameraFront = vec3.normalize(front);

  if (keys['w']) cameraPos = vec3.add(cameraPos, vec3.multiply(cameraFront, cameraSpeed));
  if (keys['s']) cameraPos = vec3.subtract(cameraPos, vec3.multiply(cameraFront, cameraSpeed));
  if (keys['a']) {
    var left = vec3.normalize(vec3.cross(cameraFront, cameraUp));
    cameraPos = vec3.subtract(cameraPos, vec3.multiply(left, cameraSpeed));
  }
  if (keys['d']) {
    var right = vec3.normalize(vec3.cross(cameraFront, cameraUp));
    cameraPos = vec3.add(cameraPos, vec3.multiply(right, cameraSpeed));
  }

  cameraPos[0] = clamp(cameraPos[0], -380.0, 380.0); 
  cameraPos[1] = clamp(cameraPos[1],   20.0, 480.0); 
  cameraPos[2] = clamp(cameraPos[2], -480.0, 480.0); 
}

//-------------------------------------------------------------------------------------

var m4 = {

  perspective: function(fieldOfViewInRadians, aspect, near, far) {
    var f = Math.tan(Math.PI * 0.5 - 0.5 * fieldOfViewInRadians);
    var rangeInv = 1.0 / (near - far);

    return [
      f / aspect, 0, 0, 0,
      0, f, 0, 0,
      0, 0, (near + far) * rangeInv, -1,
      0, 0, near * far * rangeInv * 2, 0,
    ];
  },

  projection: function(width, height, depth) {
    // Note: This matrix flips the Y axis so 0 is at the top.
    return [
       2 / width, 0, 0, 0,
       0, -2 / height, 0, 0,
       0, 0, 2 / depth, 0,
      -1, 1, 0, 1,
    ];
  },

  multiply: function(a, b) {
    var a00 = a[0 * 4 + 0];
    var a01 = a[0 * 4 + 1];
    var a02 = a[0 * 4 + 2];
    var a03 = a[0 * 4 + 3];
    var a10 = a[1 * 4 + 0];
    var a11 = a[1 * 4 + 1];
    var a12 = a[1 * 4 + 2];
    var a13 = a[1 * 4 + 3];
    var a20 = a[2 * 4 + 0];
    var a21 = a[2 * 4 + 1];
    var a22 = a[2 * 4 + 2];
    var a23 = a[2 * 4 + 3];
    var a30 = a[3 * 4 + 0];
    var a31 = a[3 * 4 + 1];
    var a32 = a[3 * 4 + 2];
    var a33 = a[3 * 4 + 3];
    var b00 = b[0 * 4 + 0];
    var b01 = b[0 * 4 + 1];
    var b02 = b[0 * 4 + 2];
    var b03 = b[0 * 4 + 3];
    var b10 = b[1 * 4 + 0];
    var b11 = b[1 * 4 + 1];
    var b12 = b[1 * 4 + 2];
    var b13 = b[1 * 4 + 3];
    var b20 = b[2 * 4 + 0];
    var b21 = b[2 * 4 + 1];
    var b22 = b[2 * 4 + 2];
    var b23 = b[2 * 4 + 3];
    var b30 = b[3 * 4 + 0];
    var b31 = b[3 * 4 + 1];
    var b32 = b[3 * 4 + 2];
    var b33 = b[3 * 4 + 3];
    return [
      b00 * a00 + b01 * a10 + b02 * a20 + b03 * a30,
      b00 * a01 + b01 * a11 + b02 * a21 + b03 * a31,
      b00 * a02 + b01 * a12 + b02 * a22 + b03 * a32,
      b00 * a03 + b01 * a13 + b02 * a23 + b03 * a33,
      b10 * a00 + b11 * a10 + b12 * a20 + b13 * a30,
      b10 * a01 + b11 * a11 + b12 * a21 + b13 * a31,
      b10 * a02 + b11 * a12 + b12 * a22 + b13 * a32,
      b10 * a03 + b11 * a13 + b12 * a23 + b13 * a33,
      b20 * a00 + b21 * a10 + b22 * a20 + b23 * a30,
      b20 * a01 + b21 * a11 + b22 * a21 + b23 * a31,
      b20 * a02 + b21 * a12 + b22 * a22 + b23 * a32,
      b20 * a03 + b21 * a13 + b22 * a23 + b23 * a33,
      b30 * a00 + b31 * a10 + b32 * a20 + b33 * a30,
      b30 * a01 + b31 * a11 + b32 * a21 + b33 * a31,
      b30 * a02 + b31 * a12 + b32 * a22 + b33 * a32,
      b30 * a03 + b31 * a13 + b32 * a23 + b33 * a33,
    ];
  },

  translation: function(tx, ty, tz) {
    return [
       1,  0,  0,  0,
       0,  1,  0,  0,
       0,  0,  1,  0,
       tx, ty, tz, 1,
    ];
  },

  xRotation: function(angleInRadians) {
    var c = Math.cos(angleInRadians);
    var s = Math.sin(angleInRadians);

    return [
      1, 0, 0, 0,
      0, c, s, 0,
      0, -s, c, 0,
      0, 0, 0, 1,
    ];
  },

  yRotation: function(angleInRadians) {
    var c = Math.cos(angleInRadians);
    var s = Math.sin(angleInRadians);

    return [
      c, 0, -s, 0,
      0, 1, 0, 0,
      s, 0, c, 0,
      0, 0, 0, 1,
    ];
  },

  zRotation: function(angleInRadians) {
    var c = Math.cos(angleInRadians);
    var s = Math.sin(angleInRadians);

    return [
       c, s, 0, 0,
      -s, c, 0, 0,
       0, 0, 1, 0,
       0, 0, 0, 1,
    ];
  },

  scaling: function(sx, sy, sz) {
    return [
      sx, 0,  0,  0,
      0, sy,  0,  0,
      0,  0, sz,  0,
      0,  0,  0,  1,
    ];
  },

  translate: function(m, tx, ty, tz) {
    return m4.multiply(m, m4.translation(tx, ty, tz));
  },

  xRotate: function(m, angleInRadians) {
    return m4.multiply(m, m4.xRotation(angleInRadians));
  },

  yRotate: function(m, angleInRadians) {
    return m4.multiply(m, m4.yRotation(angleInRadians));
  },

  zRotate: function(m, angleInRadians) {
    return m4.multiply(m, m4.zRotation(angleInRadians));
  },

  scale: function(m, sx, sy, sz) {
    return m4.multiply(m, m4.scaling(sx, sy, sz));
  },

  inverse: function(m) {
    var m00 = m[0 * 4 + 0];
    var m01 = m[0 * 4 + 1];
    var m02 = m[0 * 4 + 2];
    var m03 = m[0 * 4 + 3];
    var m10 = m[1 * 4 + 0];
    var m11 = m[1 * 4 + 1];
    var m12 = m[1 * 4 + 2];
    var m13 = m[1 * 4 + 3];
    var m20 = m[2 * 4 + 0];
    var m21 = m[2 * 4 + 1];
    var m22 = m[2 * 4 + 2];
    var m23 = m[2 * 4 + 3];
    var m30 = m[3 * 4 + 0];
    var m31 = m[3 * 4 + 1];
    var m32 = m[3 * 4 + 2];
    var m33 = m[3 * 4 + 3];
    var tmp_0  = m22 * m33;
    var tmp_1  = m32 * m23;
    var tmp_2  = m12 * m33;
    var tmp_3  = m32 * m13;
    var tmp_4  = m12 * m23;
    var tmp_5  = m22 * m13;
    var tmp_6  = m02 * m33;
    var tmp_7  = m32 * m03;
    var tmp_8  = m02 * m23;
    var tmp_9  = m22 * m03;
    var tmp_10 = m02 * m13;
    var tmp_11 = m12 * m03;
    var tmp_12 = m20 * m31;
    var tmp_13 = m30 * m21;
    var tmp_14 = m10 * m31;
    var tmp_15 = m30 * m11;
    var tmp_16 = m10 * m21;
    var tmp_17 = m20 * m11;
    var tmp_18 = m00 * m31;
    var tmp_19 = m30 * m01;
    var tmp_20 = m00 * m21;
    var tmp_21 = m20 * m01;
    var tmp_22 = m00 * m11;
    var tmp_23 = m10 * m01;

    var t0 = (tmp_0 * m11 + tmp_3 * m21 + tmp_4 * m31) -
             (tmp_1 * m11 + tmp_2 * m21 + tmp_5 * m31);
    var t1 = (tmp_1 * m01 + tmp_6 * m21 + tmp_9 * m31) -
             (tmp_0 * m01 + tmp_7 * m21 + tmp_8 * m31);
    var t2 = (tmp_2 * m01 + tmp_7 * m11 + tmp_10 * m31) -
             (tmp_3 * m01 + tmp_6 * m11 + tmp_11 * m31);
    var t3 = (tmp_5 * m01 + tmp_8 * m11 + tmp_11 * m21) -
             (tmp_4 * m01 + tmp_9 * m11 + tmp_10 * m21);

    var d = 1.0 / (m00 * t0 + m10 * t1 + m20 * t2 + m30 * t3);

    return [
      d * t0,
      d * t1,
      d * t2,
      d * t3,
      d * ((tmp_1 * m10 + tmp_2 * m20 + tmp_5 * m30) -
           (tmp_0 * m10 + tmp_3 * m20 + tmp_4 * m30)),
      d * ((tmp_0 * m00 + tmp_7 * m20 + tmp_8 * m30) -
           (tmp_1 * m00 + tmp_6 * m20 + tmp_9 * m30)),
      d * ((tmp_3 * m00 + tmp_6 * m10 + tmp_11 * m30) -
           (tmp_2 * m00 + tmp_7 * m10 + tmp_10 * m30)),
      d * ((tmp_4 * m00 + tmp_9 * m10 + tmp_10 * m20) -
           (tmp_5 * m00 + tmp_8 * m10 + tmp_11 * m20)),
      d * ((tmp_12 * m13 + tmp_15 * m23 + tmp_16 * m33) -
           (tmp_13 * m13 + tmp_14 * m23 + tmp_17 * m33)),
      d * ((tmp_13 * m03 + tmp_18 * m23 + tmp_21 * m33) -
           (tmp_12 * m03 + tmp_19 * m23 + tmp_20 * m33)),
      d * ((tmp_14 * m03 + tmp_19 * m13 + tmp_22 * m33) -
           (tmp_15 * m03 + tmp_18 * m13 + tmp_23 * m33)),
      d * ((tmp_17 * m03 + tmp_20 * m13 + tmp_23 * m23) -
           (tmp_16 * m03 + tmp_21 * m13 + tmp_22 * m23)),
      d * ((tmp_14 * m22 + tmp_17 * m32 + tmp_13 * m12) -
           (tmp_16 * m32 + tmp_12 * m12 + tmp_15 * m22)),
      d * ((tmp_20 * m32 + tmp_12 * m02 + tmp_19 * m22) -
           (tmp_18 * m22 + tmp_21 * m32 + tmp_13 * m02)),
      d * ((tmp_18 * m12 + tmp_23 * m32 + tmp_15 * m02) -
           (tmp_22 * m32 + tmp_14 * m02 + tmp_19 * m12)),
      d * ((tmp_22 * m22 + tmp_16 * m02 + tmp_21 * m12) -
           (tmp_20 * m12 + tmp_23 * m22 + tmp_17 * m02)),
    ];
  },

  cross: function(a, b) {
    return [
       a[1] * b[2] - a[2] * b[1],
       a[2] * b[0] - a[0] * b[2],
       a[0] * b[1] - a[1] * b[0],
    ];
  },

  add: function(a, b) {
    return [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
  },

  subtractVectors: function(a, b) {
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
  },

  normalize: function(v) {
    var length = Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    // make sure we don't divide by 0.
    if (length > 0.00001) {
      return [v[0] / length, v[1] / length, v[2] / length];
    } else {
      return [0, 0, 0];
    }
  },

  lookAt: function(cameraPosition, target, up) {
    var zAxis = m4.normalize(
        m4.subtractVectors(cameraPosition, target));
    var xAxis = m4.normalize(m4.cross(up, zAxis));
    var yAxis = m4.normalize(m4.cross(zAxis, xAxis));

    return [
      xAxis[0], xAxis[1], xAxis[2], 0,
      yAxis[0], yAxis[1], yAxis[2], 0,
      zAxis[0], zAxis[1], zAxis[2], 0,
      cameraPosition[0],
      cameraPosition[1],
      cameraPosition[2],
      1,
    ];
  },

  transformVector: function(m, v) {
    var dst = [];
    for (var i = 0; i < 4; ++i) {
      dst[i] = 0.0;
      for (var j = 0; j < 4; ++j) {
        dst[i] += v[j] * m[j * 4 + i];
      }
    }
    return dst;
  },

  transpose: function(m) {
    return [
      m[0], m[4], m[8], m[12],
      m[1], m[5], m[9], m[13],
      m[2], m[6], m[10], m[14],
      m[3], m[7], m[11], m[15],
    ];
  },

};

//---------------------------------------------------------------------

var vec3 = {
  add: function(a, b) {
    return [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
  },
  subtract: function(a, b) {
    return [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
  },
  multiply: function(v, s) {
    return [v[0] * s, v[1] * s, v[2] * s];
  },
  cross: function(a, b) {
    return [
      a[1] * b[2] - a[2] * b[1],
      a[2] * b[0] - a[0] * b[2],
      a[0] * b[1] - a[1] * b[0]
    ];
  },
  normalize: function(v) {
    var len = Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len > 0.00001) return [v[0] / len, v[1] / len, v[2] / len];
    return [0, 0, 0];
  }
};

main();
