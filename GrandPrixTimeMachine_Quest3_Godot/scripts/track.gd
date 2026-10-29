class_name GPTrack
extends RefCounted
## Fuji-inspired, NOT a surveyed version of Fuji Speedway.
const SAMPLE_COUNT := 320
const ROAD_WIDTH := 0.070
const CONTROLS := [
    Vector2(-0.50, -0.19), Vector2(-0.20, -0.23), Vector2(0.28, -0.23),
    Vector2(0.50, -0.20), Vector2(0.55, -0.07), Vector2(0.49, 0.12),
    Vector2(0.32, 0.23), Vector2(0.02, 0.26), Vector2(-0.15, 0.14),
    Vector2(-0.32, 0.29), Vector2(-0.53, 0.18), Vector2(-0.53, -0.02),
]
var points: Array[Vector3] = []
var total_length := 0.0

func _init() -> void:
    var raw: Array[Vector3] = []
    var lengths: Array[float] = [0.0]
    for i in range(SAMPLE_COUNT + 1):
        raw.append(_raw_position(float(i) / float(SAMPLE_COUNT)))
        if i > 0:
            total_length += raw[i].distance_to(raw[i - 1])
            lengths.append(total_length)
    var start := 0
    for i in range(SAMPLE_COUNT):
        var target := total_length * float(i) / float(SAMPLE_COUNT)
        while start < SAMPLE_COUNT - 1 and lengths[start + 1] < target:
            start += 1
        var extent := lengths[start + 1] - lengths[start]
        var ratio := (target - lengths[start]) / extent if extent > 0.000001 else 0.0
        points.append(raw[start].lerp(raw[start + 1], clampf(ratio, 0.0, 1.0)))

func _raw_position(t: float) -> Vector3:
    var x := fposmod(t, 1.0) * float(CONTROLS.size())
    var i := int(floor(x))
    var u: float = x - floor(x)
    var n := CONTROLS.size()
    var a: Vector2 = CONTROLS[(i - 1 + n) % n]
    var b: Vector2 = CONTROLS[i % n]
    var c: Vector2 = CONTROLS[(i + 1) % n]
    var d: Vector2 = CONTROLS[(i + 2) % n]
    var p: Vector2 = 0.5 * ((2.0 * b) + (-a + c) * u + (2.0 * a - 5.0 * b + 4.0 * c - d) * u * u + (-a + 3.0 * b - 3.0 * c + d) * u * u * u)
    return Vector3(p.x, 0.026, p.y)

func at(t: float) -> Vector3:
    var index_f := fposmod(t, 1.0) * float(SAMPLE_COUNT)
    var index := int(floor(index_f)) % SAMPLE_COUNT
    return points[index].lerp(points[(index + 1) % SAMPLE_COUNT], index_f - floor(index_f))

func tangent(t: float) -> Vector3:
    return (at(t + 0.0012) - at(t - 0.0012)).normalized()

func right(t: float) -> Vector3:
    return Vector3.UP.cross(tangent(t)).normalized()
