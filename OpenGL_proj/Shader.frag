//
// ================================================
// | Grafica pe calculator                        |
// ================================================
// | Laboratorul X - 10_02_Shader.frag |
// =====================================
// 
//  Shaderul de fragment / Fragment shader - afecteaza culoarea pixelilor;
//

#version 330 core
 
in vec3 FragPos;
in vec3 Normal;
in vec3 inLightPos;
in vec3 inLightDirSun;
in vec3 inViewPos;
in vec4 ex_Color;
in vec2 ex_TexCoord;    //coordonate de texturare

out vec4 out_Color;

uniform vec3 lightColor;    // Culoare lumina interior
uniform vec3 sunColor;      // Culoare lumina exterior
uniform int codCol;
uniform int lightSource;    // Sursa luminii (exterior/soare, interior/lustra)
uniform sampler2D objectTexture;
uniform int useTexture;   // 0 = normal, 1 = texturat
uniform float materialSpecularStrength;
uniform float materialShininess;

in vec4 FragPosLightSpace;
uniform sampler2D shadowMap;

float Shadow(vec4 fpLS, vec3 n, vec3 ldir)
{
    vec3 p = fpLS.xyz / fpLS.w;
    p = p*0.5 + 0.5;
    if(p.z > 1.0) return 0.0; //adancimea fragmentului curent (vazut de lumina)

    float bias = max(0.0025*(1.0 - dot(n, ldir)), 0.0005);

    float s=0.0;
    vec2 ts = 1.0 / textureSize(shadowMap,0);
    for(int x=-1;x<=1;x++)
    for(int y=-1;y<=1;y++){
        float d = texture(shadowMap, p.xy + vec2(x,y)*ts).r; //distanta pana la cel mai apropiat obiect vazut de lumina pe acea directie
        s += (p.z - bias > d) ? 1.0 : 0.0;
        //fragmentul e mai departe decat ce e in shadow map 
        //exista ceva intre lumina si fragment -> e in umbra
    }
    return s/9.0;
}


void main(void) {
    if (codCol == 1) {
        out_Color = vec4(0.0, 0.0, 0.0, 0.7); // Umbra [cite: 41]
        return;
    }

    int sunState, lightState;

    if(lightSource == 0){
        lightState = 1;
        sunState = 0;
    }
    else{
        lightState = 0;
        sunState = 1;
    }

    // 1. LUMINA PUNCTIFORMA (INTERIOR)
    vec3 normala = normalize(Normal);
    vec3 lightDirPoint = normalize(inLightPos - FragPos);
    float diffPoint = max(dot(normala, lightDirPoint), 0.0);
    vec3 diffusePoint = diffPoint * lightColor;

    // SPECULAR 
    vec3 viewDirPoint = normalize(inViewPos - FragPos);
    vec3 reflectDirPoint = normalize(reflect(-lightDirPoint, normala));
    float specPoint = pow(max(dot(viewDirPoint, reflectDirPoint), 0.0), materialShininess); //trebuie shiness variabile
    vec3 specularPoint = materialSpecularStrength * specPoint * lightColor; //strength variabil


    // 2. LUMINA DIRECTIONALA (EXTERIOR) ---
    // inLightDirSun trebuie sa fie directia Dinspre sursa
    vec3 lightDirSun = normalize(-inLightDirSun); 
    float diffSun = max(dot(normala, lightDirSun), 0.0);
    vec3 diffuseSun = diffSun * sunColor;

    // SPECULAR !!! Nu sunt sigur ca lumina directionala are componenta specular
    vec3 viewDirSun = normalize(inViewPos - FragPos);
    vec3 reflectDirSun = normalize(reflect(-lightDirSun, normala));
    float specSun = pow(max(dot(viewDirSun, reflectDirSun), 0.0), materialShininess);
    vec3 sunSpecularColor = mix(sunColor, vec3(1.0), 0.7); 
    vec3 specularSun = materialSpecularStrength * specSun * sunSpecularColor;

    // 3. COMPONENTA AMBIENTALA
    float ambientStrength;

    if(lightState == 0)
        ambientStrength = 0.9f;
    else
        ambientStrength = 0.2f; 

    if(useTexture == 1){
        ambientStrength = sunState == 1 ? 0.5f : 0.05f;
    }

    vec3 ambient = ambientStrength * (lightColor * 0.5 + sunSpecularColor * 0.5);

    //TEXTURARE
    vec4 baseColor = ex_Color;
    
    if (useTexture == 1) {
        baseColor = texture(objectTexture, ex_TexCoord);
        baseColor.a *= ex_Color.a;
    }

    // Combinare Componente in Rezultat
    // Adunam contributiile ambelor surse
    vec3 n = normalize(Normal);
    vec3 lSun = normalize(-inLightDirSun);

    float shadow = (sunState==1) ? Shadow(FragPosLightSpace, n, lSun) : 0.0;
    vec3 sunTerm = (diffuseSun + specularSun) * (1.0 - shadow);

    vec3 result = (ambient + lightState*(diffusePoint+specularPoint) + sunState*sunTerm) * baseColor.rgb;


    out_Color = vec4(result, baseColor.a); //opacitate culorii, pentru obeicte transparente
}