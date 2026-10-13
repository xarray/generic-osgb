# generic_wrappers

A **zero-OSG-dependency** re-implementation of the OpenSceneGraph serialization
subsystem (the `osgDB` "wrappers"). It reads `.osgt` (ASCII) and `.osgb` (binary)
files and reconstructs the object graph **without linking against OpenSceneGraph**.

The `generic_osg/` directory contains one translation unit per OSG class, adapted
from OSG's own serializer sources so that the same read logic can be driven
standalone.

---

## 1. How it works (simulating OSG serialize wrappers)

OSG stores objects through a per-class *wrapper*: `REGISTER_OBJECT_WRAPPER(name,
prototype, class, associates)` declares a list of serializable properties using
serializer macros (`ADD_INT_SERIALIZER`, `ADD_OBJECT_SERIALIZER`,
`ADD_VEC3F_SERIALIZER`, `BEGIN_ENUM_SERIALIZER`, ...). When loading, OSG uses the
class name read from the stream to find the wrapper and then reads properties in
the declared order.

This project re-implements that mechanism standalone:

* **Registry** — `Rewrapper` / `RewrapperManager` (in `GenericReserializer.h`)
  hold, per class name, an ordered list of `(propertyName, BaseSerializer)` pairs,
  mirroring the OSG wrapper layout.
* **Serializer kinds** — `ValueSerializer<T>`, `BoolSerializer`,
  `StringSerializer`, `ObjectSerializer`, `ImageSerializer`, `VectorSerializer`,
  `ListSerializer`, `EnumSerializer`, `BitFlagSerializer`, `GLenumSerializer`, and
  `UserSerializer` (a custom read callback). Each knows how to pull its value off
  the stream.
* **Registration** — the `REGISTER_OBJECT_WRAPPER` macro expands into a static
  `RegisterWrapperProxy` that registers the wrapper into `RewrapperManager`
  **before `main()` runs**. Because those registrars are never referenced
  explicitly, the static library must be linked with *whole-archive*
  (`--whole-archive` / `/WHOLEARCHIVE`) or the linker would drop them.
* **Dispatch** — `SerializerVisitor` walks a wrapper and routes every serializer
  to its read path (`readSerializerValue`), keeping the reader generic.
* **Math / base layer** — `GenericMath.h` defines a minimal `osg` namespace
  (`Referenced`/`ref_ptr`, `Vec<N,T>` and the `Vec2*/Vec3*/Vec4*` families,
  `Quat`, `Plane`, `Matrix`, `BoundingBox*`, `BoundingSphere*`, GL typedefs) so
  the adapted `generic_osg/*.cpp` sources compile with no `libosg`.
* **I/O layer** — `GenericInputStream.h/.cpp` provide `InputIterator`
  (`BinaryInputIterator` / `AsciiInputIterator`) and `InputStream`. The format is
  detected from the first 8 header bytes; `.osgt` is parsed as text, `.osgb` is
  read as raw binary (byte order handled via `setByteSwap`).
* **Output** — reading does **not** construct real OSG objects. `readObject()`
  walks the stream and records the result into `InputUserData`: a scene tree
  (`TreeNode` = class name, id, file offset, parent/child links) plus explicit
  commands (`ud.add("setXxx", ...)` / `ud.add("addChild", ...)`) emitted by the
  user-serializer `read*` functions. Those commands are forwarded to the optional
  `SerializerVisitor` and/or recorded into a neutral `SceneIR` (see `SceneIR.h`)
  that a host adapter can replay to rebuild the graph in another engine.

Entry point (`GenericReader.h`):

```cpp
osgVerse::loadRewrappers();
osgVerse::InputUserData ud;
osgVerse::ObjectTypeAndID oid = osgVerse::readObjectFile("model.osgb", ud);
// oid._type is the top-level class name, e.g. "osg::MatrixTransform"
```

### Current format support

* `.osgb` — **uncompressed** binary and `.osgt` — ASCII text.
* zlib-compressed `.osgb` streams are **not** supported yet
  (`InputStream::decompress()` rejects any compressor other than `"0"`).

---

## 2. Why dependency-free `.osgb` reading matters

`.osgb` is OSG's compact, widely used runtime format, but consuming it normally
means building and linking the full OpenSceneGraph library — a large, platform-
and toolchain-sensitive C++ dependency with ABI constraints.

Reading `.osgb` without OSG makes it possible to:

* **Inspect and validate** assets (structure, versions, class usage) on any
  machine, in CI, or in offline tooling.
* **Convert / migrate** OSG assets into other renderers and engines without an
  OSG build step.
* **Ship a lightweight decoder** into environments where `libosg` cannot be used:
  browsers/WASM, mobile, embedded, sandboxes, and server-side pipelines.
* **Own the format layer** — custom memory handling, streaming, and re-serialization
  strategies are under direct control instead of being tied to OSG internals.

The whole library depends only on the C++17 standard library.

---

## 3. Build

Requirements: a C++17 compiler and CMake >= 3.10. No third-party libraries.

```bash
cmake -S . -B build
cmake --build build
```

This produces the dependency-free static library `osgVerseGenericReserializer`
and two executables that depend only on it:

* `reserializing_test` — parses a file and prints the object scene tree.
* `null_integration` — rebuilds the neutral `SceneIR` and prints everything the
  IR describes (object/command histograms, data payloads, full scene tree).

Run them on any `.osgt` / `.osgb` file:

```bash
./build/reserializing_test <file.osgt|file.osgb> [--force-ascii]
./build/null_integration  <file.osgt|file.osgb>
```

Output of `reserializing_test` reports the number of registered rewrappers, the
top-level object class, and the parsed scene tree:

```
Rewrappers: 461
Top object  : osg::MatrixTransform (id=1)
Scene tree  : 519 objects read
- osg::MatrixTransform (id=1, @0x19)
  ...
Done.
```

`--force-ascii` skips binary detection and parses the input as ASCII text.

### Engine examples

Concrete engine adapters live under `examples/`; each example may pull in its own
engine SDK and is off by default. The OpenSceneGraph "echo back" example rebuilds
a real osg scene (through `osgDB::ObjectWrapperManager`) and renders it with
`osgViewer`:

```bash
cmake -S . -B build -DENABLE_OSG_INTEGRATION=ON \
      -DOSG_INTEGRATION_ROOT=/path/to/osg-sdk     # contains include/ and lib/
cmake --build build
./build/osg_integration <file.osgt|file.osgb>          # render
./build/osg_integration <file.osgt|file.osgb> --list   # rebuild + report only
```

### Directory layout

```
.                       dependency-free core (reader, IR, math layer)
generic_osg/            one wrapper per osg core class
generic_osgterrain/     osgTerrain      generic_osganimation/  osgAnimation
generic_osgparticle/    osgParticle     generic_osgvolume/     osgVolume
generic_osgsim/         osgSim          generic_osgmanipulator/ osgManipulator
generic_osgshadow/      osgShadow       generic_osgui/         osgUI
generic_osgga/          osgGA           generic_osgfx/         osgFX
generic_osgviewer/      osgViewer       generic_osgtext/       osgText
generic_osgutil/        osgUtil
examples/               example programs (null_integration, osg_integration, ...)
```

Every osg library shipped with OpenSceneGraph has been ported this way: create
`generic_osg<lib>/`, take the wrappers from
`OpenSceneGraph/src/osgWrappers/serializers/osg<lib>/`, include
`GenericReserializer.h`, turn the `read*` functions into the
`ud.add("setXxx", ...)` form, drop the `check*`/`write*` helpers, then add the
directory to the library sources in `CMakeLists.txt`.

---

## 4. Embedding and rendering in 3D engines

> To be filled in.

* OSG (as echo back) -
* VSG (VulkanSceneGraph) -
* Web (three.js / WebGL / WebGPU) —
* Babylon.js —
* Godot —
* Unity —
* Unreal Engine —
