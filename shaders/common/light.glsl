#define LIGHT_DISTANT 0
#define LIGHT_AREA 1

struct light_t {
    vec3 intensity;
    int emission_tex;
    vec3 direction;
    int type;
    uint mesh;
    int transform;
};
