# androidx graphics-shapes and MaterialShapes (port)

`src/lamina/shapes/` is a C++ port of Kotlin code from the Android Open Source Project.

- Repository: https://github.com/androidx/androidx
- Commit: `1b31f4ddbd0d24677bdfc3e0f097ce5706c2f772`
- Paths:
  - `graphics/graphics-shapes/src/commonMain/kotlin/androidx/graphics/shapes/` — `CornerRounding.kt`, `Cubic.kt`, `FeatureMapping.kt`, `Features.kt`, `FloatMapping.kt`, `Morph.kt`, `Point.kt`, `PolygonMeasure.kt`, `RoundedPolygon.kt`, `Shapes.kt`, `Utils.kt`
  - `compose/material3/material3/src/commonMain/kotlin/androidx/compose/material3/MaterialShapes.kt` (the 35 shapes and their `customPolygon` / `doRepeat` helpers)
  - `compose/material3/material3/src/commonMain/kotlin/androidx/compose/material3/internal/ShapeUtil.kt` (`RoundedPolygon.transformed(Matrix)` and the cubic-to-path conversion)
  - `compose/ui/ui-graphics/src/commonMain/kotlin/androidx/compose/ui/graphics/Matrix.kt` (`rotateZ`, `scale` and `map` as used by the shapes above)

The algorithms, constants and tolerances are translated as they are. The code has been changed from Kotlin to C++.

Copyright The Android Open Source Project

Licensed under the Apache License, Version 2.0 (the "License"); you may not use these files except in compliance with the License. You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
