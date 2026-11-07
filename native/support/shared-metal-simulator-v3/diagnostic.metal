#include <metal_stdlib>
using namespace metal;
struct VertexOut { float4 position [[position]]; };
vertex VertexOut md_vertex(uint id [[vertex_id]]) {
    const float2 p[3] = {float2(-0.75, -0.75), float2(0.75, -0.75), float2(0, 0.75)};
    VertexOut out;
    out.position = float4(p[id], 0, 1);
    return out;
}
fragment float4 md_fragment() { return float4(0, 1, 0, 1); }
