#version 460 core

// Výstupní barva pixelu
out vec4 FragColor;


uniform vec3 ambient_intensity = vec3(0.3, 0.3, 0.3); // Mírné základní světlo, ať není tma
uniform vec3 diffuse_intensity = vec3(0.8, 0.8, 0.8); // Hlavní barva světla
uniform vec3 specular_intensity = vec3(1.0, 1.0, 1.0); // Barva odlesku (bílá)

uniform vec3 ambient_material = vec3(1.0, 1.0, 1.0);
uniform vec3 diffuse_material = vec3(1.0, 1.0, 1.0);
uniform vec3 specular_material = vec3(1.0, 1.0, 1.0);
uniform float specular_shinines = 32.0; // Jak moc je povrch lesklý (1 = matný, 128 = zrcadlo)

// Aktivní texturovací jednotka
uniform sampler2D tex0;

// Vstup z Vertex Shaderu (MUSÍ se jmenovat stejně jako ve .vert)
in VS_OUT {
    vec3 N;
    vec3 L;
    vec3 V;
    vec2 texCoord;
} fs_in;

void main(void) {
    // Normalizace vektorů (ujistíme se, že mají délku přesně 1.0)
    vec3 N = normalize(fs_in.N);
    vec3 L = normalize(fs_in.L);
    vec3 V = normalize(fs_in.V);

    // Výpočet odrazu světla (Reflect)
    // -L protože světlo svítí DO povrchu, ale my chceme vektor OD povrchu
    vec3 R = reflect(-L, N);

    // 1. AMBIENTNÍ složka (všudypřítomné světlo)
    vec3 ambient = ambient_material * ambient_intensity;
    
    // 2. DIFÚZNÍ složka (hlavní světlo z daného směru, dot product dělá stínování)
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_material * diffuse_intensity;
    
    // 3. SPEKULÁRNÍ složka (odlesk, tzv. "prasátko")
    // OPRAVA: Odstraněna přebytečná hvězdička
    vec3 specular = pow(max(dot(R, V), 0.0), specular_shinines) * specular_material * specular_intensity;

    // Sečteme (Ambient + Diffuse), vynásobíme texturou, a nakonec přidáme čistý odlesk
    // OPRAVA: fs_in.texCoord místo samotného texCoord
    FragColor = vec4( (ambient + diffuse) * texture(tex0, fs_in.texCoord) + specular, 1.0);
}