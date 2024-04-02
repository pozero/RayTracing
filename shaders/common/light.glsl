#define LIGHT_DISTANT 0
#define LIGHT_AREA_SINGLE_SIDED 1
#define LIGHT_AREA_DOUBLE_SIDED 2
#define LIGHT_SKY 3

struct light_t {
    vec3 intensity;
    int emission_tex;
    vec3 direction;
    int type;
    uint mesh;
    int transform;
};
