#version 430 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in ShadingData {
    vec3 frag_pos;
    vec3 normal;
} fs_in;

out vec4 colour;

const uint num_point_lights = 2u;

// attr          | base alignment | aligned offset
// -----------------------------------------------------
// dir_enabled   | 4              | 0
// point_enabled | 4              | 4
// dir_light     | 16             | 16  (direction)
//               | 16             | 32  (ambient)
//               | 16             | 48  (diffuse)
//               | 16             | 64  (specular)
// point_lights  | 16             | 80  (position)
//               | 16             | 96  (ambient)
//               | 16             | 112 (diffuse)
//               | 16             | 128 (specular)
//               | 4              | 140 (constant -- even though specular has a base alignment of 16)
//               | 4              | 144 (linear)
//               | 4              | 148 (quadratic)
//               | 8              | 152 (struct padding)
// subsequent array members follow the same offsets but from 160, 240, etc...
layout (std140) uniform Lights {
    bool dir_lights_enabled;
    bool point_lights_enabled;

    DirectionalLight dir_light;
    PointLight point_lights[num_point_lights];
};

uniform Material material;

vec4 DirectionalShading(DirectionalLight light, vec3 normal, vec3 view_dir) {
    vec3 ambient = light.ambient * material.ambient;

    vec3 light_dir = normalize(-light.direction);
    float diffuse_strength = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = light.diffuse * diffuse_strength * material.diffuse;

    vec3 half_dir = normalize(light_dir + view_dir);
    float specular_intensity = pow(max(dot(normal, half_dir), 0.0), material.shininess);
    vec3 specular = light.specular * material.specular * specular_intensity;

    return vec4(ambient + diffuse + specular, 1.0f);
}

vec4 PointShading(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir) {
    vec3 ambient = light.ambient * material.ambient;

    vec3 light_dir = normalize(light.position - frag_pos);
    float diffuse_strength = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = light.diffuse * diffuse_strength * material.diffuse;

    vec3 half_dir = normalize(light_dir + view_dir);
    float specular_intensity = pow(max(dot(normal, half_dir), 0.0), material.shininess);
    vec3 specular = light.specular * material.specular * specular_intensity;

    float distance = length(light.position - frag_pos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    return vec4((ambient + diffuse + specular) * attenuation, 1.0f);
}

void main() {
    colour = vec4(0.0f);

    if (dir_lights_enabled) {
        colour = DirectionalShading(dir_light, normalize(fs_in.normal), normalize(-fs_in.frag_pos));
    }

    if (point_lights_enabled) {
        for (uint i = 0u; i < num_point_lights; ++i) {
            colour += PointShading(point_lights[i], normalize(fs_in.normal), fs_in.frag_pos, normalize(-fs_in.frag_pos));
        }
    }
}
