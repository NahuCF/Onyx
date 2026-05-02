# Frustum Culling Math — In Depth

## What is a plane?

A plane in 3D space is a flat infinite surface. You can define it with:
1. A **normal vector** `N = (a, b, c)` — a direction perpendicular to the plane's surface
2. A **point on the plane** `P0` — any point that lies on it

```
         N (normal)
         ↑
         │
─────────┼──────────── the plane
         │
         P0 (a point on the plane)
```

## The plane equation

Any point `P` lies on the plane if and only if the vector from `P0` to `P` is perpendicular to `N`. Two vectors are perpendicular when their dot product is zero:

```
N · (P - P0) = 0
```

Expanding with `N = (a, b, c)`, `P = (x, y, z)`, `P0 = (x0, y0, z0)`:

```
a*(x - x0) + b*(y - y0) + c*(z - z0) = 0
```

Distribute:

```
a*x + b*y + c*z - (a*x0 + b*y0 + c*z0) = 0
```

That last term `(a*x0 + b*y0 + c*z0)` is a constant — we call it `-d`:

```
d = -(a*x0 + b*y0 + c*z0)
```

So the plane equation becomes:

```
a*x + b*y + c*z + d = 0
```

Every point `(x, y, z)` satisfying this equation is on the plane. That's why we store a plane as `vec4(a, b, c, d)`.

## What happens when a point is NOT on the plane?

If you plug in a point that's **not** on the plane, you don't get zero — you get a positive or negative number:

```
result = a*x + b*y + c*z + d
```

- `result = 0` → point is exactly on the plane
- `result > 0` → point is on the **same side** as the normal points toward
- `result < 0` → point is on the **opposite side** from the normal

This is what "signed" means — the sign tells you **which side** of the plane the point is on. Like a thermometer: positive = one side, negative = the other.

```
        Normal N
         ↑
         │
  - - - -│- - - -  ← the plane (result = 0)
         │
    result > 0      (same side as N)
         │
═════════════════
         │
    result < 0      (opposite side from N)
```

## Is `result` the actual distance?

**Only if the normal is unit length** (length = 1).

Consider a simple example. A horizontal plane at height 5, with normal pointing up:

```
N = (0, 1, 0),  P0 = (0, 5, 0)
d = -(0*0 + 1*5 + 0*0) = -5
Plane equation: 0*x + 1*y + 0*z - 5 = 0  →  y - 5 = 0
```

Test a point at `(0, 8, 0)` (3 units above the plane):

```
result = 0*0 + 1*8 + 0*0 - 5 = 3     ✓ correct distance!
```

Now what if we used `N = (0, 2, 0)` (same direction, but length 2 instead of 1)?

```
d = -(0*0 + 2*5 + 0*0) = -10
result = 0*0 + 2*8 + 0*0 - 10 = 6    ✗ says "6" but real distance is 3
```

The result is scaled by the length of the normal. To get the true distance in world units (Euclidean distance), you divide by the normal's length:

```
true distance = result / length(N) = 6 / 2 = 3    ✓
```

That's what `NormalizePlane` does — divides the entire `vec4(a, b, c, d)` by `length(a, b, c)`, so after normalization `length(a, b, c) = 1` and the formula gives you real distances directly.

## What is a frustum?

The camera's view volume is a truncated pyramid (frustum) bounded by **6 planes**: left, right, top, bottom, near, far. Everything inside all 6 planes is visible. Anything fully outside any single plane is invisible.

## Why does frustum culling care about distance?

It doesn't care about the exact distance — it only cares about the **sign**. Each frustum plane has its normal pointing **inward** (toward visible space). So:

```
result > 0  →  point is inside (visible side)
result < 0  →  point is outside (culled side)
```

For `IsBoxVisible`, we just need to know: is the box entirely on the negative side of any plane? If yes, it's invisible. The actual distance value doesn't matter — just positive vs negative.

## Plane extraction (Gribb/Hartmann method)

The `Update()` method in `Frustum.cpp` extracts the 6 planes directly from the view-projection matrix using row addition/subtraction. A point `P` is inside the frustum when its clip-space coordinates satisfy `-w <= x,y,z <= w`. Rearranging those inequalities for each axis gives the 6 plane equations directly from the matrix rows. For example, the left plane (`x >= -w`) becomes row3 + row0, the right plane (`x <= w`) becomes row3 - row0, etc.

## The p-vertex (positive vertex) method

An AABB has 8 corners. The naive approach tests all 8 against every plane — 48 dot products. The p-vertex trick reduces this to **1 dot product per plane** (6 total).

The key insight: **you only need to test the single corner that is most likely to be inside the plane.** If even that "most favorable" corner is outside, the entire box must be outside.

For a plane with normal `N = (a, b, c)`:

```
positive.x = (a >= 0) ? max.x : min.x
positive.y = (b >= 0) ? max.y : min.y
positive.z = (c >= 0) ? max.z : min.z
```

**Why this works:** Each component of the normal tells you which direction the plane faces along that axis. If `a >= 0`, the plane faces toward `+X`, so the corner furthest in that direction is `max.x`. If `a < 0`, the plane faces toward `-X`, so `min.x` is further in the plane's direction. You do this per-component, constructing the single corner that maximizes `dot(N, corner)`.

### Visual example (2D, for simplicity)

```
         Normal N = (1, 1)
              ↗
    ┌─────────┐
    │  AABB   │ ← max corner (max.x, max.y) = p-vertex
    │         │    This is the corner "most aligned" with N
    │         │
    └─────────┘
  min corner
```

If the normal were `(-1, 1)` (pointing up-left), the p-vertex would be `(min.x, max.y)` — the top-left corner.

## The full algorithm (IsBoxVisible)

```
For each of the 6 frustum planes:
    1. Compute the p-vertex (corner most aligned with plane normal)
    2. Compute signed distance from p-vertex to plane
    3. If distance < 0:
         → Even the "best" corner is behind this plane
         → The ENTIRE box is outside this plane
         → Return false (culled)

If we pass all 6 planes → the box is at least partially inside → Return true (visible)
```

### Concrete example

```
Plane: y - 5 = 0  (horizontal plane at y=5, normal pointing up)

Box: min=(2, 1, 2), max=(4, 3, 4)

Normal = (0, 1, 0)
         a≥0? no→min.x=2   b≥0? yes→max.y=3   c≥0? no→min.z=2
         (doesn't matter)    (this is the one     (doesn't matter)
                              that matters)

p-vertex = (2, 3, 2)

distance = 0*2 + 1*3 + 0*2 - 5 = -2  →  NEGATIVE

The "best" corner (y=3) is still 2 units below the plane (y=5).
Every other corner has y ≤ 3, so they're all even further below.
The entire box is outside this plane → culled.
```

The whole `IsBoxVisible` function is just this test repeated 6 times, once per frustum plane. If the box survives all 6, it's visible.

## Limitations

This is a **conservative test** — it can produce false positives but never false negatives:
- If it returns `false`, the box is **guaranteed** outside the frustum
- If it returns `true`, the box is **probably** inside but could be outside in rare edge cases (box straddles the corner of the frustum where no single plane rejects it, but the intersection of planes does). In practice this almost never matters visually — you draw a few extra meshes at worst.

## Source code reference

- `Onyx/Source/Graphics/Frustum.h` — class declaration
- `Onyx/Source/Graphics/Frustum.cpp` — implementation
- `Onyx/Source/Graphics/SceneRenderer.cpp` — usage in shadow and color passes
