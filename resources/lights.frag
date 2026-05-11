#version 460 core

out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in;

uniform sampler2D tex0;


struct DirLight {
    vec3 direction; // Směr odkud to svítí
    vec3 ambient;   // Globální šero v celém bludišti
    vec3 diffuse;   // Barva dopadajícího světla
    vec3 specular;  // Odlesky
};

// Struktura pro BODOVÉ SVĚTLO
struct PointLight {
    vec3 position; // Pozice ve View Space
    
    vec3 color;
    float intensity;

    // Útlum (Attenuation)
    float constant;
    float linear;
    float quadratic;
};

// Struktura pro BATERKU
struct SpotLight {
    vec3 position;  // Pozice ve View Space
    vec3 direction; // Směr ve View Space
    
    vec3 color;
    float intensity;
    
    float cutOff;       // Kosinus úhlu kužele 
    float spotExponent; // Jak moc to ke krajům slábne
    
    float constant;
    float linear;
    float quadratic;
};

// Proměnné pro světla
uniform DirLight dirLight;
uniform PointLight pointLights[3];
uniform SpotLight spotLight;

uniform bool use_point_lights = true;
uniform bool use_spot_light = true;

// Globální vlastnosti materiálu
uniform float shininess = 32.0;
uniform float object_alpha = 1.0;



vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    
    // Difúzní (Diffuse)
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Spekulární (Specular)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Výsledek (Ambient se stará o to, ať není úplná tma)
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    vec3 specular = light.specular * spec;
    
    return (ambient + diffuse + specular);
}


// Výpočet bodového světla

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
	if (light.intensity <= 0.0) {
        return vec3(0.0);
    }

    vec3 lightDir = normalize(light.position - fragPos);
    
    // Difúzní (Diffuse)
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Spekulární (Specular)
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Útlum (Attenuation) - vzorec z COMPAT-OLD
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    vec3 diffuse = light.color * diff * light.intensity;
    vec3 specular = light.color * spec * light.intensity; // Obarvíme odlesk barvou světla
    
    return (diffuse + specular) * attenuation;
}


// Výpočet baterky (Spotlight)
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    
    // Difúzní a spekulární počítáme stejně
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    
    // Útlum vzdáleností (stejný jako u Point light)
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // ÚTLUM KUŽELE (Spotlight cone) - vzorec z COMPAT-OLD
    float spotDot = dot(-lightDir, normalize(light.direction));
    float spotAttenuation = 0.0;
    
    if(spotDot > light.cutOff) { // Jsme uvnitř kužele!
        spotAttenuation = pow(spotDot, light.spotExponent);
    }
    
    vec3 diffuse = light.color * diff * light.intensity;
    vec3 specular = light.color * spec * light.intensity;
    
    return (diffuse + specular) * attenuation * spotAttenuation;
}

void main(void) {
    vec3 norm = normalize(fs_in.Normal);
    vec3 viewDir = normalize(-fs_in.FragPos); // Kamera je vždy v (0,0,0) ve View Space
    
    // 1. ZÁKLAD = Měsíční (Směrové) světlo a Ambientní šero
    vec3 result = CalcDirLight(dirLight, norm, viewDir);
    
    // 2. Přičteme 3 bodová světla
    if (use_point_lights) {
        for(int i = 0; i < 3; i++) {
            result += CalcPointLight(pointLights[i], norm, fs_in.FragPos, viewDir);
        }
    }
    
    // 3. Přičteme baterku
    if (use_spot_light) {
        result += CalcSpotLight(spotLight, norm, fs_in.FragPos, viewDir);
    }
    
    // 4. Vynásobíme finální barvu světla barvou naší textury
    vec4 texColor = texture(tex0, fs_in.TexCoord);
    vec4 final_color = vec4(result * texColor.rgb, texColor.a * object_alpha);
    
    // 5. MLHA / TMA 
    float distance = length(fs_in.FragPos);
    
    // Od 15 metrů začne tma houstnout, ve 30 metrech zdi úplně zmizí
    float fog_start = 15.0; 
    float fog_end = 30.0;   
    
    // Výpočet prolnutí (0.0 = úplná tma, 1.0 = normální barva)
    float fog_factor = clamp((fog_end - distance) / (fog_end - fog_start), 0.0, 1.0);
    
    // Barva tmy (musí být PŘESNĚ stejná jako glClearColor v C++)
    vec3 fog_color = vec3(0.005, 0.005, 0.01); 
    
    // Smícháme barvu zdi a barvu tmy dohromady!
    FragColor = mix(vec4(fog_color, final_color.a), final_color, fog_factor);
}