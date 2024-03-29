#define PI 3.1415926
#define INFINITY 1.0 / 0.0

struct stack_t {
    uint top;
    uint data[64];
}; 

void push_stack(inout stack_t stk, 
                const in uint d) {
    stk.data[stk.top] = d;
    ++ stk.top;
}

bool pop_stack(inout stack_t stk,
               inout uint d) {
    if (stk.top > 0) {
        d = stk.data[stk.top - 1];
        -- stk.top;
        return true;
    } else {
        return false;
    }
}

void clear_stack(inout stack_t stk) {
    stk.top = 0;
}

struct vector_t {
    uint size;
    uint data[16];
};

void push_vector(inout vector_t vec,
                 const in uint d) {
    vec.data[vec.size] = d;
    ++ vec.size;
}

vec3 unpack_vec3(const in float array[3]) {
    return vec3(array[0], array[1], array[2]);
}

float square(const in float val) {
    return val;
}

float pow5(const in float val) {
    const float squared = square(val);
    return square(squared) * val;
}
