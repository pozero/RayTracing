#define LIGHT_DISTANT 0
#define LIGHT_AREA 1

struct light_t {
    vec3 intensity;
    vec3 direction;
    int mesh;
    int transform;
    int emission_tex;
    int type;
};
