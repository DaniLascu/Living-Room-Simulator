//
// ================================================
// | Grafica pe calculator                        |
// ================================================
// | Laboratorul X - 10_02_Shader.vert |
// =====================================
// 
//  Shaderul de varfuri / Vertex shader - afecteaza geometria scenei; 

#version 330 core

layout(location=0) in vec4 in_Position;
layout(location=1) in vec4 in_Color;
layout(location=2) in vec3 in_Normal;
layout(location=3) in vec2 in_TexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec3 inLightPos;      // Pentru lumina punctiforma (interior)
out vec3 inLightDirSun;   // Pentru lumina directionala (exterior)
out vec3 inViewPos;
out vec4 ex_Color;
out vec2 ex_TexCoord;

uniform mat4 matrUmbra;
uniform mat4 myMatrix;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightPos;    // Pozitia sursei din interior
uniform vec3 sunDir;      // Directia soarelui (transmisa din C++)
uniform vec3 viewPos;
uniform int codCol;

out vec4 FragPosLightSpace;
uniform mat4 lightSpaceMatrix;

void main(void) {
    if (codCol == 0)
        gl_Position = projection * view * myMatrix * in_Position;
    if (codCol == 1)
        gl_Position = projection * view * matrUmbra * myMatrix * in_Position;

    FragPos = vec3(myMatrix * in_Position);
    Normal = mat3(myMatrix) * in_Normal; 
    
    inLightPos = lightPos; 
    inLightDirSun = sunDir; // Directia luminii exterioare
    inViewPos = viewPos;
    ex_Color = in_Color;
    ex_TexCoord = in_TexCoord;

    FragPosLightSpace = lightSpaceMatrix * myMatrix * in_Position;
}