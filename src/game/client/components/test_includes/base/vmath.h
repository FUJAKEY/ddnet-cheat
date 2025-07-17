#pragma once
// Минимальные заглушки для base/vmath.h

// Minimal vec2 definition for testing
struct vec2 { 
    float x, y; 
    vec2() : x(0), y(0) {} 
    vec2(float nx, float ny) : x(nx), y(ny) {} 
    vec2 operator+(const vec2& other) const { return vec2(x + other.x, y + other.y); }
    vec2 operator-(const vec2& other) const { return vec2(x - other.x, y - other.y); }
    vec2 operator*(float f) const { return vec2(x * f, y * f); }
};
float length(const vec2& v) { return v.x * v.x + v.y * v.y; }
vec2 normalize(const vec2& v) { float l = length(v); return l > 0 ? vec2(v.x/l, v.y/l) : vec2(0,0); }
int round_to_int(float f) { return (int)(f + 0.5f); }
int maximum(int a, int b) { return a > b ? a : b; }
void mem_zero(void* ptr, size_t size) { memset(ptr, 0, size); }
float abs(float f) { return f < 0 ? -f : f; }
const float pi = 3.14159f;
float cos(float f) { return ::cos(f); }
float sin(float f) { return ::sin(f); }

